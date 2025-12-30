static inline bool inVizMode() { return mode == 5 || mode == 6; }

// External variables for envelope visualization
extern uint8_t selected_operator;
extern uint8_t tfichannel;
extern uint8_t fmsettings[6][50];
extern uint8_t temp_external_cc_sync;
extern uint8_t external_cc_sync;
extern uint8_t poly_multi_timbral;
extern uint8_t temp_poly_multi_timbral;

void oled_clear_protected(void);
void oled_print_protected(int x, int y, const char* text);
void oled_refresh_protected(void);
void oled_complete_update_protected(void (*update_function)());

void oled_print(int x, int y, const char* text) { oled_print_protected(x, y, text); }
void oled_clear(void)                            { oled_clear_protected(); }
void oled_refresh(void)                          { oled_refresh_protected(); }

void updateFileDisplay(void) {
    if (n == 0) return;
    
    // Only update if we're in a file browsing mode
    if (mode != 1 && mode != 3) return;
    
    // Rate limit display updates to reduce OLED overhead
    if (millis() - last_display_update < 50) {
        display_needs_refresh = true; // Mark for later update
        return;
    }
    
    char display_buffer[32];
    uint16_t current_count = getCurrentTfiCount();
    const char* mode_str = getTfiBrowseModeString();

    if (mode == 3) {
        if (poly_multi_timbral == 1) {
            // Poly-multi mode: show channel like mono mode
            uint16_t display_index = (current_count > 0) ? getCurrentTfiIndex(tfichannel) + 1 : 0;
            sprintf(display_buffer, "C%d %s %03d/%03d", tfichannel, mode_str, display_index, current_count);
        } else {
            // Standard poly mode: no channel indicator
            uint16_t display_index = (current_count > 0) ? getCurrentTfiIndex(1) + 1 : 0;
            sprintf(display_buffer, "%s %03d/%03d", mode_str, display_index, current_count);
        }
    } else {
        uint16_t display_index = (current_count > 0) ? getCurrentTfiIndex(tfichannel) + 1 : 0;
        sprintf(display_buffer, "C%d %s %03d/%03d", tfichannel, mode_str, display_index, current_count);
    }
    
    oled_clear();
    oled_print(0, 0, display_buffer);
    
    // Add MIDI info only if recent and in correct modes
    if (last_midi_time > 0 && (millis() - last_midi_time) < midi_display_timeout && (mode == 1 || mode == 3)) {
        char midi_buffer[16];
        char note_name[8];
        
        midiNoteToString(last_midi_note, note_name);
        sprintf(midi_buffer, "C%d %s", last_midi_channel, note_name);
        oled_print(75, 0, midi_buffer);
    }
    
    uint16_t file_idx;
    if (mode == 3 && poly_multi_timbral == 0) {
        // Standard poly mode: all channels use same TFI
        file_idx = tfifilenumber[0];
    } else {
        // Mono mode or poly-multi mode: use current channel's TFI
        file_idx = tfifilenumber[tfichannel-1];
    }

    bool is_user_file = (file_idx < user_file_count);
    if (!is_user_file && current_count > 0 && file_idx < n) {
        char folder_line[22];
        snprintf(folder_line, 22, ">%s", file_folders[file_idx]);
        oled_print(0, 8, folder_line);
    } else {
        oled_print(0, 8, "                    ");
    }

    if (current_count > 0 && file_idx < n) {
        char filename_line[22];
        snprintf(filename_line, 22, ">%s", filenames[file_idx]);
        oled_print(0, 16, filename_line);
    } else {
        oled_print(0, 16, ">No files");
    }

    oled_print(0, 24, "                    ");

    if (showing_loading) {
        oled_print(0, 24, "Loading TFI...");
    } else {
        if (preview_mode) {
            oled_print(0, 24, "BTN1:Play BTN2:Exit");
        } else {
            oled_print(0, 24, "BTN1:Load BTN2:Mode");
        }
    }
    
    showAccelerationFeedback();
    oled_refresh();
    
    last_display_update = millis();
    display_needs_refresh = false;
}

void channelselect(void) {
    if (n == 0) return;  // No files available
    updateFileDisplay();
}

