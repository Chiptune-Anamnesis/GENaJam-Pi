const uint16_t peak_hold_duration = 800;                                  // Peak hold time in ms
const uint8_t velocity_decay_rate = 2;
void oled_complete_update_protected(void (*update_function)());
extern uint8_t viz_sub_mode;  // 0=bar graph, 1=asteroids

void updateVelocityViz(uint8_t channel, uint8_t velocity) {
    if (channel >= 11 || channel >= sizeof(viz_data.voice_velocity)) return;

    mutex_enter_blocking(&viz_mutex);

    viz_data.voice_velocity[channel] = velocity;
    viz_data.needs_update = true;

    // Update peak hold if this velocity is higher
    if (velocity > viz_data.voice_peak[channel]) {
        viz_data.voice_peak[channel] = velocity;
        viz_data.voice_peak_time[channel] = millis();
    }

    mutex_exit(&viz_mutex);
}

void clearVelocityViz(uint8_t channel) {
    if (channel >= 11 || channel >= sizeof(viz_data.voice_velocity)) return;

    mutex_enter_blocking(&viz_mutex);
    viz_data.voice_velocity[channel] = 0;
    viz_data.needs_update = true;
    mutex_exit(&viz_mutex);
}

void visualizerDisplay(void) {
    // Serialize the entire frame to avoid Core0/Core1 overlap
    oled_complete_update_protected([]{
        // --- your existing body starts here ---
        // Process MIDI at start of display update
        handle_midi_input();

        display.clearDisplay();

        // Draw title
        display.setCursor(0, 0);
        display.setTextSize(1);
        if (mode == 5) {
            display.print("MONO VIZ");
        } else {
            display.print("POLY VIZ");
        }

        // Add MIDI performance stats in top right - very compact
        display.setCursor(70, 0);
        float khz = midi_hz / 1000.0;
        display.print(khz, 1);  // Show 1 decimal place
        display.print("k ");
        if (max_midi_gap < 1000) {
            display.print("<1");
        } else {
            display.print((int)(max_midi_gap / 1000));
        }
        display.print("m");

        // Process MIDI after header drawing
        handle_midi_input();

        // Draw 11 channels across 128 pixels (11.6 pixels per channel)
        for (int channel = 0; channel < 11; channel++) {
            // Process MIDI every few channels during intensive drawing
            if (channel % 4 == 0) {
                handle_midi_input();
            }

            int x_start = channel * 11 + channel;  // 11 pixels + 1 spacing = 12 total per channel
            int bar_width = 10;                    // Narrow bars to fit 11 channels
            int bar_height = 16;                   // Shorter bars to fit labels
            int y_base = 31;                       // Bottom of screen

            // Calculate bar heights (using snapshot struct in your code)
            int current_bar = map(viz_data.voice_velocity[channel], 0, 127, 0, bar_height);
            int peak_bar    = map(viz_data.voice_peak[channel],     0, 127, 0, bar_height);

            // Draw main velocity bar
            if (current_bar > 0) {
                display.fillRect(x_start, y_base - current_bar, bar_width, current_bar, SSD1306_WHITE);
            }
            // Draw peak hold line
            if (viz_data.voice_peak[channel] > 0) {
                display.drawFastHLine(x_start, y_base - peak_bar, bar_width, SSD1306_WHITE);
            }

            // Channel label
            display.setCursor(x_start + 2, 18);
            display.setTextSize(1);
            if (channel < 6) {
                display.print(channel + 1);  // FM 1–6
            } else if (channel < 9) {
                display.print("PSG");
            } else if (channel == 9) {
                display.print("N");           // PSG Noise
            } else {
                display.print("DAC");
            }
        }

        // Process MIDI before the expensive display.display() call
        handle_midi_input();
        display.display();
        // --- your existing body ends here ---
    });
}

