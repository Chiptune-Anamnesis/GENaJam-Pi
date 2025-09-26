void scandir(bool saved) {
    n = 0;
    user_file_count = 0;

    scanDirectoryRecursive("/tfi/user", saved);
    user_file_count = n;

    scanDirectoryRecursive("/tfi", saved);

    sortFilesByFolder();

    // Clamp selections if list shrank
    for (int i = 0; i < 6; ++i) {
        if (n == 0) {
            tfifilenumber[i] = 0;
        } else if (tfifilenumber[i] >= n) {
            tfifilenumber[i] = n - 1;
        }
    }
}

void scanDirectoryRecursive(const char* path, bool saved) {
    // Check file limits based on scanning phase
    bool is_user_folder = (strcmp(path, "/tfi/user") == 0);
    if (is_user_folder && user_file_count >= nMaxUser) return;  // User limit
    if (!is_user_folder && (n - user_file_count) >= nMaxLibrary) return;  // Library limit
    if (n >= nMaxTotal) return;  // Total limit
    
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return;
    }
    
    // Show progress during scanning
    if (strcmp(path, "/tfi/user") == 0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Scanning user files...");
        display.display();
    } else if (strcmp(path, "/tfi") == 0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Scanning Lib files...");
        display.display();
    }
    
    while (n < nMaxTotal) {
        File entry = dir.openNextFile();
        if (!entry) break;
        
        const char* name = entry.name();
        if (!name || strlen(name) == 0) {
            entry.close();
            continue;
        }
        
        // Skip system files/folders
        if (name[0] == '.' || 
            strstr(name, "System Volume") != NULL ||
            strstr(name, "$RECYCLE") != NULL) {
            entry.close();
            continue;
        }
        
        if (entry.isDirectory()) {
            // Skip user folder when scanning main tfi directory
            if (strcmp(path, "/tfi") == 0 && strcmp(name, "user") == 0) {
                entry.close();
                continue;
            }

            // Recursively scan subdirectory
            char subPath[96];
            if (strcmp(path, "/") == 0) {
                snprintf(subPath, 96, "/%s", name);
            } else {
                snprintf(subPath, 96, "%s/%s", path, name);
            }
            entry.close();
            scanDirectoryRecursive(subPath, saved);
            
        } else {
            // Check for TFI files
            int nameLen = strlen(name);
            bool isTFI = (nameLen >= 4 &&
                         name[nameLen-4] == '.' &&
                         (name[nameLen-3] == 't' || name[nameLen-3] == 'T') &&
                         (name[nameLen-2] == 'f' || name[nameLen-2] == 'F') &&
                         (name[nameLen-1] == 'i' || name[nameLen-1] == 'I'));
                         
            if (isTFI) {
                bool is_user_folder = (strcmp(path, "/tfi/user") == 0);
                if (is_user_folder && user_file_count >= nMaxUser) {
                    entry.close();
                    continue;
                }
                if (!is_user_folder && (n - user_file_count) >= nMaxLibrary) {
                    entry.close();
                    continue;
                }
                if (n >= nMaxTotal) {
                    entry.close();
                    continue;
                }

                int required_path_length = strlen(path) + 1 + strlen(name) + 1;
                if (required_path_length > FullNameChars) {
                    entry.close();
                    continue;
                }

                int dispLen = nameLen - 4;
                if (dispLen > MaxNumberOfChars) {
                    entry.close();
                    continue;
                }

                // Create display name with folder prefix if not in tfi root
                if (strcmp(path, "/tfi") == 0) {
                    // TFI root file - just use filename without .tfi
                    strncpy(filenames[n], name, dispLen);
                    filenames[n][dispLen] = '\0';

                    // Store full path
                    snprintf(fullnames[n], FullNameChars, "/tfi/%s", name);

                } else {
                    // Subfolder file - just use filename without .tfi extension
                    strncpy(filenames[n], name, dispLen);
                    filenames[n][dispLen] = '\0';

                    // Store full path for loading
                    snprintf(fullnames[n], FullNameChars, "%s/%s", path, name);
                }
                
                // Check if this is the saved file we're looking for
                char fullPath[FullNameChars];
                snprintf(fullPath, FullNameChars, "%s/%s", path, name);
                if (saved && strcmp(name, savefilefull) == 0) {
                    for (int i = 0; i < 6; i++) tfifilenumber[i] = n;
                }

                extractFolderName(fullPath, file_folders[n]);

                n++;
                
                // Update progress display occasionally
                if (n % 20 == 0) {
                    display.clearDisplay();
                    display.setCursor(0, 0);
                    display.print("Scanning files...");
                    display.setCursor(0, 16);
                    display.print("Found: ");
                    display.print(n);
                    display.display();
                }
            }
            entry.close();
        }
        
        // Yield periodically to prevent watchdog timeout
        if (n % 10 == 0) {
            delay(1);
            handle_midi_input();
        }
    }
    
    dir.close();
}

