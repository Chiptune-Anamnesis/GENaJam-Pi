void tfisend(int opnarray[42], int sendchannel) {
  // Send all TFI data to appropriate CCs with periodic MIDI processing

  midi_send_cc(sendchannel, 14, opnarray[0] * 16);  // Algorithm
  midi_send_cc(sendchannel, 15, opnarray[1] * 16);  // Feedback

  midi_send_cc(sendchannel, 20, opnarray[2] * 8);   // OP1 Multiplier
  midi_send_cc(sendchannel, 21, opnarray[12] * 8);  // OP3 Multiplier
  midi_send_cc(sendchannel, 22, opnarray[22] * 8);  // OP2 Multiplier
  midi_send_cc(sendchannel, 23, opnarray[32] * 8);  // OP4 Multiplier

  midi_send_cc(sendchannel, 24, opnarray[3] * 32);   // OP1 Detune
  midi_send_cc(sendchannel, 25, opnarray[13] * 32);  // OP3 Detune
  midi_send_cc(sendchannel, 26, opnarray[23] * 32);  // OP2 Detune
  midi_send_cc(sendchannel, 27, opnarray[33] * 32);  // OP4 Detune
  delayMicroseconds(1500);
  handle_midi_input();  // Process MIDI after detune section

  midi_send_cc(sendchannel, 16, 127 - opnarray[4]);   // OP1 Total Level
  midi_send_cc(sendchannel, 17, 127 - opnarray[14]);  // OP3 Total Level
  midi_send_cc(sendchannel, 18, 127 - opnarray[24]);  // OP2 Total Level
  midi_send_cc(sendchannel, 19, 127 - opnarray[34]);  // OP4 Total Level

  midi_send_cc(sendchannel, 39, opnarray[5] * 32);   // OP1 Rate Scaling
  midi_send_cc(sendchannel, 40, opnarray[15] * 32);  // OP3 Rate Scaling
  midi_send_cc(sendchannel, 41, opnarray[25] * 32);  // OP2 Rate Scaling
  midi_send_cc(sendchannel, 42, opnarray[35] * 32);  // OP4 Rate Scaling
  handle_midi_input();                               // Process MIDI after rate scaling

  midi_send_cc(sendchannel, 43, opnarray[6] * 4);   // OP1 Attack Rate
  midi_send_cc(sendchannel, 44, opnarray[16] * 4);  // OP3 Attack Rate
  midi_send_cc(sendchannel, 45, opnarray[26] * 4);  // OP2 Attack Rate
  midi_send_cc(sendchannel, 46, opnarray[36] * 4);  // OP4 Attack Rate

  midi_send_cc(sendchannel, 47, opnarray[7] * 4);   // OP1 1st Decay Rate
  midi_send_cc(sendchannel, 48, opnarray[17] * 4);  // OP3 1st Decay Rate
  midi_send_cc(sendchannel, 49, opnarray[27] * 4);  // OP2 1st Decay Rate
  midi_send_cc(sendchannel, 50, opnarray[37] * 4);  // OP4 1st Decay Rate
  handle_midi_input();                              // Process MIDI after envelope rates

  midi_send_cc(sendchannel, 55, opnarray[10] * 8);  // OP1 2nd Total Level
  midi_send_cc(sendchannel, 56, opnarray[20] * 8);  // OP3 2nd Total Level
  midi_send_cc(sendchannel, 57, opnarray[30] * 8);  // OP2 2nd Total Level
  midi_send_cc(sendchannel, 58, opnarray[40] * 8);  // OP4 2nd Total Level

  midi_send_cc(sendchannel, 51, opnarray[8] * 8);   // OP1 2nd Decay Rate
  midi_send_cc(sendchannel, 52, opnarray[18] * 8);  // OP3 2nd Decay Rate
  midi_send_cc(sendchannel, 53, opnarray[28] * 8);  // OP2 2nd Decay Rate
  midi_send_cc(sendchannel, 54, opnarray[38] * 8);  // OP4 2nd Decay Rate
  handle_midi_input();                              // Process MIDI after sustain/decay

  midi_send_cc(sendchannel, 59, opnarray[9] * 8);   // OP1 Release Rate
  midi_send_cc(sendchannel, 60, opnarray[19] * 8);  // OP3 Release Rate
  midi_send_cc(sendchannel, 61, opnarray[29] * 8);  // OP2 Release Rate
  midi_send_cc(sendchannel, 62, opnarray[39] * 8);  // OP4 Release Rate

  midi_send_cc(sendchannel, 90, opnarray[11] * 8);  // OP1 SSG-EG
  midi_send_cc(sendchannel, 91, opnarray[21] * 8);  // OP3 SSG-EG
  midi_send_cc(sendchannel, 92, opnarray[31] * 8);  // OP2 SSG-EG
  midi_send_cc(sendchannel, 93, opnarray[41] * 8);  // OP4 SSG-EG
  handle_midi_input();                              // Process MIDI after SSG-EG

  midi_send_cc(sendchannel, 75, 90);   // FM Level
  midi_send_cc(sendchannel, 76, 90);   // AM Level
  midi_send_cc(sendchannel, 77, 127);  // Stereo (centered)

  midi_send_cc(sendchannel, 70, 0);  // OP1 Amplitude Modulation (off)
  midi_send_cc(sendchannel, 71, 0);  // OP3 Amplitude Modulation (off)
  midi_send_cc(sendchannel, 72, 0);  // OP2 Amplitude Modulation (off)
  midi_send_cc(sendchannel, 73, 0);  // OP4 Amplitude Modulation (off)

  // Store TFI settings in global array for editing (unchanged)
  fmsettings[sendchannel - 1][0] = opnarray[0] * 16;           // Algorithm
  fmsettings[sendchannel - 1][1] = opnarray[1] * 16;           // Feedback
  fmsettings[sendchannel - 1][2] = opnarray[2] * 8;            // OP1 Multiplier
  fmsettings[sendchannel - 1][12] = opnarray[12] * 8;          // OP3 Multiplier
  fmsettings[sendchannel - 1][22] = opnarray[22] * 8;          // OP2 Multiplier
  fmsettings[sendchannel - 1][32] = opnarray[32] * 8;          // OP4 Multiplier
  fmsettings[sendchannel - 1][3] = opnarray[3] * 32;           // OP1 Detune
  fmsettings[sendchannel - 1][13] = opnarray[13] * 32;         // OP3 Detune
  fmsettings[sendchannel - 1][23] = opnarray[23] * 32;         // OP2 Detune
  fmsettings[sendchannel - 1][33] = opnarray[33] * 32;         // OP4 Detune
  fmsettings[sendchannel - 1][4] = 127 - opnarray[4];          // OP1 Total Level
  fmsettings[sendchannel - 1][14] = 127 - opnarray[14];        // OP3 Total Level
  fmsettings[sendchannel - 1][24] = 127 - opnarray[24];        // OP2 Total Level
  fmsettings[sendchannel - 1][34] = 127 - opnarray[34];        // OP4 Total Level
  fmsettings[sendchannel - 1][5] = opnarray[5] * 32;           // OP1 Rate Scaling
  fmsettings[sendchannel - 1][15] = opnarray[15] * 32;         // OP3 Rate Scaling
  fmsettings[sendchannel - 1][25] = opnarray[25] * 32;         // OP2 Rate Scaling
  fmsettings[sendchannel - 1][35] = opnarray[35] * 32;         // OP4 Rate Scaling
  fmsettings[sendchannel - 1][6] = opnarray[6] * 4;            // OP1 Attack Rate
  fmsettings[sendchannel - 1][16] = opnarray[16] * 4;          // OP3 Attack Rate
  fmsettings[sendchannel - 1][26] = opnarray[26] * 4;          // OP2 Attack Rate
  fmsettings[sendchannel - 1][36] = opnarray[36] * 4;          // OP4 Attack Rate
  fmsettings[sendchannel - 1][7] = opnarray[7] * 4;            // OP1 1st Decay Rate
  fmsettings[sendchannel - 1][17] = opnarray[17] * 4;          // OP3 1st Decay Rate
  fmsettings[sendchannel - 1][27] = opnarray[27] * 4;          // OP2 1st Decay Rate
  fmsettings[sendchannel - 1][37] = opnarray[37] * 4;          // OP4 1st Decay Rate
  fmsettings[sendchannel - 1][10] = 127 - (opnarray[10] * 8);  // OP1 2nd Total Level
  fmsettings[sendchannel - 1][20] = 127 - (opnarray[20] * 8);  // OP3 2nd Total Level
  fmsettings[sendchannel - 1][30] = 127 - (opnarray[30] * 8);  // OP2 2nd Total Level
  fmsettings[sendchannel - 1][40] = 127 - (opnarray[40] * 8);  // OP4 2nd Total Level
  fmsettings[sendchannel - 1][8] = opnarray[8] * 8;            // OP1 2nd Decay Rate
  fmsettings[sendchannel - 1][18] = opnarray[18] * 8;          // OP3 2nd Decay Rate
  fmsettings[sendchannel - 1][28] = opnarray[28] * 8;          // OP2 2nd Decay Rate
  fmsettings[sendchannel - 1][38] = opnarray[38] * 8;          // OP4 2nd Decay Rate
  fmsettings[sendchannel - 1][9] = opnarray[9] * 8;            // OP1 Release Rate
  fmsettings[sendchannel - 1][19] = opnarray[19] * 8;          // OP3 Release Rate
  fmsettings[sendchannel - 1][29] = opnarray[29] * 8;          // OP2 Release Rate
  fmsettings[sendchannel - 1][39] = opnarray[39] * 8;          // OP4 Release Rate
  fmsettings[sendchannel - 1][11] = opnarray[11] * 8;          // OP1 SSG-EG
  fmsettings[sendchannel - 1][21] = opnarray[21] * 8;          // OP3 SSG-EG
  fmsettings[sendchannel - 1][31] = opnarray[31] * 8;          // OP2 SSG-EG
  fmsettings[sendchannel - 1][41] = opnarray[41] * 8;          // OP4 SSG-EG
  fmsettings[sendchannel - 1][42] = 90;                        // FM Level
  fmsettings[sendchannel - 1][43] = 90;                        // AM Level
  fmsettings[sendchannel - 1][44] = 127;                       // Stereo (centered)
  fmsettings[sendchannel - 1][45] = 0;                         // OP1 Amplitude Modulation
  fmsettings[sendchannel - 1][46] = 0;                         // OP3 Amplitude Modulation
  fmsettings[sendchannel - 1][47] = 0;                         // OP2 Amplitude Modulation
  fmsettings[sendchannel - 1][48] = 0;                         // OP4 Amplitude Modulation
  fmsettings[sendchannel - 1][49] = 0;                         // Patch is unedited
}

