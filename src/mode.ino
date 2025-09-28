void modechange(int modetype) {
  // modetype: 1 = SELECT button, 2 = POLY button
  if (modetype == 1) {
    cycleEditMode();
  } else if (modetype == 2) {
    togglePolyMode();
  }
}

void modechangemessage(void) {
  if ((millis() - messagestart) > messagedelay && refreshscreen == 1) {
    switch (mode) {
      case 1:
        channelselect();
        refreshscreen = 0;
        break;
      case 2:
        fmparamdisplay();
        refreshscreen = 0;
        break;
      case 3:
        channelselect();
        refreshscreen = 0;
        break;
      case 4:
        fmparamdisplay();
        refreshscreen = 0;
        break;
      case 5:  // MONO VISUALIZER
        //visualizerDisplay();
        refreshscreen = 0;
        break;
      case 6:  // POLY VISUALIZER
        //visualizerDisplay();
        refreshscreen = 0;
        break;
      case 7:  // MONO PRESET MANAGER
        presetManagerDisplay();
        refreshscreen = 0;
        break;
      case 8:  // POLY PRESET MANAGER - DISABLED
        oled_clear();
        oled_print(0, 0, "POLY PRESETS");
        oled_print(0, 16, "DISABLED");
        oled_print(0, 24, "Use MONO mode only");
        oled_refresh();
        refreshscreen = 0;
        break;
      case 9:  // SETTINGS
        settingsDisplay();
        refreshscreen = 0;
        break;
    }
  }
}

void cycleEditMode() {
  if (poly_mode == 0) { // MONO
    // NEW ORDER: VIZ(0) -> PRESET(1) -> EDIT(2) -> BANK_MGR(3) -> SETTINGS(4) -> VIZ(0)
    edit_mode++;
    if (edit_mode > 4) edit_mode = 0;
} else { // POLY
    // NEW ORDER: VIZ(0) -> PRESET(1) -> EDIT(2) -> VIZ(0)
    // Bank MGR (3) and Settings (4) are disabled in POLY
    edit_mode++;
    if (edit_mode > 2) edit_mode = 0;  // Only cycle through 0,1,2
}
  
  // Update the global mode variable
  updateGlobalMode();
  
  // Show mode change message
  showModeMessage();
}

void showModeMessage() {
  tfi_pending_load = false;
  showing_loading = false;
  
  // Update visualizer
  updateVisualizerMode(mode);
  
  messagestart = millis();
  refreshscreen = 1;
  
  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);
  
  // Display the appropriate mode message based on global mode
  switch(mode) {
    case 1:  // MONO PRESET
      display.print("MONO | Preset");
      primePotBaselines();
      break;
    case 2:  // MONO EDIT
      display.print("MONO | FM Edit");
      primePotBaselines();
      break;
    case 3:  // POLY PRESET
      display.print("POLY | Preset");
      break;
    case 4:  // POLY EDIT
      display.print("POLY | FM Edit");
      primePotBaselines();
      break;
    case 5:  // MONO VIZ
      display.print("MONO | Visualizer");
      break;
    case 6:  // POLY VIZ
      display.print("POLY | Visualizer");
      break;
    case 7:  // MONO BANK MGR
      display.print("MONO | Bank Mgr");
      break;
    case 9:  // SETTINGS
      display.print("SETTINGS");
      temp_midichannel = midichannel;
      temp_region = region;
      temp_velocity_curve = velocity_curve;
      settings_changed = false;
      settings_screen = 1;
      // Initialize pot values
      primePotBaselines();
      break;
  }
  
  display.display();
  mutex_exit(&display_mutex);
  
  // Delay for non-visualizer modes
  if (mode != 5 && mode != 6) {
    delay(20);
  }
}

void showVizSubModeMessage() {
  messagestart = millis();
  refreshscreen = 1;

  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);

  if (poly_mode == 0) {
    display.print("MONO | ");
  } else {
    display.print("POLY | ");
  }

  if (viz_sub_mode == 0) {
    display.print("Bar Graph");
  } else if (viz_sub_mode == 1) {
    display.print("Asteroids");
  } else if (viz_sub_mode == 2) {
    display.print("Starfighter");
  } else {
    display.print("Neural Net");
  }

  display.display();
  mutex_exit(&display_mutex);

  delay(20);
}

void togglePolyMode() {
  prev_poly_mode = poly_mode;
  poly_mode = !poly_mode;
  
   // If switching to poly mode and was in preset manager, go to preset mode
  if (poly_mode == 1 && edit_mode == 3) {
    edit_mode = 0;
  }
  
  // Update the global mode variable
  updateGlobalMode();
  
  // Show mode change message
  showModeMessage();
  // Only reset voices when actually changing poly mode
  if (prev_poly_mode != poly_mode) {
    resetVoicesAndNotes();

    // When switching from MONO to POLY, apply current TFI to all channels
    if (poly_mode == 1 && prev_poly_mode == 0) {
      uint16_t current_tfi = tfifilenumber[tfichannel-1];

      // Set all channels to use the same TFI as the current mono channel
      for (int i = 0; i < 6; i++) {
        tfifilenumber[i] = current_tfi;
      }

      // Apply the TFI to all channels immediately
      if (n > 0 && current_tfi < n) {
        applyTFIToAllChannelsImmediate();
      }
    }
  }
}

void updateGlobalMode() {
  if (poly_mode == 0) { // MONO
    switch(edit_mode) {
      case 0: mode = 5; break;  // MONO VIZ
      case 1: mode = 1; break;  // MONO PRESET
      case 2: mode = 2; break;  // MONO EDIT
      case 3: mode = 7; break;  // MONO BANK_MGR
      case 4: mode = 9; break;  // SETTINGS
    }
} else { // POLY
  switch(edit_mode) {
    case 0: mode = 6; break;  // POLY VIZ
    case 1: mode = 3; break;  // POLY PRESET
    case 2: mode = 4; break;  // POLY EDIT
    case 3: mode = 6; break;  // Fallback to VIZ (bank mgr disabled)
    case 4: mode = 6; break;  // Fallback to VIZ (settings disabled)
  }
}
}