void scanPresetDir(void) {
    preset_n = 0;
    
    File dir = SD.open("/presets");  // Changed from "/" to "/presets"
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return;
    }
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Scanning presets...");
    display.display();
    
    while (preset_n < nMax - 10) {
        File entry = dir.openNextFile();
        if (!entry) break;
        
        const char* name = entry.name();
        if (!name || strlen(name) == 0) {
            entry.close();
            continue;
        }
        
        // Skip system files/folders
        if (name[0] == '.' || 
            strstr(name, "System Volume") != NULL ||
            strstr(name, "$RECYCLE") != NULL) {
            entry.close();
            continue;
        }
        
        if (!entry.isDirectory()) {
            // Check for .mdm_pre files
            int nameLen = strlen(name);
            bool isPreset = (nameLen >= 8 &&
                           name[nameLen-8] == '.' &&
                           (name[nameLen-7] == 'm' || name[nameLen-7] == 'M') &&
                           (name[nameLen-6] == 'd' || name[nameLen-6] == 'D') &&
                           (name[nameLen-5] == 'm' || name[nameLen-5] == 'M') &&
                           name[nameLen-4] == '_' &&
                           (name[nameLen-3] == 'p' || name[nameLen-3] == 'P') &&
                           (name[nameLen-2] == 'r' || name[nameLen-2] == 'R') &&
                           (name[nameLen-1] == 'e' || name[nameLen-1] == 'E'));
                           
            if (isPreset) {
                // Store display name without extension
                int dispLen = nameLen - 8;
                if (dispLen > MaxNumberOfChars) dispLen = MaxNumberOfChars;
                strncpy(presetfilenames[preset_n], name, dispLen);
                presetfilenames[preset_n][dispLen] = '\0';
                
                // Store full path with presets folder
                snprintf(presetfullnames[preset_n], FullNameChars, "/presets/%s", name);  // Changed path
                
                preset_n++;
                
                // Update progress display occasionally
                if (preset_n % 10 == 0) {
                    display.clearDisplay();
                    display.setCursor(0, 0);
                    display.print("Scanning presets...");
                    display.setCursor(0, 16);
                    display.print("Found: ");
                    display.print(preset_n);
                    display.display();
                }
            }
        }
        entry.close();
        
        // Yield periodically
        if (preset_n % 5 == 0) {
            delay(1);
            handle_midi_input();
        }
    }
    
    dir.close();

    // Sort preset files alphabetically to ensure proper order
    for (int i = 0; i < preset_n - 1; i++) {
        for (int j = 0; j < preset_n - i - 1; j++) {
            if (strcmp(presetfilenames[j], presetfilenames[j + 1]) > 0) {
                // Swap both arrays
                char temp_name[MaxNumberOfChars + 1];
                char temp_full[FullNameChars];

                strcpy(temp_name, presetfilenames[j]);
                strcpy(presetfilenames[j], presetfilenames[j + 1]);
                strcpy(presetfilenames[j + 1], temp_name);

                strcpy(temp_full, presetfullnames[j]);
                strcpy(presetfullnames[j], presetfullnames[j + 1]);
                strcpy(presetfullnames[j + 1], temp_full);
            }
        }
    }

    // Clamp selection if list shrank
    if (preset_n == 0) {
        presetfilenumber = 0;
    } else if (presetfilenumber >= preset_n) {
        presetfilenumber = preset_n - 1;
    }
}

void tfiselect(void) {
    if (n == 0) return;  // No files available

    uint16_t idx = tfifilenumber[tfichannel-1];
    if (idx >= n) return;

    // Always use delayed loading for smoother browsing experience
    tfi_select_time = millis();
    tfi_pending_load = true;
    pending_tfi_channel = tfichannel;
    showing_loading = false;

    // Update display immediately to show the selection
    if (booted == 1 && (mode == 1 || mode == 3)) {
        updateFileDisplay();
    }
}

