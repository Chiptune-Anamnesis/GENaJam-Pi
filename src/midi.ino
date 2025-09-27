extern uint8_t viz_sub_mode;  // 0=bar graph, 1=asteroids
extern bool system_ready;    // MIDI blocking flag during boot/setup

void setup_midi(void) {
    // Set UART pins first
    Serial1.setTX(MIDI_TX_PIN);
    Serial1.setRX(MIDI_RX_PIN);
    
    // Initialize Serial1 manually at MIDI baud rate BEFORE calling MIDI.begin()
    // This prevents the MIDI library from hanging during UART initialization
    Serial1.begin(MIDI_BAUD_RATE);
    delay(100);  // Give UART time to stabilize
    
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
//if (!usb_midi.begin()) {
 //   Serial.println("Failed to initialize USB MIDI");
//} else {
 //   Serial.println("USB MIDI initialized successfully");
//}
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
      //  updateMidiDisplay(channel, note);
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
    
    // Rest of existing note-off logic unchanged...
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
    
    // ADD THIS: Modulation wheel handling (same as original)
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
        // Send pitch bend to all 6 FM channels (like the original)
        for (int i = 1; i <= 6; i++) {
            midi_send_pitch_bend(i, bend);
            handle_midi_input(); // Keep MIDI responsive during loops
        }
    } else { // mono mode
        if (channel >= 1 && channel <= 6) {
            midi_send_pitch_bend(channel, bend);
        } else {
            // Pass other channels straight through
            MIDI.sendPitchBend(bend, channel);
        }
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
    // First reset all MIDI controllers and notes
    for (int ch = 1; ch <= 16; ++ch) {
        midi_send_cc(ch, 64, 0);   // Sustain Off
        midi_send_cc(ch, 120, 0);  // All Sound Off
        midi_send_cc(ch, 121, 0);  // Reset All Controllers
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
    // Initialize Genesis FM chip to default state
    // Prevents leftover register values from affecting sound after GenaJam reboot

    // Global FM settings
    for (int ch = 1; ch <= 6; ch++) {
        // Reset channel algorithms (CC 70-75) to basic Algorithm 0
        midi_send_cc(ch, 69 + ch, 0);

        // Reset feedback (CC 76-81) to 0
        midi_send_cc(ch, 75 + ch, 0);

        // Reset stereo/LFO (CC 82) to stereo both
        midi_send_cc(ch, 82, 64);
    }

    // Reset all operator parameters to defaults for each channel
    for (int ch = 1; ch <= 6; ch++) {
        // Operator 1 (CC 14-19)
        midi_send_cc(ch, 14, 63);  // Attack Rate default
        midi_send_cc(ch, 15, 0);   // Decay Rate default
        midi_send_cc(ch, 16, 0);   // Sustain Rate default
        midi_send_cc(ch, 17, 7);   // Release Rate default
        midi_send_cc(ch, 18, 15);  // Sustain Level default
        midi_send_cc(ch, 19, 127); // Total Level (volume) default

        // Operator 2 (CC 20-25)
        midi_send_cc(ch, 20, 63);
        midi_send_cc(ch, 21, 0);
        midi_send_cc(ch, 22, 0);
        midi_send_cc(ch, 23, 7);
        midi_send_cc(ch, 24, 15);
        midi_send_cc(ch, 25, 127);

        // Operator 3 (CC 26-31)
        midi_send_cc(ch, 26, 63);
        midi_send_cc(ch, 27, 0);
        midi_send_cc(ch, 28, 0);
        midi_send_cc(ch, 29, 7);
        midi_send_cc(ch, 30, 15);
        midi_send_cc(ch, 31, 127);

        // Operator 4 (CC 32-37)
        midi_send_cc(ch, 32, 63);
        midi_send_cc(ch, 33, 0);
        midi_send_cc(ch, 34, 0);
        midi_send_cc(ch, 35, 7);
        midi_send_cc(ch, 36, 15);
        midi_send_cc(ch, 37, 0);    // OP4 Total Level (carrier)

        // Reset frequency multipliers (CC 38-41) to 1x
        midi_send_cc(ch, 38, 8);   // Op1 Mult
        midi_send_cc(ch, 39, 8);   // Op2 Mult
        midi_send_cc(ch, 40, 8);   // Op3 Mult
        midi_send_cc(ch, 41, 8);   // Op4 Mult

        // Reset detune (CC 42-45) to 0
        midi_send_cc(ch, 42, 64);  // Op1 Detune (64 = center/0)
        midi_send_cc(ch, 43, 64);  // Op2 Detune
        midi_send_cc(ch, 44, 64);  // Op3 Detune
        midi_send_cc(ch, 45, 64);  // Op4 Detune
    }

    delay(50); // Allow time for all MIDI messages to be processed
}