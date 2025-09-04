#pragma once
#include <Arduino.h>
#include <Control_Surface.h>
#include <memory>

// Forward declarations
struct HapticStep;
using HapticEffect = std::vector<HapticStep>;

// External effect declarations
extern const HapticEffect EFFECT_CONST_VIBE;
extern const HapticEffect EFFECT_PULSE;
extern const HapticEffect EFFECT_RAMP_UP;
extern const HapticEffect EFFECT_TWO_PULSE;
extern const HapticEffect EFFECT_STRONG_BUZZ;
extern const HapticEffect EFFECT_PULSE_PURR;

class EffectEncoder {
public:
    EffectEncoder() : encoder({21, 38}, MIDI_CC::Pan, 1), oldValue(0) {}
    
    void begin() {
        // No additional initialization needed
    }
    
    int update() {
        int num = encoder.getValue() % 24;
        
        if (num != oldValue) {
            int effectIndex = num / 4;
            int oldEffectIndex = oldValue / 4;
            
            if (oldEffectIndex != effectIndex) {
                oldValue = num;
                return effectIndex;
            }
            oldValue = num;
        }
        return -1; // No change
    }
    
    std::shared_ptr<HapticEffect> getEffect(int effectIndex) {
        switch(effectIndex) {
            case 0: return std::make_shared<HapticEffect>(EFFECT_CONST_VIBE);
            case 1: return std::make_shared<HapticEffect>(EFFECT_PULSE);
            case 2: return std::make_shared<HapticEffect>(EFFECT_RAMP_UP);
            case 3: return std::make_shared<HapticEffect>(EFFECT_TWO_PULSE);
            case 4: return std::make_shared<HapticEffect>(EFFECT_STRONG_BUZZ);
            case 5: return std::make_shared<HapticEffect>(EFFECT_PULSE_PURR);
            default: return std::make_shared<HapticEffect>(EFFECT_CONST_VIBE);
        }
    }

private:
    CCAbsoluteEncoder encoder;
    int oldValue;
};
