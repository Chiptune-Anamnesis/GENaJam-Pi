void bootprompt(void) {
  uint8_t currentpotvalue[4];
  uint8_t lastDisplayedChannel = 255;
  uint8_t lastDisplayedRegion = 255;

  menuprompt = 0;

  // Protected display update for boot prompt
  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("MIDI CH / REGION");
  display.display();
  mutex_exit(&display_mutex);

  while (menuprompt == 0) {
    handle_midi_input();

    // Read OP1 (channel 0) for MIDI channel
    selectMuxChannel(0);
    delayMicroseconds(50);
    currentpotvalue[0] = analogRead(MUX_SIG_PIN) >> 3;

    // Read OP3 (channel 2) for region
    selectMuxChannel(2);
    delayMicroseconds(50);
    currentpotvalue[2] = analogRead(MUX_SIG_PIN) >> 3;

    midichannel = (currentpotvalue[0] / 8) + 1;
    if (midichannel > 16) midichannel = 16;
    if (midichannel < 1) midichannel = 1;

    uint8_t currentRegion = (currentpotvalue[2] < 64) ? 0 : 1;

    // Only update display if values changed (reduce flicker)
    if (midichannel != lastDisplayedChannel || currentRegion != lastDisplayedRegion) {
      mutex_enter_blocking(&display_mutex);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("MIDI CH / REGION");

      display.setCursor(0, 16);
      display.print("CH: ");
      display.print(midichannel);
      display.print("  REG: ");
      if (currentRegion == 0) {
        display.print("NTSC");
      } else {
        display.print("PAL");
      }

      display.setCursor(0, 24);
      display.print("PRESET to confirm");

      display.display();
      mutex_exit(&display_mutex);

      lastDisplayedChannel = midichannel;
      lastDisplayedRegion = currentRegion;
    }

    // Check for button press to confirm settings
    lcd_key = read_buttons();
    switch (lcd_key) {
      case btnSELECT:
        region = currentRegion;

        // Apply region setting immediately
        if (region == 0) {
          midi_send_cc(1, 83, 75);  // NTSC
        } else {
          midi_send_cc(1, 83, 1);  // PAL
        }

        // Save settings to EEPROM
        flash_write_settings(region, midichannel, velocity_curve);

        // Show confirmation
        mutex_enter_blocking(&display_mutex);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("SETTINGS SAVED!");
        display.setCursor(0, 16);
        display.print("CH:");
        display.print(midichannel);
        display.print(" REG:");
        display.print(region == 0 ? "NTSC" : "PAL");
        display.display();
        mutex_exit(&display_mutex);

        delay(1500);
        menuprompt = 1;  // Exit the loop
        break;

      case btnUP:
      case btnDOWN:
      case btnLEFT:
      case btnRIGHT:
        // Ignore other buttons during boot prompt
        break;

      default:
        break;
    }

    // Prevent excessive polling
    delay(50);
  }

  // Final confirmation display
  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Ready to boot!");
  display.display();
  mutex_exit(&display_mutex);

  delay(1000);
}

void saveSettingsToSD(void) {
  // Create settings folder if it doesn't exist
  if (!SD.exists("/settings")) {
    SD.mkdir("/settings");
  }

  // Remove existing file to ensure clean overwrite
  if (SD.exists("/settings/settings.ini")) {
    SD.remove("/settings/settings.ini");
  }

  // Open settings file for writing (now creates a fresh file)
  File settingsFile = SD.open("/settings/settings.ini", FILE_WRITE);
  if (!settingsFile) {
    return; // Failed to create file
  }

  // Write settings in INI format
  settingsFile.println("[GENAJAM_SETTINGS]");
  settingsFile.print("region=");
  settingsFile.println(region);
  settingsFile.print("midichannel=");
  settingsFile.println(midichannel);
  settingsFile.print("velocity_curve=");
  settingsFile.println(velocity_curve);
  settingsFile.println("# 0=NTSC, 1=PAL");
  settingsFile.println("# MIDI channel 1-16");
  settingsFile.println("# Velocity curve 0-4");

  settingsFile.flush(); // Ensure data is written to SD card
  settingsFile.close();
  delay(50); // SD card write completion
}

bool loadSettingsFromSD(void) {
  // Check if settings file exists
  if (!SD.exists("/settings/settings.ini")) {
    // Settings file doesn't exist
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("No settings.ini");
    display.display();
    delay(1000);
    return false; // File doesn't exist
  }

  // Settings file found
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Loading SD settings");
  display.display();
  delay(500);

  File settingsFile = SD.open("/settings/settings.ini", FILE_READ);
  if (!settingsFile) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Can't open settings");
    display.display();
    delay(1000);
    return false; // Failed to open file
  }

  String line;
  while (settingsFile.available()) {
    line = settingsFile.readStringUntil('\n');
    line.trim();

    // Skip comments and empty lines
    if (line.length() == 0 || line.startsWith("#") || line.startsWith("[")) {
      continue;
    }

    // Parse key=value pairs
    int equalPos = line.indexOf('=');
    if (equalPos > 0) {
      String key = line.substring(0, equalPos);
      String value = line.substring(equalPos + 1);

      if (key == "region") {
        region = value.toInt();
        if (region > 1) region = 0; // Validate
      } else if (key == "midichannel") {
        midichannel = value.toInt();
        if (midichannel < 1 || midichannel > 16) midichannel = 1; // Validate
      } else if (key == "velocity_curve") {
        velocity_curve = value.toInt();
        if (velocity_curve > 4) velocity_curve = 4; // Validate
      }
    }
  }

  settingsFile.close();

  // Settings loaded successfully
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("SD settings loaded");
  display.setCursor(0, 16);
  display.print("CH:");
  display.print(midichannel);
  display.print(" REG:");
  display.print(region);
  display.display();
  delay(1500);

  return true; // Successfully loaded
}

void saveSettings(void) {
  bool needs_restart = false;

  if (temp_midichannel != midichannel) {
    midichannel = temp_midichannel;
    needs_restart = true;
  }

  if (temp_region != region) {
    region = temp_region;
    needs_restart = true;

    // Apply region change immediately
    if (region == 0) {
      midi_send_cc(1, 83, 75);  // NTSC
    } else {
      midi_send_cc(1, 83, 1);  // PAL
    }
  }

  if (temp_velocity_curve != velocity_curve) {
    velocity_curve = temp_velocity_curve;
    needs_restart = true;
  }

  if (needs_restart) {
    flash_write_settings(region, midichannel, velocity_curve);
    saveSettingsToSD(); // Also save to SD card backup

    oled_clear();
    oled_print(0, 0, "SETTINGS SAVED!");
    if (needs_restart) {
      oled_print(0, 16, "Changes applied");
    }
    oled_refresh();
    delay(1500);
  }

  settings_changed = false;
}