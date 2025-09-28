# GENajam-Pi v1.36

A Raspberry Pi Pico (RP2040) port of the GENajam MIDI controller for Little-scale's GENMDM module. This modernized version features enhanced file browsing, real-time MIDI visualization, and improved responsiveness.

## Overview

GENajam-Pi transforms your Raspberry Pi Pico into a powerful MIDI controller for 6-channel FM synthesis via the GENMDM module. It provides intuitive control over TFI (Texas Instruments Format) instruments with real-time parameter editing and advanced file management.

## Key Features

### Core Functionality
- **6-channel FM synthesis control** via GENMDM module
- **Polyphonic and monophonic modes** with intelligent voice allocation
- **Real-time parameter editing** with 4 potentiometers
- **TFI file management** (load, save, delete) with SD card support
- **OLED display** (128x32) for visual feedback
- **MIDI panic button** for emergency note-off

### Enhanced Features
- **Dual-Core Processing** - Core 0 handles MIDI, Core 1 handles visualization
- **Advanced File Organization** - Automatic user/library file separation
- **Real-time MIDI Visualization** - Multiple visualization modes with bar graphs, animations
- **TFI Preview Mode** - Auto-load files while browsing with manual sound preview
- **Accelerated Navigation** - Hold buttons for fast file scrolling
- **USB + TRS MIDI** - Both USB MIDI IN and TRS 3.5mm MIDI IN/OUT
- **Savestate System** - Save and load complete 6-channel configurations
- **Settings Persistence** - MIDI channel, region (NTSC/PAL), velocity curves
- **Smart Voice Management** - True mono mode with last-note priority
- **Stuckless Mode Switching** - Automatic note-off when changing modes

## Operation Modes

### Mode Structure
- **MONO/POLY Toggle**: Press POLY button to switch between monophonic and polyphonic operation
- **Mode Cycling**: Press PRESET button to cycle through available modes

### MONO Mode Cycle
**PRESET Button:** VIZ → PRESETS → FM EDIT → BANK MGR → SETTINGS → VIZ...

#### Visualizer Mode
- **4 Visualization Types** (UP/DOWN to cycle):
  - Bar Graph - Classic frequency bars
  - Asteroids - Space debris field animation
  - Starfighter - Space flight simulation
  - Neural Net - Hexagonal network visualization
- Real-time MIDI activity display for channels 1-11

#### Presets Mode
- Browse and load TFI files per channel (1-6)
- **UP/DOWN**: Select FM channel (1-6)
- **LEFT/RIGHT**: Browse TFI files
- **OPT1**: Load selected TFI to current channel
- **OPT2**: Toggle between ALL and PREVIEW modes

#### FM Edit Mode
- Real-time FM parameter editing for current channel
- **UP/DOWN**: Select channel (1-6)
- **LEFT/RIGHT**: Navigate through 13 parameter screens
- **4 Potentiometers**: Adjust parameters in real-time
- **OPT2**: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel)

#### Bank Manager Mode
- Save/load complete 6-channel TFI configurations
- **LEFT/RIGHT**: Browse saved preset banks
- **OPT1**: Load selected preset bank
- **OPT2**: Save current configuration as new preset
- **DELETE**: Delete selected preset

#### Settings Mode
- **LEFT/RIGHT**: Navigate settings (MIDI Channel, Region, Velocity Curve)
- **UP/DOWN** or **Potentiometers**: Adjust values
- **OPT2**: Save changes to EEPROM and SD card
- **PRESET**: Exit without saving

### POLY Mode Cycle
**PRESET Button:** VIZ → PRESETS → FM EDIT → VIZ...
*(Bank Manager and Settings disabled in POLY mode)*

#### Visualizer Mode
- Same 4 visualization types as MONO mode
- Shows polyphonic activity across all 6 channels

#### Presets Mode
- Browse TFI files for all 6 channels simultaneously
- **LEFT/RIGHT**: Browse TFI files
- **OPT1**: Load selected TFI to all 6 channels
- **OPT2**: Toggle between ALL and PREVIEW modes
- **UP**: Save prompt for current TFI
- **DELETE**: Delete TFI file

#### FM Edit Mode
- Real-time editing affects all 6 channels simultaneously
- **LEFT/RIGHT**: Navigate parameter screens
- **4 Potentiometers**: Adjust parameters for all channels
- **OPT2**: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel)

## Browse Modes

### ALL Mode
- Shows both library and user files combined
- User files appear first in the list
- Complete file browsing with folder navigation

### PREVIEW Mode
- **Auto-Loading**: TFI files automatically load when browsing with LEFT/RIGHT
- **Manual Preview**: Press and hold OPT1 to play a preview note (Middle C)
- **Usage**: Toggle with OPT2 button: ALL → PREVIEW → ALL
- **Display**: Shows "PRVW" mode and "OPT1:Play OPT2:Exit"

