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
            
            // Map effect index to LED positions going clockwise from LED 23
            // Effect 0: LEDs 23,22,21,20  Effect 1: LEDs 19,18,17,16  etc.
            uint8_t startLED = 23 - (effectIndex * 4);
            
            for (uint8_t i = 0; i < 24; i++) {
                // Check if this LED should be lit for current effect
                bool shouldLight = false;
                for (uint8_t j = 0; j < 4; j++) {
                    uint8_t effectLED = (startLED - j + 24) % 24;
                    if (i == effectLED) {
                        shouldLight = true;
                        break;
                    }
                }
                
                if (shouldLight) {
                    ledRing.LEDRingSmall_Set_RGB(i, rainbowColors[effectIndex]);
                } else {
                    ledRing.LEDRingSmall_Set_RGB(i, 0x000000);
                }
            }
        //}
    }
    
    /**
     * @brief Update LED display for heat level visualization
     * 
     * Creates a line segment from LED 23 to current heat level with gradient (clockwise):
     * - LED 23: Blue (0x0000FF) - start position
     * - LED 11: Orange (0xFF8000) 
     * - LED 0: Red (0xFF0000) - end position
     * 
     * @param heatLevel Heat level (0-127 CC value)
     */
    void updateHeatDisplay(uint8_t heatLevel) {
        // Map heat level (0-127) to number of LEDs to light (0-23)
        uint8_t numLEDs = (heatLevel * 23) / 127;
        
        for (uint8_t i = 0; i < 24; i++) {
            // Calculate position from start (LED 23 going clockwise to LED 0)
            uint8_t positionFromStart = (24 - 1 - i) % 24; // LED 23=0, LED 22=1, ..., LED 0=23
            
            if (positionFromStart <= numLEDs) {
                // Light up LED with gradient color based on position from start
                ledRing.LEDRingSmall_Set_RGB(i, calculateHeatGradientColor(positionFromStart));
            } else {
                // Turn off LED
                ledRing.LEDRingSmall_Set_RGB(i, 0x000000);
            }
        }
    }
    
    void refresh() {
        ledRing.LEDRingSmall_GlobalCurrent(0x10);
        ledRing.LEDRingSmall_PWM_MODE();
    }

private:
    LEDRingSmall ledRing;
    uint32_t rainbowColors[6];
    int currentEffect;
    
    /**
     * @brief Calculate gradient color for heat display
     * 
     * Creates smooth gradient: Blue (LED 0) → Orange (LED 12) → Red (LED 23)
     * 
     * @param ledIndex LED position (0-23)
     * @return 24-bit RGB color value
     */
    uint32_t calculateHeatGradientColor(uint8_t ledIndex) {
        uint8_t r, g, b;
        
        if (ledIndex <= 12) {
            // First half: Blue → Orange (0x0000FF → 0xFF8000)
            float ratio = (float)ledIndex / 12.0f;
            r = (uint8_t)(0x00 + ratio * (0xFF - 0x00));
            g = (uint8_t)(0x00 + ratio * (0x80 - 0x00));
            b = (uint8_t)(0xFF - ratio * (0xFF - 0x00));
        } else {
            // Second half: Orange → Red (0xFF8000 → 0xFF0000)
            float ratio = (float)(ledIndex - 12) / 11.0f;
            r = 0xFF; // Stay at max red
            g = (uint8_t)(0x80 - ratio * (0x80 - 0x00));
            b = 0x00; // Stay at min blue
        }
        
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
};