void tfiLoadImmediate(void) {
    if (n == 0) return;

    uint16_t idx = tfifilenumber[tfichannel-1];
    if (idx >= n) return;

    dataFile = SD.open(fullnames[idx], FILE_READ);
    if (!dataFile) {
        oled_clear();
        oled_print(0, 0, "SD: Cannot read");
        oled_print(0, 16, filenames[idx]);
        oled_print(0, 24, "Check SD card");
        oled_refresh();
        delay(2000);
        return;
    }

    // Read TFI file
    int tfiarray[42];
    for (int i = 0; i < 42; i++) {
        if (dataFile.available()) {
            tfiarray[i] = dataFile.read();
        } else {
            tfiarray[i] = 0;
        }
    }
    dataFile.close();

    tfisend(tfiarray, tfichannel);
}

void tfiLoadImmediateOnChannel(uint8_t ch) {
    if (n == 0) return;
    if (ch < 1 || ch > 6) return;

    uint16_t idx = tfifilenumber[ch - 1];
    if (idx >= n) return;

    File f = SD.open(fullnames[idx], FILE_READ);
    if (!f) {
        oled_clear();
        oled_print(0, 0, "SD: File error");
        oled_print(0, 16, "Check connection");
        oled_refresh();
        delay(1500);
        return;
    }

    int tfiarray[42];
    for (int i = 0; i < 42; i++) {
        if (f.available()) {
            tfiarray[i] = f.read();
        } else {
            tfiarray[i] = 0;
        }
    }
    f.close();

    // Send parameters explicitly to the requested channel
    tfisend(tfiarray, ch);
}

void applyTFIToAllChannelsImmediate() {
    for (uint8_t ch = 1; ch <= 6; ch++) {
        tfiLoadImmediateOnChannel(ch);
        delay(5); // Small delay between channels to prevent UART overflow
        // Only add MIDI processing if we're past the initial setup
        if (booted == 1) {
            handle_midi_input();
        }
    }
}

void loadPendingTFI(void) {
    if (n == 0) return;

    uint16_t idx = tfifilenumber[pending_tfi_channel-1];
    if (idx >= n) return;

    dataFile = SD.open(fullnames[idx], FILE_READ);
    if (!dataFile) {
        oled_clear();
        oled_print(0, 0, "SD: Read failed");
        oled_print(0, 16, filenames[idx]);
        oled_print(0, 24, "Press any button");
        oled_refresh();

        // Wait for user acknowledgment
        while (read_buttons() == btnNONE) {
            handle_midi_input();
            delay(50);
        }

        tfi_pending_load = false;
        showing_loading = false;
        return;
    }

    // Read TFI file
    int tfiarray[42];
    for (int i = 0; i < 42; i++) {
        if (dataFile.available()) {
            tfiarray[i] = dataFile.read();
        } else {
            tfiarray[i] = 0;
        }
    }
    dataFile.close();

    // Apply based on mode
    if (mode == 3 || mode == 4) {
        // POLY mode: apply to all 6 channels
        for (uint8_t ch = 1; ch <= 6; ch++) {
            tfisend(tfiarray, ch);
            delay(2);
        }
        // Update tracking for all channels in poly mode
        for (int i = 0; i < 6; i++) {
            loaded_tfi[i] = tfifilenumber[0];
        }
    } else {
        // MONO mode: apply only to the specific channel
        tfisend(tfiarray, pending_tfi_channel);
        // Update tracking for this specific channel
        loaded_tfi[pending_tfi_channel-1] = tfifilenumber[pending_tfi_channel-1];
    }

    // Clear the loading state
    tfi_pending_load = false;
    showing_loading = false;
    
    // Update display to remove "loading..." message
    if (booted == 1 && (mode == 1 || mode == 3)) {
        updateFileDisplay();
    }
}

