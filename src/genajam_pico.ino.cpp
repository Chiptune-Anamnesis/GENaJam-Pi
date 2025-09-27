# 1 "C:\\Users\\eggst\\AppData\\Local\\Temp\\tmp0sw51pn5"
#include <Arduino.h>
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/genajam_pico.ino"
const char* GENAJAM_VERSION = "v1.34";
# 10 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/genajam_pico.ino"
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
#define BTN_OPT1_PIN 21
#define BTN_OPT2_PIN 3


#define MUX_S0_PIN 10
#define MUX_S1_PIN 11
#define MUX_S2_PIN 12
#define MUX_S3_PIN 13
#define MUX_SIG_PIN 28


#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);


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


const uint16_t debouncedelay = 200;
const uint16_t messagedelay = 700;


uint8_t lcd_key = 0;
unsigned long messagestart = 0;
uint8_t refreshscreen = 0;


const uint8_t MaxNumberOfChars = 48;
uint16_t n = 0;
const uint16_t nMaxTotal = 550;
const uint16_t nMaxUser = 150;
const uint16_t nMaxLibrary = 400;
const uint16_t nMax = nMaxTotal;
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
const uint16_t initial_debounce = 300;

enum TfiBrowseMode { TFI_ALL, TFI_USER, TFI_LIBRARY };
TfiBrowseMode current_tfi_mode = TFI_ALL;
uint16_t user_file_count = 0;

char file_folders[nMaxTotal][16];
bool folder_display_enabled = true;
const uint16_t fast_debounce = 100;
const uint16_t turbo_debounce = 40;
const uint16_t hold_threshold_1 = 3000;
const uint16_t hold_threshold_2 = 6000;

uint16_t loaded_tfi[6] = { 0, 0, 0, 0, 0, 0 };

File dataFile;


uint16_t tfifilenumber[6] = { 0, 0, 0, 0, 0, 0 };
uint8_t tfichannel = 1;
uint8_t mode = 1;
uint8_t region = 0;
uint8_t midichannel = 1;
uint8_t last_midi_channel = 0;
uint8_t last_midi_note = 0;
unsigned long last_midi_time = 0;
const uint16_t midi_display_timeout = 2000;
static bool display_needs_refresh = false;
static unsigned long last_display_update = 0;


uint8_t poly_mode = 0;
uint8_t edit_mode = 1;

uint8_t prev_poly_mode = 0;



uint8_t settings_screen = 1;
uint8_t temp_midichannel = 1;
uint8_t temp_region = 0;
uint8_t velocity_curve = 1;
uint8_t temp_velocity_curve = 1;
bool settings_changed = false;
bool system_ready = false;
uint8_t viz_sub_mode = 0;
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


unsigned long midi_process_count = 0;
unsigned long midi_last_second = 0;
unsigned long max_midi_gap = 0;
unsigned long last_midi_process = 0;
float midi_hz = 0;


uint16_t presetfilenumber = 0;
uint16_t preset_n = 0;
char presetfilenames[nMax][MaxNumberOfChars + 1];
char presetfullnames[nMax][FullNameChars];
uint16_t presetsavenumber = 1;


uint8_t polynote[6] = { 0, 0, 0, 0, 0, 0 };
bool polyon[6] = { 0, 0, 0, 0, 0, 0 };
bool sustainon[6] = { 0, 0, 0, 0, 0, 0 };
bool noteheld[6] = { 0, 0, 0, 0, 0, 0 };
bool sustain = 0;
uint8_t lowestnote = 0;


uint8_t fmscreen = 1;


uint8_t fmsettings[6][50];
uint8_t lfospeed = 64;
uint8_t polypan = 64;
uint8_t polyvoicenum = 6;


uint8_t prevpotvalue[4];
bool menuprompt = 1;
bool booted = 0;
uint8_t readStablePot(uint8_t channel);
uint8_t getFilteredPotValue(uint8_t channel);
void updatePotHistory();
void selectMuxChannel(uint8_t channel);
void setup();
void loop();
void core1_entry();
void initAsteroidsDemo();
void createAsteroid(float x, float y, uint8_t size);
void createNoteAsteroid(uint8_t note, uint8_t velocity);
void fireBullet();
void updateAsteroidsPhysics();
bool asteroidsCheckCollision(float x1, float y1, float r1, float x2, float y2, float r2);
void checkAsteroidsCollisions();
void updateAsteroidsAI();
void drawAsteroidsShip();
void drawAsteroidsField();
void drawAsteroidsBullets();
void asteroidsDemo();
void updateFileDisplay(void);
void presetManagerDisplay(void);
void showAccelerationFeedback(void);
void updateMidiDisplay(uint8_t channel, uint8_t note);
void scanDirectoryRecursive(const char* path, bool saved);
void scanPresetDir(void);
void tfiLoadImmediate(void);
void tfiLoadImmediateOnChannel(uint8_t ch);
void applyTFIToAllChannelsImmediate();
void loadPendingTFI(void);
void saveCurrentPreset(void);
void loadSelectedPreset(void);
void deleteSelectedPreset(void);
void handle_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
void handle_note_off(uint8_t channel, uint8_t note, uint8_t velocity);
void handle_control_change(uint8_t channel, uint8_t cc, uint8_t value);
void handle_pitch_bend(uint8_t channel, int16_t bend);
void midiPanic(void);
void midiNoteToString(uint8_t note, char* noteStr);
void resetVoicesAndNotes();
void initializeFMChip();
void cycleEditMode();
void showModeMessage();
void showVizSubModeMessage();
void togglePolyMode();
void updateGlobalMode();
void initNeuralNetDemo();
void updateNeuralNetPhysics();
void propagateNeuralPulse(uint8_t neuron_index, unsigned long now);
void drawNeuralNetwork();
void neuralNetDemo();
void initStarfighterDemo();
void updateStarfighterPhysics();
void drawStarfighterShip();
void drawStarfield();
void drawEnvironmentalEffects();
void starfighterDemo();
void updateVelocityViz(uint8_t channel, uint8_t velocity);
void clearVelocityViz(uint8_t channel);
void visualizerDisplay(void);
void core1_visualizerDisplay(const VisualizerData& data);
void updateVisualizerMode(uint8_t new_mode);
void updateVisualizerMidiStats(float hz, unsigned long max_gap);
void polyVisualizerDisplay(void);
#line 214 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/genajam_pico.ino"
uint8_t readStablePot(uint8_t channel) {
  selectMuxChannel(channel);
  delayMicroseconds(100);


  int readings[5];
  for (int i = 0; i < 5; i++) {
    readings[i] = analogRead(MUX_SIG_PIN);
    delayMicroseconds(20);
  }


  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (readings[i] > readings[j]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }


  int average = (readings[1] + readings[2] + readings[3]) / 3;
  return average >> 3;
}


uint8_t pot_history[4][4];
uint8_t history_index = 0;
bool history_filled = false;

uint8_t getFilteredPotValue(uint8_t channel) {

  uint8_t raw_value = readStablePot(channel);


  pot_history[channel][history_index] = raw_value;


  int sum = 0;
  int count = history_filled ? 4 : (history_index + 1);

  for (int i = 0; i < count; i++) {
    sum += pot_history[channel][i];
  }

  return sum / count;
}


void updatePotHistory() {
  static unsigned long last_update = 0;
  if (millis() - last_update > 5) {
    history_index = (history_index + 1) % 4;
    if (history_index == 0) history_filled = true;
    last_update = millis();
  }
}


uint16_t savenumber = 1;
char savefilefull[] = "/tfi/user/newpatch001.tfi";


void flash_write_settings(uint8_t region_val, uint8_t midi_ch, uint8_t vel_curve);
uint8_t flash_read_setting(uint8_t offset);


void setup_hardware(void);
void setup_midi(void);
void setup_sd(void);
void setup_oled(void);
uint8_t read_buttons(void);


void modechange(int modetype);
void modechangemessage(void);
void bootprompt(void);


void scandir(bool saved);
void saveprompt(void);

void toggleTfiBrowseMode(void);
void updateTfiSelection(void);
const char* getTfiBrowseModeString(void);
uint16_t getCurrentTfiCount(void);
uint16_t getCurrentTfiIndex(uint8_t channel);
void navigateTfiRight(uint8_t channel);
void navigateTfiLeft(uint8_t channel);
void extractFolderName(const char* fullPath, char* folderName);
void sortFilesByFolder(void);
void savenew(void);
void saveoverwrite(void);
void deletefile(void);

void tfiselect(void);
void channelselect(void);
void tfisend(int opnarray[42], int sendchannel);


void fmparamdisplay(void);
void operatorparamdisplay(void);
void fmccsend(uint8_t potnumber, uint8_t potvalue);


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
  delayMicroseconds(50);
}


void oled_print(int x, int y, const char* text);
void oled_clear(void);
void oled_refresh(void);


