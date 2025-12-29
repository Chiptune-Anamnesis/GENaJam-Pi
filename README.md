# GENajam-Pi v1.39

Get one here! https://hobbychop.com

A Raspberry Pi Pico (RP2040) port of the GENajam MIDI controller for Little-scale's GENMDM module. This modernized version features enhanced file browsing, real-time MIDI visualization, external CC control, and improved responsiveness.

## Overview

GENajam-Pi transforms your Raspberry Pi Pico into a powerful MIDI controller for 6-channel FM synthesis via the GENMDM module. It provides intuitive control over TFI (Texas Instruments Format) instruments with real-time parameter editing, external MIDI CC synchronization, and advanced file management.

## Key Features

### Core Functionality
- **6-channel FM synthesis control** via GENMDM module
- **Polyphonic and monophonic modes** with intelligent voice allocation
- **Real-time parameter editing** with 4 potentiometers
- **External MIDI CC control** for ADSR parameters with live display sync
- **TFI file management** (load, save, delete) with SD card support
- **OLED display** (128x32) for visual feedback
- **MIDI panic button** for emergency note-off

### Enhanced Features
- **Dual-Core Processing** - Core 0 handles MIDI, Core 1 handles visualization
- **External CC Sync** - Control ADSR parameters from external MIDI controllers
- **Envelope Visualization** - Real-time graphical envelope display with operator cycling
- **Advanced File Organization** - Automatic user/library file separation
- **Real-time MIDI Visualization** - Multiple visualization modes with bar graphs, animations
- **TFI Preview Mode** - Auto-load files while browsing with manual sound preview
- **Accelerated Navigation** - Hold buttons for fast file scrolling
- **USB + TRS MIDI** - Both USB MIDI IN and TRS 3.5mm MIDI IN/OUT
- **Savestate System** - Save and load complete 6-channel configurations
- **Settings Persistence** - MIDI channel, region (NTSC/PAL), velocity curves, CC sync
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
- **BTN1**: Load selected TFI to current channel
- **BTN2**: Toggle between ALL and PREVIEW modes

#### FM Edit Mode
- Real-time FM parameter editing for current channel
- **UP/DOWN**: Select channel (1-6)
- **LEFT/RIGHT**: Navigate through 14 parameter screens
- **4 Potentiometers**: Adjust parameters in real-time
- **BTN1**: Cycle operators on Envelope Viz screen (screen 6)
- **BTN2**: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel)

#### Bank Manager Mode
- Save/load complete 6-channel TFI configurations
- **LEFT/RIGHT**: Browse saved preset banks
- **BTN1**: Load selected preset bank
- **BTN2**: Save current configuration as new preset
- **DELETE**: Delete selected preset

#### Settings Mode
- **LEFT/RIGHT**: Navigate settings (MIDI Channel, Region, Velocity Curve, CC Sync)
- **UP/DOWN**: Adjust values
- **BTN2**: Save changes to EEPROM and SD card
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
- **BTN1**: Load selected TFI to all 6 channels
- **BTN2**: Toggle between ALL and PREVIEW modes
- **UP**: Save prompt for current TFI
- **DELETE**: Delete TFI file

#### FM Edit Mode
- Real-time editing affects all 6 channels simultaneously
- **LEFT/RIGHT**: Navigate parameter screens
- **4 Potentiometers**: Adjust parameters for all channels
- **BTN1**: Cycle operators on Envelope Viz screen (screen 6)
- **BTN2**: Save prompt (UP=overwrite, DOWN=save new, PRESET=cancel)

## Browse Modes

### ALL Mode
- Shows both library and user files combined
- User files appear first in the list
- Complete file browsing with folder navigation

### PREVIEW Mode
- **Auto-Loading**: TFI files automatically load when browsing with LEFT/RIGHT
- **Manual Preview**: Press and hold BTN1 to play a preview note (Middle C)
- **Usage**: Toggle with BTN2 button: ALL → PREVIEW → ALL
- **Display**: Shows "PRVW" mode and "BTN1:Play BTN2:Exit"

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

## External CC Control

GENajam-Pi supports external MIDI CC control for real-time parameter manipulation from hardware controllers, DAWs, or other MIDI sources. When CC Sync is enabled (default), incoming CCs update both the GENMDM and the internal display state.

### CC Sync Behavior
- **POLY Mode**: CCs received on the configured MIDI channel sync to all 6 FM channels
- **MONO Mode**: CCs received on channels 1-6 sync to the corresponding FM channel
- **Live Display**: Envelope visualization updates in real-time when external CCs modify ADSR
- **Toggleable**: CC Sync can be disabled in Settings menu if not needed

### ADSR Parameter CCs (Synced to Display)

These CCs update both GENMDM and the internal `fmsettings[]` array, enabling live display feedback:

