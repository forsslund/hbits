#include <Arduino.h>
#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#include "LEDRingSmall.h"
#include <vector>
#include <memory>

#include "hapticeffects.h"

// Configuration flags - comment out if hardware not present
#define HAS_DRV2605   // comment out if no haptic driver
#define HAS_FSR       // comment out if no FSR sensor

#ifdef HAS_DRV2605
#include <Adafruit_DRV2605.h>
// Initialize DRV2605L
Adafruit_DRV2605 drv;

const float LRA_TARGET_HZ = 175.0;   // change to your actuator's resonance (or what you want to drive at)

// helper: compute OL_LRA_PERIOD (7-bit) from target Hz
uint8_t olPeriodFromHz(float f) {
  long v = lround(1000000.0 / (98.46 * f));
  if (v < 0)   v = 0;
  if (v > 127) v = 127;
  return (uint8_t)v;
}

float hapticVolume = 0.0;  // Default volume (0.0 to 1.0)

// ----- HapticPlayer class -----
class HapticPlayer {
public:
    HapticPlayer(Adafruit_DRV2605& drvRef, BaseType_t core = 0)
        : drv(drvRef), coreId(core), hapticVolume(1.0f), lastRealtimeValue(0) {
        // Start with empty effect
        currentEffect = std::make_shared<HapticEffect>();
    }