void setup() {

  TinyUSBDevice.setID(0x239A, 0x8014);
  TinyUSBDevice.setManufacturerDescriptor("Adafruit");
  TinyUSBDevice.setProductDescriptor("GenaJam MIDI");
  TinyUSBDevice.setSerialDescriptor("123456");


  if (!TinyUSBDevice.begin(0)) {
    Serial.println("Failed to start TinyUSB");
  }


  usb_midi.begin();

  setup_hardware();
  setup_oled();
  setup_sd();
  setup_midi();


  mutex_init(&viz_mutex);
  mutex_init(&display_mutex);


  mutex_enter_blocking(&viz_mutex);
  memset(&viz_data, 0, sizeof(viz_data));
  viz_data.current_mode = 1;
  mutex_exit(&viz_mutex);


  multicore_launch_core1(core1_entry);

  EEPROM.begin(512);


  if (!loadSettingsFromSD()) {

    region = flash_read_setting(0);
    midichannel = flash_read_setting(1);
    velocity_curve = flash_read_setting(2);

    if (region == 255) region = 0;
    if (midichannel == 255) midichannel = 1;
    if (velocity_curve == 255) velocity_curve = 4;
  }

  if (region == 0) {
    midi_send_cc(1, 83, 75);
  } else {
    midi_send_cc(1, 83, 1);
  }


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


  if (!SD.exists("/tfi/user")) {
    if (SD.mkdir("/tfi/user")) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Created user");
      display.setCursor(0, 16);
      display.print("TFI folder");
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
  display.print("User: ");
  display.print(user_file_count);
  display.print(" Lib: ");
  display.print(n - user_file_count);
  display.display();


  delay(1000);


  MIDI.read();
  while (usb_midi.available()) {
    uint8_t packet[4];
    usb_midi.readPacket(packet);
  }


  resetVoicesAndNotes();


  system_ready = true;


  for (int i = 6; i > 0; i--) {
    tfichannel = i;
    tfiLoadImmediate();
  }


  booted = 1;
  edit_mode = 0;
  updateGlobalMode();
  showModeMessage();
}
void loop() {

  handle_midi_input();


  static unsigned long last_slow_update = 0;
  static unsigned long last_fast_update = 0;
  static unsigned long last_midi_stats_update = 0;
  unsigned long current_time = millis();


  if (current_time - last_fast_update >= 16) {
    last_fast_update = current_time;


    handle_midi_input();


    if (display_needs_refresh && edit_mode == 1) {
      updateFileDisplay();
    }
  }


  if (current_time - last_midi_stats_update >= 100) {
    last_midi_stats_update = current_time;
    updateVisualizerMidiStats(midi_hz, max_midi_gap);
  }


  if (current_time - last_slow_update >= 100) {
    last_slow_update = current_time;


    if (last_midi_time > 0 && (current_time - last_midi_time) > midi_display_timeout) {
      last_midi_time = 0;
      if (edit_mode == 1) {
        display_needs_refresh = true;
      }
    }
  }




  handle_midi_input();


  lcd_key = read_buttons();
  modechangemessage();


  handle_midi_input();


  if (edit_mode == 4) {
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

          settingsDisplay();
        }
        break;

      case btnSELECT:

        edit_mode = 0;
        updateGlobalMode();
        showModeMessage();
        break;

      case btnPOLY:
        togglePolyMode();
        break;
    }
    settingsOperatorDisplay();
  }
  else if (poly_mode == 0) {
    switch(edit_mode) {
      case 0:
        switch (lcd_key) {
          case btnUP:
            viz_sub_mode = (viz_sub_mode + 1) % 4;
            showVizSubModeMessage();
            break;

          case btnDOWN:
            viz_sub_mode = (viz_sub_mode == 0) ? 3 : (viz_sub_mode - 1);
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

      case 1:
        switch (lcd_key) {
          case btnRIGHT:

            if (tfichannel >= 1 && tfichannel <= 6) {
              navigateTfiRight(tfichannel);
              updateFileDisplay();
            }
            break;

          case btnLEFT:

            if (tfichannel >= 1 && tfichannel <= 6) {
              navigateTfiLeft(tfichannel);
              updateFileDisplay();
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

            mutex_enter_blocking(&display_mutex);
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Loading TFI...");
            display.display();
            mutex_exit(&display_mutex);


            tfiLoadImmediateOnChannel(tfichannel);
            updateFileDisplay();
            break;

          case btnOPT2:

            toggleTfiBrowseMode();
            updateFileDisplay();
            break;

          case btnPOLY:
            togglePolyMode();
            break;
        }
        break;

      case 2:
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

      case 3:
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
  else {
    switch(edit_mode) {
      case 0:
        switch (lcd_key) {
          case btnUP:
            viz_sub_mode = (viz_sub_mode + 1) % 4;
            showVizSubModeMessage();
            break;

          case btnDOWN:
            viz_sub_mode = (viz_sub_mode == 0) ? 3 : (viz_sub_mode - 1);
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

      case 1:
        switch (lcd_key) {
          case btnRIGHT:

            tfichannel = 1;
            navigateTfiRight(1);
            for (int i = 1; i <= 5; i++) {
              tfifilenumber[i] = tfifilenumber[0];
            }
            updateFileDisplay();
            break;

          case btnLEFT:

            tfichannel = 1;
            navigateTfiLeft(1);
            for (int i = 1; i <= 5; i++) {
              tfifilenumber[i] = tfifilenumber[0];
            }
            updateFileDisplay();
            break;

          case btnOPT2:

            toggleTfiBrowseMode();
            updateFileDisplay();
            break;

          case btnSELECT:
            cycleEditMode();
            break;

          case btnOPT1:

            mutex_enter_blocking(&display_mutex);
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Loading TFI...");
            display.display();
            mutex_exit(&display_mutex);

            for (int i = 1; i <= 6; i++) {
              tfichannel = i;
              tfiLoadImmediateOnChannel(i);
            }
            tfichannel = 1;
            updateFileDisplay();
            break;

          case btnUP:
            saveprompt();
            break;

          case btnPOLY:
            togglePolyMode();
            break;

          case btnBLANK:
            deletefile();
            break;
        }
        break;

      case 2:
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

      case 3:


        edit_mode = 0;
        updateGlobalMode();
        showModeMessage();
        break;
    }
  }


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
      ;
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
  EEPROM.commit();
}

uint8_t flash_read_setting(uint8_t offset) {
  return EEPROM.read(offset);
}

void setup_hardware(void) {
  Serial.begin(115200);


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


  pinMode(MUX_S0_PIN, OUTPUT);
  pinMode(MUX_S1_PIN, OUTPUT);
  pinMode(MUX_S2_PIN, OUTPUT);
  pinMode(MUX_S3_PIN, OUTPUT);


  delay(200);


  Serial.println("Initializing pot values...");

  for (int i = 0; i < 4; i++) {
    selectMuxChannel(i);
    delay(20);


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
    uint8_t v = readStablePot(i);
    prevpotvalue[i] = v;
    for (uint8_t k = 0; k < 4; ++k) {
      pot_history[i][k] = v;
    }
  }
  history_index = 0;
  history_filled = true;
}

void setup_sd(void) {

  SPI.setSCK(SD_SCK_PIN);
  SPI.setTX(SD_MOSI_PIN);
  SPI.setRX(SD_MISO_PIN);
  SPI.begin();


  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Checking SD...");
  display.display();
  delay(500);


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


  handle_midi_input();


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


  if (panic_pressed) {
    if ((current_time - last_button_time) >= initial_debounce) {
      last_button_time = current_time;
      midiPanic();
      return btnNONE;
    }
  }


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


  if (current_button == last_raw_button) {
    stable_button_count++;
  } else {
    stable_button_count = 0;
    last_raw_button = current_button;
  }


  if (current_button == btnLEFT || current_button == btnRIGHT) {

    if (stable_button_count < 1) {
      return btnNONE;
    }

  } else {

    if (stable_button_count < 3) {
      return btnNONE;
    }


    if (current_button == last_returned_button &&
        (current_time - last_button_time) < initial_debounce) {
      return btnNONE;
    }
  }


  if (current_button == btnNONE) {

    if (button_is_held) {
      last_button_release_time = current_time;
    }
    button_is_held = false;
    last_held_button = btnNONE;
    button_hold_start_time = 0;
    return btnNONE;
  }


  if (current_button == btnLEFT || current_button == btnRIGHT) {

    bool is_file_browsing = (edit_mode == 1);

    if (is_file_browsing) {

      if (!button_is_held || last_held_button != current_button) {
        button_hold_start_time = current_time;
        button_is_held = true;
        last_held_button = current_button;
      }

      uint32_t hold_duration = current_time - button_hold_start_time;
      uint16_t dynamic_debounce;


      if (hold_duration > 4000) {

        dynamic_debounce = 80;
      } else if (hold_duration > 2000) {

        dynamic_debounce = 200;
      } else {

        dynamic_debounce = 400;
      }

      if ((current_time - last_button_time) >= dynamic_debounce) {
        handle_midi_input();
        last_button_time = current_time;
        last_returned_button = current_button;
        return current_button;
      }
    } else {

      button_is_held = false;
      last_held_button = btnNONE;

      if ((current_time - last_button_time) >= initial_debounce) {
        handle_midi_input();
        last_button_time = current_time;
        last_returned_button = current_button;
        return current_button;
      }
    }

  } else if (current_button != btnNONE) {
    button_is_held = false;
    last_held_button = btnNONE;

    if ((current_time - last_button_time) >= initial_debounce) {
      handle_midi_input();
      last_button_time = current_time;
      last_returned_button = current_button;
      return current_button;
    }
  } else {

    button_is_held = false;
    last_held_button = btnNONE;
  }

  return btnNONE;
}

uint8_t applyVelocityCurve(uint8_t velocity) {
    if (velocity == 0) return 0;

    float normalized = (float)velocity / 127.0f;
    float result;

    switch (velocity_curve) {
        case 0:
            result = normalized;
            break;
        case 1:
            result = pow(normalized, 0.17f);
            break;
        case 2:
            result = pow(normalized, 0.5f);
            break;
        case 3:
            result = pow(normalized, 2.0f);
            break;
        case 4:

            result = pow(normalized, 0.2f);
            break;
        default:
            result = normalized;
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


    if (current_time - last_update < 16) {
      continue;
    }


    mutex_enter_blocking(&viz_mutex);


    VisualizerData local_data = viz_data;


    bool needs_display_update = false;

    for (int channel = 0; channel < 11; channel++) {

      if (local_data.voice_velocity[channel] > 0) {
        if (local_data.voice_velocity[channel] > velocity_decay_rate) {
          local_data.voice_velocity[channel] -= velocity_decay_rate;
        } else {
          local_data.voice_velocity[channel] = 0;
        }
        needs_display_update = true;
      }


      if (local_data.voice_peak[channel] > 0 && (current_time - local_data.voice_peak_time[channel]) > peak_hold_duration) {
        local_data.voice_peak[channel] = 0;
        needs_display_update = true;
      }
    }


    memcpy(viz_data.voice_velocity, local_data.voice_velocity, sizeof(local_data.voice_velocity));
    memcpy(viz_data.voice_peak, local_data.voice_peak, sizeof(local_data.voice_peak));


    bool should_render = false;
    if ((local_data.current_mode == 5 || local_data.current_mode == 6) && (viz_sub_mode == 1 || viz_sub_mode == 2 || viz_sub_mode == 3)) {

        should_render = true;
    } else if (local_data.current_mode == 5 || local_data.current_mode == 6) {

        should_render = (needs_display_update || local_data.needs_update);
    }


    viz_data.needs_update = false;

    mutex_exit(&viz_mutex);


    if (should_render) {

      if (mutex_enter_timeout_ms(&display_mutex, 10)) {
        core1_visualizerDisplay(local_data);
        mutex_exit(&display_mutex);
        last_update = current_time;
      }

    }


    sleep_us(100);
  }
}


void toggleTfiBrowseMode(void) {
  switch (current_tfi_mode) {
    case TFI_ALL:
      current_tfi_mode = TFI_USER;
      break;
    case TFI_USER:
      current_tfi_mode = TFI_LIBRARY;
      break;
    case TFI_LIBRARY:
      current_tfi_mode = TFI_ALL;
      break;
  }
  updateTfiSelection();
}

void updateTfiSelection(void) {

  for (int i = 0; i < 6; i++) {
    uint16_t max_files = getCurrentTfiCount();
    if (max_files == 0) {
      tfifilenumber[i] = 0;
    } else {

      switch (current_tfi_mode) {
        case TFI_USER:
          if (tfifilenumber[i] >= user_file_count) {
            tfifilenumber[i] = 0;
          }
          break;
        case TFI_LIBRARY:
          if (tfifilenumber[i] < user_file_count || tfifilenumber[i] >= n) {
            tfifilenumber[i] = user_file_count;
          }
          break;
        case TFI_ALL:
          if (tfifilenumber[i] >= n) {
            tfifilenumber[i] = 0;
          }
          break;
      }
    }
  }
}

const char* getTfiBrowseModeString(void) {
  switch (current_tfi_mode) {
    case TFI_USER: return "USER";
    case TFI_LIBRARY: return "LIB";
    case TFI_ALL: return "ALL";
    default: return "ALL";
  }
}

uint16_t getCurrentTfiCount(void) {
  switch (current_tfi_mode) {
    case TFI_USER: return user_file_count;
    case TFI_LIBRARY: return (n > user_file_count) ? (n - user_file_count) : 0;
    case TFI_ALL: return n;
    default: return n;
  }
}

uint16_t getCurrentTfiIndex(uint8_t channel) {
  uint8_t safe_channel = SAFE_CHANNEL_INDEX(channel);
  uint16_t raw_index = tfifilenumber[safe_channel];

  switch (current_tfi_mode) {
    case TFI_USER:
      return raw_index;
    case TFI_LIBRARY:
      return raw_index - user_file_count;
    case TFI_ALL:
      return raw_index;
    default:
      return raw_index;
  }
}

void navigateTfiRight(uint8_t channel) {
  uint8_t safe_channel = SAFE_CHANNEL_INDEX(channel);
  uint16_t current_count = getCurrentTfiCount();
  if (current_count == 0) return;

  switch (current_tfi_mode) {
    case TFI_USER:
      tfifilenumber[safe_channel]++;
      if (tfifilenumber[safe_channel] >= user_file_count) {
        tfifilenumber[safe_channel] = 0;
      }
      break;
    case TFI_LIBRARY:
      tfifilenumber[safe_channel]++;
      if (tfifilenumber[safe_channel] >= n) {
        tfifilenumber[safe_channel] = user_file_count;
      }
      break;
    case TFI_ALL:
      tfifilenumber[safe_channel]++;
      if (tfifilenumber[safe_channel] >= n) {
        tfifilenumber[safe_channel] = 0;
      }
      break;
  }
}

void navigateTfiLeft(uint8_t channel) {
  uint8_t safe_channel = SAFE_CHANNEL_INDEX(channel);
  uint16_t current_count = getCurrentTfiCount();
  if (current_count == 0) return;

  switch (current_tfi_mode) {
    case TFI_USER:
      if (tfifilenumber[safe_channel] == 0) {
        tfifilenumber[safe_channel] = user_file_count - 1;
      } else {
        tfifilenumber[safe_channel]--;
      }
      break;
    case TFI_LIBRARY:
      if (tfifilenumber[safe_channel] <= user_file_count) {
        tfifilenumber[safe_channel] = n - 1;
      } else {
        tfifilenumber[safe_channel]--;
      }
      break;
    case TFI_ALL:
      if (tfifilenumber[safe_channel] == 0) {
        tfifilenumber[safe_channel] = n - 1;
      } else {
        tfifilenumber[safe_channel]--;
      }
      break;
  }
}

void extractFolderName(const char* fullPath, char* folderName) {
  const char* lastSlash = strrchr(fullPath, '/');
  if (lastSlash == NULL) {
    strcpy(folderName, "root");
    return;
  }

  const char* secondLastSlash = lastSlash - 1;
  while (secondLastSlash > fullPath && *secondLastSlash != '/') {
    secondLastSlash--;
  }

  if (*secondLastSlash == '/') secondLastSlash++;

  int len = lastSlash - secondLastSlash;
  if (len > 15) len = 15;
  strncpy(folderName, secondLastSlash, len);
  folderName[len] = '\0';
}

void sortFilesByFolder(void) {

  if (n <= user_file_count) return;


  for (int i = user_file_count; i < n - 1; i++) {
    for (int j = user_file_count; j < n - 1 - (i - user_file_count); j++) {
      bool should_swap = false;


      int folder_compare = strcmp(file_folders[j], file_folders[j + 1]);
      if (folder_compare > 0) {
        should_swap = true;
      } else if (folder_compare == 0) {

        if (strcmp(filenames[j], filenames[j + 1]) > 0) {
          should_swap = true;
        }
      }

      if (should_swap) {
        char temp_filename[MaxNumberOfChars + 1];
        char temp_fullname[FullNameChars];
        char temp_folder[16];

        strcpy(temp_filename, filenames[j]);
        strcpy(filenames[j], filenames[j + 1]);
        strcpy(filenames[j + 1], temp_filename);

        strcpy(temp_fullname, fullnames[j]);
        strcpy(fullnames[j], fullnames[j + 1]);
        strcpy(fullnames[j + 1], temp_fullname);

        strcpy(temp_folder, file_folders[j]);
        strcpy(file_folders[j], file_folders[j + 1]);
        strcpy(file_folders[j + 1], temp_folder);
      }
    }
  }
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/asteroids.ino"




struct AsteroidShip {
    float x, y;
    float vx, vy;
    float angle;
    bool thrusting;
    unsigned long last_shot;
};

struct Bullet {
    float x, y;
    float vx, vy;
    unsigned long created;
    bool active;
};

struct Asteroid {
    float x, y;
    float vx, vy;
    float angle;
    uint8_t size;
    bool active;
    bool is_note;
    uint8_t note_pitch;
    uint8_t velocity;
    unsigned long spawn_time;
};


#define MAX_BULLETS 8
#define MAX_ASTEROIDS 16
AsteroidShip ship;
Bullet bullets[MAX_BULLETS];
Asteroid asteroids[MAX_ASTEROIDS];
unsigned long asteroids_last_update = 0;
unsigned long asteroids_next_action = 0;
bool asteroids_initialized = false;
unsigned long last_midi_note_time = 0;
uint8_t recent_notes[8] = {0};
uint8_t recent_note_count = 0;
const float SCREEN_WIDTH = 128.0;
const float SCREEN_HEIGHT = 32.0;

void initAsteroidsDemo() {

    ship.x = SCREEN_WIDTH / 2;
    ship.y = SCREEN_HEIGHT / 2;
    ship.vx = 0;
    ship.vy = 0;
    ship.angle = random(0, 360);
    ship.thrusting = true;
    ship.last_shot = 0;


    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }


    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }


    for (int i = 0; i < 4; i++) {
        createAsteroid(random(0, (int)SCREEN_WIDTH), random(0, (int)SCREEN_HEIGHT), 0);
    }

    asteroids_initialized = true;
    asteroids_last_update = millis();
    asteroids_next_action = millis() + random(1000, 3000);
}

void createAsteroid(float x, float y, uint8_t size) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            asteroids[i].x = x;
            asteroids[i].y = y;
            asteroids[i].vx = (random(-100, 101) / 100.0) * 0.15;
            asteroids[i].vy = (random(-100, 101) / 100.0) * 0.15;
            asteroids[i].angle = random(0, 360);
            asteroids[i].size = size;
            asteroids[i].active = true;
            asteroids[i].is_note = false;
            asteroids[i].note_pitch = 0;
            asteroids[i].velocity = 0;
            asteroids[i].spawn_time = millis();
            break;
        }
    }
}

void createNoteAsteroid(uint8_t note, uint8_t velocity) {

    int asteroid_count = 0;
    int oldest_index = -1;
    unsigned long oldest_time = millis();

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            asteroid_count++;
            if (asteroids[i].spawn_time < oldest_time) {
                oldest_time = asteroids[i].spawn_time;
                oldest_index = i;
            }
        }
    }


    if (asteroid_count >= 6 && oldest_index >= 0) {
        asteroids[oldest_index].active = false;
    }


    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {

            float y_pos = map(note, 0, 127, SCREEN_HEIGHT - 4, 4);
            float x_pos = random(0, (int)SCREEN_WIDTH);


            float speed_multiplier = map(velocity, 0, 127, 0.5, 2.0);
            uint8_t size = (velocity > 90) ? 0 : (velocity > 60) ? 1 : 2;

            asteroids[i].x = x_pos;
            asteroids[i].y = y_pos;
            asteroids[i].vx = (random(-100, 101) / 100.0) * 0.1 * speed_multiplier;
            asteroids[i].vy = (random(-100, 101) / 100.0) * 0.1 * speed_multiplier;
            asteroids[i].angle = random(0, 360);
            asteroids[i].size = size;
            asteroids[i].active = true;
            asteroids[i].is_note = true;
            asteroids[i].note_pitch = note;
            asteroids[i].velocity = velocity;
            asteroids[i].spawn_time = millis();
            break;
        }
    }
}

void fireBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = ship.x;
            bullets[i].y = ship.y;
            bullets[i].vx = cos(ship.angle * PI / 180.0) * 1.5;
            bullets[i].vy = sin(ship.angle * PI / 180.0) * 1.5;
            bullets[i].created = millis();
            bullets[i].active = true;
            ship.last_shot = millis();
            break;
        }
    }
}

void updateAsteroidsPhysics() {
    unsigned long now = millis();
    float dt = (now - asteroids_last_update) / 1000.0;
    asteroids_last_update = now;


    if (ship.thrusting) {
        ship.vx += cos(ship.angle * PI / 180.0) * 0.025;
        ship.vy += sin(ship.angle * PI / 180.0) * 0.025;
    }


    ship.vx *= 0.994;
    ship.vy *= 0.994;


    ship.x += ship.vx;
    ship.y += ship.vy;
    if (ship.x < 0) ship.x = SCREEN_WIDTH;
    if (ship.x > SCREEN_WIDTH) ship.x = 0;
    if (ship.y < 0) ship.y = SCREEN_HEIGHT;
    if (ship.y > SCREEN_HEIGHT) ship.y = 0;


    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].vx;
            bullets[i].y += bullets[i].vy;


            if (bullets[i].x < 0) bullets[i].x = SCREEN_WIDTH;
            if (bullets[i].x > SCREEN_WIDTH) bullets[i].x = 0;
            if (bullets[i].y < 0) bullets[i].y = SCREEN_HEIGHT;
            if (bullets[i].y > SCREEN_HEIGHT) bullets[i].y = 0;


            if (now - bullets[i].created > 3000) {
                bullets[i].active = false;
            }
        }
    }


    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            asteroids[i].x += asteroids[i].vx;
            asteroids[i].y += asteroids[i].vy;
            asteroids[i].angle += 0.3;


            if (asteroids[i].x < 0) asteroids[i].x = SCREEN_WIDTH;
            if (asteroids[i].x > SCREEN_WIDTH) asteroids[i].x = 0;
            if (asteroids[i].y < 0) asteroids[i].y = SCREEN_HEIGHT;
            if (asteroids[i].y > SCREEN_HEIGHT) asteroids[i].y = 0;
        }
    }
}

bool asteroidsCheckCollision(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float distance = sqrt(dx * dx + dy * dy);
    return distance < (r1 + r2);
}

void checkAsteroidsCollisions() {

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        for (int j = 0; j < MAX_ASTEROIDS; j++) {
            if (!asteroids[j].active) continue;

            float asteroid_radius = (asteroids[j].size == 0) ? 6 : (asteroids[j].size == 1) ? 4 : 2;

            if (asteroidsCheckCollision(bullets[i].x, bullets[i].y, 1, asteroids[j].x, asteroids[j].y, asteroid_radius)) {

                if (asteroids[j].is_note) {

                    asteroids[j].active = false;
                } else {

                    if (asteroids[j].size < 2) {

                        for (int k = 0; k < 2; k++) {
                            createAsteroid(asteroids[j].x + random(-3, 4), asteroids[j].y + random(-3, 4), asteroids[j].size + 1);
                        }
                    }
                    asteroids[j].active = false;
                }

                bullets[i].active = false;
                break;
            }
        }
    }
}

void updateAsteroidsAI() {
    unsigned long now = millis();


    bool danger_ahead = false;
    float danger_angle = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            float dx = asteroids[i].x - ship.x;
            float dy = asteroids[i].y - ship.y;
            float dist = sqrt(dx * dx + dy * dy);
            float asteroid_radius = (asteroids[i].size == 0) ? 8 : (asteroids[i].size == 1) ? 6 : 4;


            if (dist < asteroid_radius + 12) {
                danger_ahead = true;
                danger_angle = atan2(dy, dx) * 180.0 / PI;
                break;
            }
        }
    }


    if (danger_ahead) {

        float escape_angle = danger_angle + 180.0;
        if (escape_angle >= 360.0) escape_angle -= 360.0;
        ship.angle = escape_angle;
        ship.thrusting = true;
        asteroids_next_action = now + 300;
    } else if (now > asteroids_next_action) {

        int action = random(0, 100);

        if (action < 25) {

            float nearest_dist = 1000;
            bool found_safe_target = false;
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (asteroids[i].active) {
                    float dx = asteroids[i].x - ship.x;
                    float dy = asteroids[i].y - ship.y;
                    float dist = sqrt(dx * dx + dy * dy);
                    float asteroid_radius = (asteroids[i].size == 0) ? 8 : (asteroids[i].size == 1) ? 6 : 4;


                    if (dist > asteroid_radius + 15 && dist < nearest_dist) {
                        nearest_dist = dist;
                        ship.angle = atan2(dy, dx) * 180.0 / PI;
                        found_safe_target = true;
                    }
                }
            }
            if (found_safe_target && now - ship.last_shot > 400) {
                fireBullet();
            }
        } else if (action < 45) {

            ship.angle -= random(10, 30);
        } else if (action < 65) {

            ship.angle += random(10, 30);
        } else if (action < 80) {

            ship.thrusting = true;
        } else if (action < 90) {

            ship.thrusting = false;
        } else {

            if (now - ship.last_shot > 500) {
                fireBullet();
            }
        }

        asteroids_next_action = now + random(800, 1500);
    }


    if (now - ship.last_shot > 1000 && random(0, 100) < 10) {
        fireBullet();
    }


    int active_asteroids = 0;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) active_asteroids++;
    }

    if (active_asteroids < 3 && random(0, 100) < 2) {
        createAsteroid(random(0, (int)SCREEN_WIDTH), random(0, (int)SCREEN_HEIGHT), 0);
    }
}

void handleMidiNoteForAsteroids(uint8_t note, uint8_t velocity) {
    unsigned long now = millis();


    createNoteAsteroid(note, velocity);


    if (now - last_midi_note_time < 300) {

        if (recent_note_count < 8) {
            recent_notes[recent_note_count] = note;
            recent_note_count++;
        } else {

            for (int i = 0; i < 7; i++) {
                recent_notes[i] = recent_notes[i + 1];
            }
            recent_notes[7] = note;
        }


        if (recent_note_count >= 4) {
            bool ascending = true, descending = true;
            for (int i = 1; i < recent_note_count; i++) {
                if (recent_notes[i] <= recent_notes[i-1]) ascending = false;
                if (recent_notes[i] >= recent_notes[i-1]) descending = false;
            }

            if (ascending || descending) {

                for (int i = 0; i < 3; i++) {
                    float x = 10 + i * 35;
                    float y = ascending ? (5 + i * 5) : (25 - i * 5);
                    createNoteAsteroid(note + i * (ascending ? 12 : -12), velocity);
                }
            }
        }
    } else {

        recent_note_count = 1;
        recent_notes[0] = note;
    }

    last_midi_note_time = now;
}

void drawAsteroidsShip() {

    float cos_a = cos(ship.angle * PI / 180.0);
    float sin_a = sin(ship.angle * PI / 180.0);


    int x1 = ship.x + cos_a * 4;
    int y1 = ship.y + sin_a * 4;
    int x2 = ship.x + cos((ship.angle + 135) * PI / 180.0) * 3;
    int y2 = ship.y + sin((ship.angle + 135) * PI / 180.0) * 3;
    int x3 = ship.x + cos((ship.angle - 135) * PI / 180.0) * 3;
    int y3 = ship.y + sin((ship.angle - 135) * PI / 180.0) * 3;

    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    display.drawLine(x2, y2, x3, y3, SSD1306_WHITE);
    display.drawLine(x3, y3, x1, y1, SSD1306_WHITE);


    if (ship.thrusting) {
        int fx = ship.x - cos_a * 6;
        int fy = ship.y - sin_a * 6;
        display.drawPixel(fx, fy, SSD1306_WHITE);
    }
}

void drawAsteroidsField() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            int radius = (asteroids[i].size == 0) ? 6 : (asteroids[i].size == 1) ? 4 : 2;
            int x = (int)asteroids[i].x;
            int y = (int)asteroids[i].y;

            if (asteroids[i].is_note) {

                unsigned long age = millis() - asteroids[i].spawn_time;
                bool pulse = (age / 200) % 2;

                if (pulse) {
                    display.fillCircle(x, y, radius, SSD1306_WHITE);
                } else {
                    display.drawCircle(x, y, radius, SSD1306_WHITE);
                }


                display.drawPixel(x, y, pulse ? SSD1306_BLACK : SSD1306_WHITE);
            } else {

                display.drawCircle(x, y, radius, SSD1306_WHITE);
            }
        }
    }
}

void drawAsteroidsBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            display.drawPixel((int)bullets[i].x, (int)bullets[i].y, SSD1306_WHITE);
        }
    }
}

void asteroidsDemo() {
    if (!asteroids_initialized) {
        initAsteroidsDemo();
    }


    updateAsteroidsPhysics();
    updateAsteroidsAI();
    checkAsteroidsCollisions();


    display.clearDisplay();
    drawAsteroidsShip();
    drawAsteroidsField();
    drawAsteroidsBullets();
    display.display();
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/display.ino"
static inline bool inVizMode() { return mode == 5 || mode == 6; }
void oled_clear_protected(void);
void oled_print_protected(int x, int y, const char* text);
void oled_refresh_protected(void);
void oled_complete_update_protected(void (*update_function)());

void oled_print(int x, int y, const char* text) { oled_print_protected(x, y, text); }
void oled_clear(void) { oled_clear_protected(); }
void oled_refresh(void) { oled_refresh_protected(); }

void updateFileDisplay(void) {
    if (n == 0) return;


    if (mode != 1 && mode != 3) return;


    if (millis() - last_display_update < 50) {
        display_needs_refresh = true;
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
    if (n == 0) return;




    if (mode == 1 || mode == 2) {
        midi_send_cc(tfichannel, 123, 0);
        midi_send_cc(tfichannel, 121, 0);
        tfiLoadImmediateOnChannel(tfichannel);
    }

    updateFileDisplay();
}

void fmparamdisplay(void) {
    uint8_t i;
    char line1[32] = "";
    char line2[32] = "";
    char temp_str[16];

    oled_clear();

    if (mode == 2 || mode == 4) {
        sprintf(line1, "C%d ", tfichannel);
    } else {
        strcpy(line1, "P  ");
    }

    switch(fmscreen) {
        case 1:
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


            i = fmsettings[tfichannel-1][44];
            if (i < 32) strcat(line2, "OFF");
            else if (i < 64) strcat(line2, " L ");
            else if (i < 96) strcat(line2, " R ");
            else strcat(line2, " C ");
            break;

        case 2:
            strcat(line1, "02:OP Volume");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][4],
                fmsettings[tfichannel-1][24],
                fmsettings[tfichannel-1][14],
                fmsettings[tfichannel-1][34]);
            break;

        case 3:
            strcat(line1, "03:Freq Multp");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][2],
                fmsettings[tfichannel-1][22],
                fmsettings[tfichannel-1][12],
                fmsettings[tfichannel-1][32]);
            break;

        case 4:
            strcat(line1, "04:Detune-Mul");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][3],
                fmsettings[tfichannel-1][23],
                fmsettings[tfichannel-1][13],
                fmsettings[tfichannel-1][33]);
            break;

        case 5:
            strcat(line1, "05:Rate Scale");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][5],
                fmsettings[tfichannel-1][25],
                fmsettings[tfichannel-1][15],
                fmsettings[tfichannel-1][35]);
            break;

        case 6:
            strcat(line1, "06:Attack");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][6],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][26],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][16],
                fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][36]);
            break;

        case 7:
            strcat(line1, "07:Decay 1");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][7],
                fmsettings[tfichannel-1][27],
                fmsettings[tfichannel-1][17],
                fmsettings[tfichannel-1][37]);
            break;

        case 8:
            strcat(line1, "08:Sustain");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][10],
                fmsettings[tfichannel-1][30],
                fmsettings[tfichannel-1][20],
                fmsettings[tfichannel-1][40]);
            break;

        case 9:
            strcat(line1, "09:Decay 2");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][8],
                fmsettings[tfichannel-1][28],
                fmsettings[tfichannel-1][18],
                fmsettings[tfichannel-1][38]);
            break;

        case 10:
            strcat(line1, "10:Release");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][9],
                fmsettings[tfichannel-1][29],
                fmsettings[tfichannel-1][19],
                fmsettings[tfichannel-1][39]);
            break;

        case 11:
            strcat(line1, "11:SSG-EG");
            sprintf(line2, "%3d %3d %3d %3d",
                fmsettings[tfichannel-1][11],
                fmsettings[tfichannel-1][31],
                fmsettings[tfichannel-1][21],
                fmsettings[tfichannel-1][41]);
            break;

        case 12:
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

        case 13:
            strcat(line1, "13:LFO/FM/AM");
            sprintf(line2, "    %3d %3d %3d",
                lfospeed,
                fmsettings[tfichannel-1][42],
                fmsettings[tfichannel-1][43]);
            break;
    }

    oled_print(0, 0, line1);
    oled_print(0, 16, line2);


    if (mode == 4) {
        oled_print(0, 24, "OPT2:Save");
    }

    oled_refresh();
}

