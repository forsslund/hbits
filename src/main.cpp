#include <Arduino.h>
#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#include "LEDRingSmall.h"

// Configuration flags - comment out if hardware not present
#define HAS_DRV2605   // comment out if no haptic driver
#define HAS_FSR       // comment out if no FSR sensor
//#define HAS_PUMP    // comment out if no pumps/valves

#ifdef HAS_DRV2605
#include <Adafruit_DRV2605.h>
// Initialize DRV2605L
Adafruit_DRV2605 drv;

float hapticVolume = 0.5;  // Default volume (0.0 to 1.0)

struct HapticStep {
    uint8_t amplitude; // 0-255
    uint16_t delayMs;  // hur länge denna amplitude ska spelas
};

using HapticEffect = std::vector<HapticStep>;
const HapticEffect EFFECT_PULSE_PURR = {
    {21, 10}, {0, 10},
    {21, 10}, {0, 10},
    {21, 10}, {0, 10},
    {1, 10},  {0, 1000}
};
#endif

#ifdef HAS_PUMP
// Motor pin definitions
const uint8_t MOTOR_PINS[4] = {18, 17, 10, 9};  // GPIO pins for motors {9, 10, 17, 18};
const uint8_t PWM_CHANNELS[4] = {0, 1, 2, 3};   // PWM channels for motors
#endif


// MIDI CC control for FSR
#ifdef HAS_FSR
CCPotentiometer fsr {
    A0,           // Analog pin
    {0x07},       // CC 7 = Volume
};
#endif

// Variables for LED blinking
unsigned long previousMillis = 0;
const long interval = 100;  // interval at which to blink (milliseconds)

// Variables for analog reading
unsigned long previousAnalogMillis = 0;
const long analogInterval = 100;  // interval for analog reading (milliseconds)

// Create a MIDI interface for sending MIDI messages
BluetoothMIDI_Interface midi;

// Number of LEDs in the ring
const uint8_t led_count = 24;

// Instantiate a CCAbsoluteEncoder object
CCAbsoluteEncoder enc {
  {21, 38},     // pins
  MIDI_CC::Pan, // MIDI address (CC number + optional channel)
  1,           // optional multiplier if the control isn't fast enough
};
 

//LEDRingSmall ledRing(ISSI3746_SJ1 | ISSI3746_SJ5);//no jumpers are soldered in this example
LEDRingSmall ledRing(ISSI3746_SJ2 | ISSI3746_SJ7);//Only SJ2 and SJ7 are soldered in this example
// Also check if there are the pull-UP resistors on the I2C bus. If no or the board doesn't want to work please also solder the jumper SJ9

int blink=0;
void setup(void) {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println(F("Starting LED Ring test..."));
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  Wire.begin();

  for(int i=0; i<5; i++){
    Serial.println(F("Loop" ));
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

#ifdef HAS_DRV2605
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

#ifdef HAS_PUMP
  // Initialize PWM channels for motors
  Serial.println(F("Initializing PWM for motors..."));
  for (int i = 0; i < 4; i++) {
    ledcSetup(PWM_CHANNELS[i], 1000, 8); // 1kHz, 8-bit resolution
    ledcAttachPin(MOTOR_PINS[i], PWM_CHANNELS[i]);
    ledcWrite(PWM_CHANNELS[i], 0); // Start with motors off
  }
  Serial.println(F("PWM channels initialized"));
#endif

  // Set custom Bluetooth MIDI device name (must be called before Control_Surface.begin())
  midi.setName("HBITS Vibe 1");
  Serial.println(F("Bluetooth device name set to: HBITS Vibe 1"));

  Control_Surface.begin(); // Initialize Control Surface
  
  Serial.println(F("Control Surface initialized"));

}

int oldValue = 0;
void loop() {


  #ifdef HAS_PUMP
    //Serial.println(F("Loop"));
    digitalWrite(LED_BUILTIN, 1);
    //blink = !blink;
    Serial.print(F("Inflate! "));
    ledcWrite(PWM_CHANNELS[0], 255); // M1 pump inflate (GPIO 18)
    ledcWrite(PWM_CHANNELS[1], 0);   // M2 pump deflate (GPIO 17)
    ledcWrite(PWM_CHANNELS[2], 255); // M3 valve inflate (GPIO 10)
    ledcWrite(PWM_CHANNELS[3], 0);   // M4 vlalve deflate (GPIO 9)

    delay(1000);
    Serial.print(F("1 "));
    delay(1000);
    Serial.print(F(" 2 "));
    delay(1000);
    Serial.println(F(" 3 "));

    digitalWrite(LED_BUILTIN, 0);
    Serial.print(F("Deflate! "));

    ledcWrite(PWM_CHANNELS[0], 0);    // M1 pump inflate (GPIO 18)
    ledcWrite(PWM_CHANNELS[1], 255);  // M2 pump deflate (GPIO 17)
    ledcWrite(PWM_CHANNELS[2], 0);    // M3 valve inflate (GPIO 10)
    ledcWrite(PWM_CHANNELS[3], 255);  // M4 vlalve deflate (GPIO 9)

    delay(1000);
    Serial.print(F("1 "));
    delay(1000);
    Serial.print(F(" 2 "));
    delay(1000);
    Serial.println(F(" 3 "));
#else
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
    ledRing.LEDRingSmall_Set_RED(i, i<num?0xff:0x00);
  }

    Control_Surface.loop(); // Update the Control Surface

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
#endif
}
