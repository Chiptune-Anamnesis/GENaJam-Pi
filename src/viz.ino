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
    // Check if we're in visualizer mode with asteroids sub-mode
    if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 1) {
        asteroidsDemo();  // Call asteroids demo from asteroids.ino
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 2) {
        starfighterDemo();  // Call starfighter demo from starfighter.ino
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 3) {
        neuralNetDemo();  // Call neural network demo from neuralnet.ino
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