void core1_visualizerDisplay(const VisualizerData& data) {
    // Check if we're in visualizer mode with different sub-modes
    if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 1) {
        asteroidsDemo();  // Call asteroids demo from asteroids.ino
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 2) {
        starfighterDemo();  // Call starfighter demo from starfighter.ino
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 3) {
        neuralNetDemo();  // Call neural network demo from neuralnet.ino
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 4) {
        liveEnvelopeVizDisplay();  // Call live envelope visualization
    } else if (viz_data.current_mode == 6) {  // POLY VIZ mode (bar graph)
        polyVisualizerDisplay();
    } else {  // MONO VIZ mode (bar graph) (mode 5)
        // Existing mono visualizer code
        display.clearDisplay();

        // Draw title
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("MONO VIZ");

        // Add MIDI performance stats in top right
        display.setCursor(70, 0);
        float khz = viz_data.midi_hz / 1000.0;
        display.print(khz, 1);
        display.print("k ");
        if (viz_data.max_midi_gap < 1000) {
            display.print("<1");
        } else {
            display.print((int)(viz_data.max_midi_gap / 1000));
        }
        display.print("m");

        // Draw 11 channels across 128 pixels
        for (int channel = 0; channel < 11; channel++) {
            int x_start = channel * 11 + channel;
            int bar_width = 10;
            int bar_height = 16;
            int y_base = 31;

            int current_bar = map(viz_data.voice_velocity[channel], 0, 127, 0, bar_height);
            int peak_bar = map(viz_data.voice_peak[channel], 0, 127, 0, bar_height);

            if (current_bar > 0) {
                display.fillRect(x_start, y_base - current_bar, bar_width, current_bar, SSD1306_WHITE);
            }

            if (viz_data.voice_peak[channel] > 0) {
                display.drawFastHLine(x_start, y_base - peak_bar, bar_width, SSD1306_WHITE);
            }

            display.setCursor(x_start + 2, 10);
            display.setTextSize(1);

            if (channel < 6) {
                display.print(channel + 1);
            } else if (channel < 10) {
                display.print("P");
                display.setCursor(x_start + 2, 18);
                display.print(channel - 6);
            } else {
                display.print("N");
            }
        }

        display.display();
    }
}

void updateVisualizerMode(uint8_t new_mode) {
    mutex_enter_blocking(&viz_mutex);
    viz_data.current_mode = new_mode;
    viz_data.needs_update = true;
    mutex_exit(&viz_mutex);
}

void updateVisualizerMidiStats(float hz, unsigned long max_gap) {
    mutex_enter_blocking(&viz_mutex);
    viz_data.midi_hz = hz;
    viz_data.max_midi_gap = max_gap;
    mutex_exit(&viz_mutex);
}

void oled_clear_protected(void) {
    mutex_enter_blocking(&display_mutex);
    display.clearDisplay();
    mutex_exit(&display_mutex);
}

void oled_print_protected(int x, int y, const char* text) {
    mutex_enter_blocking(&display_mutex);
    display.setCursor(x, y);
    display.print(text);
    mutex_exit(&display_mutex);
}

void oled_refresh_protected(void) {
    mutex_enter_blocking(&display_mutex);
    display.display();
    mutex_exit(&display_mutex);
}

void oled_complete_update_protected(void (*update_function)()) {
    mutex_enter_blocking(&display_mutex);
    update_function();
    mutex_exit(&display_mutex);
}

void polyVisualizerDisplay(void) {
    display.clearDisplay();

    // Title
    display.setCursor(0, 0);
    display.print("POLY VOICES");

    // Show 6 FM channels as vertical bars
    for (int i = 0; i < 6; i++) {
        int x = 10 + (i * 18);  // 18 pixels spacing between bars
        int bar_width = 16;
        int bar_height = 20;
        int y_base = 31;        // Bottom of screen

        if (polyon[i]) {
            // Draw filled vertical bar for active voice
            display.fillRect(x, y_base - bar_height, bar_width, bar_height, SSD1306_WHITE);

            // Show note name at bottom of bar (inverted colors)
            char noteStr[8];
            midiNoteToString(polynote[i], noteStr);

            // Clear space for text (creates shadow effect)
            display.fillRect(x + 2, y_base - 8, 12, 7, SSD1306_BLACK);

            // Draw inverted text
            display.setCursor(x + 3, y_base - 7);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display.print(noteStr);
            display.setTextColor(SSD1306_WHITE); // Reset color

        } else {
            // Draw empty bar outline
            display.drawRect(x, y_base - bar_height, bar_width, bar_height, SSD1306_WHITE);

            // Show "---" in empty bar
            display.setCursor(x + 3, y_base - 7);
            display.print("---");
        }
    }

    display.display();
}

