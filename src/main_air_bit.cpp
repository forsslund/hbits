#include <Arduino.h>
#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_LPS28.h>
#include "cli.h"
#include "ledcontrol.h"

// MIDI Interface
BluetoothMIDI_Interface midibt;

// CLI Interface
CLI commandInterface(Serial, midibt);

// LED Controller
LEDController ledController;

// 4 MOSFET pump/valve control setup
const uint8_t MOTOR_PINS[4] = {18, 17, 10, 9};  // GPIO pins for motors: M1 pump inflate, M2 pump deflate, M3 valve inflate, M4 valve deflate
const uint8_t PWM_CHANNELS[4] = {0, 1, 2, 3};   // PWM channels for motors
volatile uint8_t currentAirLevel = 64; // Current air level (0-127, with 64 = stopped)

// Air Control Encoder - sends CC24 MIDI messages
CCAbsoluteEncoder airEncoder {
    {38, 21},  // Encoder pins (swapped for clockwise increase)
    {24},       // CC 24 for air control
    6           // Multiplier
};

// Encoder Reset Button - connected to D12 (GPIO 47)
Button encoderResetButton {47};

// LPS28 Pressure Sensor Setup - using Adafruit LPS28 library
Adafruit_LPS28 lps28;  // LPS28 sensor object using Adafruit library
unsigned long lastPressureRead = 0;
float currentPressure = 0.0;
uint8_t pressureMidiValue = 0;

// Use default I2C pins (GPIO 21 = SDA, GPIO 22 = SCL)
// These work fine alongside encoder and other I2C devices

// Pressure Output - we'll send CC25 directly via MIDI interface

/**
 * @brief Custom MIDI sink for air/pump control
 * 
 * This sink receives MIDI messages from pipes and controls the air pressure
 * based on CC 24 messages with 64-centered mapping:
 * - CC 64: Stop (all motors off)
 * - CC 65-127: Inflation (increasing PWM)
 * - CC 0-63: Deflation (increasing PWM)
 */
class AirControlSink : public TrueMIDI_Sink {
public:
    AirControlSink() {}
    
    void sinkMIDIfromPipe(ChannelMessage msg) override {
        // Handle CC 24 messages for air control
        if (msg.getMessageType() == MIDIMessageType::ControlChange && 
            msg.getData1() == 24) {
            
            uint8_t ccValue = msg.getData2();
            currentAirLevel = ccValue;
            
            if (ccValue == 64) {
                // Stop - all motors off
                for (int i = 0; i < 4; i++) {
                    ledcWrite(PWM_CHANNELS[i], 0);
                }
                Serial.println("Air control: STOPPED (CC24=64)");
                
            } else if (ccValue > 64) {
                // Inflation mode (65-127)
                uint8_t pwmValue = map(ccValue, 65, 127, 0, 255);
                
                // M1 pump inflate (ON), M2 pump deflate (OFF)
                ledcWrite(PWM_CHANNELS[0], pwmValue);  // M1 (GPIO 18)
                ledcWrite(PWM_CHANNELS[1], 0);         // M2 (GPIO 17)
                
                // M3 valve inflate (ON), M4 valve deflate (OFF)
                ledcWrite(PWM_CHANNELS[2], pwmValue);  // M3 (GPIO 10)
                ledcWrite(PWM_CHANNELS[3], 0);         // M4 (GPIO 9)
                
                Serial.print("Air control: INFLATING ");
                Serial.print(((ccValue - 64) * 100) / 63);
                Serial.print("% (CC24=");
                Serial.print(ccValue);
                Serial.print(", PWM=");
                Serial.print(pwmValue);
                Serial.println(")");
                
            } else {
                // Deflation mode (0-63)
                uint8_t pwmValue = map(ccValue, 0, 63, 255, 0);
                
                // M1 pump inflate (OFF), M2 pump deflate (ON)
                ledcWrite(PWM_CHANNELS[0], 0);         // M1 (GPIO 18)
                ledcWrite(PWM_CHANNELS[1], pwmValue);  // M2 (GPIO 17)
                
                // M3 valve inflate (OFF), M4 valve deflate (ON)
                ledcWrite(PWM_CHANNELS[2], 0);         // M3 (GPIO 10)
                ledcWrite(PWM_CHANNELS[3], pwmValue);  // M4 (GPIO 9)
                
                Serial.print("Air control: DEFLATING ");
                Serial.print(((63 - ccValue) * 100) / 63);
                Serial.print("% (CC24=");
                Serial.print(ccValue);
                Serial.print(", PWM=");
                Serial.print(pwmValue);
                Serial.println(")");
            }
        }
    }
    