void operatorparamdisplay(void) {
    handle_midi_input();


    static unsigned long last_pot_check = 0;
    if (millis() - last_pot_check < 10) return;
    last_pot_check = millis();

    updatePotHistory();

    uint8_t currentpotvalue[4];
    int8_t difference;


    for (int i = 0; i < 4; i++) {
        currentpotvalue[i] = getFilteredPotValue(i);
    }

    bool displayNeedsUpdate = false;

    for (int i = 0; i <= 3; i++) {
        difference = prevpotvalue[i] - currentpotvalue[i];


        if (difference > 1 || difference < -1) {
            handle_midi_input();

            if (currentpotvalue[i] <= 1) currentpotvalue[i] = 0;
            if (currentpotvalue[i] >= 126) currentpotvalue[i] = 127;
            prevpotvalue[i] = currentpotvalue[i];

            if (mode == 2) {
                fmccsend(i, currentpotvalue[i]);
            } else if (mode == 4) {
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
        case 1:
            strcat(line1, " MIDI CH");
            sprintf(line2, "Channel: %d", temp_midichannel);
            if (temp_midichannel != midichannel) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 2:
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

        case 3:
            strcat(line1, " VELOCITY");
            strcpy(line2, curve_names[temp_velocity_curve]);
            if (temp_velocity_curve != velocity_curve) {
                strcat(line2, " *");
                settings_changed = true;
            }
            break;

        case 4:
            strcat(line1, " ABOUT");
            sprintf(line2, "GenaJam-Pi %s", GENAJAM_VERSION);
            break;
    }

    oled_print(0, 0, line1);
    oled_print(0, 16, line2);


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
        case 1:
            temp_midichannel++;
            if (temp_midichannel > 16) temp_midichannel = 1;
            settingsDisplay();
            break;

        case 2:
            temp_region = 1 - temp_region;
            settingsDisplay();
            break;

        case 3:
            temp_velocity_curve++;
            if (temp_velocity_curve > 4) temp_velocity_curve = 0;
            settingsDisplay();
            break;
    }
}

void settingsAdjustDown(void) {
    switch(settings_screen) {
        case 1:
            temp_midichannel--;
            if (temp_midichannel == 0) temp_midichannel = 16;
            settingsDisplay();
            break;

        case 2:
            temp_region = 1 - temp_region;
            settingsDisplay();
            break;

        case 3:
            temp_velocity_curve--;
            if (temp_velocity_curve > 4) temp_velocity_curve = 4;
            settingsDisplay();
            break;
    }
}


void settingsOperatorDisplay(void) {


    handle_midi_input();
}

void presetManagerDisplay(void) {
    oled_clear();


    if (mode == 8) {
        oled_print(0, 0, "POLY PRESETS");
        oled_print(0, 16, "DISABLED");
        oled_print(0, 24, "Use MONO mode only");
        oled_refresh();
        return;
    }


    char display_buffer[32];
    int display_index = presetfilenumber + 1;
    snprintf(display_buffer, sizeof(display_buffer), "M BANK %d/%d", display_index, preset_n);
    oled_print(0, 0, display_buffer);


    if (preset_n > 0) {
        oled_print(0, 16, presetfilenames[presetfilenumber]);
    } else {
        oled_print(0, 16, "No banks found");
    }


    oled_print(0, 24, "OPT2:Save OPT1:Load DEL");

    oled_refresh();
}

void showAccelerationFeedback(void) {
    if (!button_is_held) return;

    uint32_t hold_duration = millis() - button_hold_start_time;


    if (mode != 1 && mode != 3) return;

}

void updateMidiDisplay(uint8_t channel, uint8_t note) {
    last_midi_channel = channel;
    last_midi_note = note;
    last_midi_time = millis();


    if (mode == 1 || mode == 3) {
        updateFileDisplay();
    }
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/fileops.ino"
void scandir(bool saved) {
    n = 0;
    user_file_count = 0;

    scanDirectoryRecursive("/tfi/user", saved);
    user_file_count = n;

    scanDirectoryRecursive("/tfi", saved);

    sortFilesByFolder();


    for (int i = 0; i < 6; ++i) {
        if (n == 0) {
            tfifilenumber[i] = 0;
        } else if (tfifilenumber[i] >= n) {
            tfifilenumber[i] = n - 1;
        }
    }
}

void scanDirectoryRecursive(const char* path, bool saved) {

    bool is_user_folder = (strcmp(path, "/tfi/user") == 0);
    if (is_user_folder && user_file_count >= nMaxUser) return;
    if (!is_user_folder && (n - user_file_count) >= nMaxLibrary) return;
    if (n >= nMaxTotal) return;

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return;
    }


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


        if (name[0] == '.' ||
            strstr(name, "System Volume") != NULL ||
            strstr(name, "$RECYCLE") != NULL) {
            entry.close();
            continue;
        }

        if (entry.isDirectory()) {

            if (strcmp(path, "/tfi") == 0 && strcmp(name, "user") == 0) {
                entry.close();
                continue;
            }


            char subPath[96];
            if (strcmp(path, "/") == 0) {
                snprintf(subPath, 96, "/%s", name);
            } else {
                snprintf(subPath, 96, "%s/%s", path, name);
            }
            entry.close();
            scanDirectoryRecursive(subPath, saved);

        } else {

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


                if (strcmp(path, "/tfi") == 0) {

                    strncpy(filenames[n], name, dispLen);
                    filenames[n][dispLen] = '\0';


                    snprintf(fullnames[n], FullNameChars, "/tfi/%s", name);

                } else {

                    strncpy(filenames[n], name, dispLen);
                    filenames[n][dispLen] = '\0';


                    snprintf(fullnames[n], FullNameChars, "%s/%s", path, name);
                }


                char fullPath[FullNameChars];
                snprintf(fullPath, FullNameChars, "%s/%s", path, name);
                if (saved && strcmp(name, savefilefull) == 0) {
                    for (int i = 0; i < 6; i++) tfifilenumber[i] = n;
                }

                extractFolderName(fullPath, file_folders[n]);

                n++;


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


        if (n % 10 == 0) {
            delay(1);
            handle_midi_input();
        }
    }

    dir.close();
}

void scanPresetDir(void) {
    preset_n = 0;

    File dir = SD.open("/presets");
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


        if (name[0] == '.' ||
            strstr(name, "System Volume") != NULL ||
            strstr(name, "$RECYCLE") != NULL) {
            entry.close();
            continue;
        }

        if (!entry.isDirectory()) {

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

                int dispLen = nameLen - 8;
                if (dispLen > MaxNumberOfChars) dispLen = MaxNumberOfChars;
                strncpy(presetfilenames[preset_n], name, dispLen);
                presetfilenames[preset_n][dispLen] = '\0';


                snprintf(presetfullnames[preset_n], FullNameChars, "/presets/%s", name);

                preset_n++;


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


        if (preset_n % 5 == 0) {
            delay(1);
            handle_midi_input();
        }
    }

    dir.close();


    for (int i = 0; i < preset_n - 1; i++) {
        for (int j = 0; j < preset_n - i - 1; j++) {
            if (strcmp(presetfilenames[j], presetfilenames[j + 1]) > 0) {

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


    if (preset_n == 0) {
        presetfilenumber = 0;
    } else if (presetfilenumber >= preset_n) {
        presetfilenumber = preset_n - 1;
    }
}

void tfiselect(void) {
    if (n == 0) return;

    uint16_t idx = tfifilenumber[tfichannel-1];
    if (idx >= n) return;


    tfi_select_time = millis();
    tfi_pending_load = true;
    pending_tfi_channel = tfichannel;
    showing_loading = false;


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


    tfisend(tfiarray, ch);
}

void applyTFIToAllChannelsImmediate() {
    for (uint8_t ch = 1; ch <= 6; ch++) {
        tfiLoadImmediateOnChannel(ch);
        delay(5);

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


        while (read_buttons() == btnNONE) {
            handle_midi_input();
            delay(50);
        }

        tfi_pending_load = false;
        showing_loading = false;
        return;
    }


    int tfiarray[42];
    for (int i = 0; i < 42; i++) {
        if (dataFile.available()) {
            tfiarray[i] = dataFile.read();
        } else {
            tfiarray[i] = 0;
        }
    }
    dataFile.close();


    if (mode == 3 || mode == 4) {

        for (uint8_t ch = 1; ch <= 6; ch++) {
            tfisend(tfiarray, ch);
            delay(2);
        }

        for (int i = 0; i < 6; i++) {
            loaded_tfi[i] = tfifilenumber[0];
        }
    } else {

        tfisend(tfiarray, pending_tfi_channel);

        loaded_tfi[pending_tfi_channel-1] = tfifilenumber[pending_tfi_channel-1];
    }


    tfi_pending_load = false;
    showing_loading = false;


    if (booted == 1 && (mode == 1 || mode == 3)) {
        updateFileDisplay();
    }
}

void savenew(void) {
    oled_clear();
    oled_print(0, 0, "SAVING...");
    oled_print(0, 16, "please wait");
    oled_refresh();


    while (savenumber < 1000) {
        sprintf(savefilefull, "/tfi/user/newpatch%03d.tfi", savenumber);
        if (!SD.exists(savefilefull)) {
            break;
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


    int tfiarray[42];
    tfichannel = 1;

    tfiarray[0] = fmsettings[tfichannel-1][0] / 16;
    tfiarray[1] = fmsettings[tfichannel-1][1] / 16;
    tfiarray[2] = fmsettings[tfichannel-1][2] / 8;
    tfiarray[12] = fmsettings[tfichannel-1][12] / 8;
    tfiarray[22] = fmsettings[tfichannel-1][22] / 8;
    tfiarray[32] = fmsettings[tfichannel-1][32] / 8;
    tfiarray[3] = fmsettings[tfichannel-1][3] / 32;
    tfiarray[13] = fmsettings[tfichannel-1][13] / 32;
    tfiarray[23] = fmsettings[tfichannel-1][23] / 32;
    tfiarray[33] = fmsettings[tfichannel-1][33] / 32;
    tfiarray[4] = fmsettings[tfichannel-1][4];
    tfiarray[14] = fmsettings[tfichannel-1][14];
    tfiarray[24] = fmsettings[tfichannel-1][24];
    tfiarray[34] = fmsettings[tfichannel-1][34];
    tfiarray[5] = fmsettings[tfichannel-1][5] / 32;
    tfiarray[15] = fmsettings[tfichannel-1][15] / 32;
    tfiarray[25] = fmsettings[tfichannel-1][25] / 32;
    tfiarray[35] = fmsettings[tfichannel-1][35] / 32;
    tfiarray[6] = fmsettings[tfichannel-1][6] / 4;
    tfiarray[16] = fmsettings[tfichannel-1][16] / 4;
    tfiarray[26] = fmsettings[tfichannel-1][26] / 4;
    tfiarray[36] = fmsettings[tfichannel-1][36] / 4;
    tfiarray[7] = fmsettings[tfichannel-1][7] / 4;
    tfiarray[17] = fmsettings[tfichannel-1][17] / 4;
    tfiarray[27] = fmsettings[tfichannel-1][27] / 4;
    tfiarray[37] = fmsettings[tfichannel-1][37] / 4;
    tfiarray[10] = (127 - fmsettings[tfichannel-1][10]) / 8;
    tfiarray[20] = (127 - fmsettings[tfichannel-1][20]) / 8;
    tfiarray[30] = (127 - fmsettings[tfichannel-1][30]) / 8;
    tfiarray[40] = (127 - fmsettings[tfichannel-1][40]) / 8;
    tfiarray[8] = fmsettings[tfichannel-1][8] / 8;
    tfiarray[18] = fmsettings[tfichannel-1][18] / 8;
    tfiarray[28] = fmsettings[tfichannel-1][28] / 8;
    tfiarray[38] = fmsettings[tfichannel-1][38] / 8;
    tfiarray[9] = fmsettings[tfichannel-1][9] / 8;
    tfiarray[19] = fmsettings[tfichannel-1][19] / 8;
    tfiarray[29] = fmsettings[tfichannel-1][29] / 8;
    tfiarray[39] = fmsettings[tfichannel-1][39] / 8;
    tfiarray[11] = fmsettings[tfichannel-1][11] / 8;
    tfiarray[21] = fmsettings[tfichannel-1][21] / 8;
    tfiarray[31] = fmsettings[tfichannel-1][31] / 8;
    tfiarray[41] = fmsettings[tfichannel-1][41] / 8;

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

    scandir(true);


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


    int tfiarray[42];
    tfichannel = 1;

    tfiarray[0] = fmsettings[tfichannel-1][0] / 16;
    tfiarray[1] = fmsettings[tfichannel-1][1] / 16;
    tfiarray[2] = fmsettings[tfichannel-1][2] / 8;
    tfiarray[3] = fmsettings[tfichannel-1][3] / 32;
    tfiarray[4] = fmsettings[tfichannel-1][4];
    tfiarray[5] = fmsettings[tfichannel-1][5];
    tfiarray[6] = fmsettings[tfichannel-1][6];
    tfiarray[7] = fmsettings[tfichannel-1][7];
    tfiarray[8] = fmsettings[tfichannel-1][8];
    tfiarray[9] = fmsettings[tfichannel-1][9];
    tfiarray[10] = fmsettings[tfichannel-1][10];
    tfiarray[11] = fmsettings[tfichannel-1][11];
    tfiarray[12] = fmsettings[tfichannel-1][12] / 8;
    tfiarray[13] = fmsettings[tfichannel-1][13] / 32;
    tfiarray[14] = fmsettings[tfichannel-1][14] / 16;
    tfiarray[15] = fmsettings[tfichannel-1][15] / 16;
    tfiarray[16] = fmsettings[tfichannel-1][16];
    tfiarray[17] = fmsettings[tfichannel-1][17];
    tfiarray[18] = fmsettings[tfichannel-1][18];
    tfiarray[19] = fmsettings[tfichannel-1][19];
    tfiarray[20] = fmsettings[tfichannel-1][20];
    tfiarray[21] = fmsettings[tfichannel-1][21];
    tfiarray[22] = fmsettings[tfichannel-1][22] / 8;
    tfiarray[23] = fmsettings[tfichannel-1][23] / 32;
    tfiarray[24] = fmsettings[tfichannel-1][24] / 16;
    tfiarray[25] = fmsettings[tfichannel-1][25] / 16;
    tfiarray[26] = fmsettings[tfichannel-1][26];
    tfiarray[27] = fmsettings[tfichannel-1][27];
    tfiarray[28] = fmsettings[tfichannel-1][28];
    tfiarray[29] = fmsettings[tfichannel-1][29];
    tfiarray[30] = fmsettings[tfichannel-1][30];
    tfiarray[31] = fmsettings[tfichannel-1][31];
    tfiarray[32] = fmsettings[tfichannel-1][32] / 8;
    tfiarray[33] = fmsettings[tfichannel-1][33] / 32;
    tfiarray[34] = fmsettings[tfichannel-1][34] / 16;
    tfiarray[35] = fmsettings[tfichannel-1][35];
    tfiarray[36] = fmsettings[tfichannel-1][36];
    tfiarray[37] = fmsettings[tfichannel-1][37];
    tfiarray[38] = fmsettings[tfichannel-1][38];
    tfiarray[39] = fmsettings[tfichannel-1][39];
    tfiarray[40] = fmsettings[tfichannel-1][40];
    tfiarray[41] = fmsettings[tfichannel-1][41];

    for (int i = 0; i < 42; i++) dataFile.write((uint8_t)tfiarray[i]);
    dataFile.close();

    oled_clear();
    oled_print(0, 0, "SAVED!");
    oled_print(0, 16, filenames[idx]);
    oled_refresh();
    delay(500);

    messagestart = millis();
    refreshscreen = 1;

    scandir(true);
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
            case btnBLANK:
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

                scandir(false);
                if (n == 0) { messagestart = millis(); refreshscreen = 1; return; }
                if (tfifilenumber[tfichannel-1] >= n) tfifilenumber[tfichannel-1] = n-1;

                messagestart = millis();
                refreshscreen = 1;
                menuprompt = 1;
                break;
            }
            case btnSELECT:
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


    char presetfilefull[64];
    while (presetsavenumber < 1000) {
        sprintf(presetfilefull, "/presets/%03d.mdm_pre", presetsavenumber);
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


    presetFile.write('T');
    presetFile.write('F');
    presetFile.write('N');
    presetFile.write('P');
    presetFile.write(1);


    for (int channel = 0; channel < 6; channel++) {
        uint16_t tfi_index = tfifilenumber[channel];

        if (tfi_index < n && n > 0) {

            const char* filename = fullnames[tfi_index];
            uint8_t len = strlen(filename);
            presetFile.write(len);
            presetFile.write((const uint8_t*)filename, len);
        } else {

            presetFile.write((uint8_t)0);
        }
    }


    presetFile.write((uint8_t)lfospeed);
    presetFile.write((uint8_t)polypan);
    presetFile.write((uint8_t)polyvoicenum);
    presetFile.write((uint8_t)midichannel);
    presetFile.write((uint8_t)region);

    presetFile.flush();
    presetFile.close();


    if (SD.exists(presetfilefull)) {
        oled_clear();
        oled_print(0, 0, "PRESET SAVED!");
        oled_print(0, 16, &presetfilefull[1]);
        oled_refresh();
        delay(1500);

        scanPresetDir();


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

        for (int channel = 0; channel < 6; channel++) {
            if (!presetFile.available()) break;

            uint8_t filename_len = presetFile.read();

            if (filename_len == 0) {

                skipped_channels++;
                continue;
            }


            char saved_filename[FullNameChars];
            if (filename_len >= FullNameChars) filename_len = FullNameChars - 1;

            presetFile.read((uint8_t*)saved_filename, filename_len);
            saved_filename[filename_len] = '\0';


            int found_index = -1;
            for (int i = 0; i < n; i++) {
                if (strcmp(fullnames[i], saved_filename) == 0) {
                    found_index = i;
                    break;
                }
            }

            if (found_index >= 0) {

                tfifilenumber[channel] = found_index;
                loaded_tfi[channel] = found_index;
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


        for (int channel = 0; channel < 6; channel++) {
            if (presetFile.available() >= 2) {
                uint8_t low_byte = presetFile.read();
                uint8_t high_byte = presetFile.read();
                uint16_t tfi_index = low_byte | (high_byte << 8);

                if (tfi_index < n) {
                    tfifilenumber[channel] = tfi_index;
                    loaded_tfi[channel] = tfi_index;
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


    if (presetFile.available()) lfospeed = presetFile.read();
    if (presetFile.available()) polypan = presetFile.read();
    if (presetFile.available()) polyvoicenum = presetFile.read();
    if (presetFile.available()) {
        uint8_t preset_midi_ch = presetFile.read();

        if (mode == 7) {
            midichannel = preset_midi_ch;
        }
    }
    if (presetFile.available()) region = presetFile.read();

    presetFile.close();


    oled_clear();
    oled_print(0, 0, "BANK LOADED!");
    char summary[32];
    sprintf(summary, "CH:%d Skip:%d", loaded_channels, skipped_channels);
    oled_print(0, 16, summary);
    oled_refresh();
    delay(2000);


    if (mode == 7) {

        tfichannel = 1;


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
            case btnBLANK:
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

                scanPresetDir();
                if (presetfilenumber >= preset_n && preset_n > 0) {
                    presetfilenumber = preset_n - 1;
                }

                menuprompt = 1;
                presetManagerDisplay();
                break;

            case btnSELECT:
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
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/midi.ino"
extern uint8_t viz_sub_mode;
extern bool system_ready;

void setup_midi(void) {

    Serial1.setTX(MIDI_TX_PIN);
    Serial1.setRX(MIDI_RX_PIN);



    Serial1.begin(MIDI_BAUD_RATE);
    delay(100);


    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();


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


    MIDI.setHandleProgramChange([](byte channel, byte number) {
        if (channel < 1 || channel > 6) MIDI.sendProgramChange(number, channel);
    });
    MIDI.setHandleAfterTouchChannel([](byte channel, byte pressure) {
        if (channel < 1 || channel > 6) MIDI.sendAfterTouch(pressure, channel);
    });
    MIDI.setHandleAfterTouchPoly([](byte channel, byte note, byte pressure) {
        if (channel < 1 || channel > 6) MIDI.sendAfterTouch(note, pressure, channel);
    });





}

void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value) {

    if (channel < 1 || channel > 16) return;
    if (cc > 127) return;
    if (value > 127) return;

    MIDI.sendControlChange(cc, value, channel);
}

void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {

    if (channel < 1 || channel > 16) return;
    if (note > 127) return;
    if (velocity > 127) return;

    MIDI.sendNoteOn(note, velocity, channel);
}

void midi_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {

    if (channel < 1 || channel > 16) return;
    if (note > 127) return;
    if (velocity > 127) return;

    MIDI.sendNoteOff(note, velocity, channel);
}

void midi_send_pitch_bend(uint8_t channel, int16_t bend) {
    MIDI.sendPitchBend(bend, channel);
}

void handle_midi_input(void) {

    if (!system_ready) {

        MIDI.read();
        if (usb_midi.available()) {
            uint8_t packet[4];
            usb_midi.readPacket(packet);
        }
        return;
    }


    static unsigned long system_ready_time = 0;
    if (system_ready_time == 0) {
        system_ready_time = millis();
    }

    if (millis() - system_ready_time < 500) {

        MIDI.read();
        while (usb_midi.available()) {
            uint8_t packet[4];
            usb_midi.readPacket(packet);
        }
        return;
    }

    static unsigned long last_meaningful_time = 0;
    static bool had_midi_activity = false;


    bool uart_midi_available = MIDI.read();


    bool usb_midi_available = false;
    if (usb_midi.available()) {
        uint8_t packet[4];
        usb_midi.readPacket(packet);

        uint8_t cable = (packet[0] >> 4) & 0x0F;
        uint8_t code = packet[0] & 0x0F;
        uint8_t channel = packet[1] & 0x0F;


        switch (code) {
            case 0x09:
                if (packet[3] > 0) {
                    handle_note_on(channel + 1, packet[2], packet[3]);
                } else {
                    handle_note_off(channel + 1, packet[2], packet[3]);
                }
                usb_midi_available = true;
                break;

            case 0x08:
                handle_note_off(channel + 1, packet[2], packet[3]);
                usb_midi_available = true;
                break;

            case 0x0B:
                handle_control_change(channel + 1, packet[2], packet[3]);
                usb_midi_available = true;
                break;

            case 0x0E:
                int16_t bend = (packet[3] << 7) | packet[2];
                bend -= 8192;
                handle_pitch_bend(channel + 1, bend);
                usb_midi_available = true;
                break;
        }
    }


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

    if (channel < 1 || channel > 16 || note > 127) return;


    if (mode == 1 || mode == 3) {

    }


    if ((mode == 5 || mode == 6) && channel >= 1 && channel <= 11) {
        updateVelocityViz(channel - 1, velocity);
    }


    if ((mode == 5 || mode == 6) && viz_sub_mode == 1 && channel >= 1 && channel <= 6) {
        handleMidiNoteForAsteroids(note, velocity);
    }


    if ((mode == 5 || mode == 6) && viz_sub_mode == 2 && channel >= 1 && channel <= 6) {
        handleMidiNoteForStarfighter(note, velocity);
    }


    if ((mode == 5 || mode == 6) && viz_sub_mode == 3 && channel >= 1 && channel <= 6) {
        handleMidiNoteForNeuralNet(note, velocity);
    }


    velocity = applyVelocityCurve(velocity);

    bool repeatnote = false;

    if (mode == 3 || mode == 4 || mode == 6) {
        if (channel == midichannel) {


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

                lowestnote = polynote[0];
                for (int i = 0; i < 6 && i < sizeof(polynote); i++) {
                    if (polynote[i] < lowestnote && polynote[i] != 0) {
                        lowestnote = polynote[i];
                    }
                    handle_midi_input();
                }


                long randchannel = random(0, 6);


                if (polynote[randchannel] == lowestnote) {
                    randchannel++;
                    if (randchannel == 6) randchannel = 0;
                }


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

    if (channel >= 1 && channel <= 6) {

        if (polyon[channel-1] && polynote[channel-1] != note) {
            midi_send_note_off(channel, polynote[channel-1], 0);
        }

        polyon[channel-1] = true;
        polynote[channel-1] = note;
        noteheld[channel-1] = true;
        midi_send_note_on(channel, note, velocity);
    } else {

        MIDI.sendNoteOn(note, velocity, channel);
    }
}
}

void handle_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {

    if (channel < 1 || channel > 16 || note > 127) return;


    if ((mode == 5 || mode == 6) && channel >= 1 && channel <= 11) {
        clearVelocityViz(channel - 1);
    }


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

    if (channel >= 1 && channel <= 6) {

        if (polynote[channel-1] == note) {
            if (sustain && sustainon[channel-1]) {
                noteheld[channel-1] = false;
                return;
            }
            polyon[channel-1] = false;
            polynote[channel-1] = 0;
            noteheld[channel-1] = false;
            midi_send_note_off(channel, note, velocity);
        }

    } else {
        MIDI.sendNoteOff(note, velocity, channel);
    }
}
}

void handle_control_change(uint8_t channel, uint8_t cc, uint8_t value) {
    if (cc == 64) {
        if (value == 0) {
            sustain = false;

            if (mode == 3 || mode == 4) {
                for (int i = 5; i >= 0; i--) {
                    handle_midi_input();
                    if (!noteheld[i] && sustainon[i]) {
                        midi_send_note_off(i + 1, polynote[i], 0);
                        sustainon[i] = false;
                        polyon[i] = false;
                        polynote[i] = 0;
                    }
                }
            } else {
                for (int i = 0; i < 6; i++) {
                    sustainon[i] = false;
                    if (!noteheld[i] && polyon[i]) {
                        midi_send_note_off(i + 1, polynote[i], 0);
                        polyon[i] = false;
                    }
                }
            }
        } else {
            sustain = true;
        }
    }


    if (cc == 1) {
        if (value <= 5) {
            midi_send_cc(1, 74, 0);
        } else {
            midi_send_cc(1, 74, 70);
        }
    }


    if (mode == 3 || mode == 4) {
        if (channel == midichannel) {

            for (int i = 1; i <= 6; i++) {
                midi_send_cc(i, cc, value);
            }
        } else {

            MIDI.sendControlChange(cc, value, channel);
        }
    } else {
        if (channel >= 1 && channel <= 6) {
            midi_send_cc(channel, cc, value);
        } else {

            MIDI.sendControlChange(cc, value, channel);
        }
    }
}

void handle_pitch_bend(uint8_t channel, int16_t bend) {
    if (mode == 3 || mode == 4) {

        for (int i = 1; i <= 6; i++) {
            midi_send_pitch_bend(i, bend);
            handle_midi_input();
        }
    } else {
        if (channel >= 1 && channel <= 6) {
            midi_send_pitch_bend(channel, bend);
        } else {

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

    uint8_t octave = (note / 12) - 1;
    uint8_t noteIndex = note % 12;


    if (note < 12) {
        sprintf(noteStr, "%s%d", noteNames[noteIndex], octave + 1);
    } else if (note > 127) {
        strcpy(noteStr, "---");
    } else {
        sprintf(noteStr, "%s%d", noteNames[noteIndex], octave);
    }
}

void resetVoicesAndNotes() {

    for (int ch = 1; ch <= 16; ++ch) {
        midi_send_cc(ch, 64, 0);
        midi_send_cc(ch, 120, 0);
        midi_send_cc(ch, 121, 0);
        midi_send_cc(ch, 123, 0);
    }


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




    for (int ch = 1; ch <= 6; ch++) {

        midi_send_cc(ch, 69 + ch, 0);


        midi_send_cc(ch, 75 + ch, 0);


        midi_send_cc(ch, 82, 64);
    }


    for (int ch = 1; ch <= 6; ch++) {

        midi_send_cc(ch, 14, 63);
        midi_send_cc(ch, 15, 0);
        midi_send_cc(ch, 16, 0);
        midi_send_cc(ch, 17, 7);
        midi_send_cc(ch, 18, 15);
        midi_send_cc(ch, 19, 127);


        midi_send_cc(ch, 20, 63);
        midi_send_cc(ch, 21, 0);
        midi_send_cc(ch, 22, 0);
        midi_send_cc(ch, 23, 7);
        midi_send_cc(ch, 24, 15);
        midi_send_cc(ch, 25, 127);


        midi_send_cc(ch, 26, 63);
        midi_send_cc(ch, 27, 0);
        midi_send_cc(ch, 28, 0);
        midi_send_cc(ch, 29, 7);
        midi_send_cc(ch, 30, 15);
        midi_send_cc(ch, 31, 127);


        midi_send_cc(ch, 32, 63);
        midi_send_cc(ch, 33, 0);
        midi_send_cc(ch, 34, 0);
        midi_send_cc(ch, 35, 7);
        midi_send_cc(ch, 36, 15);
        midi_send_cc(ch, 37, 0);


        midi_send_cc(ch, 38, 8);
        midi_send_cc(ch, 39, 8);
        midi_send_cc(ch, 40, 8);
        midi_send_cc(ch, 41, 8);


        midi_send_cc(ch, 42, 64);
        midi_send_cc(ch, 43, 64);
        midi_send_cc(ch, 44, 64);
        midi_send_cc(ch, 45, 64);
    }

    delay(50);
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/mode.ino"
void modechange(int modetype) {

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
      case 5:

        refreshscreen = 0;
        break;
      case 6:

        refreshscreen = 0;
        break;
      case 7:
        presetManagerDisplay();
        refreshscreen = 0;
        break;
      case 8:
        oled_clear();
        oled_print(0, 0, "POLY PRESETS");
        oled_print(0, 16, "DISABLED");
        oled_print(0, 24, "Use MONO mode only");
        oled_refresh();
        refreshscreen = 0;
        break;
      case 9:
        settingsDisplay();
        refreshscreen = 0;
        break;
    }
  }
}

void cycleEditMode() {
  if (poly_mode == 0) {

    edit_mode++;
    if (edit_mode > 4) edit_mode = 0;
} else {


    edit_mode++;
    if (edit_mode > 2) edit_mode = 0;
}


  updateGlobalMode();


  showModeMessage();
}

void showModeMessage() {
  tfi_pending_load = false;
  showing_loading = false;


  updateVisualizerMode(mode);

  messagestart = millis();
  refreshscreen = 1;

  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);


  switch(mode) {
    case 1:
      display.print("MONO | Preset");
      primePotBaselines();
      break;
    case 2:
      display.print("MONO | FM Edit");
      primePotBaselines();
      break;
    case 3:
      display.print("POLY | Preset");
      break;
    case 4:
      display.print("POLY | FM Edit");
      primePotBaselines();
      break;
    case 5:
      display.print("MONO | Visualizer");
      break;
    case 6:
      display.print("POLY | Visualizer");
      break;
    case 7:
      display.print("MONO | Bank Mgr");
      break;
    case 9:
      display.print("SETTINGS");
      temp_midichannel = midichannel;
      temp_region = region;
      temp_velocity_curve = velocity_curve;
      settings_changed = false;
      settings_screen = 1;

      primePotBaselines();
      break;
  }

  display.display();
  mutex_exit(&display_mutex);


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


  if (poly_mode == 1 && edit_mode == 3) {
    edit_mode = 0;
  }


  updateGlobalMode();


  showModeMessage();

  if (prev_poly_mode != poly_mode) {
    resetVoicesAndNotes();
  }
}

void updateGlobalMode() {
  if (poly_mode == 0) {
    switch(edit_mode) {
      case 0: mode = 5; break;
      case 1: mode = 1; break;
      case 2: mode = 2; break;
      case 3: mode = 7; break;
      case 4: mode = 9; break;
    }
} else {
  switch(edit_mode) {
    case 0: mode = 6; break;
    case 1: mode = 3; break;
    case 2: mode = 4; break;
    case 3: mode = 6; break;
    case 4: mode = 6; break;
  }
}
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/neuralnet.ino"



struct Neuron {
    float x, y;
    uint8_t brightness;
    uint8_t base_brightness;
    unsigned long activation_time;
    bool is_active;
    uint8_t connections[4];
    uint8_t connection_count;
    uint8_t note_channel;
};

struct NeuralConnection {
    uint8_t from_neuron;
    uint8_t to_neuron;
    float pulse_position;
    bool is_pulsing;
    unsigned long pulse_start_time;
    uint8_t pulse_brightness;
};


#define MAX_NEURONS 12
#define MAX_CONNECTIONS 16
Neuron nn_neurons[MAX_NEURONS];
NeuralConnection nn_connections[MAX_CONNECTIONS];
bool neuralnet_initialized = false;
unsigned long neuralnet_last_update = 0;
const float NN_SCREEN_WIDTH = 128.0;
const float NN_SCREEN_HEIGHT = 32.0;
const uint16_t NEURON_ACTIVATION_DURATION = 800;
const uint16_t PULSE_TRAVEL_TIME = 600;

void initNeuralNetDemo() {




    nn_neurons[0].x = 64;
    nn_neurons[0].y = 16;
    nn_neurons[0].brightness = 80;
    nn_neurons[0].base_brightness = 80;
    nn_neurons[0].is_active = false;
    nn_neurons[0].activation_time = 0;
    nn_neurons[0].connection_count = 0;
    nn_neurons[0].note_channel = 0;


    float inner_radius = 20;
    for (int i = 1; i < 7; i++) {
        float angle = (i - 1) * 60.0 * PI / 180.0;
        nn_neurons[i].x = 64 + cos(angle) * inner_radius;
        nn_neurons[i].y = 16 + sin(angle) * inner_radius * 0.6;
        nn_neurons[i].brightness = 50;
        nn_neurons[i].base_brightness = 50;
        nn_neurons[i].is_active = false;
        nn_neurons[i].activation_time = 0;
        nn_neurons[i].connection_count = 0;
        nn_neurons[i].note_channel = (i - 1) % 6;
    }


    float outer_radius = 35;
    for (int i = 7; i < 12; i++) {
        float angle = ((i - 7) * 72.0 - 36.0) * PI / 180.0;
        nn_neurons[i].x = 64 + cos(angle) * outer_radius;
        nn_neurons[i].y = 16 + sin(angle) * outer_radius * 0.5;
        nn_neurons[i].brightness = 30;
        nn_neurons[i].base_brightness = 30;
        nn_neurons[i].is_active = false;
        nn_neurons[i].activation_time = 0;
        nn_neurons[i].connection_count = 0;
        nn_neurons[i].note_channel = (i - 7) % 5;
    }


    int connection_index = 0;


    for (int inner = 1; inner < 7; inner++) {
        if (connection_index < MAX_CONNECTIONS) {
            nn_connections[connection_index].from_neuron = 0;
            nn_connections[connection_index].to_neuron = inner;
            nn_connections[connection_index].is_pulsing = false;
            nn_connections[connection_index].pulse_position = 0;


            if (nn_neurons[0].connection_count < 4) {
                nn_neurons[0].connections[nn_neurons[0].connection_count] = connection_index;
                nn_neurons[0].connection_count++;
            }
            connection_index++;
        }
    }


    for (int i = 1; i < 7; i++) {
        int next = (i == 6) ? 1 : (i + 1);
        if (connection_index < MAX_CONNECTIONS) {
            nn_connections[connection_index].from_neuron = i;
            nn_connections[connection_index].to_neuron = next;
            nn_connections[connection_index].is_pulsing = false;
            nn_connections[connection_index].pulse_position = 0;


            if (nn_neurons[i].connection_count < 4) {
                nn_neurons[i].connections[nn_neurons[i].connection_count] = connection_index;
                nn_neurons[i].connection_count++;
            }
            connection_index++;
        }
    }


    for (int inner = 1; inner < 7; inner++) {
        int outer = 7 + ((inner - 1) % 5);
        if (outer < MAX_NEURONS && connection_index < MAX_CONNECTIONS) {
            nn_connections[connection_index].from_neuron = inner;
            nn_connections[connection_index].to_neuron = outer;
            nn_connections[connection_index].is_pulsing = false;
            nn_connections[connection_index].pulse_position = 0;


            if (nn_neurons[inner].connection_count < 4) {
                nn_neurons[inner].connections[nn_neurons[inner].connection_count] = connection_index;
                nn_neurons[inner].connection_count++;
            }
            connection_index++;
        }
    }

    neuralnet_initialized = true;
    neuralnet_last_update = millis();
}

void updateNeuralNetPhysics() {
    unsigned long now = millis();


    if ((now - neuralnet_last_update) < 20) return;
    neuralnet_last_update = now;


    for (int i = 0; i < MAX_NEURONS; i++) {
        if (nn_neurons[i].is_active) {

            if (now - nn_neurons[i].activation_time > NEURON_ACTIVATION_DURATION) {
                nn_neurons[i].is_active = false;
                nn_neurons[i].brightness = nn_neurons[i].base_brightness;
            } else {

                float fade_progress = (float)(now - nn_neurons[i].activation_time) / NEURON_ACTIVATION_DURATION;
                nn_neurons[i].brightness = 255 - (fade_progress * (255 - nn_neurons[i].base_brightness));
            }
        } else {

            float breath = sin(now * 0.003 + i * 0.5) * 10;
            nn_neurons[i].brightness = constrain(nn_neurons[i].base_brightness + breath, 20, 80);
        }
    }


    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (nn_connections[i].is_pulsing) {
            float elapsed = now - nn_connections[i].pulse_start_time;
            nn_connections[i].pulse_position = elapsed / PULSE_TRAVEL_TIME;


            nn_connections[i].pulse_brightness = 255 - (nn_connections[i].pulse_position * 155);

            if (nn_connections[i].pulse_position >= 1.0) {

                uint8_t target = nn_connections[i].to_neuron;
                if (target < MAX_NEURONS) {
                    nn_neurons[target].is_active = true;
                    nn_neurons[target].activation_time = now;
                    nn_neurons[target].brightness = 255;


                    propagateNeuralPulse(target, now);
                }

                nn_connections[i].is_pulsing = false;
                nn_connections[i].pulse_position = 0;
            }
        }
    }
}

void propagateNeuralPulse(uint8_t neuron_index, unsigned long now) {
    if (neuron_index >= MAX_NEURONS) return;


    for (int i = 0; i < nn_neurons[neuron_index].connection_count; i++) {
        uint8_t connection_idx = nn_neurons[neuron_index].connections[i];
        if (connection_idx < MAX_CONNECTIONS && !nn_connections[connection_idx].is_pulsing) {
            nn_connections[connection_idx].is_pulsing = true;
            nn_connections[connection_idx].pulse_position = 0;
            nn_connections[connection_idx].pulse_start_time = now;
            nn_connections[connection_idx].pulse_brightness = 255;
        }
    }
}

void handleMidiNoteForNeuralNet(uint8_t note, uint8_t velocity) {
    unsigned long now = millis();





    if (note < 48) {
        nn_neurons[0].is_active = true;
        nn_neurons[0].activation_time = now;
        nn_neurons[0].brightness = constrain(velocity * 2, 150, 255);
        propagateNeuralPulse(0, now);
    }


    else if (note >= 48 && note < 84) {
        int ring_neuron = 1 + ((note - 48) % 6);
        nn_neurons[ring_neuron].is_active = true;
        nn_neurons[ring_neuron].activation_time = now;
        nn_neurons[ring_neuron].brightness = constrain(velocity * 2, 100, 255);
        propagateNeuralPulse(ring_neuron, now);
    }


    else if (note >= 84) {
        int outer_neuron = 7 + ((note - 84) % 5);
        nn_neurons[outer_neuron].is_active = true;
        nn_neurons[outer_neuron].activation_time = now;
        nn_neurons[outer_neuron].brightness = constrain(velocity * 2, 100, 255);
        propagateNeuralPulse(outer_neuron, now);
    }


    if (velocity > 110) {

        int cascade_neuron = 1 + random(0, 6);
        nn_neurons[cascade_neuron].is_active = true;
        nn_neurons[cascade_neuron].activation_time = now + 100;
        nn_neurons[cascade_neuron].brightness = 255;
        propagateNeuralPulse(cascade_neuron, now + 100);
    }


    if (note < 36 && velocity > 80) {
        for (int i = 1; i < 4; i++) {
            nn_neurons[i].is_active = true;
            nn_neurons[i].activation_time = now + (i * 50);
            nn_neurons[i].brightness = 255;
            propagateNeuralPulse(i, now + (i * 50));
        }
    }
}

void drawNeuralNetwork() {

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        uint8_t from_idx = nn_connections[i].from_neuron;
        uint8_t to_idx = nn_connections[i].to_neuron;

        if (from_idx < MAX_NEURONS && to_idx < MAX_NEURONS) {
            int x1 = (int)nn_neurons[from_idx].x;
            int y1 = (int)nn_neurons[from_idx].y;
            int x2 = (int)nn_neurons[to_idx].x;
            int y2 = (int)nn_neurons[to_idx].y;


            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);


            if (nn_connections[i].is_pulsing) {
                float pos = nn_connections[i].pulse_position;
                int pulse_x = x1 + (pos * (x2 - x1));
                int pulse_y = y1 + (pos * (y2 - y1));


                if (pulse_x >= 0 && pulse_x < 128 && pulse_y >= 0 && pulse_y < 32) {
                    display.fillRect(pulse_x - 1, pulse_y - 1, 3, 3, SSD1306_WHITE);
                    display.drawPixel(pulse_x, pulse_y, SSD1306_WHITE);
                }
            }
        }
    }


    for (int i = 0; i < MAX_NEURONS; i++) {
        int x = (int)nn_neurons[i].x;
        int y = (int)nn_neurons[i].y;

        if (x >= 0 && x < 128 && y >= 0 && y < 32) {
            if (nn_neurons[i].is_active) {

                display.fillRect(x - 2, y - 2, 5, 5, SSD1306_WHITE);
                display.drawPixel(x, y, SSD1306_WHITE);


                if (nn_neurons[i].brightness > 200) {
                    display.drawPixel(x - 3, y, SSD1306_WHITE);
                    display.drawPixel(x + 3, y, SSD1306_WHITE);
                    display.drawPixel(x, y - 3, SSD1306_WHITE);
                    display.drawPixel(x, y + 3, SSD1306_WHITE);
                }
            } else {

                display.drawRect(x - 1, y - 1, 3, 3, SSD1306_WHITE);
                display.drawPixel(x, y, SSD1306_WHITE);
            }
        }
    }
}

void neuralNetDemo() {
    if (!neuralnet_initialized) {
        initNeuralNetDemo();
    }


    updateNeuralNetPhysics();


    display.clearDisplay();


    static unsigned long title_start_time = 0;
    static bool title_shown = false;

    if (!title_shown) {
        title_start_time = millis();
        title_shown = true;
    }


    if ((millis() - title_start_time) < 2000) {
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("NEURAL NET");
    }


    drawNeuralNetwork();

    display.display();
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/send.ino"
void tfisend(int opnarray[42], int sendchannel) {


  midi_send_cc(sendchannel, 14, opnarray[0] * 16);
  midi_send_cc(sendchannel, 15, opnarray[1] * 16);

  midi_send_cc(sendchannel, 20, opnarray[2] * 8);
  midi_send_cc(sendchannel, 21, opnarray[12] * 8);
  midi_send_cc(sendchannel, 22, opnarray[22] * 8);
  midi_send_cc(sendchannel, 23, opnarray[32] * 8);

  midi_send_cc(sendchannel, 24, opnarray[3] * 32);
  midi_send_cc(sendchannel, 25, opnarray[13] * 32);
  midi_send_cc(sendchannel, 26, opnarray[23] * 32);
  midi_send_cc(sendchannel, 27, opnarray[33] * 32);
  delayMicroseconds(1500);
  handle_midi_input();

  midi_send_cc(sendchannel, 16, 127 - opnarray[4]);
  midi_send_cc(sendchannel, 17, 127 - opnarray[14]);
  midi_send_cc(sendchannel, 18, 127 - opnarray[24]);
  midi_send_cc(sendchannel, 19, 127 - opnarray[34]);

  midi_send_cc(sendchannel, 39, opnarray[5] * 32);
  midi_send_cc(sendchannel, 40, opnarray[15] * 32);
  midi_send_cc(sendchannel, 41, opnarray[25] * 32);
  midi_send_cc(sendchannel, 42, opnarray[35] * 32);
  handle_midi_input();

  midi_send_cc(sendchannel, 43, opnarray[6] * 4);
  midi_send_cc(sendchannel, 44, opnarray[16] * 4);
  midi_send_cc(sendchannel, 45, opnarray[26] * 4);
  midi_send_cc(sendchannel, 46, opnarray[36] * 4);

  midi_send_cc(sendchannel, 47, opnarray[7] * 4);
  midi_send_cc(sendchannel, 48, opnarray[17] * 4);
  midi_send_cc(sendchannel, 49, opnarray[27] * 4);
  midi_send_cc(sendchannel, 50, opnarray[37] * 4);
  handle_midi_input();

  midi_send_cc(sendchannel, 55, opnarray[10] * 8);
  midi_send_cc(sendchannel, 56, opnarray[20] * 8);
  midi_send_cc(sendchannel, 57, opnarray[30] * 8);
  midi_send_cc(sendchannel, 58, opnarray[40] * 8);

  midi_send_cc(sendchannel, 51, opnarray[8] * 8);
  midi_send_cc(sendchannel, 52, opnarray[18] * 8);
  midi_send_cc(sendchannel, 53, opnarray[28] * 8);
  midi_send_cc(sendchannel, 54, opnarray[38] * 8);
  handle_midi_input();

  midi_send_cc(sendchannel, 59, opnarray[9] * 8);
  midi_send_cc(sendchannel, 60, opnarray[19] * 8);
  midi_send_cc(sendchannel, 61, opnarray[29] * 8);
  midi_send_cc(sendchannel, 62, opnarray[39] * 8);

  midi_send_cc(sendchannel, 90, opnarray[11] * 8);
  midi_send_cc(sendchannel, 91, opnarray[21] * 8);
  midi_send_cc(sendchannel, 92, opnarray[31] * 8);
  midi_send_cc(sendchannel, 93, opnarray[41] * 8);
  handle_midi_input();

  midi_send_cc(sendchannel, 75, 90);
  midi_send_cc(sendchannel, 76, 90);
  midi_send_cc(sendchannel, 77, 127);

  midi_send_cc(sendchannel, 70, 0);
  midi_send_cc(sendchannel, 71, 0);
  midi_send_cc(sendchannel, 72, 0);
  midi_send_cc(sendchannel, 73, 0);


  fmsettings[sendchannel - 1][0] = opnarray[0] * 16;
  fmsettings[sendchannel - 1][1] = opnarray[1] * 16;
  fmsettings[sendchannel - 1][2] = opnarray[2] * 8;
  fmsettings[sendchannel - 1][12] = opnarray[12] * 8;
  fmsettings[sendchannel - 1][22] = opnarray[22] * 8;
  fmsettings[sendchannel - 1][32] = opnarray[32] * 8;
  fmsettings[sendchannel - 1][3] = opnarray[3] * 32;
  fmsettings[sendchannel - 1][13] = opnarray[13] * 32;
  fmsettings[sendchannel - 1][23] = opnarray[23] * 32;
  fmsettings[sendchannel - 1][33] = opnarray[33] * 32;
  fmsettings[sendchannel - 1][4] = 127 - opnarray[4];
  fmsettings[sendchannel - 1][14] = 127 - opnarray[14];
  fmsettings[sendchannel - 1][24] = 127 - opnarray[24];
  fmsettings[sendchannel - 1][34] = 127 - opnarray[34];
  fmsettings[sendchannel - 1][5] = opnarray[5] * 32;
  fmsettings[sendchannel - 1][15] = opnarray[15] * 32;
  fmsettings[sendchannel - 1][25] = opnarray[25] * 32;
  fmsettings[sendchannel - 1][35] = opnarray[35] * 32;
  fmsettings[sendchannel - 1][6] = opnarray[6] * 4;
  fmsettings[sendchannel - 1][16] = opnarray[16] * 4;
  fmsettings[sendchannel - 1][26] = opnarray[26] * 4;
  fmsettings[sendchannel - 1][36] = opnarray[36] * 4;
  fmsettings[sendchannel - 1][7] = opnarray[7] * 4;
  fmsettings[sendchannel - 1][17] = opnarray[17] * 4;
  fmsettings[sendchannel - 1][27] = opnarray[27] * 4;
  fmsettings[sendchannel - 1][37] = opnarray[37] * 4;
  fmsettings[sendchannel - 1][10] = 127 - (opnarray[10] * 8);
  fmsettings[sendchannel - 1][20] = 127 - (opnarray[20] * 8);
  fmsettings[sendchannel - 1][30] = 127 - (opnarray[30] * 8);
  fmsettings[sendchannel - 1][40] = 127 - (opnarray[40] * 8);
  fmsettings[sendchannel - 1][8] = opnarray[8] * 8;
  fmsettings[sendchannel - 1][18] = opnarray[18] * 8;
  fmsettings[sendchannel - 1][28] = opnarray[28] * 8;
  fmsettings[sendchannel - 1][38] = opnarray[38] * 8;
  fmsettings[sendchannel - 1][9] = opnarray[9] * 8;
  fmsettings[sendchannel - 1][19] = opnarray[19] * 8;
  fmsettings[sendchannel - 1][29] = opnarray[29] * 8;
  fmsettings[sendchannel - 1][39] = opnarray[39] * 8;
  fmsettings[sendchannel - 1][11] = opnarray[11] * 8;
  fmsettings[sendchannel - 1][21] = opnarray[21] * 8;
  fmsettings[sendchannel - 1][31] = opnarray[31] * 8;
  fmsettings[sendchannel - 1][41] = opnarray[41] * 8;
  fmsettings[sendchannel - 1][42] = 90;
  fmsettings[sendchannel - 1][43] = 90;
  fmsettings[sendchannel - 1][44] = 127;
  fmsettings[sendchannel - 1][45] = 0;
  fmsettings[sendchannel - 1][46] = 0;
  fmsettings[sendchannel - 1][47] = 0;
  fmsettings[sendchannel - 1][48] = 0;
  fmsettings[sendchannel - 1][49] = 0;
}

void fmccsend(uint8_t potnumber, uint8_t potvalue) {



  switch (fmscreen) {


    case 1:
      {

        oled_clear();
        if (polypan > 64) {
          oled_print(1, 16, "L R ON");
        } else {
          oled_print(1, 16, "L R OFF");
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][44] = 127;
          midi_send_cc(tfichannel, 77, 127);
        }

        if (potnumber == 0) { polypan = potvalue; }
        if (potnumber == 1) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][0] = potvalue;
          midi_send_cc(tfichannel, 14, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][1] = potvalue;
          midi_send_cc(tfichannel, 15, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][44] = potvalue;
          midi_send_cc(tfichannel, 77, potvalue);
        }
        break;
      }


    case 2:
      {
        if (potnumber == 0) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][4] = potvalue;
          midi_send_cc(tfichannel, 16, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][24] = potvalue;
          midi_send_cc(tfichannel, 18, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][14] = potvalue;
          midi_send_cc(tfichannel, 17, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][34] = potvalue;
          midi_send_cc(tfichannel, 19, potvalue);
        }
        break;
      }


    case 3:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][2] = potvalue;
          midi_send_cc(tfichannel, 20, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][22] = potvalue;
          midi_send_cc(tfichannel, 22, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][12] = potvalue;
          midi_send_cc(tfichannel, 21, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][32] = potvalue;
          midi_send_cc(tfichannel, 23, potvalue);
        }
        break;
      }


    case 4:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][3] = potvalue;
          midi_send_cc(tfichannel, 24, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][23] = potvalue;
          midi_send_cc(tfichannel, 26, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][13] = potvalue;
          midi_send_cc(tfichannel, 25, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][33] = potvalue;
          midi_send_cc(tfichannel, 27, potvalue);
        }
        break;
      }


    case 5:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][5] = potvalue;
          midi_send_cc(tfichannel, 39, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][25] = potvalue;
          midi_send_cc(tfichannel, 41, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][15] = potvalue;
          midi_send_cc(tfichannel, 40, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][35] = potvalue;
          midi_send_cc(tfichannel, 42, potvalue);
        }
        break;
      }


    case 6:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][6] = potvalue;
          midi_send_cc(tfichannel, 43, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][26] = potvalue;
          midi_send_cc(tfichannel, 45, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][16] = potvalue;
          midi_send_cc(tfichannel, 44, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][36] = potvalue;
          midi_send_cc(tfichannel, 46, potvalue);
        }
        break;
      }


    case 7:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][7] = potvalue;
          midi_send_cc(tfichannel, 47, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][27] = potvalue;
          midi_send_cc(tfichannel, 49, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][17] = potvalue;
          midi_send_cc(tfichannel, 48, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][37] = potvalue;
          midi_send_cc(tfichannel, 50, potvalue);
        }
        break;
      }


    case 8:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][10] = 127 - potvalue;
          midi_send_cc(tfichannel, 55, 127 - potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][30] = 127 - potvalue;
          midi_send_cc(tfichannel, 57, 127 - potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][20] = 127 - potvalue;
          midi_send_cc(tfichannel, 56, 127 - potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][40] = 127 - potvalue;
          midi_send_cc(tfichannel, 58, 127 - potvalue);
        }
        break;
      }


    case 9:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][8] = potvalue;
          midi_send_cc(tfichannel, 51, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][28] = potvalue;
          midi_send_cc(tfichannel, 53, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][18] = potvalue;
          midi_send_cc(tfichannel, 52, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][38] = potvalue;
          midi_send_cc(tfichannel, 54, potvalue);
        }
        break;
      }


    case 10:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][9] = potvalue;
          midi_send_cc(tfichannel, 59, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][29] = potvalue;
          midi_send_cc(tfichannel, 61, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][19] = potvalue;
          midi_send_cc(tfichannel, 60, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][39] = potvalue;
          midi_send_cc(tfichannel, 62, potvalue);
        }
        break;
      }


    case 11:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][11] = potvalue;
          midi_send_cc(tfichannel, 90, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][31] = potvalue;
          midi_send_cc(tfichannel, 92, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][21] = potvalue;
          midi_send_cc(tfichannel, 91, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][41] = potvalue;
          midi_send_cc(tfichannel, 93, potvalue);
        }
        break;
      }


    case 12:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][45] = potvalue;
          midi_send_cc(tfichannel, 70, potvalue);
        }
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][47] = potvalue;
          midi_send_cc(tfichannel, 72, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][46] = potvalue;
          midi_send_cc(tfichannel, 71, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][48] = potvalue;
          midi_send_cc(tfichannel, 73, potvalue);
        }
        break;
      }


    case 13:
      {

        oled_clear();
        oled_print(1, 16, "   ");

        if (potnumber == 1) {
          lfospeed = potvalue;
          midi_send_cc(1, 1, potvalue);
        }
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][42] = potvalue;
          midi_send_cc(tfichannel, 75, potvalue);
        }
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][43] = potvalue;
          midi_send_cc(tfichannel, 76, potvalue);
        }
        break;
      }

  }
}

