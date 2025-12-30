# GENajam-Pi v1.41

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
- **Settings Persistence** - MIDI channel, region (NTSC/PAL), velocity curves, CC sync, poly-multi
- **Smart Voice Management** - True mono mode with last-note priority
- **Stuckless Mode Switching** - Automatic note-off when changing modes
- **Poly Multi-Timbral Mode** - Assign different TFI instruments per voice channel while maintaining polyphony

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
- **Live Display**: All FM parameter screens update in real-time when external CCs modify values
- **Toggleable**: CC Sync can be disabled in Settings menu if not needed
- **Rate Limited**: Display updates are rate-limited to 20Hz to prevent MIDI throughput issues

### FM Parameter CCs (Synced to Display)

These CCs update both GENMDM and the internal `fmsettings[]` array, enabling live display feedback on the corresponding FM Edit screens:

| CC | Parameter | Operator | Screen |
|----|-----------|----------|--------|
| 14 | Algorithm | - | 1 |
| 15 | Feedback | - | 1 |
| **Total Level** ||||
| 16 | Total Level | OP1 | 2 |
| 17 | Total Level | OP3 | 2 |
| 18 | Total Level | OP2 | 2 |
| 19 | Total Level | OP4 | 2 |
| **Multiplier** ||||
| 20 | Multiplier | OP1 | 3 |
| 21 | Multiplier | OP3 | 3 |
| 22 | Multiplier | OP2 | 3 |
| 23 | Multiplier | OP4 | 3 |
| **Detune** ||||
| 24 | Detune | OP1 | 4 |
| 25 | Detune | OP3 | 4 |
| 26 | Detune | OP2 | 4 |
| 27 | Detune | OP4 | 4 |
| **Rate Scaling** ||||
| 39 | Rate Scaling | OP1 | 5 |
| 40 | Rate Scaling | OP3 | 5 |
| 41 | Rate Scaling | OP2 | 5 |
| 42 | Rate Scaling | OP4 | 5 |
| **Attack Rate** ||||
| 43 | Attack Rate | OP1 | 6/7 |
| 44 | Attack Rate | OP3 | 6/7 |
| 45 | Attack Rate | OP2 | 6/7 |
| 46 | Attack Rate | OP4 | 6/7 |
| **Decay 1 Rate** ||||
| 47 | Decay 1 Rate | OP1 | 8 |
| 48 | Decay 1 Rate | OP3 | 8 |
| 49 | Decay 1 Rate | OP2 | 8 |
| 50 | Decay 1 Rate | OP4 | 8 |
| **Decay 2 Rate** ||||
| 51 | Decay 2 Rate | OP1 | 10 |
| 52 | Decay 2 Rate | OP3 | 10 |
| 53 | Decay 2 Rate | OP2 | 10 |
| 54 | Decay 2 Rate | OP4 | 10 |
| **Sustain Level** ||||
| 55 | Sustain Level | OP1 | 9 |
| 56 | Sustain Level | OP3 | 9 |
| 57 | Sustain Level | OP2 | 9 |
| 58 | Sustain Level | OP4 | 9 |
| **Release Rate** ||||
| 59 | Release Rate | OP1 | 11 |
| 60 | Release Rate | OP3 | 11 |
| 61 | Release Rate | OP2 | 11 |
| 62 | Release Rate | OP4 | 11 |
| **Amp Modulation** ||||
| 70 | Amp Modulation | OP1 | 13 |
| 71 | Amp Modulation | OP3 | 13 |
| 72 | Amp Modulation | OP2 | 13 |
| 73 | Amp Modulation | OP4 | 13 |
| **SSG-EG** ||||
| 90 | SSG-EG | OP1 | 12 |
| 91 | SSG-EG | OP3 | 12 |
| 92 | SSG-EG | OP2 | 12 |
| 93 | SSG-EG | OP4 | 12 |

### Special CC Handling

| CC | Function |
|----|----------|
| 1 | Modulation Wheel (≤5 = LFO off, >5 = LFO on) |
| 64 | Sustain Pedal |

### Pass-Through CCs (Not Synced to Display)

These CCs are forwarded to GENMDM but don't update internal display state (global LFO parameters):

| CC | Parameter |
|----|-----------|
| 74 | LFO Enable |
| 75 | LFO Speed |
| 76 | FM Depth |
| 77 | AM Depth |
| 83 | Region (75=NTSC, 1=PAL) |

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
1. Download the latest `genajam-pico-v1.41.uf2` file from releases
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
| 1/6 | MIDI Channel | 1-16 |
| 2/6 | Region | NTSC/USA (60Hz), PAL/EUR (50Hz) |
| 3/6 | Velocity Curve | Linear, Soft, Medium, Hard, ImpBox |
| 4/6 | CC Sync | ON (default), OFF |
| 5/6 | Poly Multi | OFF (default), ON - Per-channel TFI/editing in POLY mode |
| 6/6 | About | Version info |

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

### v1.40 Changes
- Added Poly Multi-Timbral mode (POLY-MT) - assign different TFI instruments to each of the 6 voice channels while maintaining polyphonic voice allocation
- New settings option (5/6): POLY-MT toggle (OFF by default, ON enables per-channel TFI/editing in POLY mode)
- POLY PRESET mode with POLY-MT enabled: UP/DOWN navigates channels (like MONO), LEFT/RIGHT browses TFI for current channel
- POLY FM EDIT mode with POLY-MT enabled: knob changes only affect the currently selected channel
- Display shows channel indicator (C1-C6) in POLY modes when POLY-MT is enabled
- Settings now persist poly_multi_timbral to EEPROM and SD card

### v1.39 Changes
- Added external MIDI CC sync for ALL FM parameters (Algorithm, Feedback, TL, Mult, Detune, RS, ADSR, SSG-EG, Amp Mod)
- Added Envelope Visualization screen (screen 6) with graphical ADSR display
- Added CC Sync setting (toggleable in Settings menu, default ON)
- Added BTN1 operator cycling on envelope visualization screen
- Optimized poly mode CC sync with deferred display updates (prevents note hanging)
- Rate-limited display updates to 20Hz for smooth CC automation without MIDI dropouts
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

