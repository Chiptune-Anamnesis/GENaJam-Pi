extern uint8_t viz_sub_mode;  // 0=bar graph, 1=asteroids
extern bool system_ready;    // MIDI blocking flag during boot/setup

void setup_midi(void) {
    // Set UART pins first
    Serial1.setTX(MIDI_TX_PIN);
    Serial1.setRX(MIDI_RX_PIN);
    
    // Initialize Serial1 manually at MIDI baud rate BEFORE calling MIDI.begin()
    // This prevents the MIDI library from hanging during UART initialization
    Serial1.begin(MIDI_BAUD_RATE);
    delay(100);  // UART stabilization
    
    // Initialize MIDI library
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
    
    // Set handlers to call your existing functions directly
    MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
        handle_note_on(channel, note, velocity);
    });
    
    MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
        handle_note_off(channel, note, velocity);
    });
    
    MIDI.setHandleControlChange([](byte channel, byte cc, byte value) {
        handle_control_change(channel, cc, value);
    });
    
    MIDI.setHandlePitchBend([](byte channel, int bend) {
        handle_pitch_bend(channel, (int16_t)bend);
    });
    
    // Forward Program Change & Aftertouch for channels outside 1..6
    MIDI.setHandleProgramChange([](byte channel, byte number) {
        if (channel < 1 || channel > 6) MIDI.sendProgramChange(number, channel);
    });
    MIDI.setHandleAfterTouchChannel([](byte channel, byte pressure) {
        if (channel < 1 || channel > 6) MIDI.sendAfterTouch(pressure, channel);
    });
    MIDI.setHandleAfterTouchPoly([](byte channel, byte note, byte pressure) {
        if (channel < 1 || channel > 6) MIDI.sendAfterTouch(note, pressure, channel);
    });
// USB MIDI initialization handled in main setup
}

void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    // Validate inputs before sending
    if (channel < 1 || channel > 16) return;
    if (cc > 127) return;
    if (value > 127) return;

    MIDI.sendControlChange(cc, value, channel);
}

void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Validate inputs before sending
    if (channel < 1 || channel > 16) return;
    if (note > 127) return;
    if (velocity > 127) return;

    MIDI.sendNoteOn(note, velocity, channel);
}

void midi_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Validate inputs before sending
    if (channel < 1 || channel > 16) return;
    if (note > 127) return;
    if (velocity > 127) return;

    MIDI.sendNoteOff(note, velocity, channel);
}

void midi_send_pitch_bend(uint8_t channel, int16_t bend) {
    MIDI.sendPitchBend(bend, channel);
}