void savenew(void) {
    oled_clear();
    oled_print(0, 0, "SAVING...");
    oled_print(0, 16, "please wait");
    oled_refresh();
    
    // Find new patch filename
    while (savenumber < 1000) {
        sprintf(savefilefull, "/tfi/user/newpatch%03d.tfi", savenumber);
        if (!SD.exists(savefilefull)) {
            break;  // Found available filename
        }
        savenumber++;
    }
    
    dataFile = SD.open(savefilefull, FILE_WRITE);
    if (!dataFile) {
        oled_clear();
        oled_print(0, 0, "SD: Write failed");
        oled_print(0, 16, "Check SD card");
        oled_print(0, 24, "Space available?");
        oled_refresh();
        delay(3000);
        return;
    }
    
    // Convert fmsettings back into a tfi file
    int tfiarray[42];
    tfichannel = 1;
    
    tfiarray[0] = fmsettings[tfichannel-1][0] / 16;    // Algorithm
    tfiarray[1] = fmsettings[tfichannel-1][1] / 16;    // Feedback
    tfiarray[2] = fmsettings[tfichannel-1][2] / 8;     // OP1 Multiplier
    tfiarray[12] = fmsettings[tfichannel-1][12] / 8;   // OP3 Multiplier
    tfiarray[22] = fmsettings[tfichannel-1][22] / 8;   // OP2 Multiplier
    tfiarray[32] = fmsettings[tfichannel-1][32] / 8;   // OP4 Multiplier
    tfiarray[3] = fmsettings[tfichannel-1][3] / 32;    // OP1 Detune
    tfiarray[13] = fmsettings[tfichannel-1][13] / 32;  // OP3 Detune
    tfiarray[23] = fmsettings[tfichannel-1][23] / 32;  // OP2 Detune
    tfiarray[33] = fmsettings[tfichannel-1][33] / 32;  // OP4 Detune
    tfiarray[4] = 127 - fmsettings[tfichannel-1][4];   // OP1 Total Level
    tfiarray[14] = 127 - fmsettings[tfichannel-1][14]; // OP3 Total Level
    tfiarray[24] = 127 - fmsettings[tfichannel-1][24]; // OP2 Total Level
    tfiarray[34] = 127 - fmsettings[tfichannel-1][34]; // OP4 Total Level
    tfiarray[5] = fmsettings[tfichannel-1][5] / 32;    // OP1 Rate Scaling
    tfiarray[15] = fmsettings[tfichannel-1][15] / 32;  // OP3 Rate Scaling
    tfiarray[25] = fmsettings[tfichannel-1][25] / 32;  // OP2 Rate Scaling
    tfiarray[35] = fmsettings[tfichannel-1][35] / 32;  // OP4 Rate Scaling
    tfiarray[6] = fmsettings[tfichannel-1][6] / 4;     // OP1 Attack Rate
    tfiarray[16] = fmsettings[tfichannel-1][16] / 4;   // OP3 Attack Rate
    tfiarray[26] = fmsettings[tfichannel-1][26] / 4;   // OP2 Attack Rate
    tfiarray[36] = fmsettings[tfichannel-1][36] / 4;   // OP4 Attack Rate
    tfiarray[7] = fmsettings[tfichannel-1][7] / 4;     // OP1 1st Decay Rate
    tfiarray[17] = fmsettings[tfichannel-1][17] / 4;   // OP3 1st Decay Rate
    tfiarray[27] = fmsettings[tfichannel-1][27] / 4;   // OP2 1st Decay Rate
    tfiarray[37] = fmsettings[tfichannel-1][37] / 4;   // OP4 1st Decay Rate
    tfiarray[10] = (127 - fmsettings[tfichannel-1][10]) / 8; // OP1 2nd Total Level
    tfiarray[20] = (127 - fmsettings[tfichannel-1][20]) / 8; // OP3 2nd Total Level
    tfiarray[30] = (127 - fmsettings[tfichannel-1][30]) / 8; // OP2 2nd Total Level
    tfiarray[40] = (127 - fmsettings[tfichannel-1][40]) / 8; // OP4 2nd Total Level
    tfiarray[8] = fmsettings[tfichannel-1][8] / 8;     // OP1 2nd Decay Rate
    tfiarray[18] = fmsettings[tfichannel-1][18] / 8;   // OP3 2nd Decay Rate
    tfiarray[28] = fmsettings[tfichannel-1][28] / 8;   // OP2 2nd Decay Rate
    tfiarray[38] = fmsettings[tfichannel-1][38] / 8;   // OP4 2nd Decay Rate
    tfiarray[9] = fmsettings[tfichannel-1][9] / 8;     // OP1 Release Rate
    tfiarray[19] = fmsettings[tfichannel-1][19] / 8;   // OP3 Release Rate
    tfiarray[29] = fmsettings[tfichannel-1][29] / 8;   // OP2 Release Rate
    tfiarray[39] = fmsettings[tfichannel-1][39] / 8;   // OP4 Release Rate
    tfiarray[11] = fmsettings[tfichannel-1][11] / 8;   // OP1 SSG-EG
    tfiarray[21] = fmsettings[tfichannel-1][21] / 8;   // OP3 SSG-EG
    tfiarray[31] = fmsettings[tfichannel-1][31] / 8;   // OP2 SSG-EG
    tfiarray[41] = fmsettings[tfichannel-1][41] / 8;   // OP4 SSG-EG
    
    for (int i = 0; i < 42; i++) {
        dataFile.write((uint8_t)tfiarray[i]);
    }
    
    dataFile.close();
    
    oled_clear();
    oled_print(0, 0, "SAVED!");
    oled_print(0, 16, savefilefull);
    oled_refresh();
    delay(2000);
    
    messagestart = millis();
    refreshscreen = 1;
    
    scandir(true);  // Rescan directory
    
    // Show filename on screen
    char display_buffer[32];
    sprintf(display_buffer, "P %03d/%03d", tfifilenumber[0] + 1, n);
    oled_clear();
    oled_print(0, 0, display_buffer);
    oled_print(0, 16, savefilefull);
    oled_refresh();
}