    // Required overrides for other MIDI message types (no-op for our use case)
    void sinkMIDIfromPipe(SysExMessage) override {}
    void sinkMIDIfromPipe(SysCommonMessage) override {}
    void sinkMIDIfromPipe(RealTimeMessage) override {}
};

// Instantiate the air control sink
AirControlSink airSink;

/**
 * @brief MIDI Routing Setup
 * 
 * Creates a clean pipe-based routing system with three explicit routes:
 * 
 *  ┌─────────────────┐    ┌──────────────┐    ┌─────────────────┐
 *  │ Air Encoder     │───▶│ Control_     │───▶│ Bluetooth Out   │ Route 1
 *  │ CC 24           │    │ Surface      │    │ (transmission)  │
 *  └─────────────────┘    └──────────────┘    └─────────────────┘
 *                                                       ▲
 *                                                       │ Route 2
 *  ┌─────────────────┐    ┌──────────────┐             │
 *  │ External MIDI   │───▶│ Bluetooth In │─────────────┘
 *  │ Controller      │    │ CC24 → Air   │
 *  └─────────────────┘    └──────────────┘
 *                         
 *  ┌─────────────────┐    ┌──────────────┐
 *  │ Air Encoder     │───▶│ AirSink      │ Route 3
 *  │ CC 24           │    │ (direct)     │
 *  └─────────────────┘    └──────────────┘
 */

// Pipe factory for proper routing management (need 2 pipes for separation of concerns)
MIDI_PipeFactory<3> pipeFactory;

/**
 * @brief Update pressure reading and send CC25 MIDI message
 * 
 * Uses status-based reading following official Adafruit patterns.
 * Only reads when data is ready, more efficient than timer-based polling.
 * Initial mapping: 950-1050 hPa → 0-127 MIDI range (adjustable based on actual readings)
 */