// ============================================================================
// LIVE ENVELOPE VISUALIZATION (viz_sub_mode 4)
// Shows realtime envelope animation for all 6 playing voices
// ============================================================================

extern VoiceEnvelopeState voice_env[6];
extern uint8_t env_trail_history[6][ENV_TRAIL_LENGTH];
extern uint8_t env_trail_index;
extern unsigned long last_env_update;
extern const unsigned long ENV_UPDATE_INTERVAL;
extern const uint8_t carrier_mask[8];
extern uint8_t fmsettings[6][50];

// Cache ADSR parameters from carrier operators for a voice
void cacheEnvelopeParams(uint8_t voice) {
    if (voice >= 6) return;

    // Get algorithm for this channel (stored in fmsettings[ch][0] bits 4-6)
    uint8_t algorithm = (fmsettings[voice][0] >> 4) & 0x07;
    uint8_t mask = carrier_mask[algorithm];

    // Average ADSR values from all carrier operators
    uint16_t ar_sum = 0, d1r_sum = 0, sl_sum = 0, rr_sum = 0;
    uint8_t carrier_count = 0;

    // Check each operator (OP1=bit0, OP2=bit1, OP3=bit2, OP4=bit3)
    // fmsettings offsets: OP1=0, OP3=10, OP2=20, OP4=30 (YM2612 layout)
    const int op_offsets[4] = {0, 20, 10, 30};  // OP1, OP2, OP3, OP4

    for (int op = 0; op < 4; op++) {
        if (mask & (1 << op)) {
            int offset = op_offsets[op];
            ar_sum += fmsettings[voice][6 + offset];   // Attack Rate
            d1r_sum += fmsettings[voice][7 + offset];  // Decay 1 Rate
            sl_sum += fmsettings[voice][10 + offset];  // Sustain Level (D1L)
            rr_sum += fmsettings[voice][9 + offset];   // Release Rate
            carrier_count++;
        }
    }

    if (carrier_count > 0) {
        voice_env[voice].attack_rate = ar_sum / carrier_count;
        voice_env[voice].decay1_rate = d1r_sum / carrier_count;
        voice_env[voice].sustain_level = sl_sum / carrier_count;
        voice_env[voice].release_rate = rr_sum / carrier_count;
    }
}

// Trigger envelope on note on
void triggerEnvelope(uint8_t voice, uint8_t note, uint8_t velocity) {
    if (voice >= 6) return;

    voice_env[voice].active = true;
    voice_env[voice].note = note;
    voice_env[voice].velocity = velocity;
    voice_env[voice].note_on_time = millis();
    voice_env[voice].note_off_time = 0;
    voice_env[voice].phase = ENV_PHASE_ATTACK;
    voice_env[voice].level = 0;
    voice_env[voice].attack_pulse = 127;  // Max brightness for attack pulse

    // Cache ADSR params from carrier operators
    cacheEnvelopeParams(voice);
}

// Release envelope on note off
void releaseEnvelope(uint8_t voice) {
    if (voice >= 6) return;

    if (voice_env[voice].active && voice_env[voice].phase != ENV_PHASE_RELEASE) {
        voice_env[voice].note_off_time = millis();
        voice_env[voice].phase = ENV_PHASE_RELEASE;
    }
}

