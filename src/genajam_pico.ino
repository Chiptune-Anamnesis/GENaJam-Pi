const char* GENAJAM_VERSION = "v1.31";
// GENajam Arduino Pico Port - Crunchypotato 2025-SEPTEMBER
// Originally by/forked from JAMATAR 2021-AUGUST
// Version information
// Safety macros for array bounds checking
// --------------------
// This is a front end for Little-scale's GENMDM module for Mega Drive
// Now for: Raspberry Pi Pico (RP2040) using Arduino IDE
// OLED display via I2C, SD card via SPI
#define SAFE_CHANNEL_INDEX(ch) ((ch >= 1 && ch <= 6) ? (ch-1) : 0)
#define SAFE_POLY_INDEX(idx) ((idx >= 0 && idx < 6) ? idx : 0)
#define SAFE_TFI_INDEX(idx) ((idx >= 0 && idx < n && n > 0) ? idx : 0)


#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <EEPROM.h>
#include <MIDI.h>
#include "pico/multicore.h"
#include "pico/mutex.h"
#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MIDI usb_midi;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// Pin definitions
#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9
#define SD_CS_PIN 5
#define SD_MISO_PIN 4
#define SD_SCK_PIN 6
#define SD_MOSI_PIN 7
#define MIDI_RX_PIN 1
#define MIDI_TX_PIN 0
#define POT_OP1_PIN 26
#define POT_OP2_PIN 27
#define BTN_PRESET_PIN 20
#define BTN_LEFT_PIN 14
#define BTN_RIGHT_PIN 15
#define BTN_CH_UP_PIN 16
#define BTN_CH_DOWN_PIN 17
#define BTN_MONO_POLY_PIN 18
#define BTN_DELETE_PIN 2
#define BTN_PANIC_PIN 22
#define BTN_OPT1_PIN 21  // Future Use
#define BTN_OPT2_PIN 3   // Future Use

// Multiplexer control pins
#define MUX_S0_PIN 10
#define MUX_S1_PIN 11
#define MUX_S2_PIN 12
#define MUX_S3_PIN 13
#define MUX_SIG_PIN 28

// Display dimensions and setup
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Button definitions
#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnPOLY 5
#define btnBLANK 6
#define btnOPT1 7
#define btnOPT2 9
#define btnNONE 8

// MIDI constants
#define MIDI_BAUD_RATE 31250

struct VisualizerData {
  uint8_t voice_velocity[11];
  uint8_t voice_peak[11];
  unsigned long voice_peak_time[11];
  uint8_t current_mode;
  bool needs_update;
  float midi_hz;
  unsigned long max_midi_gap;
};

VisualizerData viz_data;
mutex_t viz_mutex;
mutex_t display_mutex;

// Timing constants
const uint16_t debouncedelay = 200;
const uint16_t messagedelay = 700;

// Global variables
uint8_t lcd_key = 0;
unsigned long messagestart = 0;
uint8_t refreshscreen = 0;

// File handling - OPTIMIZED for memory efficiency
const uint8_t MaxNumberOfChars = 48;  // Increased for better filename support
uint16_t n = 0;
const uint16_t nMax = 500;             // Reduced to prevent memory exhaustion
char filenames[nMax][MaxNumberOfChars + 1];
static const uint8_t FullNameChars = 96;
char fullnames[nMax][FullNameChars];
unsigned long tfi_select_time = 0;
bool tfi_pending_load = false;
uint16_t pending_tfi_channel = 1;
bool showing_loading = false;
unsigned long button_hold_start_time = 0;
uint8_t last_held_button = btnNONE;
bool button_is_held = false;
uint64_t last_button_release_time = 0;
const uint16_t initial_debounce = 300;   // Increased debounce for better stability
const uint16_t fast_debounce = 100;      // Fast debounce after acceleration
const uint16_t turbo_debounce = 40;      // Turbo speed for long holds
const uint16_t hold_threshold_1 = 3000;  // 3 seconds to start acceleration (much longer delay)
const uint16_t hold_threshold_2 = 6000;  // 6 seconds to go turbo (much longer delay)
// Current TFI actually loaded on each channel
uint16_t loaded_tfi[6] = { 0, 0, 0, 0, 0, 0 };

File dataFile;

// MIDI and settings
uint16_t tfifilenumber[6] = { 0, 0, 0, 0, 0, 0 };
uint8_t tfichannel = 1;
uint8_t mode = 1; // Default to MONO Mode
uint8_t region = 0;
uint8_t midichannel = 1;
uint8_t last_midi_channel = 0;
uint8_t last_midi_note = 0;
unsigned long last_midi_time = 0;
const uint16_t midi_display_timeout = 2000;
static bool display_needs_refresh = false;
static unsigned long last_display_update = 0;

