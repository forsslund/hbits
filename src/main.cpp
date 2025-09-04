#include <Arduino.h>
#include <Control_Surface.h>
#include "hapticplayer.h"
#include "cli.h"

// MIDI Interface
BluetoothMIDI_Interface midibt;

// Haptic Player
HapticPlayer hapticPlayer;

// CLI Interface
CLI commandInterface(Serial, midibt);

// FSR Input Element
CCPotentiometer fsr {
    A0,     // Analog pin
    {0x16}, // CC 22 (0x16 in hex)
};

/**
 * @brief Custom MIDI sink for haptic volume control
 * 
 * This sink receives MIDI messages from pipes and controls the haptic player
 * based on CC 22 messages. It follows the TrueMIDI_Sink interface properly.
 */
class HapticVolumeSink : public TrueMIDI_Sink {
public:
    explicit HapticVolumeSink(HapticPlayer& player) : haptic(player) {}
    
    void sinkMIDIfromPipe(ChannelMessage msg) override {
        // Handle CC 22 messages for haptic volume control
        if (msg.getMessageType() == MIDIMessageType::ControlChange && 
            msg.getData1() == 22) {
            
            float volume = msg.getData2() / 127.0f;
            haptic.setVolume(volume);
            
            Serial.print("Haptic volume: ");
            Serial.print(volume, 3);
            Serial.print(" (CC22=");
            Serial.print(msg.getData2());
            Serial.println(")");
        }
    }
    
    // Required overrides for other MIDI message types (no-op for our use case)
    void sinkMIDIfromPipe(SysExMessage) override {}
    void sinkMIDIfromPipe(SysCommonMessage) override {}
    void sinkMIDIfromPipe(RealTimeMessage) override {}

private:
    HapticPlayer& haptic;
};

// Instantiate the haptic sink
HapticVolumeSink hapticSink(hapticPlayer);

/**
 * @brief MIDI Routing Setup
 * 
 * Creates a clean pipe-based routing system with three explicit routes:
 * 
 *  ┌─────────────────┐    ┌──────────────┐    ┌─────────────────┐
 *  │   FSR (A0)      │───▶│ Control_     │───▶│ HapticVolume    │ Route 1
 *  │   CC 22         │    │ Surface      │    │ Sink            │
 *  └─────────────────┘    └──────┬───────┘    └─────────┬───────┘
 *                                │                      ▲
 *                                ▼                      │ Route 3
 *  ┌─────────────────┐    ┌──────────────┐             │
 *  │ External MIDI   │───▶│ Bluetooth    │─────────────┘
 *  │ Controller      │    │ Interface    │
 *  └─────────────────┘    └──────▲───────┘
 *                                │ Route 2
 *                                │
 *                         ┌──────┴───────┐
 *                         │ Control_     │
 *                         │ Surface      │
 *                         └──────────────┘
 */

// Pipe factory for proper routing management (need 3 pipes for separation of concerns)
MIDI_PipeFactory<3> pipeFactory;

void setup() {
    Serial.begin(115200);
    Serial.println("=== Clean MIDI Haptic Controller ===");
    
    // Set up clean pipe-based routing BEFORE Control_Surface.begin()
    // Three explicit, unidirectional routes for clear separation of concerns:
    // 
    // Route 1: Control_Surface (FSR) → HapticSink (standalone haptic control)
    Control_Surface >> pipeFactory >> hapticSink;
    
    // Route 2: Control_Surface (FSR) → Bluetooth (FSR data transmission)
    Control_Surface >> pipeFactory >> midibt;
    
    // Route 3: Bluetooth → HapticSink (external MIDI control of haptics)
    midibt >> pipeFactory >> hapticSink;
    
    // Initialize the Control Surface system
    Control_Surface.begin();
    
    // Initialize haptic system
    hapticPlayer.setVolume(0.0f);
    hapticPlayer.setEffect(std::make_shared<HapticEffect>(EFFECT_CONST_VIBE));
    hapticPlayer.start();
    
    // Initialize CLI
    commandInterface.begin();
    
    Serial.println("Routing configured:");
    Serial.println("  Route 1: FSR (A0) → CC22 → Haptic Volume (standalone)");
    Serial.println("  Route 2: FSR (A0) → CC22 → Bluetooth Out (transmission)");
    Serial.println("  Route 3: Bluetooth In → CC22 → Haptic Volume (external control)");
    Serial.println("Ready!");
}

void loop() {
    // Update all MIDI processing and routing
    Control_Surface.loop();
    
    // Handle CLI interface
    commandInterface.update();
}