// Simulate envelope progression
void updateEnvelopeSimulation(void) {
    unsigned long now = millis();
    if (now - last_env_update < ENV_UPDATE_INTERVAL) return;
    last_env_update = now;

    // Update trail history
    env_trail_index = (env_trail_index + 1) % ENV_TRAIL_LENGTH;

    for (int v = 0; v < 6; v++) {
        // Store current level in trail history before updating
        env_trail_history[v][env_trail_index] = voice_env[v].level;

        // Decay attack pulse
        if (voice_env[v].attack_pulse > 0) {
            voice_env[v].attack_pulse = (voice_env[v].attack_pulse > 20) ?
                voice_env[v].attack_pulse - 20 : 0;
        }

        if (!voice_env[v].active) continue;

        // Simulate envelope based on phase
        switch (voice_env[v].phase) {
            case ENV_PHASE_ATTACK: {
                // Attack: ramp up quickly (rate affects speed)
                uint8_t attack_step = 8 + (voice_env[v].attack_rate / 4);
                if (voice_env[v].level + attack_step >= 127) {
                    voice_env[v].level = 127;
                    voice_env[v].phase = ENV_PHASE_DECAY1;
                } else {
                    voice_env[v].level += attack_step;
                }
                break;
            }

            case ENV_PHASE_DECAY1: {
                // Decay to sustain level
                uint8_t decay_step = 2 + (voice_env[v].decay1_rate / 8);
                // Sustain level is inverted: 0 = max sustain, 127 = no sustain
                uint8_t target = 127 - voice_env[v].sustain_level;
                if (voice_env[v].level <= target + decay_step) {
                    voice_env[v].level = target;
                    voice_env[v].phase = ENV_PHASE_SUSTAIN;
                } else {
                    voice_env[v].level -= decay_step;
                }
                break;
            }

            case ENV_PHASE_SUSTAIN:
                // Stay at sustain level until note off
                break;

            case ENV_PHASE_RELEASE: {
                // Release: decay to zero
                uint8_t release_step = 2 + (voice_env[v].release_rate / 6);
                if (voice_env[v].level <= release_step) {
                    voice_env[v].level = 0;
                    voice_env[v].active = false;
                    voice_env[v].phase = ENV_PHASE_OFF;
                } else {
                    voice_env[v].level -= release_step;
                }
                break;
            }
        }
    }
}

// Helper: convert rate to time in ms (approximation for visualization)
uint16_t rateToTime(uint8_t rate) {
    if (rate >= 120) return 20;   // Very fast
    if (rate >= 100) return 50;
    if (rate >= 80) return 100;
    if (rate >= 60) return 200;
    if (rate >= 40) return 400;
    if (rate >= 20) return 800;
    return 1500;  // Slow
}

// Calculate envelope position (0.0-1.0) based on time and ADSR rates
float getEnvelopePosition(uint8_t voice) {
    if (!voice_env[voice].active && voice_env[voice].phase == ENV_PHASE_OFF) {
        return -1.0f;  // Not active
    }

    unsigned long now = millis();
    unsigned long elapsed;

    if (voice_env[voice].phase == ENV_PHASE_RELEASE) {
        elapsed = now - voice_env[voice].note_off_time;
    } else {
        elapsed = now - voice_env[voice].note_on_time;
    }

    // Calculate segment durations based on rates
    uint16_t attack_time = rateToTime(voice_env[voice].attack_rate);
    uint16_t decay_time = rateToTime(voice_env[voice].decay1_rate);
    uint16_t release_time = rateToTime(voice_env[voice].release_rate);

    // Position mapping: 0.0-0.25 = attack, 0.25-0.5 = decay, 0.5-0.75 = sustain, 0.75-1.0 = release
    if (voice_env[voice].phase == ENV_PHASE_RELEASE) {
        // In release phase
        float release_progress = (float)elapsed / release_time;
        if (release_progress > 1.0f) release_progress = 1.0f;
        return 0.75f + (release_progress * 0.25f);
    } else if (elapsed < attack_time) {
        // Attack phase
        return (float)elapsed / attack_time * 0.25f;
    } else if (elapsed < attack_time + decay_time) {
        // Decay phase
        return 0.25f + ((float)(elapsed - attack_time) / decay_time * 0.25f);
    } else {
        // Sustain phase
        return 0.5f + min(0.24f, (float)(elapsed - attack_time - decay_time) / 2000.0f * 0.25f);
    }
}

