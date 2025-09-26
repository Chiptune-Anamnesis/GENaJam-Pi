static inline bool inVizMode() { return mode == 5 || mode == 6; }
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
        uint16_t display_index = (current_count > 0) ? getCurrentTfiIndex(1) + 1 : 0;
        sprintf(display_buffer, "%s %03d/%03d", mode_str, display_index, current_count);
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
    
    uint16_t browsed_idx = (mode == 3) ? tfifilenumber[0] : tfifilenumber[tfichannel-1];

    bool is_user_file = (browsed_idx < user_file_count);
    if (!is_user_file && current_count > 0) {
        char folder_line[22];
        snprintf(folder_line, 22, ">%s", file_folders[browsed_idx]);
        oled_print(0, 8, folder_line);
    } else {
        oled_print(0, 8, "                    ");
    }

    if (current_count > 0) {
        char filename_line[22];
        snprintf(filename_line, 22, ">%s", filenames[browsed_idx]);
        oled_print(0, 16, filename_line);
    } else {
        oled_print(0, 16, ">No files");
    }

    oled_print(0, 24, "                    ");

    if (showing_loading) {
        oled_print(0, 24, "Loading TFI...");
    } else {
        oled_print(0, 24, "OPT1:Load OPT2:Mode");
    }
    
    showAccelerationFeedback();
    oled_refresh();
    
    last_display_update = millis();
    display_needs_refresh = false;
}

void channelselect(void) {
    if (n == 0) return;  // No files available

    // When switching MONO channels, immediately initialize the newly selected channel
    // with the currently selected TFI so it has a valid sound/params.
    // Also send All Notes Off / Reset All Controllers for a clean slate.
    if (mode == 1 || mode == 2) { // MONO | Preset or MONO | Edit
        midi_send_cc(tfichannel, 123, 0); // All Notes Off (per-channel)
        midi_send_cc(tfichannel, 121, 0); // Reset All Controllers (if supported)
        tfiLoadImmediateOnChannel(tfichannel); // Apply the current TFI to this channel now
    }

    updateFileDisplay();
}

void fmparamdisplay(void) {
    uint8_t i;
    char line1[32] = "";
    char line2[32] = "";
    char temp_str[16];
    
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
            
        case 6: // Attack Rate
            strcat(line1, "06:Attack");
            sprintf(line2, "%3d %3d %3d %3d", 
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][6],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][26],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][16],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][36]);
            break;
            
        case 7: // Decay Rate 1
            strcat(line1, "07:Decay 1");
            sprintf(line2, "%3d %3d %3d %3d", 
                fmsettings[tfichannel-1][7],
                fmsettings[tfichannel-1][27],
                fmsettings[tfichannel-1][17],
                fmsettings[tfichannel-1][37]);
            break;
            
        case 8: // Sustain (Total Level 2)
            strcat(line1, "08:Sustain");
            sprintf(line2, "%3d %3d %3d %3d", 
                fmsettings[tfichannel-1][10],
                fmsettings[tfichannel-1][30],
                fmsettings[tfichannel-1][20],
                fmsettings[tfichannel-1][40]);
            break;
            
        case 9: // Decay Rate 2
            strcat(line1, "09:Decay 2");
            sprintf(line2, "%3d %3d %3d %3d", 
                fmsettings[tfichannel-1][8],
                fmsettings[tfichannel-1][28],
                fmsettings[tfichannel-1][18],
                fmsettings[tfichannel-1][38]);
            break;
            
        case 10: // Release Rate
            strcat(line1, "10:Release");
            sprintf(line2, "%3d %3d %3d %3d", 
                fmsettings[tfichannel-1][9],
                fmsettings[tfichannel-1][29],
                fmsettings[tfichannel-1][19],
                fmsettings[tfichannel-1][39]);
            break;
            
        case 11: // SSG-EG
            strcat(line1, "11:SSG-EG");
            sprintf(line2, "%3d %3d %3d %3d", 
                fmsettings[tfichannel-1][11],
                fmsettings[tfichannel-1][31],
                fmsettings[tfichannel-1][21],
                fmsettings[tfichannel-1][41]);
            break;
            
        case 12: // Amp Mod
            strcat(line1, "12:Amp Mod");
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
            
        case 13: // LFO/FM/AM Level
            strcat(line1, "13:LFO/FM/AM");
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
        oled_print(0, 24, "OPT2:Save");
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
                for (int c = 6; c >= 1; c--) {
                    tfichannel = c;
                    fmccsend(i, currentpotvalue[i]);
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

    sprintf(line1, "SETTINGS %d/4", settings_screen);

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

        case 4: // Info/About
            strcat(line1, " ABOUT");
            sprintf(line2, "GenaJam-Pi %s", GENAJAM_VERSION);
            break;
    }
    
    oled_print(0, 0, line1);
    oled_print(0, 16, line2);
    
    // Show controls on line 3
    if (settings_screen < 4) {
        if (settings_changed) {
            oled_print(0, 24, "U/D:Change OPT2:Save");
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
    oled_print(0, 24, "OPT2:Save OPT1:Load DEL");
    
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