void saveoverwrite(void) {
    uint16_t idx = tfifilenumber[tfichannel-1];
    if (idx >= n) return;

    dataFile = SD.open(fullnames[idx], FILE_WRITE);
    if (!dataFile) {
        oled_clear(); oled_print(0, 0, "CANNOT WRITE TFI"); oled_refresh(); return;
    }

    // Convert fmsettings back into a tfi file
    int tfiarray[42];
    tfichannel = 1;

    tfiarray[0] = fmsettings[tfichannel-1][0] / 16;    // Algorithm
    tfiarray[1] = fmsettings[tfichannel-1][1] / 16;    // Feedback
    tfiarray[2] = fmsettings[tfichannel-1][2] / 8;     // OP1 Multiplier
    tfiarray[3] = fmsettings[tfichannel-1][3] / 32;    // OP1 Attack Rate
    tfiarray[4] = fmsettings[tfichannel-1][4];         // OP1 Decay Rate
    tfiarray[5] = fmsettings[tfichannel-1][5];         // OP1 Sustain Rate
    tfiarray[6] = fmsettings[tfichannel-1][6];         // OP1 Release Rate
    tfiarray[7] = fmsettings[tfichannel-1][7];         // OP1 Sustain Level
    tfiarray[8] = fmsettings[tfichannel-1][8];         // OP1 Total Level
    tfiarray[9] = fmsettings[tfichannel-1][9];         // OP1 Key Scaling
    tfiarray[10] = fmsettings[tfichannel-1][10];       // OP1 Detune
    tfiarray[11] = fmsettings[tfichannel-1][11];       // OP1 Amplitude Modulation
    tfiarray[12] = fmsettings[tfichannel-1][12] / 8;   // OP3 Multiplier
    tfiarray[13] = fmsettings[tfichannel-1][13] / 32;  // OP3 Attack Rate
    tfiarray[14] = fmsettings[tfichannel-1][14] / 16;  // OP3 Decay Rate
    tfiarray[15] = fmsettings[tfichannel-1][15] / 16;  // OP3 Sustain Rate
    tfiarray[16] = fmsettings[tfichannel-1][16];       // OP3 Release Rate
    tfiarray[17] = fmsettings[tfichannel-1][17];       // OP3 Sustain Level
    tfiarray[18] = fmsettings[tfichannel-1][18];       // OP3 Total Level
    tfiarray[19] = fmsettings[tfichannel-1][19];       // OP3 Key Scaling
    tfiarray[20] = fmsettings[tfichannel-1][20];       // OP3 Detune
    tfiarray[21] = fmsettings[tfichannel-1][21];       // OP3 Amplitude Modulation
    tfiarray[22] = fmsettings[tfichannel-1][22] / 8;   // OP2 Multiplier
    tfiarray[23] = fmsettings[tfichannel-1][23] / 32;  // OP2 Attack Rate
    tfiarray[24] = fmsettings[tfichannel-1][24] / 16;  // OP2 Decay Rate
    tfiarray[25] = fmsettings[tfichannel-1][25] / 16;  // OP2 Sustain Rate
    tfiarray[26] = fmsettings[tfichannel-1][26];       // OP2 Release Rate
    tfiarray[27] = fmsettings[tfichannel-1][27];       // OP2 Sustain Level
    tfiarray[28] = fmsettings[tfichannel-1][28];       // OP2 Total Level
    tfiarray[29] = fmsettings[tfichannel-1][29];       // OP2 Key Scaling
    tfiarray[30] = fmsettings[tfichannel-1][30];       // OP2 Detune
    tfiarray[31] = fmsettings[tfichannel-1][31];       // OP2 Amplitude Modulation
    tfiarray[32] = fmsettings[tfichannel-1][32] / 8;   // OP4 Multiplier
    tfiarray[33] = fmsettings[tfichannel-1][33] / 32;  // OP4 Attack Rate
    tfiarray[34] = fmsettings[tfichannel-1][34] / 16;  // OP4 Decay Rate
    tfiarray[35] = fmsettings[tfichannel-1][35];       // OP4 Sustain Rate
    tfiarray[36] = fmsettings[tfichannel-1][36];       // OP4 Release Rate
    tfiarray[37] = fmsettings[tfichannel-1][37];       // OP4 Sustain Level
    tfiarray[38] = fmsettings[tfichannel-1][38];       // OP4 Total Level
    tfiarray[39] = fmsettings[tfichannel-1][39];       // OP4 Key Scaling
    tfiarray[40] = fmsettings[tfichannel-1][40];       // OP4 Detune
    tfiarray[41] = fmsettings[tfichannel-1][41];       // OP4 Amplitude Modulation

    for (int i = 0; i < 42; i++) dataFile.write((uint8_t)tfiarray[i]);
    dataFile.close();

    oled_clear();
    oled_print(0, 0, "SAVED!");
    oled_print(0, 16, filenames[idx]);
    oled_refresh();
    delay(500);

    messagestart = millis();
    refreshscreen = 1;

    scandir(true); // Rescan directory
}

