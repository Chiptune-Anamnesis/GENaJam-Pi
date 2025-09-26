# GENajam Pico v1.31

A Raspberry Pi Pico (RP2040) port of the GENajam MIDI controller for Little-scale's GENMDM module. This is a modernized version of the original Arduino Mega 2560 GENajam by JAMATAR, featuring enhanced file browsing, real-time MIDI display, and improved responsiveness.

## Features

### Core Functionality
- **6-channel FM synthesis control** via GENMDM module
- **Polyphonic and monophonic modes** with intelligent voice allocation
- **Real-time parameter editing** with 4 potentiometers
- **TFI file management** (load, save, delete) with SD card support
- **OLED display** (128x32) for visual feedback
- **MIDI panic button** for emergency note-off

### Enhancements
- **Recursive directory scanning** - finds TFI files in subdirectories
- **Accelerated navigation** - Hold left/right for dynamic (2 modes) fast file scrolling.
- **Bar graph Viz for MONO Mode** - Bar Graph shows incoming MIDI CH. 1-11.
- **Bar graph Viz for POLY Mode** - Bar Graph shows incoming notes for 6 FM channels.
- **Dual-Core** - Core 0 handles MIDI, etc. Core 1 handles the Visualizer.
- **Savestate of TFI instruments** - Save and load tfi configuration to all channels.
- **USB MIDI and TRS MIDI** - Both USB (MIDI IN) and TRS TYPE A 3.5 (IN and OUT).
- **Settings mode available on booted device** - Choose Poly Midi Ch, NTSC/PAL.
- **Stuckless mode switching** - Switching between poly and mono sends a note off command for all notes to avoid stuck notes.
- **True mono mode** - One note at a time per channel with last-note priority.
- **Velocity Settings** - Hard / Medium / Soft / Original

## Hardware Requirements

### Core Components
- Raspberry Pi Pico (RP2040)
- 128x32 OLED display (SSD1306, I2C)
- MicroSD card module (SPI)
- TBD

### Pin Configuration
```
OLED (I2C):
- SDA: Pin 8
- SCL: Pin 9

SD Card (SPI):
- CS:   Pin 5
- MISO: Pin 4
- SCK:  Pin 6
- MOSI: Pin 7

MIDI:
- TX: Pin 0
- RX: Pin 1

Potentiometers (Analog):
- Multiplexer: Pin 28
- Multiplexer: C0-C3 to Pots
- Multiplexer S0-S3: Pin 10-13

Buttons (Digital, with pullups):
- Preset/Edit: Pin 20
- Left:        Pin 14
- Right:       Pin 15
- CH Up:       Pin 16
- CH Down:     Pin 17
- Mono/Poly:   Pin 18
- Delete:      Pin 19
- Panic:       Pin 21
```

## Operation Modes

### Mode 1: Mono Preset
- Browse and load TFI files per channel
- Use UP/DOWN to select FM channel (1-6)
- Use LEFT/RIGHT to browse presets
- Immediate loading for responsive channel switching

### Mode 1a: Mono Edit
- Real-time FM parameter editing
- 4 pots control selected parameter screen
- 13 parameter screens available
- Changes apply to current channel only

### Mode 2: Poly Preset
- Browse presets for polyphonic play
- All 6 channels loaded with same preset
- 2-second delayed loading prevents browsing lag
- DELETE button removes files

### Mode 2a: Poly Edit
- Real-time editing in poly mode
- Parameter changes affect all 6 channels
- Same 13 parameter screens as mono mode

### Mode 3: MIDI Visualization
- Real-time bar meter for channels 1-11

### Mode 4a: Mono Bank
- Save / Load TFI presets for all channels

### Mode 4b: Poly Bank
- Save / Load TFI preset

### Mode Navigation

## Mono
- Cycle: VIZ → PRESETS → FM EDIT → BANK MGR → SETTINGS → VIZ...  
## Poly
Cycle: VIZ → PRESETS → FM EDIT → VIZ... (BANK MGR and SETTINGS are disabled in POLY mode) 

## Mode Switching:


  ├─ PRESET Button: Cycle through modes within current MONO/POLY setting
  
  └─ POLY Button: Toggle between MONO ↔ POLY modes
  

  VISUALIZER SUB-MODES (UP/DOWN in VIZ mode):
  
  ├─ Bar Graph     (Classic frequency bars)
  
  ├─ Asteroids     (Space debris field)
  
  ├─ Starfighter   (Space flight simulation)
  
  └─ Neural Net    (Hexagonal network visualization)
  

  PRESETS MODE:
  
  ├─ LEFT/RIGHT: Browse TFI files
  
  ├─ UP/DOWN: Select channel (MONO) or all channels (POLY)
  
  └─ OPT1: Load selected TFI
  

  FM EDIT MODE:
  
  ├─ LEFT/RIGHT: Navigate FM parameter screens
  
  ├─ UP/DOWN: Select channel
  
  ├─ Potentiometers: Adjust FM parameters in real-time
  
  └─ OPT2: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel)
  

  BANK MGR MODE (MONO only):
  
  ├─ LEFT/RIGHT: Browse saved presets
  
  ├─ OPT2: Save current settings as preset
  
  ├─ OPT1: Load selected preset
  
  └─ DELETE: Delete selected preset
  

  SETTINGS MODE (MONO only):
  
  ├─ LEFT/RIGHT: Navigate setting categories
  
  ├─ UP/DOWN: Adjust values
  
  └─ OPT2: Confirm and save changes
  