## File Management

### TFI File Organization
- **User Files**: Stored in `/tfi/user/` - your saved instruments
- **Library Files**: Stored in `/tfi/` and subdirectories - pre-made instruments
- **Combined Browsing**: ALL mode shows both types (user files first)
- **Smart Limits**: 150 user files max, 400 library files max

### File Operations
- **Save New**: Automatically numbered in `/tfi/user/` (newpatch001.tfi, etc.)
- **Overwrite**: Update existing files in place
- **Delete**: Remove files with confirmation prompt
- **Preview**: Test instruments before loading

### SD Card Structure
```
/
├── tfi/                    ← Library TFI files and folders
│   ├── bass/
│   ├── lead/
│   ├── drums/
│   └── user/               ← User-saved TFI files
├── presets/                ← Bank preset files (.mdm_pre)
└── settings/               ← Settings backup (.ini)
```

## MIDI Features

### Voice Management
- **6-voice polyphony** with intelligent voice stealing
- **Sustain pedal support** (CC#64)
- **Modulation wheel control** (CC#1) - Binary LFO on/off (threshold at value 5)
- **Pitch bend** - Affects all voices in poly mode, individual voices in mono
- **Velocity curves** - Hard/Medium/Soft/Original response

### MIDI I/O
- **USB MIDI**: Input only
- **TRS MIDI**: 3.5mm Type A - Input and Output
- **MIDI Panic**: Emergency all-notes-off functionality

## Hardware Requirements

### Essential Components
- Raspberry Pi Pico (RP2040)
- 128x32 OLED display (SSD1306, I2C)
- MicroSD card module (SPI)
- 4 Potentiometers via analog multiplexer
- 8 Digital buttons with pullup resistors

### Controls
- **PRESET/Edit Button**: Mode cycling
- **MONO/POLY Button**: Mode switching
- **LEFT/RIGHT Buttons**: Navigation
- **UP/DOWN Buttons**: Channel/option selection
- **OPT1/OPT2 Buttons**: Special functions
- **DELETE Button**: File deletion
- **PANIC Button**: Emergency stop

## Installation

### Quick Setup (Recommended)
1. Download the latest `genajam-pico-v1.36.uf2` file from releases
2. Hold the BOOTSEL button on your Pico and connect to USB
3. Drag and drop the UF2 file onto the RPI-RP2 drive
4. The Pico will automatically reboot and start GenaJam

### Configuration
1. **Boot Setup**: Hold PRESET on startup to configure MIDI channel and region
2. **SD Card**: Load TFI files onto SD card (folders will be created automatically)
3. **GENMDM**: Connect via MIDI OUT
4. **Ready**: Power on and start making music!

## FM Parameter Editing

### 13 Parameter Screens
1. **Algorithm, Feedback, Pan** - Core FM structure and stereo positioning
2. **OP Volume** - Individual operator volume levels (Total Level)
3. **Frequency Multiple** - Harmonic ratios for each operator
4. **Detune** - Fine frequency adjustments per operator
5. **Rate Scaling** - Envelope scaling with pitch
6. **Attack** - Envelope attack rates
7. **Decay 1** - Initial decay rates
8. **Sustain** - Sustain levels (2nd Total Level)
9. **Decay 2** - Secondary decay rates
10. **Release** - Release rates
11. **SSG-EG** - Special envelope generator modes
12. **Amp Modulation** - Amplitude modulation on/off per operator
13. **LFO/FM/AM** - Global modulation controls and levels

## Technical Specifications

### Performance
- **Dual-core processing** for smooth operation
- **Real-time MIDI processing** with minimal latency
- **Efficient file handling** supports hundreds of TFI files
- **Smart button acceleration** for fast navigation

### File Limits
- **User TFI Files**: 150 maximum
- **Library TFI Files**: 400 maximum
- **File Paths**: 96 characters maximum
- **Display Names**: 48 characters maximum

## Version History

### v1.36 Changes
- Simplified browse modes to ALL and PREVIEW only
- Fixed TFI save/load consistency issues
- Improved pitch bend handling
- Enhanced mode switching with automatic TFI application
- Removed USER and LIBRARY browse modes for simplicity

## Credits

- **Original GENajam**: [JAMATAR](https://github.com/jamatarmusic/GENajam) (2021)
- **GENMDM Module**: Little-scale
- **Velocity Curve**: impbox
- **Pico Port**: Crunchypotato (2025)

## License

Open source hardware and software project. Use and modify as needed for your musical creations.

---

*Built for musicians who want hands-on control of FM synthesis with modern reliability and features.*