// Mode
uint8_t poly_mode = 0;      // 0 = MONO, 1 = POLY
uint8_t edit_mode = 1;      // NEW ORDER: 0 = VIZ, 1 = PRESET, 2 = EDIT, 3 = BANK_MGR (mono only), 4 = SETTINGS
                               // Start with PRESET mode (1) instead of VIZ for better boot experience
uint8_t prev_poly_mode = 0;  // Track poly mode changes for resetVoicesAndNotes()


// Settings
uint8_t settings_screen = 1;
uint8_t temp_midichannel = 1;
uint8_t temp_region = 0;
uint8_t velocity_curve = 1;  // 0=Linear, 1=Soft, 2=Medium, 3=Hard
uint8_t temp_velocity_curve = 1;
bool settings_changed = false;
bool system_ready = false;  // MIDI blocking flag during boot/setup
uint8_t viz_sub_mode = 0;  // 0=bar graph, 1=asteroids, 2=starfighter, 3=neural network
void settingsDisplay(void);
void settingsAdjustUp(void);
void settingsAdjustDown(void);
void settingsOperatorDisplay(void);
void saveSettings(void);
void saveSettingsToSD(void);
bool loadSettingsFromSD(void);
uint8_t applyVelocityCurve(uint8_t velocity);
void handleMidiNoteForAsteroids(uint8_t note, uint8_t velocity);
void handleMidiNoteForStarfighter(uint8_t note, uint8_t velocity);
void handleMidiNoteForNeuralNet(uint8_t note, uint8_t velocity);
void showVizSubModeMessage(void);

// MIDI monitoring
unsigned long midi_process_count = 0;
unsigned long midi_last_second = 0;
unsigned long max_midi_gap = 0;
unsigned long last_midi_process = 0;
float midi_hz = 0;

// Save state
uint16_t presetfilenumber = 0;
uint16_t preset_n = 0;
char presetfilenames[nMax][MaxNumberOfChars + 1];
char presetfullnames[nMax][FullNameChars];
uint16_t presetsavenumber = 1;  // How fast bars decay

// Polyphony settings
uint8_t polynote[6] = { 0, 0, 0, 0, 0, 0 };
bool polyon[6] = { 0, 0, 0, 0, 0, 0 };
bool sustainon[6] = { 0, 0, 0, 0, 0, 0 };
bool noteheld[6] = { 0, 0, 0, 0, 0, 0 };
bool sustain = 0;
uint8_t lowestnote = 0;  // Add this back - needed for voice stealing

// FM parameter screen navigation
uint8_t fmscreen = 1;

// Global FM settings
uint8_t fmsettings[6][50];
uint8_t lfospeed = 64;
uint8_t polypan = 64;
uint8_t polyvoicenum = 6;

// Potentiometer values
uint8_t prevpotvalue[4];
bool menuprompt = 1;
bool booted = 0;
uint8_t readStablePot(uint8_t channel) {
  selectMuxChannel(channel);
  delayMicroseconds(100);  // Longer settling time

  // Take multiple readings and filter out outliers
  int readings[5];
  for (int i = 0; i < 5; i++) {
    readings[i] = analogRead(MUX_SIG_PIN);
    delayMicroseconds(20);
  }

  // Sort the readings
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (readings[i] > readings[j]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }

  // Use the middle 3 readings (discard highest and lowest)
  int average = (readings[1] + readings[2] + readings[3]) / 3;
  return average >> 3;
}

// Add these global variables at the top of your file
uint8_t pot_history[4][4];  // Store last 4 readings for each pot (faster response on PCB)
uint8_t history_index = 0;
bool history_filled = false;

uint8_t getFilteredPotValue(uint8_t channel) {
  // Get raw reading
  uint8_t raw_value = readStablePot(channel);

  // Store in circular buffer
  pot_history[channel][history_index] = raw_value;

  // Calculate moving average
  int sum = 0;
  int count = history_filled ? 4 : (history_index + 1);

  for (int i = 0; i < count; i++) {
    sum += pot_history[channel][i];
  }

  return sum / count;
}

// Call this once per loop cycle to update the history index
void updatePotHistory() {
  static unsigned long last_update = 0;
  if (millis() - last_update > 5) {  // Update every 5ms (faster for PCB)
    history_index = (history_index + 1) % 4;
    if (history_index == 0) history_filled = true;
    last_update = millis();
  }
}

