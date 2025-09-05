# MIDI CC22 Parser

A Node.js tool for parsing MIDI files and extracting Control Change messages for controller 22 (CC22) with accurate timestamps.

## Overview

This tool parses the `haptic.mid` file in the project root and extracts all CC22 (Control Change controller 22) messages, displaying them with precise timestamps in milliseconds.

## Features

- Parses Standard MIDI files (Format 0 and 1)
- Extracts CC22 control change messages specifically
- Calculates accurate timestamps from MIDI ticks
- Handles tempo changes for precise timing
- Displays results in a readable format with timestamps

## Installation

```bash
cd tools
npm install
```

## Usage

### Run the parser:
```bash
cd tools
node midi-parser.js
```

### Or use npm script:
```bash
cd tools
npm start
```

## Output Format

The tool outputs:
- MIDI file information (format, tracks, time division)
- Track-by-track analysis
- CC22 messages with timestamps in format `[M:SS.mmm] CC22 (Channel X): value`
- Summary with total count of CC22 messages found

### Example Output:
```
MIDI CC22 Parser
================
Parsing MIDI file: /home/jonas/hbits/haptic.mid
MIDI Format: 1
Tracks: 2
Time division: 480 ticks per quarter note

=== Track 0 ===
[0:00.000] Tempo change: 120 BPM

=== Track 1 ===
[0:00.550] CC22 (Channel 0): 1
[0:00.550] CC22 (Channel 0): 2
[0:00.660] CC22 (Channel 0): 3
...

=== Summary ===
Total CC22 messages found: 378
```

## Technical Details

### MIDI CC22
- CC22 refers to Control Change messages with controller number 22
- These are MIDI messages with status bytes 0xB0-0xBF (control change)
- Data byte 1 = 22 (controller number)
- Data byte 2 = 0-127 (controller value)

### Timestamp Calculation
- Converts MIDI ticks to milliseconds using the file's time division
- Accounts for tempo changes throughout the file
- Default tempo: 120 BPM (500,000 microseconds per quarter note)
- Formula: `(ticks × microsecondsPerQuarter) / timeDivision / 1000`

### Dependencies
- `midi-file`: Lightweight MIDI file parser for Node.js

## File Structure
```
tools/
├── package.json          # Node.js project configuration
├── midi-parser.js         # Main parser script
└── README.md             # This documentation
```

## Requirements
- Node.js (any recent version)
- The `haptic.mid` file in the project root directory

## Notes
- The tool automatically looks for `haptic.mid` in the parent directory
- If no CC22 messages are found, the tool will notify you
- The parser handles both tempo changes and multiple tracks correctly