    void start() {
        // Set DRV to realtime mode for continuous playback in open-loop for LRA
        // --- 1) Tell the chip it's an LRA (not ERM): FEEDBACK (0x1A), set bit7 ---
        uint8_t fb = drv.readRegister8(0x1A);      
        drv.writeRegister8(0x1A, fb | 0x80);       // N_ERM_LRA = 1 (LRA)

        // --- 2) CONTROL3 (0x1D): LRA open-loop, unsigned RTP (0..127) ---
        uint8_t c3 = drv.readRegister8(0x1D);      
        c3 |= 0x01;                                // LRA_OPEN_LOOP = 1
        c3 &= ~(1 << 5);                           // DATA_FORMAT_RTP = 0 (unsigned)
        drv.writeRegister8(0x1D, c3);

        // --- 3) Set open-loop LRA frequency ---
        uint8_t ol = olPeriodFromHz(LRA_TARGET_HZ);
        drv.writeRegister8(0x20, ol);

        // --- 4) Set output clamp ---
        drv.writeRegister8(0x17, 0x60);            

        // --- 5) Enter RTP mode ---
        drv.writeRegister8(0x01, 0x05);
        
        Serial.println(F("Starting haptic background task..."));
        
        xTaskCreatePinnedToCore(
            [](void* param) {
                auto self = static_cast<HapticPlayer*>(param);
                Serial.println(F("Haptic task started on core 0"));
                
                while (true) {
                    auto effect = self->currentEffect; // Get current effect (atomic)
                    
                    if (effect && !effect->empty()) {
                        // Play through the entire effect
                        for (const auto& step : *effect) {
                            // Scale amplitude by volume
                            uint8_t scaledAmp = static_cast<uint8_t>(step.amplitude * self->hapticVolume);
                            self->drv.setRealtimeValue(scaledAmp);
                            self->lastRealtimeValue = scaledAmp;  // Store the value for debug access
                            
                            vTaskDelay(pdMS_TO_TICKS(step.delayMs));
                        }
                    } else {
                        // No effect or empty effect, just wait
                        self->drv.setRealtimeValue(0);
                        self->lastRealtimeValue = 0;  // Store the value for debug access
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
            },
            "HapticTask",
            4096,  // Stack size
            this,
            1,     // Priority
            nullptr,
            coreId
        );
    }

    void setEffect(std::shared_ptr<HapticEffect> effect) {
        currentEffect = effect; // Atomic pointer assignment
        Serial.println(F("Haptic effect changed"));
    }

    void setVolume(float vol) {
        hapticVolume = constrain(vol, 0.0f, 1.0f);
        //Serial.print(F("Haptic volume set to: "));
        //Serial.println(hapticVolume);
    }

    float getVolume() const {
        return hapticVolume;
    }

    uint8_t getLastSetRealtimeValue() const {
        return lastRealtimeValue;
    }

private:
    Adafruit_DRV2605& drv;
    BaseType_t coreId;
    volatile float hapticVolume;
    volatile uint8_t lastRealtimeValue;
    std::shared_ptr<HapticEffect> currentEffect;
};

// Global haptic player instance
HapticPlayer player(drv, 0);

#endif

// MIDI CC control for FSR
#ifdef HAS_FSR
CCPotentiometer fsr {
    A0,           // Analog pin
    {0x16},       // CC 22 (0x16 in hex)
};
#endif

// Variables for LED blinking
unsigned long previousMillis = 0;
const long interval = 100;  // interval at which to blink (milliseconds)

// Variables for analog reading
unsigned long previousAnalogMillis = 0;
const long analogInterval = 50;  // interval for analog reading (milliseconds)

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



void updateLedRing(int effectIndex) {
  // Rainbow colors for each effect (6 colors for 6 effects)
  uint32_t rainbowColors[6] = {
    0xFF0000, // Red - Effect 0 (CONST_VIBE)
    0xFF8000, // Orange - Effect 1 (PULSE)  
    0xFFFF00, // Yellow - Effect 2 (RAMP_UP)
    0x00FF00, // Green - Effect 3 (TWO_PULSE)
    0x0080FF, // Blue - Effect 4 (STRONG_BUZZ)
    0x8000FF  // Purple - Effect 5 (PULSE_PURR)
  };
  
  for (uint8_t i = 0; i < 24; i++) {
    if (i >= effectIndex * 4 && i < (effectIndex + 1) * 4) {
      // Light up the 4 LEDs for current effect with rainbow color
      ledRing.LEDRingSmall_Set_RGB(i, rainbowColors[effectIndex]);
    } else {
      // Turn off other LEDs
      ledRing.LEDRingSmall_Set_RGB(i, 0x000000);
    }
  }
}

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
  ledRing.LEDRingSmall_PWMFrequencyEnable(1);
  ledRing.LEDRingSmall_SpreadSpectrum(0b0010110);
  ledRing.LEDRingSmall_GlobalCurrent(0x10);
  ledRing.LEDRingSmall_SetScaling(0xFF);
  ledRing.LEDRingSmall_PWM_MODE();
  
  Serial.println(F("LED Ring initialization complete"));

#ifdef HAS_DRV2605
  // Initialize DRV2605L
  Serial.println("Initializing DRV2605L...");
  if (!drv.begin()) {
    Serial.println("Could not find DRV2605L");
    while (1);
  }
  
  Serial.println(F("DRV2605L haptic driver initialized"));
  
  // Set initial haptic volume
  player.setVolume(hapticVolume);
  player.setEffect(std::make_shared<HapticEffect>(EFFECT_CONST_VIBE));
  
  // Start the background haptic task
  player.start();
  
  Serial.println(F("Haptic system ready"));
#else
  Serial.println(F("DRV2605L haptic driver disabled"));
#endif

  // Set custom Bluetooth MIDI device name (must be called before Control_Surface.begin())
  midi.setName("HBITS Vibe 1");
  Serial.println(F("Bluetooth device name set to: HBITS Vibe 1"));

  Control_Surface.begin(); // Initialize Control Surface
  
  Serial.println(F("Control Surface initialized"));

  updateLedRing(0);
}

int oldValue = 0;
void loop() {
  ledRing.LEDRingSmall_GlobalCurrent(0x10);
  ledRing.LEDRingSmall_PWM_MODE();

  // Use encoder to switch haptic effects
  int num = enc.getValue();
  // Rotate
  num=num%24;

  if(num!=oldValue){
    Serial.print(F("Encoder value: "));
    Serial.println(num);
    
    #ifdef HAS_DRV2605
    // Map encoder value to different effects (0-23 -> 0-5 effects)
    // Use integer division for even distribution: 4 steps per effect
    int effectIndex = num / 4;
    
    int oldEffectIndex = oldValue / 4;

    if(oldEffectIndex != effectIndex){    
      switch(effectIndex) {
        case 0:
          player.setEffect(std::make_shared<HapticEffect>(EFFECT_CONST_VIBE));
          Serial.println(F("Effect: 0. CONST_VIBE"));      
          break;
        case 1:
          player.setEffect(std::make_shared<HapticEffect>(EFFECT_PULSE));
          Serial.println(F("Effect: 1. PULSE"));
          break;
        case 2:
          player.setEffect(std::make_shared<HapticEffect>(EFFECT_RAMP_UP));
          Serial.println(F("Effect: 2. RAMP_UP"));
          break;
        case 3:
          player.setEffect(std::make_shared<HapticEffect>(EFFECT_TWO_PULSE));
          Serial.println(F("Effect: 3. TWO_PULSE"));
          break;
        case 4:
          player.setEffect(std::make_shared<HapticEffect>(EFFECT_STRONG_BUZZ));
          Serial.println(F("Effect: 4. STRONG_BUZZ"));
          break;
        case 5:
          player.setEffect(std::make_shared<HapticEffect>(EFFECT_PULSE_PURR));
          Serial.println(F("Effect: 5. PULSE_PURR"));
          break;
      }
    }
    #endif
    
    oldValue = num;
    updateLedRing(effectIndex);
  }
  if(num>24){
    num=24;
  }

  for (uint8_t i = 0; i < 24; i++) {
    // Set LED color based on encoder value (1 step = 1 additional led light)
    //ledRing.LEDRingSmall_Set_RED(i, i<num?0xff:0x00);    
  }

    Control_Surface.loop(); // Update the Control Surface

    // Read analog value from A0 every 100ms and control haptic volume
    #ifdef HAS_FSR
    unsigned long currentMillis = millis();
    if (currentMillis - previousAnalogMillis >= analogInterval) {
        previousAnalogMillis = currentMillis;
        
        // Read raw ADC value (0-1023 for 10-bit ADC)
        int rawValue = fsr.getRawValue();
        
        #ifdef HAS_DRV2605
        // Map FSR pressure to haptic volume (0-8200 -> 0.0-1.0)
        // Note: the FSR reading value is mathematically improved by Control Surface
        //       library to allow for a larger range. We have seen values up above 10000.
        float newVolume = map(rawValue, 0, 8200, 0, 100) / 100.0f;
        newVolume = constrain(newVolume, 0.0f, 1.0f);
        
        // Only update if volume changed significantly (avoid constant updates)
        if (abs(newVolume - player.getVolume()) > 0.05f) {
          player.setVolume(newVolume);
        }
        #endif
        
        // Output to terminal with haptic debug info
        //Serial.print(F("A0 Raw through fsr: "));
        //Serial.print(rawValue);
        //Serial.print(F("=Volume: "));
        //Serial.print(newVolume);        
        //#ifdef HAS_DRV2605
        //Serial.print(F("  Haptic: "));
        //Serial.print(player.getLastSetRealtimeValue());
        //#endif        
        //Serial.println();
    }
    #endif

}