void fmparamdisplay(void) {
    uint8_t i;
    char line1[32] = "";
    char line2[32] = "";
    char temp_str[16];

    // Special case: fmscreen 6 is envelope visualization
    if (fmscreen == 6) {
        envelopeVizDisplay();
        return;
    }

    oled_clear();

    if (mode == 2 || mode == 4) { // MONO EDIT or POLY EDIT
        sprintf(line1, "C%d ", tfichannel);
    } else {
        strcpy(line1, "P  ");
    }

    switch(fmscreen) {
        case 1: // Algorithm, Feedback, Pan
            strcat(line1, "01:Alg FB Pan");

            if (polypan > 64) {
                strcpy(line2, "<> ON  ");
            } else {
                strcpy(line2, "<>OFF  ");
            }

            i = fmsettings[tfichannel-1][0];
            sprintf(temp_str, "%d ", i / 16);
            strcat(line2, temp_str);

            i = fmsettings[tfichannel-1][1];
            sprintf(temp_str, "%3d ", i);
            strcat(line2, temp_str);

            // Pan display
            i = fmsettings[tfichannel-1][44];
            if (i < 32) strcat(line2, "OFF");
            else if (i < 64) strcat(line2, " L ");
            else if (i < 96) strcat(line2, " R ");
            else strcat(line2, " C ");
            break;

        case 2: // Total Level (OP Volume)
            strcat(line1, "02:OP Volume");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][4],
                fmsettings[tfichannel-1][24],
                fmsettings[tfichannel-1][14],
                fmsettings[tfichannel-1][34]);
            break;

        case 3: // Frequency Multiple
            strcat(line1, "03:Freq Multp");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][2],
                fmsettings[tfichannel-1][22],
                fmsettings[tfichannel-1][12],
                fmsettings[tfichannel-1][32]);
            break;

        case 4: // Detune
            strcat(line1, "04:Detune-Mul");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][3],
                fmsettings[tfichannel-1][23],
                fmsettings[tfichannel-1][13],
                fmsettings[tfichannel-1][33]);
            break;

        case 5: // Rate Scaling
            strcat(line1, "05:Rate Scale");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][5],
                fmsettings[tfichannel-1][25],
                fmsettings[tfichannel-1][15],
                fmsettings[tfichannel-1][35]);
            break;

        // Case 6 is handled above (envelope viz)

        case 7: // Attack Rate (was 6)
            strcat(line1, "07:Attack");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][6],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][26],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][16],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][36]);
            break;

        case 8: // Decay Rate 1 (was 7)
            strcat(line1, "08:Decay 1");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][7],
                fmsettings[tfichannel-1][27],
                fmsettings[tfichannel-1][17],
                fmsettings[tfichannel-1][37]);
            break;

        case 9: // Sustain (was 8)
            strcat(line1, "09:Sustain");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][10],
                fmsettings[tfichannel-1][30],
                fmsettings[tfichannel-1][20],
                fmsettings[tfichannel-1][40]);
            break;

        case 10: // Decay Rate 2 (was 9)
            strcat(line1, "10:Decay 2");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][8],
                fmsettings[tfichannel-1][28],
                fmsettings[tfichannel-1][18],
                fmsettings[tfichannel-1][38]);
            break;

        case 11: // Release Rate (was 10)
            strcat(line1, "11:Release");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][9],
                fmsettings[tfichannel-1][29],
                fmsettings[tfichannel-1][19],
                fmsettings[tfichannel-1][39]);
            break;

        case 12: // SSG-EG (was 11)
            strcat(line1, "12:SSG-EG");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][11],
                fmsettings[tfichannel-1][31],
                fmsettings[tfichannel-1][21],
                fmsettings[tfichannel-1][41]);
            break;

        case 13: // Amp Mod (was 12)
            strcat(line1, "13:Amp Mod");
            strcpy(line2, "");
            for (int op = 0; op < 4; op++) {
                int setting_idx = (op == 0) ? 45 : (op == 1) ? 47 : (op == 2) ? 46 : 48;
                i = fmsettings[tfichannel-1][setting_idx];
                if (i < 64) {
                    strcat(line2, "OFF ");
                } else {
                    strcat(line2, " ON ");
                }
            }
            break;

        case 14: // LFO/FM/AM Level (was 13)
            strcat(line1, "14:LFO/FM/AM");
            sprintf(line2, "    %3d %3d %3d",
                lfospeed,
                fmsettings[tfichannel-1][42],
                fmsettings[tfichannel-1][43]);
            break;
    }

    oled_print(0, 0, line1);
    oled_print(0, 16, line2);

    // Show save option for POLY mode
    if (mode == 4) { // POLY EDIT mode
        oled_print(0, 24, "BTN2:Save");
    }

    oled_refresh();
}

