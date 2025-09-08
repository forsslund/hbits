#include <Arduino.h>
#include <Control_Surface.h>
#include <Adafruit_INA219.h>
#include "cli.h"
#include "ledcontrol.h"

// MIDI Interface
BluetoothMIDI_Interface midibt;

// CLI Interface
CLI commandInterface(Serial, midibt);

// LED Controller
LEDController ledController;

// INA219 Current Sensor
Adafruit_INA219 ina219;

// Heat control PWM setup
const uint8_t HEAT_PIN = 18;        // GPIO pin for MOSFET M0
const uint8_t HEAT_PWM_CHANNEL = 0; // PWM channel for heat control
volatile uint8_t currentHeatLevel = 0; // Current heat level (0-127)

// Timing for INA219 readings
unsigned long lastINA219Reading = 0;
const unsigned long INA219_INTERVAL = 50; // Read every 50ms

// Power limiting variables
const float MAX_POWER_W = 8.0; // Maximum allowed power in watts
uint8_t maxAllowedHeatLevel = 127; // Auto-discovered power limit (starts at max)
float averagePower = 0.0; // Running average power
bool powerLimitActive = false; // Flag to indicate if power limiting is active

// Heat Control Encoder - sends CC23 MIDI messages
CCAbsoluteEncoder heatEncoder {
    {38, 21},  // Encoder pins (swapped for clockwise increase)
    {23},       // CC 23 for heat control
    6           // Multiplier
};

/**
 * @brief Custom MIDI sink for heat control
 * 
 * This sink receives MIDI messages from pipes and controls the heat level
 * based on CC 23 messages. It follows the TrueMIDI_Sink interface properly.
 */
class HeatControlSink : public TrueMIDI_Sink {
public:
    HeatControlSink() {}
    
    void sinkMIDIfromPipe(ChannelMessage msg) override {
        // Handle CC 23 messages for heat control
        if (msg.getMessageType() == MIDIMessageType::ControlChange && 
            msg.getData1() == 23) {
            
            uint8_t requestedHeatLevel = msg.getData2();
            
            // Apply power limiting: clamp to discovered maximum
            currentHeatLevel = min(requestedHeatLevel, maxAllowedHeatLevel);
            
            // Convert CC23 value (0-127) to PWM duty cycle (0-255)
            uint8_t pwmValue = (currentHeatLevel * 255) / 127;
            ledcWrite(HEAT_PWM_CHANNEL, pwmValue);
            
            // Check if power limiting is active
            if (requestedHeatLevel > maxAllowedHeatLevel) {
                powerLimitActive = true;
            } else {
                powerLimitActive = false;
            }
            
            Serial.print("Heat level: ");
            Serial.print((currentHeatLevel * 100) / 127);
            Serial.print("% (CC23=");
            Serial.print(currentHeatLevel);
            if (powerLimitActive) {
                Serial.print(" POWER LIMITED from ");
                Serial.print(requestedHeatLevel);
            }
            Serial.print(", PWM=");
            Serial.print(pwmValue);
            Serial.println(")");
        }
    }
    
    // Required overrides for other MIDI message types (no-op for our use case)
    void sinkMIDIfromPipe(SysExMessage) override {}
    void sinkMIDIfromPipe(SysCommonMessage) override {}
    void sinkMIDIfromPipe(RealTimeMessage) override {}
};

// Structure to hold averaged sensor readings
struct AveragedReadings {
    float current_A;
    float voltage_V;
    float power_W;
};

// Function to get averaged readings from multiple samples
AveragedReadings getAveragedReadings(int numSamples = 10) {
    float totalCurrent = 0.0;
    float totalVoltage = 0.0;
    float totalPower = 0.0;
    
    for (int i = 0; i < numSamples; i++) {
        totalCurrent += ina219.getCurrent_mA();
        totalVoltage += ina219.getBusVoltage_V();
        totalPower += ina219.getPower_mW();
        delay(1); // Small delay between samples
    }
    
    AveragedReadings readings;
    readings.current_A = (totalCurrent / numSamples) / 1000.0; // Convert mA to A
    readings.voltage_V = totalVoltage / numSamples;
    readings.power_W = (totalPower / numSamples) / 1000.0; // Convert mW to W
    
    return readings;
}

// Instantiate the heat control sink
HeatControlSink heatSink;

/**
 * @brief MIDI Routing Setup
 * 
 * Creates a clean pipe-based routing system with two explicit routes:
 * 
 *  ┌─────────────────┐    ┌──────────────┐    ┌─────────────────┐
 *  │ Heat Encoder    │───▶│ Control_     │───▶│ Bluetooth Out   │ Route 1
 *  │ CC 23           │    │ Surface      │    │ (transmission)  │
 *  └─────────────────┘    └──────────────┘    └─────────────────┘
 *                                                       ▲
 *                                                       │ Route 2
 *  ┌─────────────────┐    ┌──────────────┐             │
 *  │ External MIDI   │───▶│ Bluetooth In │─────────────┘
 *  │ Controller      │    │ CC23 → Heat  │
 *  └─────────────────┘    └──────────────┘
 */

