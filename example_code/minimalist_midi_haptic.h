#include <Arduino.h>
#include <Control_Surface.h>
#include "hapticplayer.h"

// Instantiate the two MIDI interfaces
BluetoothMIDI_Interface midibt;
USBDebugMIDI_Interface mididbg;
 
// Instantiate a MIDI pipe to connect the two interfaces
BidirectionalMIDI_Pipe mpipe;

HapticPlayer hapticPlayer;

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

void setup() {
  // Initialize Serial communication for terminal output
  Serial.begin(115200);
  delay(2000); // Give time for serial connection to establish
  Serial.println("=== MIDI Bridge Starting ===");
  Serial.println("ESP32 Nano with Control Surface Library");
  
  // Create a bidirectional route between the interfaces:
  // all input to one interface is sent to the output of the other
  midibt | mpipe | mididbg;
  

  MIDI_Interface::beginAll();
  
  midibt.setCallbacks(hapticCallback);
  
  hapticPlayer.setVolume(0.5);
  hapticPlayer.setEffect(std::make_shared<HapticEffect>(EFFECT_CONST_VIBE));
  hapticPlayer.start();
}
 
void loop() {
  // Continuously handle MIDI input
  MIDI_Interface::updateAll();
  
  // Periodic status output (every 10 seconds)
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 10000) {
    Serial.print("Status: Running for ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    lastStatusPrint = millis();
  }
  
  // Check for serial input commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
      Serial.println("MIDI Bridge Status:");
      Serial.print("  Uptime: ");
      Serial.print(millis() / 1000);
      Serial.println(" seconds");
      Serial.println("  Bluetooth MIDI: Connected");
      Serial.println("  USB Debug MIDI: Active");
    }
    else if (command == "help") {
      Serial.println("Available commands:");
      Serial.println("  status - Show bridge status");
      Serial.println("  help   - Show this help");
      Serial.println("  test   - Send test MIDI message");
    }
    else if (command == "test") {
      Serial.println("Sending test MIDI note...");
      // Send a test note on message
      midibt.sendNoteOn({60, CHANNEL_1}, 100); // Middle C, velocity 100
      delay(500);
      midibt.sendNoteOff({60, CHANNEL_1}, 0);
      Serial.println("Test MIDI message sent");
    }
    else if (command.length() > 0) {
      Serial.print("Unknown command: ");
      Serial.println(command);
      Serial.println("Type 'help' for available commands");
    }
  }
}