| CC | Parameter | Operator |
|----|-----------|----------|
| **Attack Rate** |||
| 43 | Attack Rate | OP1 |
| 44 | Attack Rate | OP3 |
| 45 | Attack Rate | OP2 |
| 46 | Attack Rate | OP4 |
| **Decay 1 Rate** |||
| 47 | Decay 1 Rate | OP1 |
| 48 | Decay 1 Rate | OP3 |
| 49 | Decay 1 Rate | OP2 |
| 50 | Decay 1 Rate | OP4 |
| **Decay 2 Rate** |||
| 51 | Decay 2 Rate | OP1 |
| 52 | Decay 2 Rate | OP3 |
| 53 | Decay 2 Rate | OP2 |
| 54 | Decay 2 Rate | OP4 |
| **Sustain Level** |||
| 55 | Sustain Level | OP1 |
| 56 | Sustain Level | OP3 |
| 57 | Sustain Level | OP2 |
| 58 | Sustain Level | OP4 |
| **Release Rate** |||
| 59 | Release Rate | OP1 |
| 60 | Release Rate | OP3 |
| 61 | Release Rate | OP2 |
| 62 | Release Rate | OP4 |

### Special CC Handling

| CC | Function |
|----|----------|
| 1 | Modulation Wheel (≤5 = LFO off, >5 = LFO on) |
| 64 | Sustain Pedal |

### Pass-Through CCs (Not Synced to Display)

These CCs are forwarded to GENMDM but don't update internal display state:

| CC | Parameter |
|----|-----------|
| 14 | Algorithm |
| 15 | Feedback |
| 16-19 | Total Level (OP1, OP3, OP2, OP4) |
| 20-23 | Multiplier (OP1, OP3, OP2, OP4) |
| 24-27 | Detune (OP1, OP3, OP2, OP4) |
| 39-42 | Rate Scaling (OP1, OP3, OP2, OP4) |
| 70-73 | Amplitude Modulation (OP1, OP3, OP2, OP4) |
| 74 | LFO Enable |
| 75 | LFO Speed |
| 76 | FM Depth |
| 77 | AM Depth |
| 83 | Region (75=NTSC, 1=PAL) |
| 90-93 | SSG-EG (OP1, OP3, OP2, OP4) |

**Note:** The operator order in GenMDM CC numbering is OP1, OP3, OP2, OP4 (not sequential) due to YM2612 register layout.

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
- **BTN1/BTN2 Buttons**: Special functions (load, save, operator cycling)
- **DELETE Button**: File deletion
- **PANIC Button**: Emergency stop

## Installation

### Quick Setup (Recommended)
1. Download the latest `genajam-pico-v1.39.uf2` file from releases
2. Hold the BOOTSEL button on your Pico and connect to USB
3. Drag and drop the UF2 file onto the RPI-RP2 drive
4. The Pico will automatically reboot and start GenaJam

### Configuration
1. **SD Card**: Load TFI files onto SD card (folders will be created automatically)
2. **GENMDM**: Connect via MIDI OUT

## FM Parameter Editing

### 14 Parameter Screens
1. **Algorithm, Feedback, Pan** - Core FM structure and stereo positioning
2. **OP Volume** - Individual operator volume levels (Total Level)
3. **Frequency Multiple** - Harmonic ratios for each operator
4. **Detune** - Fine frequency adjustments per operator
5. **Rate Scaling** - Envelope scaling with pitch
6. **Envelope Visualization** - Graphical ADSR display with BTN1 operator cycling
7. **Attack** - Envelope attack rates (all 4 operators)
8. **Decay 1** - Initial decay rates (all 4 operators)
9. **Sustain** - Sustain levels / 2nd Total Level (all 4 operators)
10. **Decay 2** - Secondary decay rates (all 4 operators)
11. **Release** - Release rates (all 4 operators)
12. **SSG-EG** - Special envelope generator modes
13. **Amp Modulation** - Amplitude modulation on/off per operator
14. **LFO/FM/AM** - Global modulation controls and levels

### Envelope Visualization Screen (Screen 6)
- **Graphical Display**: Real-time envelope shape visualization
- **Operator Selection**: Press BTN1 to cycle through OP1-OP4
- **4 Potentiometers**: Control A/D/S/R for the selected operator
- **Live Updates**: Display updates when external CCs modify parameters
- **Header**: Shows current operator and channel (e.g., "OP1 ENVELOPE CH1")

## Settings Menu

Navigate with LEFT/RIGHT, adjust with UP/DOWN, save with BTN2.

| Screen | Setting | Options |
|--------|---------|---------|
| 1/5 | MIDI Channel | 1-16 |
| 2/5 | Region | NTSC/USA (60Hz), PAL/EUR (50Hz) |
| 3/5 | Velocity Curve | Linear, Soft, Medium, Hard, ImpBox |
| 4/5 | CC Sync | ON (default), OFF |
| 5/5 | About | Version info |

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

### v1.39 Changes
- Added external MIDI CC control for ADSR parameters with live display sync
- Added Envelope Visualization screen (screen 6) with graphical ADSR display
- Added CC Sync setting (toggleable in Settings menu, default ON)
- Added BTN1 operator cycling on envelope visualization screen
- Renamed OPT1/OPT2 button labels to BTN1/BTN2 to match newest PCB silkscreen
- Fixed TFI Edit changes being wiped out by changing to TFI select mode
- Renumbered FM parameter screens (6-13 → 7-14) to accommodate envelope viz

## Credits

- **Original GENajam**: [JAMATAR](https://github.com/jamatarmusic/GENajam) (2021)
- **GENMDM Module**: Little-scale
- **Velocity Curve**: impbox
- **Pico Port**: Crunchypotato (2025)

## License

Open source hardware and software project. Use and modify as needed for your musical creations.

---


*Built for musicians who want hands-on control of FM synthesis with modern reliability and features.*