void saveprompt(void) {
  menuprompt = 0;

  oled_clear();
  oled_print(0, 0, "UP: overwrite");
  oled_print(0, 8, "DOWN: save new");
  oled_print(0, 16, "PRST: Cancel");
  oled_refresh();

  while (menuprompt == 0) {
    handle_midi_input();
    lcd_key = read_buttons();
    switch (lcd_key) {
      case btnRIGHT:
      case btnLEFT:
      case btnSELECT:
        menuprompt = 1;
        messagestart = millis();
        oled_clear();
        oled_print(0, 0, "save cancelled");
        refreshscreen = 1;
        oled_refresh();
        break;

      case btnUP:
        menuprompt = 1;
        saveoverwrite();
        break;

      case btnDOWN:
        menuprompt = 1;
        savenew();
        break;
    }
  }
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/settings.ino"
void bootprompt(void) {
  uint8_t currentpotvalue[4];
  uint8_t lastDisplayedChannel = 255;
  uint8_t lastDisplayedRegion = 255;

  menuprompt = 0;


  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("MIDI CH / REGION");
  display.display();
  mutex_exit(&display_mutex);

  while (menuprompt == 0) {
    handle_midi_input();


    selectMuxChannel(0);
    delayMicroseconds(50);
    currentpotvalue[0] = analogRead(MUX_SIG_PIN) >> 3;


    selectMuxChannel(2);
    delayMicroseconds(50);
    currentpotvalue[2] = analogRead(MUX_SIG_PIN) >> 3;

    midichannel = (currentpotvalue[0] / 8) + 1;
    if (midichannel > 16) midichannel = 16;
    if (midichannel < 1) midichannel = 1;

    uint8_t currentRegion = (currentpotvalue[2] < 64) ? 0 : 1;


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


    lcd_key = read_buttons();
    switch (lcd_key) {
      case btnSELECT:
        region = currentRegion;


        if (region == 0) {
          midi_send_cc(1, 83, 75);
        } else {
          midi_send_cc(1, 83, 1);
        }


        flash_write_settings(region, midichannel, velocity_curve);


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
        menuprompt = 1;
        break;

      case btnUP:
      case btnDOWN:
      case btnLEFT:
      case btnRIGHT:

        break;

      default:
        break;
    }


    delay(50);
  }


  mutex_enter_blocking(&display_mutex);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Ready to boot!");
  display.display();
  mutex_exit(&display_mutex);

  delay(1000);
}

void saveSettingsToSD(void) {

  if (!SD.exists("/settings")) {
    SD.mkdir("/settings");
  }


  File settingsFile = SD.open("/settings/settings.ini", FILE_WRITE);
  if (!settingsFile) {
    return;
  }


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

  settingsFile.flush();
  settingsFile.close();
  delay(50);
}

bool loadSettingsFromSD(void) {

  if (!SD.exists("/settings/settings.ini")) {
    return false;
  }

  File settingsFile = SD.open("/settings/settings.ini", FILE_READ);
  if (!settingsFile) {
    return false;
  }

  String line;
  while (settingsFile.available()) {
    line = settingsFile.readStringUntil('\n');
    line.trim();


    if (line.length() == 0 || line.startsWith("#") || line.startsWith("[")) {
      continue;
    }


    int equalPos = line.indexOf('=');
    if (equalPos > 0) {
      String key = line.substring(0, equalPos);
      String value = line.substring(equalPos + 1);

      if (key == "region") {
        region = value.toInt();
        if (region > 1) region = 0;
      } else if (key == "midichannel") {
        midichannel = value.toInt();
        if (midichannel < 1 || midichannel > 16) midichannel = 1;
      } else if (key == "velocity_curve") {
        velocity_curve = value.toInt();
        if (velocity_curve > 4) velocity_curve = 4;
      }
    }
  }

  settingsFile.close();
  return true;
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


    if (region == 0) {
      midi_send_cc(1, 83, 75);
    } else {
      midi_send_cc(1, 83, 1);
    }
  }

  if (temp_velocity_curve != velocity_curve) {
    velocity_curve = temp_velocity_curve;
    needs_restart = true;
  }

  if (needs_restart) {
    flash_write_settings(region, midichannel, velocity_curve);
    saveSettingsToSD();

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
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/starfighter.ino"



struct Star {
    float x, y;
    float speed;
    uint8_t brightness;
    uint8_t base_brightness;
    uint8_t size;
    unsigned long flash_time;
    bool midi_triggered;
    unsigned long twinkle_time;
};

struct StarfighterShip {
    float y;
    float y_offset;
    unsigned long last_engine_pulse;
    bool engine_on;
};

struct EnvironmentalEffect {
    bool hyperspace_active;
    unsigned long hyperspace_end_time;
    bool nebula_active;
    float nebula_x, nebula_y;
    unsigned long nebula_end_time;
    bool solar_flare_active;
    float flare_progress;
    unsigned long flare_end_time;
    bool warp_ripples_active;
    float ripple_radius;
    unsigned long ripple_end_time;
};


#define MAX_STARS 96
Star sf_stars[MAX_STARS];
StarfighterShip starfighter_ship;
EnvironmentalEffect env_effects;
bool starfighter_initialized = false;
unsigned long starfighter_last_update = 0;
const float SF_SCREEN_WIDTH = 128.0;
const float SF_SCREEN_HEIGHT = 32.0;
const uint16_t MIDI_FLASH_DURATION = 300;

void initStarfighterDemo() {

    starfighter_ship.y = SF_SCREEN_WIDTH / 2;
    starfighter_ship.y_offset = 0;
    starfighter_ship.last_engine_pulse = 0;
    starfighter_ship.engine_on = false;


    for (int i = 0; i < MAX_STARS; i++) {

        sf_stars[i].x = (i * 127) / MAX_STARS + (i % 7);


        sf_stars[i].y = i % 32;


        if (sf_stars[i].x > 127) sf_stars[i].x = 127;
        if (sf_stars[i].y > 31) sf_stars[i].y = 31;


        sf_stars[i].speed = 0.2 + (i % 10) * 0.18;
        sf_stars[i].brightness = 255;
        sf_stars[i].base_brightness = 255;
        sf_stars[i].size = 1;
        sf_stars[i].flash_time = 0;
        sf_stars[i].midi_triggered = false;
        sf_stars[i].twinkle_time = 0;
    }


    env_effects.hyperspace_active = false;
    env_effects.nebula_active = false;
    env_effects.solar_flare_active = false;
    env_effects.warp_ripples_active = false;

    starfighter_initialized = true;
    starfighter_last_update = millis();
}

void updateStarfighterPhysics() {
    unsigned long now = millis();
    float dt = (now - starfighter_last_update) / 1000.0;


    if (dt < 0.01) return;
    if (dt > 0.1) dt = 0.1;

    starfighter_last_update = now;


    static int oscillation_counter = 0;
    oscillation_counter++;
    if (oscillation_counter > 1000) oscillation_counter = 0;
    starfighter_ship.y = (SF_SCREEN_WIDTH / 2) + sin(oscillation_counter * 0.01) * 4;


    if (now - starfighter_ship.last_engine_pulse > 150) {
        starfighter_ship.engine_on = !starfighter_ship.engine_on;
        starfighter_ship.last_engine_pulse = now;
    }


    for (int i = 0; i < MAX_STARS; i++) {

        if (i >= MAX_STARS) break;


        sf_stars[i].x -= sf_stars[i].speed;


        if (sf_stars[i].x < -100 || sf_stars[i].x > 300) {
            sf_stars[i].x = 128 + (i % 20);
        }
        if (sf_stars[i].y < 0 || sf_stars[i].y > 31) {
            sf_stars[i].y = i % 32;
        }


        if (sf_stars[i].x < -5) {
            sf_stars[i].x = 128 + (i % 20);

        }


        if (sf_stars[i].midi_triggered) {
            if (now - sf_stars[i].flash_time > MIDI_FLASH_DURATION) {

                sf_stars[i].midi_triggered = false;
                sf_stars[i].brightness = 255;


                if (i < 30) {
                    sf_stars[i].size = 1;
                } else if (i < 55) {
                    sf_stars[i].size = (i % 2) + 1;
                } else if (i < 75) {
                    sf_stars[i].size = (i % 3) + 1;
                } else {
                    sf_stars[i].size = (i % 2) + 2;
                }
            } else {

                float flash_progress = (float)(now - sf_stars[i].flash_time) / MIDI_FLASH_DURATION;
                float twinkle = sin((now - sf_stars[i].flash_time) * 0.03) * 30;
                sf_stars[i].brightness = constrain(255 - (flash_progress * 50) + twinkle, 200, 255);
            }
        } else {

            sf_stars[i].brightness = 255;
        }
    }


    if (env_effects.hyperspace_active && now > env_effects.hyperspace_end_time) {
        env_effects.hyperspace_active = false;


        for (int i = 0; i < MAX_STARS; i++) {
            if (i >= MAX_STARS) break;


            if (i < 30) {
                sf_stars[i].size = 1;
            } else if (i < 55) {
                sf_stars[i].size = (i % 2) + 1;
            } else if (i < 75) {
                sf_stars[i].size = (i % 3) + 1;
            } else {
                sf_stars[i].size = (i % 2) + 2;
            }


            sf_stars[i].midi_triggered = false;
            sf_stars[i].brightness = 255;
        }
    }



    if (env_effects.solar_flare_active) {
        if (now > env_effects.flare_end_time) {
            env_effects.solar_flare_active = false;
        } else {
            env_effects.flare_progress = (float)(now - (env_effects.flare_end_time - 500)) / 500.0;
        }
    }

    if (env_effects.warp_ripples_active) {
        if (now > env_effects.ripple_end_time) {
            env_effects.warp_ripples_active = false;
        } else {
            float progress = (float)(now - (env_effects.ripple_end_time - 1000)) / 1000.0;
            env_effects.ripple_radius = progress * 50;
        }
    }


}

void handleMidiNoteForStarfighter(uint8_t note, uint8_t velocity) {
    unsigned long now = millis();


    static unsigned long last_effect_time = 0;
    static uint8_t effect_cooldown_counter = 0;


    if ((now - last_effect_time) < 100) return;

    effect_cooldown_counter++;
    if (effect_cooldown_counter > 250) effect_cooldown_counter = 0;


    if (velocity > 118 && effect_cooldown_counter % 40 == 0) {
        env_effects.hyperspace_active = true;
        env_effects.hyperspace_end_time = now + 1200;
        last_effect_time = now;
    }


    else if (velocity > 110 && effect_cooldown_counter % 12 == 0) {
        env_effects.solar_flare_active = true;
        env_effects.flare_progress = 0;
        env_effects.flare_end_time = now + 500;
        last_effect_time = now;
    }


    else if (note < 45 && velocity > 30 && effect_cooldown_counter % 3 == 0) {
        env_effects.warp_ripples_active = true;
        env_effects.ripple_radius = 0;
        env_effects.ripple_end_time = now + 1000;
        last_effect_time = now;
    }


    if (!env_effects.hyperspace_active) {
        int stars_to_twinkle = map(velocity, 0, 127, 3, 8);

        for (int twinkle_count = 0; twinkle_count < stars_to_twinkle; twinkle_count++) {

            int random_star = random(0, MAX_STARS);
            if (random_star >= MAX_STARS) random_star = MAX_STARS - 1;


            sf_stars[random_star].midi_triggered = true;
            sf_stars[random_star].flash_time = now;
            sf_stars[random_star].brightness = 255;


            sf_stars[random_star].size = 4;
        }
    }
}

void drawStarfighterShip() {
    int ship_x = (int)starfighter_ship.y;
    int ship_y = (int)(SF_SCREEN_HEIGHT / 2);



    display.drawLine(ship_x - 10, ship_y, ship_x + 6, ship_y - 1, SSD1306_WHITE);
    display.drawLine(ship_x - 10, ship_y, ship_x + 6, ship_y + 1, SSD1306_WHITE);
    display.drawLine(ship_x + 6, ship_y - 1, ship_x + 8, ship_y, SSD1306_WHITE);
    display.drawLine(ship_x + 6, ship_y + 1, ship_x + 8, ship_y, SSD1306_WHITE);



    display.drawLine(ship_x - 4, ship_y - 1, ship_x - 6, ship_y - 10, SSD1306_WHITE);
    display.drawLine(ship_x - 4, ship_y - 1, ship_x - 2, ship_y - 10, SSD1306_WHITE);
    display.drawLine(ship_x - 6, ship_y - 10, ship_x - 4, ship_y - 12, SSD1306_WHITE);
    display.drawLine(ship_x - 2, ship_y - 10, ship_x - 4, ship_y - 12, SSD1306_WHITE);


    display.drawLine(ship_x - 4, ship_y + 1, ship_x - 6, ship_y + 10, SSD1306_WHITE);
    display.drawLine(ship_x - 4, ship_y + 1, ship_x - 2, ship_y + 10, SSD1306_WHITE);
    display.drawLine(ship_x - 6, ship_y + 10, ship_x - 4, ship_y + 12, SSD1306_WHITE);
    display.drawLine(ship_x - 2, ship_y + 10, ship_x - 4, ship_y + 12, SSD1306_WHITE);


    display.drawPixel(ship_x - 3, ship_y - 8, SSD1306_WHITE);
    display.drawPixel(ship_x - 3, ship_y + 8, SSD1306_WHITE);
    display.drawPixel(ship_x - 1, ship_y - 6, SSD1306_WHITE);
    display.drawPixel(ship_x - 1, ship_y + 6, SSD1306_WHITE);


    display.fillRect(ship_x - 2, ship_y - 1, 6, 3, SSD1306_WHITE);
    display.drawPixel(ship_x + 1, ship_y, SSD1306_BLACK);


    if (starfighter_ship.engine_on) {

        display.drawPixel(ship_x - 11, ship_y - 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 11, ship_y + 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 12, ship_y - 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 12, ship_y + 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 13, ship_y, SSD1306_WHITE);
        display.drawPixel(ship_x - 14, ship_y, SSD1306_WHITE);
    } else {

        display.drawPixel(ship_x - 11, ship_y - 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 11, ship_y + 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 12, ship_y, SSD1306_WHITE);
    }


    display.drawPixel(ship_x - 6, ship_y, SSD1306_WHITE);
    display.drawPixel(ship_x + 3, ship_y, SSD1306_WHITE);
}

void drawStarfield() {

    for (int i = 0; i < MAX_STARS; i++) {

        if (i >= MAX_STARS) break;

        int x = (int)sf_stars[i].x;
        int y = (int)sf_stars[i].y;


        if (x >= 0 && x < 128 && y >= 0 && y < 32 && x > -10 && y > -5) {

            if (env_effects.hyperspace_active) {

                int streak_length = (sf_stars[i].speed * 4);
                if (streak_length > 12) streak_length = 12;
                for (int streak = 0; streak < streak_length && x + streak < 128; streak += 2) {
                    display.drawPixel(x + streak, y, SSD1306_WHITE);
                }
            } else {

                if (sf_stars[i].size == 1) {

                    if (x >= 0 && x < 128 && y >= 0 && y < 32)
                        display.drawPixel(x, y, SSD1306_WHITE);
                } else if (sf_stars[i].size == 2) {

                    if (x >= 0 && x < 127 && y >= 0 && y < 31) {
                        display.drawPixel(x, y, SSD1306_WHITE);
                        display.drawPixel(x + 1, y, SSD1306_WHITE);
                        display.drawPixel(x, y + 1, SSD1306_WHITE);
                        display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
                    }
                } else if (sf_stars[i].size == 3) {

                    if (x >= 1 && x < 127 && y >= 1 && y < 31) {
                        display.drawPixel(x, y, SSD1306_WHITE);
                        display.drawPixel(x + 1, y, SSD1306_WHITE);
                        display.drawPixel(x - 1, y, SSD1306_WHITE);
                        display.drawPixel(x, y + 1, SSD1306_WHITE);
                        display.drawPixel(x, y - 1, SSD1306_WHITE);
                    }
                } else {

                    if (x >= 1 && x < 127 && y >= 1 && y < 31) {
                        display.drawPixel(x, y, SSD1306_WHITE);
                        display.drawPixel(x + 1, y, SSD1306_WHITE);
                        display.drawPixel(x - 1, y, SSD1306_WHITE);
                        display.drawPixel(x, y + 1, SSD1306_WHITE);
                        display.drawPixel(x, y - 1, SSD1306_WHITE);
                        display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
                        display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
                        display.drawPixel(x + 1, y - 1, SSD1306_WHITE);
                        display.drawPixel(x - 1, y + 1, SSD1306_WHITE);
                    }
                }
            }
        }
    }
}

void drawEnvironmentalEffects() {



    if (env_effects.solar_flare_active) {
        int flare_x = env_effects.flare_progress * 140;

        for (int y = 0; y < 32; y++) {
            if (flare_x >= 0 && flare_x < 128) {
                display.drawPixel(flare_x, y, SSD1306_WHITE);
            }

            if (flare_x - 1 >= 0 && flare_x - 1 < 128) {
                display.drawPixel(flare_x - 1, y, SSD1306_WHITE);
            }
        }
    }


    if (env_effects.warp_ripples_active) {
        int ship_x = (int)starfighter_ship.y;
        int ship_y = 16;
        int radius = (int)env_effects.ripple_radius;


        if (radius > 25) radius = 25;


        if (radius > 0) {

            for (int x = ship_x - radius; x <= ship_x + radius; x += 3) {
                if (x >= 0 && x < 128) {
                    if (ship_y - radius >= 0) display.drawPixel(x, ship_y - radius, SSD1306_WHITE);
                    if (ship_y + radius < 32) display.drawPixel(x, ship_y + radius, SSD1306_WHITE);
                }
            }

            for (int y = ship_y - radius; y <= ship_y + radius; y += 3) {
                if (y >= 0 && y < 32) {
                    if (ship_x - radius >= 0) display.drawPixel(ship_x - radius, y, SSD1306_WHITE);
                    if (ship_x + radius < 128) display.drawPixel(ship_x + radius, y, SSD1306_WHITE);
                }
            }
        }
    }
}

void starfighterDemo() {
    if (!starfighter_initialized) {
        initStarfighterDemo();
    }


    updateStarfighterPhysics();


    display.clearDisplay();
    drawStarfield();
    drawEnvironmentalEffects();
    drawStarfighterShip();
    display.display();
}
# 1 "C:/Users/eggst/Documents/PlatformIO/Projects/250923-083329-pico/src/viz.ino"
const uint16_t peak_hold_duration = 800;
const uint8_t velocity_decay_rate = 2;
void oled_complete_update_protected(void (*update_function)());
extern uint8_t viz_sub_mode;

void updateVelocityViz(uint8_t channel, uint8_t velocity) {
    if (channel >= 11 || channel >= sizeof(viz_data.voice_velocity)) return;

    mutex_enter_blocking(&viz_mutex);

    viz_data.voice_velocity[channel] = velocity;
    viz_data.needs_update = true;


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

    oled_complete_update_protected([]{


        handle_midi_input();

        display.clearDisplay();


        display.setCursor(0, 0);
        display.setTextSize(1);
        if (mode == 5) {
            display.print("MONO VIZ");
        } else {
            display.print("POLY VIZ");
        }


        display.setCursor(70, 0);
        float khz = midi_hz / 1000.0;
        display.print(khz, 1);
        display.print("k ");
        if (max_midi_gap < 1000) {
            display.print("<1");
        } else {
            display.print((int)(max_midi_gap / 1000));
        }
        display.print("m");


        handle_midi_input();


        for (int channel = 0; channel < 11; channel++) {

            if (channel % 4 == 0) {
                handle_midi_input();
            }

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


            display.setCursor(x_start + 2, 18);
            display.setTextSize(1);
            if (channel < 6) {
                display.print(channel + 1);
            } else if (channel < 9) {
                display.print("PSG");
            } else if (channel == 9) {
                display.print("N");
            } else {
                display.print("DAC");
            }
        }


        handle_midi_input();
        display.display();

    });
}

void core1_visualizerDisplay(const VisualizerData& data) {

    if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 1) {
        asteroidsDemo();
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 2) {
        starfighterDemo();
    } else if ((viz_data.current_mode == 5 || viz_data.current_mode == 6) && viz_sub_mode == 3) {
        neuralNetDemo();
    } else if (viz_data.current_mode == 6) {
        polyVisualizerDisplay();
    } else {

        display.clearDisplay();


        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("MONO VIZ");


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


    display.setCursor(0, 0);
    display.print("POLY VOICES");


    for (int i = 0; i < 6; i++) {
        int x = 10 + (i * 18);
        int bar_width = 16;
        int bar_height = 20;
        int y_base = 31;

        if (polyon[i]) {

            display.fillRect(x, y_base - bar_height, bar_width, bar_height, SSD1306_WHITE);


            char noteStr[8];
            midiNoteToString(polynote[i], noteStr);


            display.fillRect(x + 2, y_base - 8, 12, 7, SSD1306_BLACK);


            display.setCursor(x + 3, y_base - 7);
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display.print(noteStr);
            display.setTextColor(SSD1306_WHITE);

        } else {

            display.drawRect(x, y_base - bar_height, bar_width, bar_height, SSD1306_WHITE);


            display.setCursor(x + 3, y_base - 7);
            display.print("---");
        }
    }

    display.display();
}