void deletefile(void) {
    menuprompt = 0;

    oled_clear();
    oled_print(0, 0, "CONFIRM DELETE");
    oled_print(0, 16, "DEL:yes PRST:no");
    oled_refresh();
    delay(1000);

    while (menuprompt == 0) {
        handle_midi_input();
        lcd_key = read_buttons();
        switch (lcd_key) {
            case btnBLANK: // confirm
            {
                uint16_t idx = tfifilenumber[tfichannel-1];
                if (idx >= n) { oled_clear(); oled_print(0, 0, "DELETE ERROR"); oled_refresh(); delay(2000); return; }

                if (!SD.remove(fullnames[idx])) {
                    oled_clear(); oled_print(0, 0, "DELETE ERROR"); oled_refresh(); delay(2000);
                    return;
                }

                oled_clear();
                oled_print(0, 0, "FILE DELETED!");
                oled_print(0, 16, filenames[idx]);
                oled_refresh();
                delay(1000);

                scandir(false);                // refresh list (also rewrites index)
                if (n == 0) { messagestart = millis(); refreshscreen = 1; return; }
                if (tfifilenumber[tfichannel-1] >= n) tfifilenumber[tfichannel-1] = n-1;

                messagestart = millis();
                refreshscreen = 1;
                menuprompt = 1;
                break;
            }
            case btnSELECT: // cancel
                oled_clear(); oled_print(0, 0, "cancelled"); oled_refresh(); delay(500);
                menuprompt = 1;
                break;
            default:
                break;
        }
    }
    refreshscreen = 1;
}

