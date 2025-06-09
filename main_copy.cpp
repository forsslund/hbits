#include <Arduino.h>
#include <Control_Surface.h>
#include <Wire.h>
#include "LEDRingSmall.h"

// Configuration flags - set to false if hardware not present
constexpr bool HAS_DRV2605 = false;  // Set to false if no haptic driver
constexpr bool HAS_FSR = false;  // Set to false if no haptic driver

#if HAS_DRV2605
#include <Adafruit_DRV2605.h>
// Initialize DRV2605L
Adafruit_DRV2605 drv;
#endif

// Variables for LED blinking
unsigned long previousMillis = 0;
const long interval = 100;  // interval at which to blink (milliseconds)

// Variables for analog reading
unsigned long previousAnalogMillis = 0;
const long analogInterval = 100;  // interval for analog reading (milliseconds)
//const uint8_t ANALOG_PIN = A0;  // Pin 19 (A0) for analog reading

// Create a MIDI interface for sending MIDI messages
BluetoothMIDI_Interface midi;

// Pin definitions
const uint8_t FSR_PIN = 19;  // Force Sensitive Resistor pin

// Motor pin definitions
const uint8_t MOTOR_PINS[4] = {9, 10, 11, 12};  // GPIO pins for motors
const uint8_t PWM_CHANNELS[4] = {0, 1, 2, 3};   // PWM channels for motors

// Motor control function
void handleMotorControl(uint8_t note, bool isOn) {
  Serial.print("MIDI Note: ");
  Serial.print(note);
  Serial.print(isOn ? " ON" : " OFF");
  
  switch(note) {
    case 60: // C4 - Motor 1 ON
      ledcWrite(PWM_CHANNELS[0], 255);
      Serial.println(" - Motor 1 ON (100%)");
      break;
    case 61: // C#4 - Motor 1 OFF
      ledcWrite(PWM_CHANNELS[0], 0);
      Serial.println(" - Motor 1 OFF (0%)");
      break;
    case 62: // D4 - Motor 2 ON
      ledcWrite(PWM_CHANNELS[1], 255);
      Serial.println(" - Motor 2 ON (100%)");
      break;
    case 63: // D#4 - Motor 2 OFF
      ledcWrite(PWM_CHANNELS[1], 0);
      Serial.println(" - Motor 2 OFF (0%)");
      break;
    case 65: // F4 - Motor 3 ON
      ledcWrite(PWM_CHANNELS[2], 255);
      Serial.println(" - Motor 3 ON (100%)");
      break;
    case 66: // F#4 - Motor 3 OFF
      ledcWrite(PWM_CHANNELS[2], 0);
      Serial.println(" - Motor 3 OFF (0%)");
      break;
    case 67: // G4 - Motor 4 ON
      ledcWrite(PWM_CHANNELS[3], 255);
      Serial.println(" - Motor 4 ON (100%)");
      break;
    case 68: // G#4 - Motor 4 OFF
      ledcWrite(PWM_CHANNELS[3], 0);
      Serial.println(" - Motor 4 OFF (0%)");
      break;
    default:
      Serial.println(" - Note not mapped to motor control");
      break;
  }
}

// Simple MIDI parsing function using low-level MIDI access
void parseMIDIInput() {
  // Process MIDI messages through the interface
  MIDIReadEvent result = midi.read();
  if (result != MIDIReadEvent::NO_MESSAGE) {
    // Get the MIDI message
    ChannelMessage msg = midi.getChannelMessage();
    
    if (msg.getMessageType() == MIDIMessageType::NoteOn || 
        msg.getMessageType() == MIDIMessageType::NoteOff) {
      uint8_t note = msg.getData1();
      uint8_t velocity = msg.getData2();
      bool isNoteOn = (msg.getMessageType() == MIDIMessageType::NoteOn) && (velocity > 0);
      
      // Handle motor control for mapped notes
      if (note >= 60 && note <= 68) {
        handleMotorControl(note, isNoteOn);
      }
    }
  }
}

// MIDI CC control for FSR
#ifdef HAS_FSR
CCPotentiometer fsr {
    A0,//FSR_PIN,           // Analog pin
    {0x07},           // CC 7 = Volume
};
#endif

// Number of LEDs in the ring
const uint8_t led_count = 24;

// Instantiate a CCAbsoluteEncoder object
CCAbsoluteEncoder enc {
  {5, 6},       // pins
  MIDI_CC::Pan, // MIDI address (CC number + optional channel)
  1,            // optional multiplier if the control isn't fast enough
};
 



/*
Board Pinout
+-----+--------+----------+-------------+
| Pin | Color  | Function | Arduino pin |
+-----+--------+----------+-------------+
|   1 | Red    | VCC      | +5V         |
|   2 | Black  | GND      | GND         |
|   3 | Yellow | VIO      | +5V         |
|   4 | Green  | SDA      | A4          |
|   5 | Blue   | SCL      | A5          |
+-----+--------+----------+-------------+
*/


LEDRingSmall ledRing(ISSI3746_SJ1 | ISSI3746_SJ5);//Only SJ2 and SJ7 are soldered in this example
// Also check if there are the pull-UP resistors on the I2C bus. If no or the board doesn't want to work please also solder the jumper SJ9