void operatorparamdisplay(void) {
    handle_midi_input();
    
    // Faster update rate for responsive PCB pots
    static unsigned long last_pot_check = 0;
    if (millis() - last_pot_check < 10) return; // 100Hz max
    last_pot_check = millis();
    
    updatePotHistory(); // Update the moving average
    
    uint8_t currentpotvalue[4];
    int8_t difference;
    
    // Read all 4 pot values with filtering
    for (int i = 0; i < 4; i++) {
        currentpotvalue[i] = getFilteredPotValue(i);
    }
    
    bool displayNeedsUpdate = false;
    
    for (int i = 0; i <= 3; i++) {
        difference = prevpotvalue[i] - currentpotvalue[i];
        
        // Lower threshold for responsive PCB pots
        if (difference > 1 || difference < -1) {
            handle_midi_input();
            
            if (currentpotvalue[i] <= 1) currentpotvalue[i] = 0;
            if (currentpotvalue[i] >= 126) currentpotvalue[i] = 127;
            prevpotvalue[i] = currentpotvalue[i];
            
            if (mode == 2) { // MONO EDIT
                fmccsend(i, currentpotvalue[i]);
            } else if (mode == 4) { // POLY EDIT
                if (poly_multi_timbral == 1) {
                    // Poly-multi mode: only affect current channel
                    fmccsend(i, currentpotvalue[i]);
                } else {
                    // Standard poly mode: affect all 6 channels
                    for (int c = 6; c >= 1; c--) {
                        tfichannel = c;
                        fmccsend(i, currentpotvalue[i]);
                    }
                }
            }
            
            displayNeedsUpdate = true;
        }
    }
    
    if (displayNeedsUpdate) {
        fmparamdisplay();
    }
}

void settingsDisplay(void) {
    char line1[32] = "";
    char line2[32] = "";
    const char* curve_names[] = {"Linear", "Soft", "Medium", "Hard", "ImpBox"};

    oled_clear();

    sprintf(line1, "SETTINGS %d/6", settings_screen);

    switch(settings_screen) {
        case 1: // MIDI Channel
            strcat(line1, " MIDI CH");
            sprintf(line2, "Channel: %d", temp_midichannel);
            if (temp_midichannel != midichannel) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 2: // Region
            strcat(line1, " REGION");
            if (temp_region == 0) {
                strcpy(line2, "NTSC/USA (60Hz)");
            } else {
                strcpy(line2, "PAL/EUR (50Hz)");
            }
            if (temp_region != region) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 3: // Velocity Curve
            strcat(line1, " VELOCITY");
            strcpy(line2, curve_names[temp_velocity_curve]);
            if (temp_velocity_curve != velocity_curve) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 4: // External CC Sync
            strcat(line1, " CC SYNC");
            if (temp_external_cc_sync == 1) {
                strcpy(line2, "ON (ADSR via MIDI)");
            } else {
                strcpy(line2, "OFF");
            }
            if (temp_external_cc_sync != external_cc_sync) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 5: // Poly Multi-Timbral
            strcat(line1, " POLY-MT");
            if (temp_poly_multi_timbral == 1) {
                strcpy(line2, "ON (per-ch TFI)");
            } else {
                strcpy(line2, "OFF (same TFI)");
            }
            if (temp_poly_multi_timbral != poly_multi_timbral) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 6: // Info/About
            strcat(line1, " ABOUT");
            sprintf(line2, "GenaJam-Pi %s", GENAJAM_VERSION);
            break;
    }

    oled_print(0, 0, line1);
    oled_print(0, 16, line2);

    // Show controls on line 3
    if (settings_screen < 6) {
        if (settings_changed) {
            oled_print(0, 24, "U/D:Change BTN2:Save");
        } else {
            oled_print(0, 24, "U/D:Change PRST:Exit");
        }
    } else {
        oled_print(0, 24, "PRST:Exit");
    }

    oled_refresh();
}

