#include <Arduino.h>
#include <Control_Surface.h>
#include "hapticplayer.h"
#include "cli.h"

// Instantiate the MIDI interface
BluetoothMIDI_Interface midibt;

// Create a MIDI pipe for local loopback routing
MIDI_Pipe loopback_pipe;

HapticPlayer hapticPlayer;

// Instantiate CLI handler
CLI commandInterface(Serial, midibt);

// Custom MIDI callback for handling CC 22 messages to control haptic volume
struct HapticVolumeCallback : FineGrainedMIDI_Callbacks<HapticVolumeCallback> {
  void onControlChange(Channel channel, uint8_t controller, uint8_t value, Cable cable) {
    // Check if this is CC 22 (haptic volume control)
    if (controller == 22) {
      // Convert MIDI value (0-127) to haptic volume (0.0-1.0)
      float volume = value / 127.0f;
      hapticPlayer.setVolume(volume);
      
      // Optional: Print debug info
      Serial.print("CC 22 received: value=");
      Serial.print(value);
      Serial.print(", haptic volume set to: ");
      Serial.println(volume);
    }
  }
} hapticCallback;

CCPotentiometer fsr {
    A0,           // Analog pin
    {0x16},       // CC 22 (0x16 in hex)
};

void setup() {
  // Initialize Serial communication for terminal output
  Serial.begin(115200);
  
  // Set up MIDI routing:
  // 1. Set Bluetooth as default for external output
  // 2. Create loopback so FSR messages also trigger the callback
  // 3. Attach callback to Bluetooth for external input
  midibt.setAsDefault();  // Route Control Surface output to Bluetooth
  Control_Surface >> loopback_pipe >> midibt;  // Local loopback: FSR → Bluetooth → callback
  
  MIDI_Interface::beginAll();
  
  // Attach callback to Bluetooth interface to receive CC 22 messages
  midibt.setCallbacks(hapticCallback);
  
  hapticPlayer.setVolume(0);
  hapticPlayer.setEffect(std::make_shared<HapticEffect>(EFFECT_CONST_VIBE));
  hapticPlayer.start();
  
  // Initialize CLI handler
  commandInterface.begin();
  
  Serial.println("=== MIDI Haptic Controller Started ===");
  Serial.println("FSR on A0 controls haptic volume via CC 22");
  Serial.println("External CC 22 via Bluetooth also supported");
}
 
void loop() {
  // Update Control Surface (handles FSR reading and MIDI generation)
  Control_Surface.loop();
  
  // Handle CLI commands and status output
  commandInterface.update();
}