void fmccsend(uint8_t potnumber, uint8_t potvalue) {
  // This matches the original 1.10 code exactly
  // OP2 and OP3 are swapped to match original ordering

  switch (fmscreen) {

    // Algorithm, Feedback, Pan
    case 1:
      {
        // Handle pan mode display (same as original)
        oled_clear();
        if (polypan > 64) {  // stereo pan on
          oled_print(1, 16, "L R ON");
        } else {  // stereo pan off
          oled_print(1, 16, "L R OFF");
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][44] = 127;
          midi_send_cc(tfichannel, 77, 127);  // reset panning to center
        }

        if (potnumber == 0) { polypan = potvalue; }  // enter pan mode
        if (potnumber == 1) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][0] = potvalue;
          midi_send_cc(tfichannel, 14, potvalue);
        }  // Algorithm
        if (potnumber == 2) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][1] = potvalue;
          midi_send_cc(tfichannel, 15, potvalue);
        }  // Feedback
        if (potnumber == 3) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][44] = potvalue;
          midi_send_cc(tfichannel, 77, potvalue);
        }  // Pan
        break;
      }

    // Total Level (OP Volume) - THESE WORK CORRECTLY, NO SCALING
    case 2:
      {
        if (potnumber == 0) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][4] = potvalue;
          midi_send_cc(tfichannel, 16, potvalue);
        }  // OP1 Total Level
        if (potnumber == 1) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][24] = potvalue;
          midi_send_cc(tfichannel, 18, potvalue);
        }  // OP2 Total Level (note: original calls this OP3)
        if (potnumber == 2) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][14] = potvalue;
          midi_send_cc(tfichannel, 17, potvalue);
        }  // OP3 Total Level (note: original calls this OP2)
        if (potnumber == 3) {
          fmsettings[SAFE_CHANNEL_INDEX(tfichannel)][34] = potvalue;
          midi_send_cc(tfichannel, 19, potvalue);
        }  // OP4 Total Level
        break;
      }

    // Multiplier - THESE WORK CORRECTLY, NO SCALING
    case 3:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][2] = potvalue;
          midi_send_cc(tfichannel, 20, potvalue);
        }  // OP1 Multiplier
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][22] = potvalue;
          midi_send_cc(tfichannel, 22, potvalue);
        }  // OP2 Multiplier (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][12] = potvalue;
          midi_send_cc(tfichannel, 21, potvalue);
        }  // OP3 Multiplier (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][32] = potvalue;
          midi_send_cc(tfichannel, 23, potvalue);
        }  // OP4 Multiplier
        break;
      }

    // Detune - THESE WORK CORRECTLY, NO SCALING
    case 4:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][3] = potvalue;
          midi_send_cc(tfichannel, 24, potvalue);
        }  // OP1 Detune
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][23] = potvalue;
          midi_send_cc(tfichannel, 26, potvalue);
        }  // OP2 Detune (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][13] = potvalue;
          midi_send_cc(tfichannel, 25, potvalue);
        }  // OP3 Detune (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][33] = potvalue;
          midi_send_cc(tfichannel, 27, potvalue);
        }  // OP4 Detune
        break;
      }

    // Rate Scaling - THESE WORK CORRECTLY, NO SCALING
    case 5:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][5] = potvalue;
          midi_send_cc(tfichannel, 39, potvalue);
        }  // OP1 Rate Scaling
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][25] = potvalue;
          midi_send_cc(tfichannel, 41, potvalue);
        }  // OP2 Rate Scaling (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][15] = potvalue;
          midi_send_cc(tfichannel, 40, potvalue);
        }  // OP3 Rate Scaling (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][35] = potvalue;
          midi_send_cc(tfichannel, 42, potvalue);
        }  // OP4 Rate Scaling
        break;
      }

    // Attack Rate - THESE WORK CORRECTLY, NO SCALING
    case 6:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][6] = potvalue;
          midi_send_cc(tfichannel, 43, potvalue);
        }  // OP1 Attack Rate
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][26] = potvalue;
          midi_send_cc(tfichannel, 45, potvalue);
        }  // OP2 Attack Rate (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][16] = potvalue;
          midi_send_cc(tfichannel, 44, potvalue);
        }  // OP3 Attack Rate (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][36] = potvalue;
          midi_send_cc(tfichannel, 46, potvalue);
        }  // OP4 Attack Rate
        break;
      }

    // Decay Rate 1 - THESE WORK CORRECTLY, NO SCALING
    case 7:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][7] = potvalue;
          midi_send_cc(tfichannel, 47, potvalue);
        }  // OP1 1st Decay Rate
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][27] = potvalue;
          midi_send_cc(tfichannel, 49, potvalue);
        }  // OP2 1st Decay Rate (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][17] = potvalue;
          midi_send_cc(tfichannel, 48, potvalue);
        }  // OP3 1st Decay Rate (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][37] = potvalue;
          midi_send_cc(tfichannel, 50, potvalue);
        }  // OP4 1st Decay Rate
        break;
      }

    // Sustain (2nd Total Level) - INVERTED CORRECTLY
    case 8:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][10] = 127 - potvalue;
          midi_send_cc(tfichannel, 55, 127 - potvalue);
        }  // OP1 2nd Total Level
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][30] = 127 - potvalue;
          midi_send_cc(tfichannel, 57, 127 - potvalue);
        }  // OP2 2nd Total Level (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][20] = 127 - potvalue;
          midi_send_cc(tfichannel, 56, 127 - potvalue);
        }  // OP3 2nd Total Level (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][40] = 127 - potvalue;
          midi_send_cc(tfichannel, 58, 127 - potvalue);
        }  // OP4 2nd Total Level
        break;
      }

    // Decay Rate 2 - THESE WORK CORRECTLY, NO SCALING
    case 9:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][8] = potvalue;
          midi_send_cc(tfichannel, 51, potvalue);
        }  // OP1 2nd Decay Rate
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][28] = potvalue;
          midi_send_cc(tfichannel, 53, potvalue);
        }  // OP2 2nd Decay Rate (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][18] = potvalue;
          midi_send_cc(tfichannel, 52, potvalue);
        }  // OP3 2nd Decay Rate (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][38] = potvalue;
          midi_send_cc(tfichannel, 54, potvalue);
        }  // OP4 2nd Decay Rate
        break;
      }

    // Release Rate - THESE WORK CORRECTLY, NO SCALING
    case 10:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][9] = potvalue;
          midi_send_cc(tfichannel, 59, potvalue);
        }  // OP1 Release Rate
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][29] = potvalue;
          midi_send_cc(tfichannel, 61, potvalue);
        }  // OP2 Release Rate (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][19] = potvalue;
          midi_send_cc(tfichannel, 60, potvalue);
        }  // OP3 Release Rate (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][39] = potvalue;
          midi_send_cc(tfichannel, 62, potvalue);
        }  // OP4 Release Rate
        break;
      }

    // SSG-EG - THESE WORK CORRECTLY, NO SCALING
    case 11:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][11] = potvalue;
          midi_send_cc(tfichannel, 90, potvalue);
        }  // OP1 SSG-EG
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][31] = potvalue;
          midi_send_cc(tfichannel, 92, potvalue);
        }  // OP2 SSG-EG (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][21] = potvalue;
          midi_send_cc(tfichannel, 91, potvalue);
        }  // OP3 SSG-EG (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][41] = potvalue;
          midi_send_cc(tfichannel, 93, potvalue);
        }  // OP4 SSG-EG
        break;
      }

    // Amp Mod - THESE WORK CORRECTLY, NO SCALING
    case 12:
      {
        if (potnumber == 0) {
          fmsettings[tfichannel - 1][45] = potvalue;
          midi_send_cc(tfichannel, 70, potvalue);
        }  // OP1 Amplitude Modulation
        if (potnumber == 1) {
          fmsettings[tfichannel - 1][47] = potvalue;
          midi_send_cc(tfichannel, 72, potvalue);
        }  // OP2 Amplitude Modulation (original calls this OP3)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][46] = potvalue;
          midi_send_cc(tfichannel, 71, potvalue);
        }  // OP3 Amplitude Modulation (original calls this OP2)
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][48] = potvalue;
          midi_send_cc(tfichannel, 73, potvalue);
        }  // OP4 Amplitude Modulation
        break;
      }

    // LFO/FM/AM Level
    case 13:
      {
        // Blank out unused pot display (like original)
        oled_clear();
        oled_print(1, 16, "   ");  // blank out the first pot display

        if (potnumber == 1) {
          lfospeed = potvalue;
          midi_send_cc(1, 1, potvalue);
        }  // LFO Speed (GLOBAL)
        if (potnumber == 2) {
          fmsettings[tfichannel - 1][42] = potvalue;
          midi_send_cc(tfichannel, 75, potvalue);
        }  // FM Level
        if (potnumber == 3) {
          fmsettings[tfichannel - 1][43] = potvalue;
          midi_send_cc(tfichannel, 76, potvalue);
        }  // AM Level
        break;
      }

  }  // end switch
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

