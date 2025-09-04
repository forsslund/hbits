#include <Arduino.h>
#include <Adafruit_DRV2605.h>
#include <vector>
#include <memory>

#include "hapticeffects.h"

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
    HapticPlayer(BaseType_t core = 0)
        : coreId(core), hapticVolume(1.0f), lastRealtimeValue(0) {
        // Start with empty effect
        currentEffect = std::make_shared<HapticEffect>();
    }

    void start() {

        // Initialize DRV2605L
        Serial.println("Initializing DRV2605L...");
        if (!drv.begin()) {
          Serial.println("Could not find DRV2605L");
          while (1);
        }


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
    Adafruit_DRV2605 drv;
    BaseType_t coreId;
    volatile float hapticVolume;
    volatile uint8_t lastRealtimeValue;
    std::shared_ptr<HapticEffect> currentEffect;
};


  // Declare player (global)
  //HapticPlayer player(0);

  // Init (in setup())
  // Set initial haptic volume
  //player.setVolume(hapticVolume);
  //player.setEffect(std::make_shared<HapticEffect>(EFFECT_CONST_VIBE));
  
  // Start the background haptic task
  //player.start();
