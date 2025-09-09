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

// 4 MOSFET pump/valve control setup
const uint8_t MOTOR_PINS[4] = {18, 17, 10, 9};  // GPIO pins for motors: M1 pump inflate, M2 pump deflate, M3 valve inflate, M4 valve deflate
const uint8_t PWM_CHANNELS[4] = {0, 1, 2, 3};   // PWM channels for motors
volatile uint8_t currentAirLevel = 64; // Current air level (0-127, with 64 = stopped)

// Air Control Encoder - sends CC23 MIDI messages
CCAbsoluteEncoder airEncoder {
    {38, 21},  // Encoder pins (swapped for clockwise increase)
    {23},       // CC 23 for air control
    6           // Multiplier
};

/**
 * @brief Custom MIDI sink for air/pump control
 * 
 * This sink receives MIDI messages from pipes and controls the air pressure
 * based on CC 23 messages with 64-centered mapping:
 * - CC 64: Stop (all motors off)
 * - CC 65-127: Inflation (increasing PWM)
 * - CC 0-63: Deflation (increasing PWM)
 */
class AirControlSink : public TrueMIDI_Sink {
public:
    AirControlSink() {}
    
    void sinkMIDIfromPipe(ChannelMessage msg) override {
        // Handle CC 23 messages for air control
        if (msg.getMessageType() == MIDIMessageType::ControlChange && 
            msg.getData1() == 23) {
            
            uint8_t ccValue = msg.getData2();
            currentAirLevel = ccValue;
            
            if (ccValue == 64) {
                // Stop - all motors off
                for (int i = 0; i < 4; i++) {
                    ledcWrite(PWM_CHANNELS[i], 0);
                }
                Serial.println("Air control: STOPPED (CC23=64)");
                
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
                Serial.print("% (CC23=");
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
                Serial.print("% (CC23=");
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
 *  │ CC 23           │    │ Surface      │    │ (transmission)  │
 *  └─────────────────┘    └──────────────┘    └─────────────────┘
 *                                                       ▲
 *                                                       │ Route 2
 *  ┌─────────────────┐    ┌──────────────┐             │
 *  │ External MIDI   │───▶│ Bluetooth In │─────────────┘
 *  │ Controller      │    │ CC23 → Air   │
 *  └─────────────────┘    └──────────────┘
 *                         
 *  ┌─────────────────┐    ┌──────────────┐
 *  │ Air Encoder     │───▶│ AirSink      │ Route 3
 *  │ CC 23           │    │ (direct)     │
 *  └─────────────────┘    └──────────────┘
 */

// Pipe factory for proper routing management (need 2 pipes for separation of concerns)
MIDI_PipeFactory<3> pipeFactory;

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
    
    // Initialize encoder to center position (64)
    airEncoder.setValue(64);
    Serial.println("Air encoder initialized to center position (CC23=64)");
            
    // Initialize CLI
    commandInterface.begin();
    
    Serial.println("Routing configured:");
    Serial.println("  Route 1: Air Encoder → CC23 → Bluetooth Out (transmission)");
    Serial.println("  Route 2: Bluetooth In → CC23 → Air Control (external control)");
    Serial.println("  Route 3: Air Encoder → CC23 → Air Control (direct control)");
    Serial.println("Ready!");
}

void loop() {
    // Update all MIDI processing and routing
    Control_Surface.loop();
    delay(5); // Limit control surface processing.

    // Update LED display to show current air level with center-based visualization
    ledController.updateAirDisplay(currentAirLevel);
    
    // Refresh LED display
    ledController.refresh();
    
    // Handle CLI interface
    commandInterface.update();
}