void saveCurrentPreset(void) {
    oled_clear();
    oled_print(0, 0, "SAVING PRESET...");
    oled_print(0, 16, "please wait");
    oled_refresh();
    
    // Find new preset filename in presets folder
    char presetfilefull[64];
    while (presetsavenumber < 1000) {
        sprintf(presetfilefull, "/presets/%03d.mdm_pre", presetsavenumber);  // Changed path
        if (!SD.exists(presetfilefull)) {
            break;
        }
        presetsavenumber++;
    }
    
    File presetFile = SD.open(presetfilefull, FILE_WRITE);
    if (!presetFile) {
        oled_clear();
        oled_print(0, 0, "SD: Preset save");
        oled_print(0, 16, "failed. Check");
        oled_print(0, 24, "SD card space");
        oled_refresh();
        delay(3000);
        presetManagerDisplay();
        return;
    }
    
    // Write header
    presetFile.write('T');  // TFI filename-based preset
    presetFile.write('F');
    presetFile.write('N');  // TFI by Name
    presetFile.write('P');  // TFNP header
    presetFile.write(1);    // Version
    
    // Save the actual TFI filenames for each channel
    for (int channel = 0; channel < 6; channel++) {
        uint16_t tfi_index = tfifilenumber[channel];
        
        if (tfi_index < n && n > 0) {
            // Write the full filename (with path and extension)
            const char* filename = fullnames[tfi_index];
            uint8_t len = strlen(filename);
            presetFile.write(len);  // Write length first
            presetFile.write((const uint8_t*)filename, len);  // Write filename
        } else {
            // No valid TFI loaded on this channel
            presetFile.write((uint8_t)0);  // Zero length = no file
        }
    }
    
    // Save global settings
    presetFile.write((uint8_t)lfospeed);
    presetFile.write((uint8_t)polypan);
    presetFile.write((uint8_t)polyvoicenum);
    presetFile.write((uint8_t)midichannel);
    presetFile.write((uint8_t)region);
    
    presetFile.flush();
    presetFile.close();
    
    // Verify the file was created
    if (SD.exists(presetfilefull)) {
        oled_clear();
        oled_print(0, 0, "PRESET SAVED!");
        oled_print(0, 16, &presetfilefull[1]);
        oled_refresh();
        delay(1500);
        
        scanPresetDir();
        
        // Find the file we just saved and select it
        for (int i = 0; i < preset_n; i++) {
            if (strstr(presetfullnames[i], presetfilefull) != NULL) {
                presetfilenumber = i;
                break;
            }
        }
    } else {
        oled_clear();
        oled_print(0, 0, "SAVE FAILED!");
        oled_refresh();
        delay(2000);
    }
    
    presetManagerDisplay();
}

void loadSelectedPreset(void) {
    if (preset_n == 0 || presetfilenumber >= preset_n) {
        oled_clear();
        oled_print(0, 0, "NO PRESET TO LOAD");
        oled_refresh();
        delay(1000);
        presetManagerDisplay();
        return;
    }
    
    oled_clear();
    oled_print(0, 0, "LOADING PRESET...");
    oled_print(0, 16, presetfilenames[presetfilenumber]);
    oled_refresh();
    
    File presetFile = SD.open(presetfullnames[presetfilenumber], FILE_READ);
    if (!presetFile) {
        oled_clear();
        oled_print(0, 0, "PRESET LOAD ERROR");
        oled_refresh();
        delay(2000);
        presetManagerDisplay();
        return;
    }
    
    // Check for different header types
    char header[4];
    header[0] = presetFile.read();
    header[1] = presetFile.read();
    header[2] = presetFile.read();
    header[3] = presetFile.read();
    
    bool isFilenameBasedPreset = (header[0] == 'T' && header[1] == 'F' && header[2] == 'N' && header[3] == 'P');
    bool isIndexBasedPreset = (header[0] == 'T' && header[1] == 'F' && header[2] == 'I' && header[3] == 'P');
    
    if (!isFilenameBasedPreset && !isIndexBasedPreset) {
        presetFile.close();
        oled_clear();
        oled_print(0, 0, "INVALID PRESET");
        oled_refresh();
        delay(2000);
        presetManagerDisplay();
        return;
    }
    
    uint8_t version = presetFile.read();
    if (version != 1) {
        presetFile.close();
        oled_clear();
        oled_print(0, 0, "WRONG VERSION");
        oled_refresh();
        delay(2000);
        presetManagerDisplay();
        return;
    }
    
    uint8_t loaded_channels = 0;
    uint8_t skipped_channels = 0;
    
    if (isFilenameBasedPreset) {
        // MONO MODE: Load individual TFIs per channel
        for (int channel = 0; channel < 6; channel++) {
            if (!presetFile.available()) break;
            
            uint8_t filename_len = presetFile.read();
            
            if (filename_len == 0) {
                // No file for this channel
                skipped_channels++;
                continue;
            }
            
            // Read the filename
            char saved_filename[FullNameChars];
            if (filename_len >= FullNameChars) filename_len = FullNameChars - 1;
            
            presetFile.read((uint8_t*)saved_filename, filename_len);
            saved_filename[filename_len] = '\0';
            
            // Find this filename in our current file list
            int found_index = -1;
            for (int i = 0; i < n; i++) {
                if (strcmp(fullnames[i], saved_filename) == 0) {
                    found_index = i;
                    break;
                }
            }
            
            if (found_index >= 0) {
                // Found the file, load it
                tfifilenumber[channel] = found_index;  // Update TFI select index
                loaded_tfi[channel] = found_index;     // Update loaded tracking
                tfichannel = channel + 1;
                
                oled_clear();
                oled_print(0, 0, "Loading CH");
                oled_print(64, 0, String(channel + 1).c_str());
                oled_print(0, 16, filenames[found_index]);
                oled_refresh();
                
                tfiLoadImmediateOnChannel(channel + 1);
                loaded_channels++;
                delay(100);
            } else {
                // File not found - keep current TFI on this channel
                skipped_channels++;
                oled_clear();
                oled_print(0, 0, "SKIP CH");
                oled_print(64, 0, String(channel + 1).c_str());
                oled_print(0, 16, "TFI not found");
                oled_refresh();
                delay(300);
            }
            
            handle_midi_input();
        }
        
    } else {
        // Load by index (old format for compatibility)
        // MONO MODE: Original index-based loading
        for (int channel = 0; channel < 6; channel++) {
            if (presetFile.available() >= 2) {
                uint8_t low_byte = presetFile.read();
                uint8_t high_byte = presetFile.read();
                uint16_t tfi_index = low_byte | (high_byte << 8);
                
                if (tfi_index < n) {
                    tfifilenumber[channel] = tfi_index;  // Update TFI select index
                    loaded_tfi[channel] = tfi_index;     // Update loaded tracking
                    tfichannel = channel + 1;
                    
                    oled_clear();
                    oled_print(0, 0, "Loading CH");
                    oled_print(64, 0, String(channel + 1).c_str());
                    oled_print(0, 16, filenames[tfi_index]);
                    oled_refresh();
                    
                    tfiLoadImmediateOnChannel(channel + 1);
                    loaded_channels++;
                    delay(100);
                } else {
                    skipped_channels++;
                }
            }
            handle_midi_input();
        }
    }
    
    // Load global settings (same position in both formats)
    if (presetFile.available()) lfospeed = presetFile.read();
    if (presetFile.available()) polypan = presetFile.read();
    if (presetFile.available()) polyvoicenum = presetFile.read();
    if (presetFile.available()) {
        uint8_t preset_midi_ch = presetFile.read();
        // Only update midichannel if we're in mono mode
        if (mode == 7) {
            midichannel = preset_midi_ch;
        }
    }
    if (presetFile.available()) region = presetFile.read();
    
    presetFile.close();
    
    // Show completion summary
    oled_clear();
    oled_print(0, 0, "BANK LOADED!");
    char summary[32];
    sprintf(summary, "CH:%d Skip:%d", loaded_channels, skipped_channels);
    oled_print(0, 16, summary);
    oled_refresh();
    delay(2000);
    
    // CRITICAL: Ensure TFI select screen reflects the loaded preset
    if (mode == 7) {  // If we're in mono preset manager
        // Set the currently selected channel to channel 1 for consistency
        tfichannel = 1;
        
        // Force a display refresh when returning to TFI select
        messagestart = millis();
        refreshscreen = 1;
    }
    
    presetManagerDisplay();
}