void settingsAdjustUp(void) {
    switch(settings_screen) {
        case 1: // MIDI Channel
            temp_midichannel++;
            if (temp_midichannel > 16) temp_midichannel = 1;
            settingsDisplay();
            break;

        case 2: // Region
            temp_region = 1 - temp_region; // Toggle between 0 and 1
            settingsDisplay();
            break;

        case 3: // Velocity Curve
            temp_velocity_curve++;
            if (temp_velocity_curve > 4) temp_velocity_curve = 0;
            settingsDisplay();
            break;

        case 4: // External CC Sync
            temp_external_cc_sync = 1 - temp_external_cc_sync; // Toggle between 0 and 1
            settingsDisplay();
            break;

        case 5: // Poly Multi-Timbral
            temp_poly_multi_timbral = 1 - temp_poly_multi_timbral; // Toggle between 0 and 1
            settingsDisplay();
            break;
    }
}

void settingsAdjustDown(void) {
    switch(settings_screen) {
        case 1: // MIDI Channel
            temp_midichannel--;
            if (temp_midichannel == 0) temp_midichannel = 16;
            settingsDisplay();
            break;

        case 2: // Region
            temp_region = 1 - temp_region; // Toggle between 0 and 1 (same as up)
            settingsDisplay();
            break;

        case 3: // Velocity Curve
            temp_velocity_curve--;
            if (temp_velocity_curve > 4) temp_velocity_curve = 4; // Handle underflow
            settingsDisplay();
            break;

        case 4: // External CC Sync
            temp_external_cc_sync = 1 - temp_external_cc_sync; // Toggle between 0 and 1
            settingsDisplay();
            break;

        case 5: // Poly Multi-Timbral
            temp_poly_multi_timbral = 1 - temp_poly_multi_timbral; // Toggle between 0 and 1
            settingsDisplay();
            break;
    }
}


void settingsOperatorDisplay(void) {
    // Settings menu no longer uses potentiometer control
    // Only handle MIDI input to keep system responsive
    handle_midi_input();
}

void presetManagerDisplay(void) {
    oled_clear();
    
    // Check if we're in poly preset manager mode
    if (mode == 8) {
        oled_print(0, 0, "POLY PRESETS");
        oled_print(0, 16, "DISABLED");
        oled_print(0, 24, "Use MONO mode only");
        oled_refresh();
        return;
    }
    
    // Show mode and counts for MONO preset manager only
    char display_buffer[32];
    int display_index = presetfilenumber + 1;  // Normal 1-based numbering
    snprintf(display_buffer, sizeof(display_buffer), "M BANK %d/%d", display_index, preset_n);
    oled_print(0, 0, display_buffer);

    // Show the actual bank filename (shows the real bank number from SD card)
    if (preset_n > 0) {
        oled_print(0, 16, presetfilenames[presetfilenumber]);
    } else {
        oled_print(0, 16, "No banks found");
    }
    
    // Show controls
    oled_print(0, 24, "BTN2:Save BTN1:Load DEL");
    
    oled_refresh();
}

void showAccelerationFeedback(void) {
    if (!button_is_held) return;
    
    uint32_t hold_duration = millis() - button_hold_start_time;
    
    // Only show feedback in preset browsing modes
    if (mode != 1 && mode != 3) return;
    
}

void updateMidiDisplay(uint8_t channel, uint8_t note) {
    last_midi_channel = channel;
    last_midi_note = note;
    last_midi_time = millis();

    // Refresh display if we're in a file browsing mode
    if (mode == 1 || mode == 3) {
        updateFileDisplay();
    }
}

// Convert envelope rate to pixel width for visualization
// Higher rate = faster = fewer pixels (steeper slope)
int rateToPixels(uint8_t rate, int maxPixels) {
    if (rate == 0) return maxPixels;  // Slowest
    if (rate >= 127) return 2;        // Fastest

    // Inverse logarithmic relationship
    int pixels = maxPixels - (rate * maxPixels / 140);
    if (pixels < 2) pixels = 2;
    if (pixels > maxPixels) pixels = maxPixels;
    return pixels;
}

