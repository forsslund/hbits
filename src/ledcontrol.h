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
    
    /**
     * @brief Update LED display for air level visualization with center-based display
     * 
     * Creates a center-based visualization:
     * - LED 0 (12 o'clock): Center position (CC 64 = stopped)
     * - LEDs 1-12 (clockwise): Inflation side (CC 65-127)
     * - LEDs 23-13 (counterclockwise): Deflation side (CC 63-0)
     * 
     * @param airLevel Air level (0-127 CC value, 64 = center/stopped)
     */
    void updateAirDisplay(uint8_t airLevel) {
        // Clear all LEDs first
        for (uint8_t i = 0; i < 24; i++) {
            ledRing.LEDRingSmall_Set_RGB(i, 0x000000);
        }
        
        if (airLevel == 64) {
            // Center position - light only LED 0 with white
            ledRing.LEDRingSmall_Set_RGB(0, 0xFFFFFF);
            
        } else if (airLevel > 64) {
            // Inflation mode (65-127) - counterclockwise from center
            uint8_t inflationLevel = airLevel - 64; // 1-63
            uint8_t numLEDs = (inflationLevel * 12) / 63; // Map to 0-12 LEDs
            
            // Always light center LED
            ledRing.LEDRingSmall_Set_RGB(0, 0xFFFFFF);
            
            // Light inflation LEDs (23-13 counterclockwise)
            for (uint8_t i = 1; i <= numLEDs; i++) {
                uint8_t ledIndex = (24 - i) % 24; // LED 23, 22, 21, ..., 13
                // Red color for inflation
                ledRing.LEDRingSmall_Set_RGB(ledIndex, 0xFF0000);
            }
            
        } else {
            // Deflation mode (0-63) - clockwise from center
            uint8_t deflationLevel = 63 - airLevel; // 0-63 reversed
            uint8_t numLEDs = (deflationLevel * 12) / 63; // Map to 0-12 LEDs
            
            // Always light center LED
            ledRing.LEDRingSmall_Set_RGB(0, 0xFFFFFF);
            
            // Light deflation LEDs (1-12 clockwise)
            for (uint8_t i = 1; i <= numLEDs; i++) {
                // Blue color for deflation
                ledRing.LEDRingSmall_Set_RGB(i, 0x0000FF);
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