// Save file variables
uint16_t savenumber = 1;
char savefilefull[] = "/tfi/newpatch001.tfi";

// Flash storage replacement with EEPROM
void flash_write_settings(uint8_t region_val, uint8_t midi_ch, uint8_t vel_curve);
uint8_t flash_read_setting(uint8_t offset);

// Core functions
void setup_hardware(void);
void setup_midi(void);
void setup_sd(void);
void setup_oled(void);
uint8_t read_buttons(void);

// Mode and display functions
void modechange(int modetype);
void modechangemessage(void);
void bootprompt(void);

// File operations
void scandir(bool saved);
void saveprompt(void);
void savenew(void);
void saveoverwrite(void);
void deletefile(void);

void tfiselect(void);
void channelselect(void);
void tfisend(int opnarray[42], int sendchannel);

// FM synthesis functions
void fmparamdisplay(void);
void operatorparamdisplay(void);
void fmccsend(uint8_t potnumber, uint8_t potvalue);

// MIDI functions
void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value);
void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
void midi_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity);
void midi_send_pitch_bend(uint8_t channel, int16_t bend);
void handle_midi_input(void);
void initializeFMChip(void);

void selectMuxChannel(uint8_t channel) {
  digitalWrite(MUX_S0_PIN, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(MUX_S1_PIN, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(MUX_S2_PIN, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(MUX_S3_PIN, (channel & 0x08) ? HIGH : LOW);
  delayMicroseconds(50);  // Small settling time
}

// Utility functions
void oled_print(int x, int y, const char* text);
void oled_clear(void);
void oled_refresh(void);

// Initialize hardware and start dual-core operation
void setup() {
  // USB MIDI device configuration
  TinyUSBDevice.setID(0x239A, 0x8014); // Adafruit VID/PID for MIDI
  TinyUSBDevice.setManufacturerDescriptor("Adafruit");
  TinyUSBDevice.setProductDescriptor("GenaJam MIDI");
  TinyUSBDevice.setSerialDescriptor("123456");

   // Initialize USB stack
  if (!TinyUSBDevice.begin(0)) {
    Serial.println("Failed to start TinyUSB");
  }
  
  // Start USB MIDI
  usb_midi.begin();

  setup_hardware();
  setup_oled();
  setup_sd();
  setup_midi();

  // Initialize mutex
  mutex_init(&viz_mutex);
  mutex_init(&display_mutex);

  // Initialize shared data
  mutex_enter_blocking(&viz_mutex);
  memset(&viz_data, 0, sizeof(viz_data));
  viz_data.current_mode = 1;  // Start in mono mode
  mutex_exit(&viz_mutex);

  // Start core 1
  multicore_launch_core1(core1_entry);

  EEPROM.begin(512);

  // Try to load settings from SD card backup first
  if (!loadSettingsFromSD()) {
    // Fall back to EEPROM if SD card backup doesn't exist
    region = flash_read_setting(0);
    midichannel = flash_read_setting(1);
    velocity_curve = flash_read_setting(2);

    if (region == 255) region = 0;
    if (midichannel == 255) midichannel = 1;
    if (velocity_curve == 255) velocity_curve = 4; // Default to ImpBox curve (original)
  }

  if (region == 0) {
    midi_send_cc(1, 83, 75);
  } else {
    midi_send_cc(1, 83, 1);
  }

  // Create necessary folders if they don't exist
  if (!SD.exists("/tfi")) {
    if (SD.mkdir("/tfi")) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Created TFI");
      display.setCursor(0, 16);
      display.print("folder");
      display.display();
      delay(1000);
    }
  }

  if (!SD.exists("/presets")) {
    if (SD.mkdir("/presets")) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Created presets");
      display.setCursor(0, 16);
      display.print("folder");
      display.display();
      delay(1000);
    }
  }

  scandir(false);
  scanPresetDir();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Setup complete!");
  display.setCursor(0, 8);
  display.print("Files found: ");
  display.print(n);
  display.setCursor(0, 24);
  display.print("Please wait..");
  display.display();

  // CLEAR any accumulated MIDI data before enabling processing
  delay(100);  // Let any pending MIDI settle

  // Aggressive buffer clearing - drain everything multiple times
  for (int i = 0; i < 10; i++) {
    MIDI.read();  // Clear UART MIDI buffer
    while (usb_midi.available()) {
      uint8_t packet[4];
      usb_midi.readPacket(packet);  // Clear USB MIDI buffer
    }
    delay(10);  // Small delay between clears
  }

  // Reset all MIDI state to prevent phantom notes
  resetVoicesAndNotes();

  // ENABLE MIDI processing - setup is now complete
  system_ready = true;

  // Additional settling time after enabling
  delay(200);

  // Initialize all channels with immediate loading (no delay during setup)
  for (int i = 6; i > 0; i--) {
    tfichannel = i;
    tfiLoadImmediate();
  }

  // Set booted flag to skip old boot prompt
  booted = 1;

  // Wait 2 seconds then auto-start visualizer
  delay(2000);
  edit_mode = 0; // Switch to VISUALIZER mode (now at position 0)
  updateGlobalMode();
  showModeMessage();
}
void loop() {
  // CRITICAL: Process MIDI first and most frequently for lowest latency
  handle_midi_input();

  // Static timers for non-critical updates
  static unsigned long last_slow_update = 0;
  static unsigned long last_fast_update = 0;
  static unsigned long last_midi_stats_update = 0;
  unsigned long current_time = millis();

  // Fast updates every 16ms (60 FPS max)
  if (current_time - last_fast_update >= 16) {
    last_fast_update = current_time;

    // Process MIDI again before any display work
    handle_midi_input();

    // Handle cached display updates for preset modes
    if (display_needs_refresh && edit_mode == 1) {  // Preset mode now at position 1
      updateFileDisplay();
    }
  }

  // Update MIDI stats for core 1 visualizer every 100ms
  if (current_time - last_midi_stats_update >= 100) {
    last_midi_stats_update = current_time;
    updateVisualizerMidiStats(midi_hz, max_midi_gap);
  }

  // Slow updates every 100ms
  if (current_time - last_slow_update >= 100) {
    last_slow_update = current_time;

    // MIDI display timeout check (non-critical)
    if (last_midi_time > 0 && (current_time - last_midi_time) > midi_display_timeout) {
      last_midi_time = 0;
      if (edit_mode == 1) { // Preset mode now at position 1
        display_needs_refresh = true;
      }
    }
  }

  // TFI loading removed - now only loads when SELECT button is pressed

  // Process MIDI before button reading
  handle_midi_input();

  // Button reading and mode handling (keep responsive)
  lcd_key = read_buttons();
  modechangemessage();

  // Process MIDI before entering mode switch
  handle_midi_input();

  // Main control logic based on current modes
  if (edit_mode == 4) { // SETTINGS mode
    switch (lcd_key) {
      case btnRIGHT:
        settings_screen++;
        if (settings_screen > 4) settings_screen = 1;
        settingsDisplay();
        break;

      case btnLEFT:
        settings_screen--;
        if (settings_screen == 0) settings_screen = 4;
        settingsDisplay();
        break;

      case btnUP:
        settingsAdjustUp();
        break;

      case btnDOWN:
        settingsAdjustDown();
        break;

      case btnOPT2:
        if (settings_changed) {
          saveSettings();
          // Stay in settings menu after saving
          settingsDisplay();
        }
        break;

      case btnSELECT:
        // PRESET button now always exits settings menu
        edit_mode = 0; // Return to preset mode
        updateGlobalMode();
        showModeMessage();
        break;

      case btnPOLY:
        togglePolyMode();
        break;
    }
    settingsOperatorDisplay();
  }
  else if (poly_mode == 0) { // MONO MODE
    switch(edit_mode) {
      case 0: // MONO VIZ
        switch (lcd_key) {
          case btnUP:
            viz_sub_mode = (viz_sub_mode + 1) % 4;  // 0=bar graph, 1=asteroids, 2=starfighter, 3=neural network
            showVizSubModeMessage();
            break;

          case btnDOWN:
            viz_sub_mode = (viz_sub_mode == 0) ? 3 : (viz_sub_mode - 1);  // Cycle backward through modes
            showVizSubModeMessage();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnPOLY:
            togglePolyMode();
            break;
        }
        break;

      case 1: // MONO PRESET
        switch (lcd_key) {
          case btnRIGHT:
            // Just browse, don't load automatically
            if (tfichannel >= 1 && tfichannel <= 6) {
              tfifilenumber[SAFE_CHANNEL_INDEX(tfichannel)]++;
              if (tfifilenumber[SAFE_CHANNEL_INDEX(tfichannel)] >= n) {
                tfifilenumber[SAFE_CHANNEL_INDEX(tfichannel)] = 0;
              }
              updateFileDisplay(); // Just update display, no loading
            }
            break;

          case btnLEFT:
            // Just browse, don't load automatically
            if (tfichannel >= 1 && tfichannel <= 6) {
              if (tfifilenumber[SAFE_CHANNEL_INDEX(tfichannel)] == 0) {
                tfifilenumber[SAFE_CHANNEL_INDEX(tfichannel)] = n - 1;
              } else {
                tfifilenumber[SAFE_CHANNEL_INDEX(tfichannel)]--;
              }
              updateFileDisplay(); // Just update display, no loading
            }
            break;

          case btnUP:
            tfichannel++;
            if (tfichannel > 6) tfichannel = 1;
            channelselect();
            break;

          case btnDOWN:
            tfichannel--;
            if (tfichannel == 0) tfichannel = 6;
            channelselect();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnOPT1:
            // Show loading message
            mutex_enter_blocking(&display_mutex);
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Loading TFI...");
            display.display();
            mutex_exit(&display_mutex);

            // Load the currently browsed TFI immediately
            tfiLoadImmediateOnChannel(tfichannel);
            updateFileDisplay();
            break;

          case btnPOLY:
            togglePolyMode();
            break;
        }
        break;

      case 2: // MONO EDIT
        switch (lcd_key) {
          case btnRIGHT:
            fmscreen++;
            if (fmscreen == 14) fmscreen = 1;
            fmparamdisplay();
            break;

          case btnLEFT:
            fmscreen--;
            if (fmscreen == 0) fmscreen = 13;
            fmparamdisplay();
            break;

          case btnUP:
            tfichannel++;
            if (tfichannel > 6) tfichannel = 1;
            fmparamdisplay();
            break;

          case btnDOWN:
            tfichannel--;
            if (tfichannel == 0) tfichannel = 6;
            fmparamdisplay();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnPOLY:
            togglePolyMode();
            break;
        }
        operatorparamdisplay();
        break;

      case 3: // MONO BANK_MGR
        switch (lcd_key) {
          case btnRIGHT:
            presetfilenumber++;
            if (presetfilenumber >= preset_n) {
              presetfilenumber = 0;
            }
            presetManagerDisplay();
            break;

          case btnLEFT:
            if (presetfilenumber == 0) {
              presetfilenumber = preset_n > 0 ? preset_n - 1 : 0;
            } else {
              presetfilenumber--;
            }
            presetManagerDisplay();
            break;

          case btnOPT2:
            saveCurrentPreset();
            break;

          case btnOPT1:
            loadSelectedPreset();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnPOLY:
            togglePolyMode();
            break;

          case btnBLANK:
            deleteSelectedPreset();
            break;
        }
        break;
    }
  } 
  else { // POLY MODE - ALL modes work except preset manager
    switch(edit_mode) {
      case 0: // POLY VIZ
        switch (lcd_key) {
          case btnUP:
            viz_sub_mode = (viz_sub_mode + 1) % 4;  // 0=bar graph, 1=asteroids, 2=starfighter, 3=neural network
            showVizSubModeMessage();
            break;

          case btnDOWN:
            viz_sub_mode = (viz_sub_mode == 0) ? 3 : (viz_sub_mode - 1);  // Cycle backward through modes
            showVizSubModeMessage();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnPOLY:
            togglePolyMode();
            break;
        }
        break;

      case 1: // POLY PRESET
        switch (lcd_key) {
          case btnRIGHT:
            // Just browse, don't load automatically
            tfichannel = 1;
            tfifilenumber[tfichannel - 1]++;
            if (tfifilenumber[tfichannel - 1] >= n) {
              tfifilenumber[tfichannel - 1] = 0;
            }
            for (int i = 1; i <= 5; i++) {
              tfifilenumber[i] = tfifilenumber[0];
            }
            updateFileDisplay(); // Just update display, no loading
            break;

          case btnLEFT:
            // Just browse, don't load automatically
            tfichannel = 1;
            if (tfifilenumber[tfichannel - 1] == 0) {
              tfifilenumber[tfichannel - 1] = n - 1;
            } else {
              tfifilenumber[tfichannel - 1]--;
            }
            for (int i = 1; i <= 5; i++) {
              tfifilenumber[i] = tfifilenumber[0];
            }
            updateFileDisplay(); // Just update display, no loading
            break;

          case btnOPT2:
            saveprompt();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnOPT1:
            // Show loading message
            mutex_enter_blocking(&display_mutex);
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Loading TFI...");
            display.display();
            mutex_exit(&display_mutex);

            // Load the currently browsed TFI immediately on all poly channels
            for (int i = 1; i <= 6; i++) {
              tfichannel = i;
              tfiLoadImmediateOnChannel(i);
            }
            tfichannel = 1; // Reset to channel 1
            updateFileDisplay();
            break;

          case btnPOLY:
            togglePolyMode();
            break;

          case btnBLANK:
            deletefile();
            break;
        }
        break;

      case 2: // POLY EDIT
        switch (lcd_key) {
          case btnRIGHT:
            fmscreen++;
            if (fmscreen == 14) fmscreen = 1;
            fmparamdisplay();
            break;

          case btnLEFT:
            fmscreen--;
            if (fmscreen == 0) fmscreen = 13;
            fmparamdisplay();
            break;

          case btnUP:
            tfichannel++;
            if (tfichannel > 6) tfichannel = 1;
            fmparamdisplay();
            break;

          case btnDOWN:
            tfichannel--;
            if (tfichannel == 0) tfichannel = 6;
            fmparamdisplay();
            break;

          case btnOPT2:
            saveprompt();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnPOLY:
            togglePolyMode();
            break;
        }
        operatorparamdisplay();
        break;

      case 3: // POLY - Should never actually be 3 in poly mode
        // Fallback for invalid edit_mode in poly
        // Reset to preset mode
        edit_mode = 0;
        updateGlobalMode();
        showModeMessage();
        break;
    }
  }

  // Final MIDI processing at end of loop
  handle_midi_input();
}

void setup_oled(void) {
  Wire.setSDA(OLED_SDA_PIN);
  Wire.setSCL(OLED_SCL_PIN);
  Wire.setClock(400000);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (1)
      ;  // Halt if display fails to initialize
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("GenaJam-Pi BOOTING!");
  display.setCursor(0, 16);
  display.print("Crunchy ");
  display.print(GENAJAM_VERSION);
  display.display();
  delay(1000);
}

void flash_write_settings(uint8_t region_val, uint8_t midi_ch, uint8_t vel_curve) {
  EEPROM.write(0, region_val);
  EEPROM.write(1, midi_ch);
  EEPROM.write(2, vel_curve);
  EEPROM.commit();  // Save changes to flash memory
}

uint8_t flash_read_setting(uint8_t offset) {
  return EEPROM.read(offset);
}

void setup_hardware(void) {
  Serial.begin(115200);

  // Initialize button pins with pullup resistors
  pinMode(BTN_PRESET_PIN, INPUT_PULLUP);
  pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BTN_CH_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_CH_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_MONO_POLY_PIN, INPUT_PULLUP);
  pinMode(BTN_DELETE_PIN, INPUT_PULLUP);
  pinMode(BTN_PANIC_PIN, INPUT_PULLUP);
  pinMode(BTN_OPT1_PIN, INPUT_PULLUP);
  pinMode(BTN_OPT2_PIN, INPUT_PULLUP);

  // Initialize multiplexer control pins
  pinMode(MUX_S0_PIN, OUTPUT);
  pinMode(MUX_S1_PIN, OUTPUT);
  pinMode(MUX_S2_PIN, OUTPUT);
  pinMode(MUX_S3_PIN, OUTPUT);

  // Let hardware settle after power-on
  delay(200);

  // Read initial pot values with proper multiplexer handling
  Serial.println("Initializing pot values...");

  for (int i = 0; i < 4; i++) {
    selectMuxChannel(i);
    delay(20);  // Longer delay for initial readings

    // Take multiple readings and average them for stable initial values
    int total = 0;
    for (int j = 0; j < 10; j++) {
      total += analogRead(MUX_SIG_PIN);
      delay(2);
    }
    prevpotvalue[i] = (total / 10) >> 3;

    Serial.print("Initial pot ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(prevpotvalue[i]);
  }

  Serial.println("Pot initialization complete");
}

static inline void primePotBaselines() {
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t v = readStablePot(i);       // uses selectMuxChannel + settling + median-of-5
    prevpotvalue[i] = v;                // baseline used by operatorparamdisplay()
    for (uint8_t k = 0; k < 4; ++k) {   // fill moving-average history (smaller buffer for PCB)
      pot_history[i][k] = v;
    }
  }
  history_index  = 0;
  history_filled = true;
}

