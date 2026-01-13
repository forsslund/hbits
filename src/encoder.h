#pragma once
#include <Arduino.h>
#include <Control_Surface.h>
#include <memory>

// Forward declarations
struct HapticStep;
using HapticEffect = std::vector<HapticStep>;

// External effect declarations
extern const HapticEffect EFFECT_CONST_VIBE;
extern const HapticEffect EFFECT_PULSE_PURR;
extern const HapticEffect EFFECT_AUDIO_NOISE;
extern const HapticEffect EFFECT_WAVE;
extern const HapticEffect EFFECT_BRADYCARDIA_HEAVY;
extern const HapticEffect EFFECT_STRONG_BUZZ;

class EffectEncoder {
public:
    EffectEncoder() : encoder({38, 21}, MIDI_CC::Pan, 1), oldValue(0) {}
    
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
            case 1: return std::make_shared<HapticEffect>(EFFECT_PULSE_PURR);
            case 2: return std::make_shared<HapticEffect>(EFFECT_AUDIO_NOISE);
            case 3: return std::make_shared<HapticEffect>(EFFECT_WAVE);
            case 4: return std::make_shared<HapticEffect>(EFFECT_BRADYCARDIA_HEAVY);
            case 5: return std::make_shared<HapticEffect>(EFFECT_STRONG_BUZZ);
            default: return std::make_shared<HapticEffect>(EFFECT_CONST_VIBE);
        }
    }

private:
    CCAbsoluteEncoder encoder;
    int oldValue;
};