void handle_midi_input(void) {
    // BLOCK MIDI processing during boot/setup to prevent audio glitches
    if (!system_ready) {
        // Drain MIDI buffers to prevent overflow
        MIDI.read();  // Drain UART MIDI buffer
        if (usb_midi.available()) {
            uint8_t packet[4];
            usb_midi.readPacket(packet);  // Drain USB MIDI buffer
        }
        return;  // Exit early - no MIDI processing during boot
    }

    // Grace period after system becomes ready - continue draining buffers
    static unsigned long system_ready_time = 0;
    if (system_ready_time == 0) {
        system_ready_time = millis();  // Record when system became ready
    }

    if (millis() - system_ready_time < 500) {  // 500ms grace period
        // Continue draining but don't process for first 500ms after ready
        MIDI.read();
        while (usb_midi.available()) {
            uint8_t packet[4];
            usb_midi.readPacket(packet);
        }
        return;
    }

    static unsigned long last_meaningful_time = 0;
    static bool had_midi_activity = false;

    // Check if there's actual UART MIDI data
    bool uart_midi_available = MIDI.read();
    
    // Check for USB MIDI data
    bool usb_midi_available = false;
    if (usb_midi.available()) {
        uint8_t packet[4];
        usb_midi.readPacket(packet);
        
        uint8_t cable = (packet[0] >> 4) & 0x0F;
        uint8_t code = packet[0] & 0x0F;
        uint8_t channel = packet[1] & 0x0F;
        
        // Parse USB MIDI packet and call handlers directly
        switch (code) {
            case 0x09: // Note On
                if (packet[3] > 0) {
                    handle_note_on(channel + 1, packet[2], packet[3]);
                } else {
                    handle_note_off(channel + 1, packet[2], packet[3]);
                }
                usb_midi_available = true;
                break;
                
            case 0x08: // Note Off
                handle_note_off(channel + 1, packet[2], packet[3]);
                usb_midi_available = true;
                break;
                
            case 0x0B: // Control Change
                handle_control_change(channel + 1, packet[2], packet[3]);
                usb_midi_available = true;
                break;
                
            case 0x0E: // Pitch Bend
                int16_t bend = (packet[3] << 7) | packet[2];
                bend -= 8192; // Convert to signed
                handle_pitch_bend(channel + 1, bend);
                usb_midi_available = true;
                break;
        }
    }
    
    // Only measure timing when there's actual MIDI activity from either source
    if (uart_midi_available || usb_midi_available) {
        unsigned long now = millis();
        if (last_meaningful_time > 0) {
            unsigned long gap = now - last_meaningful_time;
            if (gap > max_midi_gap) {
                max_midi_gap = gap;
            }
        }
        last_meaningful_time = now;
        midi_process_count++;
        had_midi_activity = true;
    }
    
    // Reset stats every 2 seconds
    static unsigned long last_reset = 0;
    unsigned long now = millis();
    if (now - last_reset > 2000) {
        midi_hz = midi_process_count / 2.0;
        midi_process_count = 0;
        max_midi_gap = 0;
        last_reset = now;
    }
}

void handle_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Bounds checking for channel and note
    if (channel < 1 || channel > 16 || note > 127) return;

    // Only update displays when actually in those modes
    if (mode == 1 || mode == 3) {
        // MIDI display updates handled elsewhere
    }

    // Only update visualizer when in visualizer modes
    if ((mode == 5 || mode == 6) && channel >= 1 && channel <= 11) {
        updateVelocityViz(channel - 1, velocity);
    }

    // Handle asteroids demo MIDI integration
    if ((mode == 5 || mode == 6) && viz_sub_mode == 1 && channel >= 1 && channel <= 6) {
        handleMidiNoteForAsteroids(note, velocity);
    }

    // Handle starfighter demo MIDI integration
    if ((mode == 5 || mode == 6) && viz_sub_mode == 2 && channel >= 1 && channel <= 6) {
        handleMidiNoteForStarfighter(note, velocity);
    }

    // Handle neural network demo MIDI integration
    if ((mode == 5 || mode == 6) && viz_sub_mode == 3 && channel >= 1 && channel <= 6) {
        handleMidiNoteForNeuralNet(note, velocity);
    }
    
    // Apply configurable velocity curve for more musical response
    velocity = applyVelocityCurve(velocity);
    
    bool repeatnote = false;
    
    if (mode == 3 || mode == 4 || mode == 6) { // if we're in poly mode
        if (channel == midichannel) {  // and is set to the global midi channel
            
            // Handle repeat notes - retrigger if same note is already playing
            for (int i = 0; i < 6; i++) {
                if (note == polynote[SAFE_POLY_INDEX(i)]) {
                    
                    if (polypan > 64) {
                        long randpan = random(33, 127);
                        midi_send_cc(i + 1, 77, randpan);
                    }
                    
                    midi_send_note_off(i + 1, polynote[i], velocity);
                    midi_send_note_on(i + 1, note, velocity);
                    noteheld[i] = true;
                    repeatnote = true;
                    break;
                }
                handle_midi_input();
            }
            
            if (!repeatnote) {
                // Find the lowest note to protect it from voice stealing
                lowestnote = polynote[0];
                for (int i = 0; i < 6 && i < sizeof(polynote); i++) {
                    if (polynote[i] < lowestnote && polynote[i] != 0) {
                        lowestnote = polynote[i];
                    }
                    handle_midi_input();
                }
                
                // Pick a random channel for voice stealing
                long randchannel = random(0, 6);
                
                // Don't steal the lowest note
                if (polynote[randchannel] == lowestnote) {
                    randchannel++;
                    if (randchannel == 6) randchannel = 0;
                }
                
                // First, look for empty voice slots
                for (int i = 0; i < 6 && i < sizeof(polynote); i++) {
                    if (polynote[i] == 0) {
                        randchannel = i;
                        break;
                    }
                    handle_midi_input();
                }
                
                if (polypan > 64) {
                    long randpan = random(33, 127);
                    midi_send_cc(randchannel + 1, 77, randpan);
                }
                
                if (polynote[randchannel] != 0) {
                    midi_send_note_off(randchannel + 1, polynote[randchannel], velocity);
                }
                midi_send_note_on(randchannel + 1, note, velocity);
                
                polynote[randchannel] = note;
                polyon[randchannel] = true;
                noteheld[randchannel] = true;
            }
            
        }
} else {
    // Mono mode or pass-through
    if (channel >= 1 && channel <= 6) {
        // Turn off previous note if any (true mono behavior)
        if (polyon[channel-1] && polynote[channel-1] != note) {
            midi_send_note_off(channel, polynote[channel-1], 0);
        }
        
        polyon[channel-1] = true;
        polynote[channel-1] = note;  // Track the current note
        noteheld[channel-1] = true;
        midi_send_note_on(channel, note, velocity);
    } else {
        // Pass other channels straight through to GENMDM
        MIDI.sendNoteOn(note, velocity, channel);
    }
}
}