void setup_sd(void) {
  // Initialize SPI pins for SD card (like your working test code)
  SPI.setSCK(SD_SCK_PIN);
  SPI.setTX(SD_MOSI_PIN);
  SPI.setRX(SD_MISO_PIN);
  SPI.begin();

  // Set CS pin as output and ensure it's high initially
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Checking SD...");
  display.display();
  delay(500);

  // Use slower speed initially for better reliability
  if (!SD.begin(SD_CS_PIN, SD_SCK_HZ(20000000), SPI)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("CANNOT FIND SD");
    display.display();
    delay(5000);
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("SD Card OK");
  display.display();
  delay(1000);

  // Display SD setup completion
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("SD setup done");
  display.display();
  delay(1000);
}

uint8_t read_buttons(void) {
  static uint64_t last_button_time = 0;
  static uint8_t last_returned_button = btnNONE;
  static uint8_t last_raw_button = btnNONE;
  static uint8_t stable_button_count = 0;
  uint64_t current_time = millis();

  // Add MIDI processing at start of button reading
  handle_midi_input();

  // Read current button states
  bool left_pressed = !digitalRead(BTN_LEFT_PIN);
  bool right_pressed = !digitalRead(BTN_RIGHT_PIN);
  bool up_pressed = !digitalRead(BTN_CH_UP_PIN);
  bool down_pressed = !digitalRead(BTN_CH_DOWN_PIN);
  bool select_pressed = !digitalRead(BTN_PRESET_PIN);
  bool poly_pressed = !digitalRead(BTN_MONO_POLY_PIN);
  bool delete_pressed = !digitalRead(BTN_DELETE_PIN);
  bool panic_pressed = !digitalRead(BTN_PANIC_PIN);
  bool opt1_pressed = !digitalRead(BTN_OPT1_PIN);
  bool opt2_pressed = !digitalRead(BTN_OPT2_PIN);

  // Check for panic button first (highest priority)
  if (panic_pressed) {
    if ((current_time - last_button_time) >= initial_debounce) {
      last_button_time = current_time;
      midiPanic();     // Call panic immediately
      return btnNONE;  // Don't return a button code, just execute panic
    }
  }

  // Determine which button is currently pressed (existing logic)
  uint8_t current_button = btnNONE;
  if (left_pressed) current_button = btnLEFT;
  else if (right_pressed) current_button = btnRIGHT;
  else if (up_pressed) current_button = btnUP;
  else if (down_pressed) current_button = btnDOWN;
  else if (select_pressed) current_button = btnSELECT;
  else if (poly_pressed) current_button = btnPOLY;
  else if (delete_pressed) current_button = btnBLANK;
  else if (opt1_pressed) current_button = btnOPT1;
  else if (opt2_pressed) current_button = btnOPT2;

  // Enhanced debouncing: require button state to be stable
  if (current_button == last_raw_button) {
    stable_button_count++;
  } else {
    stable_button_count = 0;
    last_raw_button = current_button;
  }

  // SPECIAL CASE: LEFT/RIGHT buttons get relaxed debouncing for dynamic scrolling
  if (current_button == btnLEFT || current_button == btnRIGHT) {
    // For LEFT/RIGHT: only require 1 stable read (much faster response)
    if (stable_button_count < 1) {
      return btnNONE;
    }
    // Skip the anti-double-click protection for LEFT/RIGHT to preserve speed
  } else {
    // For all OTHER buttons: strict debouncing to prevent double-clicks
    if (stable_button_count < 3) {
      return btnNONE;  // Button state not stable yet
    }

    // Prevent returning the same button too quickly (anti-double-click)
    if (current_button == last_returned_button &&
        (current_time - last_button_time) < initial_debounce) {
      return btnNONE;  // Same button pressed too soon
    }
  }

  // If no button currently pressed, reset acceleration and return
  if (current_button == btnNONE) {
    // Reset all hold state to exit acceleration mode immediately
    if (button_is_held) {
      last_button_release_time = current_time; // Record when button was released
    }
    button_is_held = false;
    last_held_button = btnNONE;
    button_hold_start_time = 0; // Reset hold timer to exit acceleration mode
    return btnNONE;
  }


  if (current_button == btnLEFT || current_button == btnRIGHT) {
    // Only apply acceleration logic for file browsing (preset modes), not FM edit modes
    bool is_file_browsing = (edit_mode == 1); // Preset mode in both MONO and POLY

    if (is_file_browsing) {
      // Use acceleration logic for TFI file browsing
      if (!button_is_held || last_held_button != current_button) {
        button_hold_start_time = current_time;
        button_is_held = true;
        last_held_button = current_button;
      }

      uint32_t hold_duration = current_time - button_hold_start_time;
      uint16_t dynamic_debounce;

      // Clean acceleration system with proper debouncing
      if (hold_duration > 4000) {
        // Acceleration mode 2 - after 4 seconds of sustained holding
        dynamic_debounce = 80;  // Fast scrolling
      } else if (hold_duration > 2000) {
        // Acceleration mode 1 - after 2 seconds of sustained holding
        dynamic_debounce = 200; // Medium speed scrolling
      } else {
        // Normal debounce to prevent bouncing on single clicks
        dynamic_debounce = 400;
      }

      if ((current_time - last_button_time) >= dynamic_debounce) {
        handle_midi_input();
        last_button_time = current_time;
        last_returned_button = current_button;  // Track what we returned
        return current_button;
      }
    } else {
      // Use regular debouncing for FM edit and other modes
      button_is_held = false;
      last_held_button = btnNONE;

      if ((current_time - last_button_time) >= initial_debounce) {
        handle_midi_input();
        last_button_time = current_time;
        last_returned_button = current_button;  // Track what we returned
        return current_button;
      }
    }

  } else if (current_button != btnNONE) {
    button_is_held = false;
    last_held_button = btnNONE;

    if ((current_time - last_button_time) >= initial_debounce) {
      handle_midi_input();
      last_button_time = current_time;
      last_returned_button = current_button;  // Track what we returned
      return current_button;
    }
  } else {
    // Default case for unhandled buttons
    button_is_held = false;
    last_held_button = btnNONE;
  }

  return btnNONE;
}

uint8_t applyVelocityCurve(uint8_t velocity) {
    if (velocity == 0) return 0;  // Return zero for zero velocity

    float normalized = (float)velocity / 127.0f;
    float result;

    switch (velocity_curve) {
        case 0: // Linear
            result = normalized;
            break;
        case 1: // Soft (easier playing)
            result = pow(normalized, 0.17f);
            break;
        case 2: // Medium (slightly compressed)
            result = pow(normalized, 0.5f);
            break;
        case 3: // Hard (requires harder playing for full volume)
            result = pow(normalized, 2.0f);
            break;
        case 4: // ImpBox (original implementation curve)
            // Slightly different from Soft for distinction
            result = pow(normalized, 0.2f);
            break;
        default:
            result = normalized; // Fallback to linear
            break;
    }

    return (uint8_t)(result * 127.0f);
}

void core1_entry() {
  const uint16_t peak_hold_duration = 800;
  const uint8_t velocity_decay_rate = 2;
  unsigned long last_update = 0;

  while (true) {
    unsigned long current_time = millis();

    // Rate limit to 60 FPS max
    if (current_time - last_update < 16) {
      continue;
    }

    // Lock mutex to safely read shared data
    mutex_enter_blocking(&viz_mutex);

    // Copy data locally to minimize mutex hold time
    VisualizerData local_data = viz_data;

    // Process velocity decay and peak hold
    bool needs_display_update = false;

    for (int channel = 0; channel < 11; channel++) {
      // Decay velocity
      if (local_data.voice_velocity[channel] > 0) {
        if (local_data.voice_velocity[channel] > velocity_decay_rate) {
          local_data.voice_velocity[channel] -= velocity_decay_rate;
        } else {
          local_data.voice_velocity[channel] = 0;
        }
        needs_display_update = true;
      }

      // Clear peak hold after timeout
      if (local_data.voice_peak[channel] > 0 && (current_time - local_data.voice_peak_time[channel]) > peak_hold_duration) {
        local_data.voice_peak[channel] = 0;
        needs_display_update = true;
      }
    }

    // Write back the updated data
    memcpy(viz_data.voice_velocity, local_data.voice_velocity, sizeof(local_data.voice_velocity));
    memcpy(viz_data.voice_peak, local_data.voice_peak, sizeof(local_data.voice_peak));

    // Check if display update is needed
    bool should_render = false;
    if ((local_data.current_mode == 5 || local_data.current_mode == 6) && (viz_sub_mode == 1 || viz_sub_mode == 2 || viz_sub_mode == 3)) {
        // Asteroids and Starfighter sub-modes always render for continuous animation
        should_render = true;
    } else if (local_data.current_mode == 5 || local_data.current_mode == 6) {
        // Other visualizer modes only render when there are updates
        should_render = (needs_display_update || local_data.needs_update);
    }

    // Reset the update flag
    viz_data.needs_update = false;

    mutex_exit(&viz_mutex);

    // Render display if needed (with display mutex protection)
    if (should_render) {
      // Try to get display mutex with timeout to avoid blocking indefinitely
      if (mutex_enter_timeout_ms(&display_mutex, 10)) {
        core1_visualizerDisplay(local_data);
        mutex_exit(&display_mutex);
        last_update = current_time;
      }
      // Skip frame if mutex unavailable
    }

    // Small delay to prevent overwhelming the system
    sleep_us(100);
  }
}