## MIDI Features

### Polyphony System
- **6-voice polyphony** with intelligent voice stealing
- **Sustain pedal support** (CC#64)
- **Modulation wheel control** (CC#1) - controls LFO on/off
- **Pitch bend** - affects all voices in poly mode
- **Velocity curve** - musical response across dynamic range

### Voice Management
- Protects lowest note from voice stealing
- Random stereo positioning in poly mode
- Proper note-off handling with sustain
- MIDI panic clears all stuck notes

## File Management

### TFI File Support
- **Recursive directory scanning** - finds files in any folder depth
- **Fast file browsing** with acceleration (hold buttons for turbo speed)
- **Save new patches** - automatically numbered (newpatch001.tfi, etc.)
- **Overwrite existing** - update files in place
- **Delete files** - with confirmation prompt

### SD Card Structure
```
/
├── tfi/                    ← All TFI files go here
│   ├── bass/
│   │   ├── bass001.tfi
│   │   └── bass002.tfi
│   ├── lead/
│   │   ├── lead001.tfi
│   │   └── lead002.tfi
│   ├── drums/
│   │   ├── kick.tfi
│   │   └── snare.tfi
│   └── newpatch001.tfi     ← New saves go here
├── presets/                ← Bank preset files
│   ├── 001.mdm_pre
│   └── 002.mdm_pre
└── settings/               ← Settings backup
    └── settings.ini
```

## Getting Started

### Quick Installation (Recommended)
1. **Download** the latest `genajam-pico-v1.31.uf2` file from releases
2. **Hold the BOOTSEL button** on your Pico and plug it into USB
3. **Drag and drop** the UF2 file onto the RPI-RP2 drive that appears
4. **Setup complete!** - The Pico will automatically reboot and start GenaJam

### Development Setup (Arduino IDE)
1. Install RP2040 board package
2. Install required libraries:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - MIDI Library
   - Wire (built-in)
   - SPI (built-in)
   - SD (built-in)
   - EEPROM (built-in)

### Initial Configuration
1. **Boot menu**: Hold PRESET on startup to configure MIDI channel and region (NTSC/PAL)
2. **Load TFI files** onto SD card in `/tfi/` folder (will be created automatically)
3. **Connect GENMDM** via MIDI OUT
4. **Power on** and start making music!

## Controls

### Navigation
- **PRESET**: Toggle between Preset/Edit modes
- **MONO/POLY**: Toggle between Mono/Poly modes
- **LEFT/RIGHT**: Browse files or parameter screens
- **UP/DOWN**: Select channel (mono) or save options (poly)
- **DELETE**: Delete files (poly mode only)
- **PANIC**: Emergency all-notes-off
- **OPT1**: Future Feature Placeholder
- **OPT2**: Future Feature Placeholder

### Editing
- **4 Potentiometers thru Multiplexer**: Control selected parameters
- Real-time visual feedback on OLED
- Parameter values update immediately
- Changes affect current channel (mono) or all channels (poly)

## Parameter Screens

1. **Algorithm, Feedback, Pan** - Core FM structure
2. **OP Volume** - Individual operator levels
3. **Frequency Multiple** - Harmonic ratios
4. **Detune** - Fine frequency adjustments
5. **Rate Scaling** - Envelope scaling with pitch
6. **Attack** - Envelope attack rates
7. **Decay 1** - Initial decay rates
8. **Sustain** - Sustain levels
9. **Decay 2** - Secondary decay rates
10. **Release** - Release rates
11. **SSG-EG** - Special envelope modes
12. **Amp Modulation** - Amplitude modulation on/off
13. **LFO/FM/AM** - Global modulation controls

## Technical Notes

### Performance Optimizations
- **Delayed loading** prevents MIDI lag during file browsing
- **MIDI processing** during parameter updates maintains responsiveness
- **Efficient file scanning** with progress display
- **Button acceleration** for fast navigation
- **Dual core** offloaded tasks to the extra core

### Memory Management
- **EEPROM storage** for MIDI channel and region settings
- **Efficient file indexing** supports 999 TFI files
- **Scans through top level folders** basic TFI file organization allowed
- **Real-time parameter tracking** for all 6 channels

## Credits

- **Original GENajam**: JAMATAR (2021)
- **GENMDM Module**: Little-scale
- **Velocity Curve**: impbox
- **Base Project**: Catskull Electronics

## License

Open source hardware and software project. Use and modify as needed for your musical creations.

---

*Built for musicians who want hands-on control of FM synthesis with modern reliability and features.*