// Envelope Visualization Screen (fmscreen 6)
// Shows the ADSR envelope shape for the selected operator
void envelopeVizDisplay(void) {
    int ch = tfichannel - 1;
    if (ch < 0 || ch > 5) ch = 0;

    // Get fmsettings indices based on selected operator
    // Operator layout: OP1=0, OP2=1, OP3=2, OP4=3
    // But fmsettings uses offset: OP1=0, OP2=20, OP3=10, OP4=30
    int op_offset;
    switch (selected_operator) {
        case 0: op_offset = 0; break;   // OP1
        case 1: op_offset = 20; break;  // OP2 (stored at +20)
        case 2: op_offset = 10; break;  // OP3 (stored at +10)
        case 3: op_offset = 30; break;  // OP4
        default: op_offset = 0; break;
    }

    // Get envelope parameters for selected operator
    uint8_t attackRate   = fmsettings[ch][6 + op_offset];   // AR
    uint8_t decay1Rate   = fmsettings[ch][7 + op_offset];   // D1R
    uint8_t sustainLevel = fmsettings[ch][10 + op_offset];  // D1L (sustain level, stored inverted)
    uint8_t decay2Rate   = fmsettings[ch][8 + op_offset];   // D2R
    uint8_t releaseRate  = fmsettings[ch][9 + op_offset];   // RR

    oled_clear();

    // Header line with better spacing: BTN1-OP# ENV. CH#
    char header[32];
    sprintf(header, "BTN1-OP%d ENV. CH%d", selected_operator + 1, tfichannel);
    oled_print(0, 0, header);

    // Drawing constants for 128x32 display
    const int BASE_Y = 30;      // Bottom of envelope area
    const int TOP_Y = 10;       // Top of envelope area (below header)
    const int MAX_HEIGHT = BASE_Y - TOP_Y;  // 20 pixels height
    const int START_X = 4;      // Left margin
    const int MAX_WIDTH = 120;  // Available width

    // Calculate sustain level in pixels (higher value = lower level visually)
    // sustainLevel is stored inverted: 0 = full sustain, 127 = no sustain
    int sustainY = TOP_Y + (sustainLevel * MAX_HEIGHT / 127);
    if (sustainY > BASE_Y) sustainY = BASE_Y;
    if (sustainY < TOP_Y) sustainY = TOP_Y;

    // Calculate segment widths based on rates
    int totalSegments = 4;
    int segmentWidth = (MAX_WIDTH - 8) / totalSegments;  // Divide available space

    int attackWidth  = rateToPixels(attackRate, segmentWidth);
    int decay1Width  = rateToPixels(decay1Rate, segmentWidth);
    int decay2Width  = segmentWidth;  // Fixed width for sustain hold
    int releaseWidth = rateToPixels(releaseRate, segmentWidth);

    // Draw envelope path using display primitives directly
    mutex_enter_blocking(&display_mutex);

    int x = START_X;

    // Attack: 0 → peak (bottom to top)
    display.drawLine(x, BASE_Y, x + attackWidth, TOP_Y, SSD1306_WHITE);
    x += attackWidth;

    // Decay 1: peak → sustain level
    display.drawLine(x, TOP_Y, x + decay1Width, sustainY, SSD1306_WHITE);
    x += decay1Width;

    // Decay 2 / Sustain: sustain level with slight decay (or flat)
    int decay2EndY = sustainY;
    if (decay2Rate > 0) {
        decay2EndY = sustainY + (decay2Rate * (BASE_Y - sustainY) / 200);
        if (decay2EndY > BASE_Y) decay2EndY = BASE_Y;
    }
    display.drawLine(x, sustainY, x + decay2Width, decay2EndY, SSD1306_WHITE);
    x += decay2Width;

    // Release: sustain → 0 (back to bottom)
    display.drawLine(x, decay2EndY, x + releaseWidth, BASE_Y, SSD1306_WHITE);

    // Draw baseline
    display.drawLine(START_X, BASE_Y, START_X + MAX_WIDTH, BASE_Y, SSD1306_WHITE);

    // Draw segment labels at bottom
    display.setCursor(START_X, 24);
    display.setTextSize(1);

    // Show current ADSR values compactly
    char values[32];
    sprintf(values, "A%3d D%3d S%3d R%3d", attackRate, decay1Rate, 127 - sustainLevel, releaseRate);
    display.print(values);

    display.display();
    mutex_exit(&display_mutex);
}