// Pipe factory for proper routing management (need 2 pipes for separation of concerns)
MIDI_PipeFactory<3> pipeFactory;

void setup() {
    Serial.begin(115200);
    Serial.println("=== HBITS Heat Controller ===");
    
    // Initialize PWM for heat control
    ledcSetup(HEAT_PWM_CHANNEL, 1000, 8); // 1kHz, 8-bit resolution
    ledcAttachPin(HEAT_PIN, HEAT_PWM_CHANNEL);
    ledcWrite(HEAT_PWM_CHANNEL, 0); // Start with heat off
    
    Serial.println("PWM heat control initialized on pin 18");
    
    // Initialize INA219 current sensor
    if (!ina219.begin()) {
        Serial.println("ERROR: Failed to find INA219 sensor!");
        Serial.println("Check wiring and I2C address (default 0x40)");
    } else {
        Serial.println("INA219 current sensor initialized");
        // Optional: set calibration for higher precision if needed
        // ina219.setCalibration_32V_2A();  // 32V, 2A max (default)
        // ina219.setCalibration_32V_1A();  // 32V, 1A max (higher precision)
        // ina219.setCalibration_16V_400mA(); // 16V, 400mA max (highest precision)
    }
    
    // Set up clean pipe-based routing BEFORE Control_Surface.begin()
    // Two explicit, unidirectional routes for clear separation of concerns:
    // 

    // Route 1: Control_Surface (FSR) → HeatSink (standalone haptic control)
    Control_Surface >> pipeFactory >> heatSink;
    
    // Route 2: Control_Surface (Heat Encoder) → Bluetooth (CC23 transmission)
    Control_Surface >> pipeFactory >> midibt;
    
    // Route 3: Bluetooth → HeatSink (external MIDI control of heat)
    midibt >> pipeFactory >> heatSink;
    
    // Set Bluetooth device name
    midibt.setName("HBITS Heat 1");
    
    // Initialize LED controller
    ledController.begin();
    
    // Initialize the Control Surface system
    Control_Surface.begin();
            
    // Initialize CLI
    commandInterface.begin();
    
    Serial.println("Routing configured:");
    Serial.println("  Route 1: Heat Encoder → CC23 → Bluetooth Out (transmission)");
    Serial.println("  Route 2: Bluetooth In → CC23 → Heat Control (external control)");
    Serial.println("Ready!");
}

void loop() {
    // Update all MIDI processing and routing
    Control_Surface.loop();
    delay(5); // Limit control surface processing.

    // Read INA219 sensor periodically
    unsigned long currentTime = millis();
    if (currentTime - lastINA219Reading >= INA219_INTERVAL) {
        lastINA219Reading = currentTime;
        
        // Get averaged readings (multiple samples for stability)
        AveragedReadings readings = getAveragedReadings(10);
        averagePower = readings.power_W;
        
        // Power limiting logic: if we exceed 8W, set current heat level as maximum
        if (averagePower > MAX_POWER_W && currentHeatLevel > 0) {
            maxAllowedHeatLevel = currentHeatLevel;
            Serial.print("POWER LIMIT DISCOVERED: Max heat level set to ");
            Serial.print(maxAllowedHeatLevel);
            Serial.print(" (");
            Serial.print((maxAllowedHeatLevel * 100) / 127);
            Serial.println("%)");
        }
        
        // Print averaged readings in a clear format
        Serial.print("INA219 - Current: ");
        Serial.print(readings.current_A, 3);
        Serial.print(" A, Voltage: ");
        Serial.print(readings.voltage_V, 2);
        Serial.print(" V, Power: ");
        Serial.print(readings.power_W, 3);
        Serial.print(" W (all avg)");
        
        // Add heat level and limit context
        Serial.print(" (Heat: ");
        Serial.print((currentHeatLevel * 100) / 127);
        Serial.print("%, Max: ");
        Serial.print((maxAllowedHeatLevel * 100) / 127);
        Serial.print("%");
        if (powerLimitActive) {
            Serial.print(" POWER LIMITED");
        }
        Serial.println(")");
    }

    // Update LED display to show current heat level as gradient line segment
    // Scale the display based on the discovered power limit (so power limit = 100% LEDs)
    uint8_t scaledHeatForDisplay;
    if (maxAllowedHeatLevel < 127) {
        // Scale current heat level to 0-127 range based on discovered maximum
        scaledHeatForDisplay = (currentHeatLevel * 127) / maxAllowedHeatLevel;
    } else {
        // No power limit discovered yet, use actual heat level
        scaledHeatForDisplay = currentHeatLevel;
    }
    ledController.updateHeatDisplay(scaledHeatForDisplay);
    
    // Refresh LED display
    ledController.refresh();
    
    // Handle CLI interface
    commandInterface.update();
}
