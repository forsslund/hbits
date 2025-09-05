#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const midiFile = require('midi-file');

/**
 * MIDI CC22 Parser
 * Parses MIDI files and extracts Control Change messages for controller 22 (CC22)
 * with accurate timestamps.
 */

// MIDI file path (relative to project root)
const MIDI_FILE_PATH = path.join(__dirname, '..', 'haptic.mid');

/**
 * Convert MIDI ticks to milliseconds
 * @param {number} ticks - MIDI ticks
 * @param {number} ticksPerQuarter - Ticks per quarter note from MIDI header
 * @param {number} tempo - Current tempo in microseconds per quarter note
 * @returns {number} Time in milliseconds
 */
function ticksToMs(ticks, ticksPerQuarter, tempo) {
    // Default tempo is 120 BPM = 500,000 microseconds per quarter note
    const microsecondsPerQuarter = tempo || 500000;
    const microsecondsPerTick = microsecondsPerQuarter / ticksPerQuarter;
    return (ticks * microsecondsPerTick) / 1000; // Convert to milliseconds
}

/**
 * Format timestamp for display
 * @param {number} ms - Time in milliseconds
 * @returns {string} Formatted timestamp
 */
function formatTimestamp(ms) {
    const totalSeconds = ms / 1000;
    const minutes = Math.floor(totalSeconds / 60);
    const seconds = (totalSeconds % 60).toFixed(3);
    return `${minutes}:${seconds.padStart(6, '0')}`;
}

/**
 * Parse MIDI file and extract CC22 messages
 */
function parseMidiForCC22() {
    try {
        // Check if MIDI file exists
        if (!fs.existsSync(MIDI_FILE_PATH)) {
            console.error(`Error: MIDI file not found at ${MIDI_FILE_PATH}`);
            process.exit(1);
        }

        // Read and parse MIDI file
        console.log(`Parsing MIDI file: ${MIDI_FILE_PATH}`);
        const midiData = fs.readFileSync(MIDI_FILE_PATH);
        const parsed = midiFile.parseMidi(midiData);
        
        console.log(`MIDI Format: ${parsed.header.format}`);
        console.log(`Tracks: ${parsed.header.numTracks}`);
        
        // Handle different timing division formats
        const timeDivision = parsed.header.timeDivision || parsed.header.ticksPerQuarter || 480;
        console.log(`Time division: ${timeDivision} ticks per quarter note`);
        console.log('');

        let cc22Count = 0;
        let currentTempo = 500000; // Default tempo (120 BPM)

        // Process each track
        parsed.tracks.forEach((track, trackIndex) => {
            console.log(`=== Track ${trackIndex} ===`);
            let currentTicks = 0;

            track.forEach((event) => {
                currentTicks += event.deltaTime;

                // Handle tempo changes
                if (event.type === 'setTempo') {
                    currentTempo = event.microsecondsPerQuarter;
                    if (currentTempo && currentTempo > 0) {
                        const bpm = Math.round(60000000 / currentTempo);
                        console.log(`[${formatTimestamp(ticksToMs(currentTicks, timeDivision, currentTempo))}] Tempo change: ${bpm} BPM`);
                    }
                }

                // Handle Control Change messages
                if (event.type === 'controller') {
                    const { channel, controllerType, value } = event;
                    
                    // Check if this is CC22 (controller 22)
                    if (controllerType === 22) {
                        const timestamp = ticksToMs(currentTicks, timeDivision, currentTempo);
                        console.log(`[${formatTimestamp(timestamp)}] CC22 (Channel ${channel}): ${value}`);
                        cc22Count++;
                    }
                }
            });
        });

        console.log('');
        console.log(`=== Summary ===`);
        console.log(`Total CC22 messages found: ${cc22Count}`);

        if (cc22Count === 0) {
            console.log('No CC22 messages found in the MIDI file.');
            console.log('Note: CC22 refers to Control Change messages with controller number 22.');
        }

    } catch (error) {
        console.error('Error parsing MIDI file:', error.message);
        process.exit(1);
    }
}

// Main execution
if (require.main === module) {
    console.log('MIDI CC22 Parser');
    console.log('================');
    parseMidiForCC22();
}

module.exports = { parseMidiForCC22, ticksToMs, formatTimestamp };
