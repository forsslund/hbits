#pragma once
#include <Arduino.h>
#include <Control_Surface.h>

class CLI {
private:
  Stream& serial;
  BluetoothMIDI_Interface& midiInterface;
  unsigned long lastStatusPrint;
  const unsigned long statusInterval;

public:
  CLI(Stream& serialRef, BluetoothMIDI_Interface& midiRef, unsigned long interval = 10000) 
    : serial(serialRef), midiInterface(midiRef), lastStatusPrint(0), statusInterval(interval) {}

  void begin() {
    lastStatusPrint = 0;
  }

  void update() {
    handlePeriodicStatus();
    handleSerialCommands();
  }

private:
  void handlePeriodicStatus() {
    if (millis() - lastStatusPrint > statusInterval) {
      serial.print("Status: Running for ");
      serial.print(millis() / 1000);
      serial.println(" seconds");
      lastStatusPrint = millis();
    }
  }

  void handleSerialCommands() {
    if (serial.available()) {
      String command = serial.readStringUntil('\n');
      command.trim();
      
      if (command == "status") {
        printStatus();
      }
      else if (command == "help") {
        printHelp();
      }
      else if (command == "test") {
        sendTestNote();
      }
      else if (command.length() > 0) {
        serial.print("Unknown command: ");
        serial.println(command);
        serial.println("Type 'help' for available commands");
      }
    }
  }

  void printStatus() {
    serial.println("MIDI Bridge Status:");
    serial.print("  Uptime: ");
    serial.print(millis() / 1000);
    serial.println(" seconds");
    
    // Check actual Bluetooth connection status
    serial.print("  Bluetooth MIDI: ");
    if (midiInterface.isConnected()) {
      serial.println("Connected");
    } else {
      serial.println("Disconnected");
    }
    
    serial.println("  USB Debug MIDI: Active");
    serial.println("  FSR Haptic Control: Standalone (no BT dependency)");
  }

  void printHelp() {
    serial.println("Available commands:");
    serial.println("  status - Show bridge status");
    serial.println("  help   - Show this help");
    serial.println("  test   - Send test MIDI message");
  }

  void sendTestNote() {
    serial.println("Sending test MIDI note...");
    // Send a test note on message
    midiInterface.sendNoteOn({60, Channel_1}, 100); // Middle C, velocity 100
    delay(500);
    midiInterface.sendNoteOff({60, Channel_1}, 0);
    serial.println("Test MIDI message sent");
  }
};