void updatePressureReading() {
    // Check if pressure data is ready using official Adafruit pattern
    if (lps28.getStatus() & LPS28_STATUS_PRESS_READY) {
        // Read pressure and temperature directly using official methods
        float pressureHPa = lps28.getPressure();
        float temperatureC = lps28.getTemperature();
        currentPressure = pressureHPa;
        
        // Map pressure to MIDI range (measured range: 880-1250 hPa)
        // Constrain to prevent wraparound below minimum pressure
        long pressureCentiPa = (long)(pressureHPa * 100);
        pressureMidiValue = map(pressureCentiPa, 88000, 125000, 0, 127);
        pressureMidiValue = constrain(pressureMidiValue, 0, 127);
        
        // Send CC25 MIDI message directly
        midibt.sendControlChange({25, Channel_1}, pressureMidiValue);
        
        // Throttle serial output to avoid flooding (only every 100ms)
        if (millis() - lastPressureRead >= 100) {
            Serial.print("Pressure: ");
            Serial.print(currentPressure, 2);
            Serial.print(" hPa, Temp: ");
            Serial.print(temperatureC, 1);
            Serial.print(" °C, MIDI CC25: ");
            Serial.println(pressureMidiValue);
            lastPressureRead = millis();
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== HBITS Air Controller ===");
    
    // Initialize PWM channels for 4 MOSFET pump/valve control
    Serial.println("Initializing PWM for pump/valve motors...");
    for (int i = 0; i < 4; i++) {
        ledcSetup(PWM_CHANNELS[i], 1000, 8); // 1kHz, 8-bit resolution
        ledcAttachPin(MOTOR_PINS[i], PWM_CHANNELS[i]);
        ledcWrite(PWM_CHANNELS[i], 0); // Start with all motors off
    }
    Serial.println("PWM channels initialized for pump/valve control");
    
    // Initialize I2C for LPS28 pressure sensor (default pins work fine with encoder)
    Serial.println("Initializing I2C for LPS28 pressure sensor...");
    Wire.begin();  // Use default I2C pins (GPIO 21 = SDA, GPIO 22 = SCL)
    
    // I2C device scanning with delay as requested
    Serial.println("Scanning I2C devices...");
    //delay(5000);  // 5 second delay so user doesn't miss I2C device detection messages
    int deviceCount = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
            deviceCount++;
        }
    }
    if (deviceCount == 0) {
        Serial.println("No I2C devices found");
    } else {
        Serial.print("Found ");
        Serial.print(deviceCount);
        Serial.println(" I2C device(s)");
    }
    
    // Try to initialize LPS28 with the detected I2C address 0x5C  
    if (!lps28.begin(&Wire, 0x5C)) {
        Serial.println("Failed to initialize LPS28 chip at address 0x5C");
        Serial.println("Continuing without pressure sensor...");
        // Don't halt - continue without pressure sensor
    } else {
        Serial.println("LPS28 Found and initialized at address 0x5C!");
        
        // Configure sensor for optimal accuracy and stability
        lps28.setDataRate(LPS28_ODR_10_HZ);
        lps28.setAveraging(LPS28_AVG_4);        // 4-sample averaging for noise reduction
        lps28.setFullScaleMode(true);           // Extended range (4060 hPa) for better resolution
        
        Serial.println("LPS28 pressure sensor configured:");
        Serial.println("  - Data rate: 10 Hz");
        Serial.println("  - Averaging: 4 samples (noise reduction)");
        Serial.println("  - Full-scale mode: Extended range (4060 hPa)");
        Serial.println("LPS28 pressure sensor ready!");
    }
    
    // Set up clean pipe-based routing BEFORE Control_Surface.begin()
    // Three explicit, unidirectional routes for clear separation of concerns:
    
    // Route 1: Control_Surface (Air Encoder) → Bluetooth (CC23 transmission)
    Control_Surface >> pipeFactory >> midibt;
    
    // Route 2: Bluetooth → AirSink (external MIDI control of air)
    midibt >> pipeFactory >> airSink;
    
    // Route 3: Control_Surface → AirSink (direct encoder control)
    Control_Surface >> pipeFactory >> airSink;
    
    // Set Bluetooth device name
    midibt.setName("HBITS Air 1");
    
    // Initialize LED controller
    ledController.begin();
    
    // Initialize the Control Surface system
    Control_Surface.begin();
    encoderResetButton.begin(); // Initialize encoder reset button.
    
    // Initialize encoder to center position (64)
    airEncoder.setValue(64);
    Serial.println("Air encoder initialized to center position (CC24=64)");
            
    // Initialize CLI
    commandInterface.begin();
    
    Serial.println("Routing configured:");
    Serial.println("  Route 1: Air Encoder → CC24 → Bluetooth Out (transmission)");
    Serial.println("  Route 2: Bluetooth In → CC24 → Air Control (external control)");
    Serial.println("  Route 3: Air Encoder → CC24 → Air Control (direct control)");
    Serial.println("  Route 4: LPS28 Pressure Sensor → CC25 → Bluetooth Out (every 100ms)");
    Serial.println("  Route 5: Encoder Reset Button → D12 (GPIO 47) → Emergency Stop");
    Serial.println("Ready!");
}

void loop() {
    // Update all MIDI processing and routing
    Control_Surface.loop();
    delay(5); // Limit control surface processing.

    // Check for encoder reset button press
    if (encoderResetButton.update() && encoderResetButton.getState() == Button::Falling) {
        // Reset encoder to center position (64) - stops pump
        airEncoder.setValue(64);
        currentAirLevel = 64;
        
        // Immediately stop all motors
        for (int i = 0; i < 4; i++) {
            ledcWrite(PWM_CHANNELS[i], 0);
        }
        
        Serial.println("ENCODER RESET BUTTON PRESSED - Pump stopped (CC24=64)");
    }

    // Update pressure reading and send CC25
    updatePressureReading();

    // Update LED display to show current air level with center-based visualization
    ledController.updateAirDisplay(currentAirLevel);
    
    // Refresh LED display
    ledController.refresh();
    
    // Handle CLI interface
    commandInterface.update();
}