int blink=0;
void setup(void) {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println(F("Starting LED Ring test..."));
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  Wire.begin();

  for(int i=0; i<10; i++){
    Serial.println(F("Loopaaa" ));
    Serial.println(i);
    digitalWrite(LED_BUILTIN, blink);
    blink = !blink;
    delay(1000);
    }


  Serial.println(F("I2C initialized"));
  
  // Scan I2C bus
  Serial.println(F("Scanning I2C bus..."));
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print(F("I2C device found at address 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println(F("No I2C devices found!"));
  }
  Wire.setClock(400000);

  Serial.println(F("Initializing LED Ring..."));
  
  ledRing.LEDRingSmall_Reset();
  Serial.println(F("Reset done"));
  delay(20);

  ledRing.LEDRingSmall_Configuration(0x01); //Normal operation
  Serial.println(F("Configuration set to normal operation"));
  
  ledRing.LEDRingSmall_PWMFrequencyEnable(1);
  Serial.println(F("PWM frequency enabled"));
  
  ledRing.LEDRingSmall_SpreadSpectrum(0b0010110);
  Serial.println(F("Spread spectrum set"));
  
  ledRing.LEDRingSmall_GlobalCurrent(0x10);
  Serial.println(F("Global current set"));
  
  ledRing.LEDRingSmall_SetScaling(0xFF);
  Serial.println(F("Scaling set"));
  
  ledRing.LEDRingSmall_PWM_MODE();
  Serial.println(F("PWM mode enabled"));
  
  Serial.println(F("LED Ring initialization complete"));

#if HAS_DRV2605
  // Initialize DRV2605L
  Serial.println("Initializing DRV2605L...");
  if (!drv.begin()) {
    Serial.println("Could not find DRV2605L");
    while (1);
  }
  
  // Set the library to use External trigger mode
  drv.setMode(DRV2605_MODE_INTTRIG);
  
  // Set up a test pattern
  drv.selectLibrary(1);
  drv.setWaveform(0, 1);  // Strong click
  drv.setWaveform(1, 13); // Double click
  drv.setWaveform(2, 14); // Short buzz 
  drv.setWaveform(3, 0);  // End pattern

  // Play the pattern
  drv.go();
  delay(1000);  // Wait for pattern to complete

  Serial.println(F("DRV2605L haptic driver initialized"));
#else
  Serial.println(F("DRV2605L haptic driver disabled"));
#endif

  // Initialize PWM channels for motors
  Serial.println(F("Initializing PWM for motors..."));
  for (int i = 0; i < 4; i++) {
    ledcSetup(PWM_CHANNELS[i], 1000, 8); // 1kHz, 8-bit resolution
    ledcAttachPin(MOTOR_PINS[i], PWM_CHANNELS[i]);
    ledcWrite(PWM_CHANNELS[i], 0); // Start with motors off
  }
  Serial.println(F("PWM channels initialized"));

  Control_Surface.begin(); // Initialize Control Surface
  
  Serial.println(F("Control Surface initialized"));
  Serial.println(F("MIDI motor control ready!"));
  Serial.println(F("MIDI Note Mapping:"));
  Serial.println(F("C4 (60) - Motor 1 ON, C#4 (61) - Motor 1 OFF"));
  Serial.println(F("D4 (62) - Motor 2 ON, D#4 (63) - Motor 2 OFF"));
  Serial.println(F("F4 (65) - Motor 3 ON, F#4 (66) - Motor 3 OFF"));
  Serial.println(F("G4 (67) - Motor 4 ON, G#4 (68) - Motor 4 OFF"));

}

int oldValue = 0;
void loop() {
    //Serial.println(F("Loop"));
    digitalWrite(LED_BUILTIN, blink);
    blink = !blink;
  
  ledRing.LEDRingSmall_GlobalCurrent(0x10);
  ledRing.LEDRingSmall_PWM_MODE();

  int num = enc.getValue();
  if(num!=oldValue){
    Serial.print(F("Encoder value: "));
    Serial.println(num);
    oldValue = num;
  }
  if(num>24){
    num=24;
  }
  for (uint8_t i = 0; i < 24; i++) {
    ledRing.LEDRingSmall_Set_RED(i, i<num*(24.0/20.0)?0xff:0x00);
    //delay(30);
  }

    Control_Surface.loop(); // Update the Control Surface
    
    // Check for MIDI input messages
    //parseMIDIInput();

    // Read analog value from A0 every 100ms
    #ifdef HAS_FSR
    unsigned long currentMillis = millis();
    if (currentMillis - previousAnalogMillis >= analogInterval) {
        previousAnalogMillis = currentMillis;
        
        // Read raw ADC value (0-1023 for 10-bit ADC)
        //int rawValue = analogRead(ANALOG_PIN);
        int rawValue = fsr.getRawValue();
        
        // Convert to voltage (assuming 3.3V reference)
        float voltage = (rawValue / 1023.0) * 3.3;
        
        // Output to terminal
        Serial.print(F("A0 Raw through fsr: "));
        Serial.print(rawValue);
        Serial.print(F("  Voltage: "));
        Serial.print(voltage, 3);  // 3 decimal places
        Serial.println(F("V"));
    }
    #endif


}