void handle_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Bounds checking for channel and note
    if (channel < 1 || channel > 16 || note > 127) return;

    // Only clear visualizer when in visualizer modes
    if ((mode == 5 || mode == 6) && channel >= 1 && channel <= 11) {
        clearVelocityViz(channel - 1);
    }
    
    // Handle polyphonic and monophonic note-off logic
    if (mode == 3 || mode == 4 || mode == 6) {
        if (channel == midichannel) {
            
            for (int i = 0; i < 6 && i < sizeof(polynote); i++) {
                handle_midi_input();
                if (note == polynote[i]) {
                    
                    if (sustain) {
                        sustainon[i] = true;
                        noteheld[i] = false;
                        break;
                    } else {
                        midi_send_note_off(i + 1, note, velocity);
                        polyon[i] = false;
                        polynote[i] = 0;
                        noteheld[i] = false;
                        break;
                    }
                }
            }            
        }
    } else {
    // Mono mode
    if (channel >= 1 && channel <= 6) {
        // Only turn off if this note matches the currently playing note
        if (polynote[channel-1] == note) {
            if (sustain && sustainon[channel-1]) {
                noteheld[channel-1] = false;
                return;
            }
            polyon[channel-1] = false;
            polynote[channel-1] = 0;  // Clear the tracked note
            noteheld[channel-1] = false;
            midi_send_note_off(channel, note, velocity);
        }
        // If the note-off doesn't match the current note, ignore it
    } else {
        MIDI.sendNoteOff(note, velocity, channel);
    }
}
}

void handle_control_change(uint8_t channel, uint8_t cc, uint8_t value) {
    if (cc == 64) { // Sustain pedal
        if (value == 0) { // sustain pedal released
            sustain = false;
            
            if (mode == 3 || mode == 4) { // poly mode
                for (int i = 5; i >= 0; i--) { // Scan for sustained channels
                    handle_midi_input();
                    if (!noteheld[i] && sustainon[i]) { // if key not held but sustained
                        midi_send_note_off(i + 1, polynote[i], 0); // turn that voice off
                        sustainon[i] = false; // turn off sustain on that channel
                        polyon[i] = false; // turn voice off
                        polynote[i] = 0; // clear the pitch on that channel
                    }
                }
            } else { // mono mode
                for (int i = 0; i < 6; i++) {
                    sustainon[i] = false;
                    if (!noteheld[i] && polyon[i]) {
                        midi_send_note_off(i + 1, polynote[i], 0);
                        polyon[i] = false;
                    }
                }
            }
        } else { // sustain pedal pressed
            sustain = true;
        }
    }
    
    // Modulation wheel handling
    if (cc == 1) { // Modulation wheel
        if (value <= 5) {
            midi_send_cc(1, 74, 0); // mod wheel below 5 turns off LFO
        } else {
            midi_send_cc(1, 74, 70); // mod wheel above 5 turns on LFO
        }
    }
    
    // Pass CC messages to appropriate channels
    if (mode == 3 || mode == 4) { // poly mode
        if (channel == midichannel) {
            // Send CC to all active FM channels in poly mode
            for (int i = 1; i <= 6; i++) {
                midi_send_cc(i, cc, value);
            }
        } else {
            // Pass other channels straight through
            MIDI.sendControlChange(cc, value, channel);
        }
    } else { // mono mode
        if (channel >= 1 && channel <= 6) {
            midi_send_cc(channel, cc, value);
        } else {
            // Pass other channels straight through
            MIDI.sendControlChange(cc, value, channel);
        }
    }
}

