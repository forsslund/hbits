#include <Arduino.h>
#include <Control_Surface.h>
#include "cli.h"
#include "ledcontrol.h"

// MIDI Interface
BluetoothMIDI_Interface midibt;

// CLI Interface
CLI commandInterface(Serial, midibt);

// LED Controller
LEDController ledController;

// Heat control PWM setup
const uint8_t HEAT_PIN = 18;        // GPIO pin for MOSFET M0
const uint8_t HEAT_PWM_CHANNEL = 0; // PWM channel for heat control
volatile uint8_t currentHeatLevel = 0; // Current heat level (0-127)

// Heat Control Encoder - sends CC23 MIDI messages
CCAbsoluteEncoder heatEncoder {
    {21, 38},  // Encoder pins
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
            
            currentHeatLevel = msg.getData2();
            
            // Convert CC23 value (0-127) to PWM duty cycle (0-255)
            uint8_t pwmValue = (currentHeatLevel * 255) / 127;
            ledcWrite(HEAT_PWM_CHANNEL, pwmValue);
            
            Serial.print("Heat level: ");
            Serial.print((currentHeatLevel * 100) / 127);
            Serial.print("% (CC23=");
            Serial.print(currentHeatLevel);
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

    // Get current encoder value and update heat level display
    uint8_t encoderValue = heatEncoder.getValue();
    
    // Map encoder value (0-23) to CC23 range (0-127) for LED display
    uint8_t displayHeatLevel = (encoderValue * 127) / 23;
    
    // Update LED display to show current heat level as rainbow bar graph
    ledController.updateDisplay(displayHeatLevel);
    
    // Refresh LED display
    ledController.refresh();
    
    // Handle CLI interface
    commandInterface.update();
}