void deleteSelectedPreset(void) {
    if (preset_n == 0 || presetfilenumber >= preset_n) {
        oled_clear();
        oled_print(0, 0, "NO PRESET TO DEL");
        oled_refresh();
        delay(1000);
        presetManagerDisplay();
        return;
    }
    
    menuprompt = 0;
    
    oled_clear();
    oled_print(0, 0, "CONFIRM DELETE");
    oled_print(0, 16, "DEL:yes PRST:no");
    oled_refresh();
    delay(1000);
    
    while (menuprompt == 0) {
        handle_midi_input();
        lcd_key = read_buttons();
        switch (lcd_key) {
            case btnBLANK: // confirm
                if (!SD.remove(presetfullnames[presetfilenumber])) {
                    oled_clear();
                    oled_print(0, 0, "DELETE ERROR");
                    oled_refresh();
                    delay(2000);
                    presetManagerDisplay();
                    return;
                }
                
                oled_clear();
                oled_print(0, 0, "PRESET DELETED!");
                oled_print(0, 16, presetfilenames[presetfilenumber]);
                oled_refresh();
                delay(1000);
                
                scanPresetDir(); // Refresh list
                if (presetfilenumber >= preset_n && preset_n > 0) {
                    presetfilenumber = preset_n - 1;
                }
                
                menuprompt = 1;
                presetManagerDisplay();
                break;
                
            case btnSELECT: // cancel
                oled_clear();
                oled_print(0, 0, "cancelled");
                oled_refresh();
                delay(500);
                menuprompt = 1;
                presetManagerDisplay();
                break;
                
            default:
                break;
        }
    }
}