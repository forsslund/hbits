#include <Arduino.h>
#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#include "LEDRingSmall.h"

// Initialize DRV2605L
Adafruit_DRV2605 drv;

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

// MIDI CC control for FSR
CCPotentiometer fsr {
    A0,//FSR_PIN,           // Analog pin
    {0x07},           // CC 7 = Volume
};

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

const  uint32_t fade1_table[24] = {0x8011EE, 0xA004DA,  0xBF00BF, 0xDA04A0, 0xEE1180, 0xFB255F, 0xFF4040, 0xFB5F25, 0xEE8011, 0xDAA004, 0xBFBF00, 0xA0DA04, 0x80EE11, 0x5FFB25, 0x40FF40, 0x25FB5F, 0x11EE80, 0x04DAA0, 0x00BFBF, 0x04A0DA, 0x1180EE, 0x255FFB, 0x4040FF, 0x5F25FB};
const  uint32_t fade2_table[24] = {0xFF0000, 0xFF0004,  0xFF0020, 0xFF006B, 0xFF00FF, 0x6B00FF, 0x2000FF, 0x0400FF, 0x0000FF, 0x0004FF, 0x0020FF, 0x006BFF, 0x00FFFF, 0x00FF6B, 0x00FF20, 0x00FF04, 0x00FF00, 0x04FF00, 0x20FF00, 0x6BFF00, 0xFFFF00, 0xFF6B00, 0xFF2000, 0xFF0400};
static uint8_t j = 0;
static uint8_t i = 0;
static uint16_t tim = 0;
static uint8_t r = 5, g = 8, b = 20, rg = 40, br = 35, bg = 17;

LEDRingSmall ledRing(ISSI3746_SJ1 | ISSI3746_SJ5);//Only SJ2 and SJ7 are soldered in this example
// Also check if there are the pull-UP resistors on the I2C bus. If no or the board doesn't want to work please also solder the jumper SJ9

int blink=0;
void setup(void) {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println(F("Starting LED Ring test..."));
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  Wire.begin();

  for(int i=0; i<5; i++){
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

  Serial.println(F("DRV complete"));

  Control_Surface.begin(); // Initialize Control Surface
  
  Serial.println(F("Control Surface initialized"));

  // Configure FSR pin as input
  //pinMode(FSR_PIN, INPUT);
}

int oldValue = 0;
void loop() {
    //Serial.println(F("Loop"));
    digitalWrite(LED_BUILTIN, blink);
    blink = !blink;
    //delay(250);
    
    /*
    Serial.print(F("FSR value: "));
    Serial.print(fsr.getValue());
    Serial.print(F("  FSR raw value: "));
    Serial.println(fsr.getRawValue());
    delay(100);
*/

  
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
  for (i = 0; i < 24; i++) {
    ledRing.LEDRingSmall_Set_RED(i, i<num*(24.0/20.0)?0xff:0x00);
    //delay(30);
  }

    Control_Surface.loop(); // Update the Control Surface

    // Read analog value from A0 every 100ms
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

  /*
  for (i = 0; i < 24; i++) {
    ledRing.LEDRingSmall_Set_RED(i, 0);
    ledRing.LEDRingSmall_Set_GREEN(i, 0xff);
    delay(30);
  }

  for (i = 0; i < 24; i++) {
    ledRing.LEDRingSmall_Set_GREEN(i, 0);
    ledRing.LEDRingSmall_Set_BLUE(i, 0xff);
    delay(30);
  }

  for (i = 0; i < 24; i++) {

    ledRing.LEDRingSmall_Set_RGB(i, 0xfc6b03);
    delay(30);
  }

  for (i = 0; i < 24; i++) {

    ledRing.LEDRingSmall_Set_RGB(i, 0xfc03c6);
    delay(30);
  }
//  delay(2000);
  //LEDRingSmall.LEDRingSmall_ClearAll();




  for (tim = 0; tim < 2000; tim++) {



    if ((tim % 5) == 0) {
      ledRing.LEDRingSmall_Set_RED(r, 0);
      r = r + 1;
      if (r >= 24)
        r = 0;
      ledRing.LEDRingSmall_Set_RED(r, 0xFF);
    }

    if ((tim % 6) == 0) {
      ledRing.LEDRingSmall_Set_GREEN(g, 0);
      g = g + 1;
      if (g >= 24)
        g = 0;
      ledRing.LEDRingSmall_Set_GREEN(g, 0xFF);
    }

    if ((tim % 8) == 0) {
      ledRing.LEDRingSmall_Set_BLUE(b, 0);
      b = b + 1;
      if (b >= 24)
        b = 0;
      ledRing.LEDRingSmall_Set_BLUE(b, 0xFF);
    }


    if ((tim % 7) == 0) {
      ledRing.LEDRingSmall_Set_BLUE(br, 0);
      ledRing.LEDRingSmall_Set_RED(br, 0);
      br = br + 1;
      if (br >= 24)
        br = 0;
      ledRing.LEDRingSmall_Set_BLUE(br, 0xFF);
      ledRing.LEDRingSmall_Set_RED(br, 0xFF);
    }


    if ((tim % 10) == 0) {
      ledRing.LEDRingSmall_Set_BLUE(bg, 0);
      ledRing.LEDRingSmall_Set_GREEN(bg, 0);
      bg = bg + 1;
      if (bg >= 24)
        bg = 0;
      ledRing.LEDRingSmall_Set_BLUE(bg, 0xFF);
      ledRing.LEDRingSmall_Set_GREEN(bg, 0xFF);
    }

    if ((tim % 11) == 0) {
      ledRing.LEDRingSmall_Set_RED(rg, 0);
      ledRing.LEDRingSmall_Set_GREEN(rg, 0);
      rg = rg + 1;
      if (rg >= 24)
        rg = 0;
      ledRing.LEDRingSmall_Set_RED(rg, 0xFF);
      ledRing.LEDRingSmall_Set_GREEN(rg, 0xFF);
    }

    delay(10);
  }


  for (tim = 0; tim < 100; tim++) {
    j = tim % 24;

    for (i = 0; i < 24; i++) {
      ledRing.LEDRingSmall_Set_RGB(i, fade1_table[j]);
      j++;
      if (j >= 24)
        j = 0;
    }
    delay(40);
  }

  for (tim = 0; tim < 100; tim++) {
    j = tim % 24;
    for (i = 0; i < 24; i++) {
      ledRing.LEDRingSmall_Set_RGB(i, fade2_table[j]);
      j++;
      if (j >= 24)
        j = 0;
    }
    delay(40);
  }

  for (i = 0xff; i > 0; i--) {

    ledRing.LEDRingSmall_GlobalCurrent(i);
    delay(20);
  }
  ledRing.LEDRingSmall_ClearAll();
  ledRing.LEDRingSmall_GlobalCurrent(0xff);
*/

}
