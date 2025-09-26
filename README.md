# GENajam-Pi v1.33

A Raspberry Pi Pico (RP2040) port of the GENajam MIDI controller for Little-scale's GENMDM module. This is a modernized version of the original Arduino GENajam by JAMATAR, featuring enhanced file browsing, real-time MIDI display, and improved responsiveness.

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
- **User/Library File Organization** - User files stored in `/tfi/user/`, library files in `/tfi/`
- **Browse Mode Toggle** - Switch between ALL, USER, or LIBRARY files with OPT2
- **Folder-Aware Browsing** - Display current folder name during library file browsing
- **Alphabetical Folder Sorting** - Library files sorted by folder name, then filename
- **Faster Startup** - Reduced delays for quicker boot to visualizer mode

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

### MONO Mode Cycle
**PRESET Button cycles through:** VIZ → PRESETS → FM EDIT → BANK MGR → SETTINGS → VIZ...

#### Visualizer Mode
- **4 Sub-modes** (UP/DOWN to cycle): Bar Graph, Asteroids, Starfighter, Neural Net
- Real-time MIDI visualization for channels 1-11
- Continuous animation during music playback

#### Presets Mode
- Browse and load TFI files per channel (1-6) with folder-aware display
- **UP/DOWN**: Select FM channel
- **LEFT/RIGHT**: Browse TFI files (shows folder name for library files)
- **OPT1**: Load selected TFI to current channel
- **OPT2**: Toggle browse mode (ALL/USER/LIBRARY)

#### FM Edit Mode
- Real-time FM parameter editing for current channel
- **UP/DOWN**: Select channel (1-6)
- **LEFT/RIGHT**: Navigate through 13 parameter screens
- **4 Potentiometers**: Adjust parameters in real-time
- **OPT2**: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel)

#### Bank Manager Mode
- Save/load complete 6-channel TFI configurations
- **LEFT/RIGHT**: Browse saved preset banks
- **OPT2**: Save current configuration as new preset
- **OPT1**: Load selected preset bank
- **DELETE**: Delete selected preset

#### Settings Mode
- **LEFT/RIGHT**: Navigate settings (MIDI Channel, Region, Velocity Curve)
- **UP/DOWN** or **Pots**: Adjust values
- **OPT2**: Save changes to EEPROM and SD card
- **PRESET**: Exit without saving

### POLY Mode Cycle
**PRESET Button cycles through:** VIZ → PRESETS → FM EDIT → VIZ...
*(BANK MGR and SETTINGS disabled in POLY mode)*

#### Visualizer Mode
- Same 4 sub-modes as MONO, shows polyphonic activity

#### Presets Mode
- Browse TFI files for all 6 channels simultaneously with folder-aware display
- **LEFT/RIGHT**: Browse TFI files (shows folder name for library files)
- **OPT1**: Load selected TFI to all 6 channels
- **OPT2**: Toggle browse mode (ALL/USER/LIBRARY)
- **UP**: Save prompt for current TFI
- **DELETE**: Delete TFI file

#### FM Edit Mode
- Real-time editing affects all 6 channels simultaneously
- **LEFT/RIGHT**: Navigate parameter screens
- **4 Potentiometers**: Adjust parameters for all channels
- **OPT2**: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel) 

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
  
  ├─ OPT1: Load selected TFI
  └─ OPT1: Toggle browse mode (ALL/USER/LIBRARY)
  

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
- **Separated file organization** - Library files in `/tfi/`, user files in `/tfi/user/`
- **Browse mode toggle** - Switch between ALL, USER, or LIBRARY files with OPT2
- **Folder-aware browsing** - Shows current folder name during library file navigation
- **Alphabetical organization** - Library files sorted by folder, then filename
- **Smart file limits** - 150 user files, 400 library files (550 total)
- **Priority file indexing** - User files appear first in ALL mode
- **Recursive directory scanning** - finds files in any folder depth
- **Fast file browsing** with acceleration (hold buttons for turbo speed)
- **Save new patches** - automatically numbered in `/tfi/user/` (newpatch001.tfi, etc.)
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

## System Limits

### File Capacity
- **User TFI files**: 150 maximum (stored in `/tfi/user/`)
- **Library TFI files**: 400 maximum (stored in `/tfi/` and subdirectories)
- **Total TFI files**: 550 maximum
- **Preset banks**: No specific limit (stored in `/presets/`)

### File Path Limits
- **File paths**: 96 characters maximum (including folder structure)
- **Display names**: 48 characters maximum (filename without `.tfi`)
- **Folder names**: 16 characters maximum for display

*Files exceeding these limits will be automatically skipped during scanning.*

## Getting Started

### Quick Installation (Recommended)
1. **Download** the latest `genajam-pico-v1.33.uf2` file from releases
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
- **OPT1**: Toggle browse mode (ALL/USER/LIBRARY)
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

- **Original GENajam**: [JAMATAR](https://github.com/jamatarmusic/GENajam) (2021)
- **GENMDM Module**: Little-scale
- **Velocity Curve**: impbox
- **Base Project**: Catskull Electronics

## License

Open source hardware and software project. Use and modify as needed for your musical creations.

---

*Built for musicians who want hands-on control of FM synthesis with modern reliability and features.*