// Draw a single envelope shape outline
void drawEnvelopeShape(uint8_t voice, bool filled, int y_offset) {
    const int BASE_Y = 30 - y_offset;
    const int TOP_Y = 2;
    const int MAX_HEIGHT = BASE_Y - TOP_Y;
    const int START_X = 0;
    const int TOTAL_WIDTH = 128;

    // Get ADSR values
    uint8_t sustainLevel = voice_env[voice].sustain_level;
    uint8_t attackRate = voice_env[voice].attack_rate;
    uint8_t decayRate = voice_env[voice].decay1_rate;
    uint8_t releaseRate = voice_env[voice].release_rate;

    // Calculate segment widths based on rates (faster = narrower)
    int attackWidth = map(127 - attackRate, 0, 127, 8, 35);
    int decayWidth = map(127 - decayRate, 0, 127, 8, 30);
    int sustainWidth = 30;  // Fixed sustain hold area
    int releaseWidth = map(127 - releaseRate, 0, 127, 8, 35);

    // Normalize to fit screen
    int totalWidth = attackWidth + decayWidth + sustainWidth + releaseWidth;
    float scale = (float)TOTAL_WIDTH / totalWidth;
    attackWidth = (int)(attackWidth * scale);
    decayWidth = (int)(decayWidth * scale);
    sustainWidth = (int)(sustainWidth * scale);
    releaseWidth = TOTAL_WIDTH - attackWidth - decayWidth - sustainWidth;

    // Calculate sustain Y position (inverted: 0 = max sustain, 127 = no sustain)
    int sustainY = TOP_Y + (sustainLevel * MAX_HEIGHT / 127);
    if (sustainY > BASE_Y) sustainY = BASE_Y;
    if (sustainY < TOP_Y) sustainY = TOP_Y;

    int x = START_X;

    // Draw outline only
    // Attack: 0 → peak
    display.drawLine(START_X, BASE_Y, START_X + attackWidth, TOP_Y, SSD1306_WHITE);
    x = START_X + attackWidth;

    // Decay: peak → sustain
    display.drawLine(x, TOP_Y, x + decayWidth, sustainY, SSD1306_WHITE);
    x += decayWidth;

    // Sustain: hold level
    display.drawLine(x, sustainY, x + sustainWidth, sustainY, SSD1306_WHITE);
    x += sustainWidth;

    // Release: sustain → 0
    display.drawLine(x, sustainY, x + releaseWidth, BASE_Y, SSD1306_WHITE);
}

// Live envelope visualization display (viz_sub_mode 4)
void liveEnvelopeVizDisplay(void) {
    // Update envelope simulation
    updateEnvelopeSimulation();

    display.clearDisplay();

    // Count active voices and find the most recently triggered one
    int activeCount = 0;
    int primaryVoice = -1;
    unsigned long latestTime = 0;

    for (int v = 0; v < 6; v++) {
        if (voice_env[v].active || voice_env[v].phase == ENV_PHASE_RELEASE) {
            activeCount++;
            if (voice_env[v].note_on_time > latestTime) {
                latestTime = voice_env[v].note_on_time;
                primaryVoice = v;
            }
        }
    }

    if (activeCount == 0) {
        // No active voices - just show empty screen
    } else if (activeCount == 1) {
        // Single voice - draw envelope shape
        drawEnvelopeShape(primaryVoice, true, 0);
    } else {
        // Multiple voices - overlay envelope shapes with vertical offset so they're visible
        // Each voice gets a small y offset to create a "fanned out" look
        int voiceIndex = 0;
        for (int v = 0; v < 6; v++) {
            if (voice_env[v].active || voice_env[v].phase == ENV_PHASE_RELEASE) {
                // Offset each voice by 2-3 pixels to create visible separation
                int y_offset = voiceIndex * 3;
                drawEnvelopeShape(v, false, y_offset);
                voiceIndex++;
            }
        }
    }

    display.display();
}