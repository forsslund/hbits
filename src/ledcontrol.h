#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "LEDRingSmall.h"

class LEDController {
public:
    LEDController() : ledRing(ISSI3746_SJ2 | ISSI3746_SJ7), currentEffect(0) {
        // Rainbow colors for each effect
        rainbowColors[0] = 0xFF0000; // Red - CONST_VIBE
        rainbowColors[1] = 0xFF8000; // Orange - PULSE  
        rainbowColors[2] = 0xFFFF00; // Yellow - RAMP_UP
        rainbowColors[3] = 0x00FF00; // Green - TWO_PULSE
        rainbowColors[4] = 0x0080FF; // Blue - STRONG_BUZZ
        rainbowColors[5] = 0x8000FF; // Purple - PULSE_PURR
    }
    
    void begin() {
        Wire.begin();
        Wire.setClock(400000);
        
        ledRing.LEDRingSmall_Reset();
        delay(20);
        
        ledRing.LEDRingSmall_Configuration(0x01);
        ledRing.LEDRingSmall_PWMFrequencyEnable(1);
        ledRing.LEDRingSmall_SpreadSpectrum(0b0010110);
        ledRing.LEDRingSmall_GlobalCurrent(0x10);
        ledRing.LEDRingSmall_SetScaling(0xFF);
        ledRing.LEDRingSmall_PWM_MODE();
        
        updateDisplay(0);
    }
    
    void updateDisplay(int effectIndex) {
        //if (effectIndex != currentEffect) {
            currentEffect = effectIndex;
            
            for (uint8_t i = 0; i < 24; i++) {
                if (i >= effectIndex * 4 && i < (effectIndex + 1) * 4) {
                    ledRing.LEDRingSmall_Set_RGB(i, rainbowColors[effectIndex]);
                } else {
                    ledRing.LEDRingSmall_Set_RGB(i, 0x000000);
                }
            }
        //}
    }
    
    void refresh() {
        ledRing.LEDRingSmall_GlobalCurrent(0x10);
        ledRing.LEDRingSmall_PWM_MODE();
    }

private:
    LEDRingSmall ledRing;
    uint32_t rainbowColors[6];
    int currentEffect;
};
