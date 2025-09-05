# Control Surface Library Multiple MIDI Message Investigation

## Issue Summary

The Control Surface library generates multiple consecutive MIDI CC messages from a single analog input change, creating what appears to be "interpolated" values between readings. This investigation aimed to understand whether this behavior originates from the library itself or external tools.

## Initial Question

**"Do Control Surface or the potentiometer class ADD messages which interpolate between readings?"**

The user observed consecutive CC22 values (25→26→27→28→29→30...) in MIDI recordings even with a 100ms delay in the main loop, questioning whether this interpolation came from:
1. The Control Surface library
2. The signalmidi.app web-based MIDI tool

## Investigation Methodology

### 1. Code Analysis
- Examined `CCPotentiometer.hpp` - Simple wrapper around `MIDIFilteredAnalog`
- Analyzed `MIDIFilteredAnalog.hpp` - Uses `FilteredAnalog` with EMA filtering and hysteresis
- Investigated `FilteredAnalog.hpp` - Complex filtering system with multiple internal states
- Checked `ContinuousCCSender.hpp` - Simple single-message sender

### 2. Debug Implementation
Added comprehensive logging to track:
- Raw analog readings from `analogRead(A0)`
- MIDI message transmission with sequence numbers
- Millisecond and microsecond timestamps

### 3. Test Results Analysis
Debug output revealed the key findings with clear evidence.

## Key Findings

### **CONFIRMED: Control Surface Library DOES Generate Multiple MIDI Messages**

The debug output definitively proved that multiple MIDI messages originate from the ESP32, not from external tools:

```
[RAW] A0: 667 @ 17974ms / 17974878μs
[ESP32 TX #208] CC22: 28 (vol=0.220) @ 17975ms / 17975344μs
[ESP32 TX #209] CC22: 41 (vol=0.323) @ 17975ms / 17975691μs
[ESP32 TX #210] CC22: 39 (vol=0.307) @ 17975ms / 17975979μs
```

**Evidence:**
- Single analog reading produces multiple MIDI messages
- Sequential message counters prove no routing duplication
- Microsecond timestamps show rapid consecutive transmission
- Pattern consistent across all test scenarios

### **Root Cause: EMA Filter + Hysteresis System**

The Control Surface library's `FilteredAnalog` class implements:
1. **Exponential Moving Average (EMA) filtering** for noise reduction
2. **Hysteresis** to prevent value oscillation
3. **Internal state buffering** that can generate multiple discrete outputs from one analog input

When analog values change rapidly (e.g., releasing FSR pressure), the filtering algorithm creates multiple state transitions that each cross hysteresis thresholds, triggering separate MIDI messages.

## Technical Details

### Library Architecture
```
CCPotentiometer → MIDIFilteredAnalog → FilteredAnalog → EMA + Hysteresis → Multiple MIDI outputs
```

### Filter Parameters (Default)
- **Filter Shift Factor**: `ANALOG_FILTER_SHIFT_FACTOR` (typically 2-4)
- **Filter Type**: `ANALOG_FILTER_TYPE` (usually uint16_t)
- **Precision**: 7-bit for standard CC messages
- **Hysteresis**: Applied after filtering to prevent jitter

### Message Flow
1. `analogRead(A0)` provides raw input
2. EMA filter processes the reading
3. Mapping function applied (if any)
4. Hysteresis determines if change is significant
5. Multiple threshold crossings → Multiple MIDI messages
6. All messages sent within same `Control_Surface.loop()` call

## Solutions Identified

### Option 1: Adjust Filter Parameters
**Approach**: Modify Control Surface library settings
- Create custom `Settings.hpp` with `ANALOG_FILTER_SHIFT_FACTOR = 0`
- Reduces or eliminates EMA filtering
- Keeps basic hysteresis for stability

### Option 2: Custom Analog Input Class
**Approach**: Bypass FilteredAnalog system entirely
- Direct `analogRead()` implementation
- Custom simple filtering/debouncing
- Single MIDI message per significant change

### Option 3: Rate Limiting in MIDI Sink
**Approach**: Throttle output in `HapticVolumeSink`
```cpp
static unsigned long lastMidiTime = 0;
unsigned long now = millis();
if (now - lastMidiTime < 50) return; // Ignore messages < 50ms apart
lastMidiTime = now;
```

### Option 4: Simple Analog Reading
**Approach**: Replace CCPotentiometer entirely
- Manual analog reading in main loop
- Direct MIDI sending
- Complete bypass of Control Surface filtering

## Recommendations

### For Quick Fix
**Option 3 (Rate Limiting)** - Easiest implementation, maintains existing architecture

### For Optimal Control
**Option 1 (Filter Parameter Adjustment)** - Addresses root cause while preserving library benefits

### For Maximum Simplicity
**Option 4 (Manual Implementation)** - Full control but loses library advantages

## Implementation Status

- **Investigation**: ✅ Complete
- **Debug Tools**: ✅ Implemented
- **Root Cause**: ✅ Identified
- **Solutions**: ✅ Designed
- **Implementation**: ❌ Deferred

## Files Modified During Investigation

- `src/main.cpp` - Enhanced with comprehensive debug logging
  - Message sequence counters
  - Raw analog value tracking
  - Microsecond timing precision

## Next Steps

1. Choose preferred solution approach
2. Implement selected option
3. Test with FSR to verify behavior change
4. Remove debug logging if desired
5. Document final solution in code comments

## Technical Notes

- Standard MIDI does not include timestamps
- Multiple messages can be sent within same millisecond
- Control Surface pipe routing does not cause duplication
- EMA filtering is fundamental to Control Surface stability
- Completely disabling filtering may introduce noise/jitter

---

**Investigation Date**: January 2025  
**Library Version**: Control Surface (latest from GitHub main branch)  
**Hardware**: Arduino Nano ESP32  
**Conclusion**: Control Surface library intentionally generates multiple MIDI messages through its sophisticated filtering system - this is normal operation, not a bug.