void handle_pitch_bend(uint8_t channel, int16_t bend) {
    if (mode == 3 || mode == 4) { // if we're in poly mode
        if (channel == midichannel) {  // only respond to the configured poly MIDI channel
            // Send pitch bend to all 6 FM channels (match v1.10 implementation)
            for (int i = 5; i >= 0; i--) {
                MIDI.sendPitchBend(bend, i+1);
                MIDI.read();
            }
        }
        // If wrong channel, ignore pitch bend
    } else { // mono mode
        MIDI.sendPitchBend(bend, channel); // just midi thru
    }
}

void midiPanic(void) {
    resetVoicesAndNotes(); 
        for (uint8_t ch = 1; ch <= 6; ch++) {
        polyon[ch-1] = false;
        polynote[ch-1] = 0;
        sustainon[ch-1] = false;
        noteheld[ch-1] = false;
    }    
    delay(1000);
    if (mode == 1 || mode == 3) {
        updateFileDisplay();
    } else if (mode == 2 || mode == 4) {
        fmparamdisplay();
    }
}

void midiNoteToString(uint8_t note, char* noteStr) {
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    uint8_t octave = (note / 12) - 1;  // MIDI note 60 = C4
    uint8_t noteIndex = note % 12;
    
    // Handle special cases for very low or high notes
    if (note < 12) {
        sprintf(noteStr, "%s%d", noteNames[noteIndex], octave + 1);
    } else if (note > 127) {
        strcpy(noteStr, "---");
    } else {
        sprintf(noteStr, "%s%d", noteNames[noteIndex], octave);
    }
}

void resetVoicesAndNotes() {
    // First reset specific MIDI controllers and notes (avoid CC#121 which disables pitch bend)
    for (int ch = 1; ch <= 16; ++ch) {
        midi_send_cc(ch, 64, 0);   // Sustain Off
        midi_send_cc(ch, 120, 0);  // All Sound Off
        // midi_send_cc(ch, 121, 0);  // Reset All Controllers - REMOVED: This disables pitch bend!
        midi_send_cc(ch, 123, 0);  // All Notes Off
    }

    // Reset voice states but preserve loaded TFI instruments
    for (int i = 0; i < 6; ++i) {
        polyon[i] = false;
        polynote[i] = 0;
        noteheld[i] = false;
        sustainon[i] = false;
    }
     for (uint8_t ch = 1; ch <= 16; ++ch) {
        for (uint8_t note = 0; note < 128; ++note) {
            midi_send_note_off(ch, note, 0);
        }
    }
    sustain = false;
    lowestnote = 0;
}

void initializeFMChip() {
    // Minimal FM chip reset - only clear essential registers to prevent leftover values
    // Avoids sending too many MIDI messages that might interfere with pitch bend

    // Just send All Sound Off and All Notes Off to clear any stuck notes/sounds
    for (int ch = 1; ch <= 6; ch++) {
        midi_send_cc(ch, 120, 0);  // All Sound Off
        midi_send_cc(ch, 123, 0);  // All Notes Off
    }

    delay(10); // Minimal delay
}