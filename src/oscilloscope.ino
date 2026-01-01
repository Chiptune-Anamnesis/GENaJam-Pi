// Oscilloscope Visualization - MIDI-Reactive Waveform Display
// Part of GenaJam Pico Project

// Waveform types
#define WAVE_SINE 0
#define WAVE_SQUARE 1
#define WAVE_SAW 2
#define WAVE_TRIANGLE 3
#define WAVE_NOISE 4

struct OscChannel {
    uint8_t waveform;        // Current waveform type
    float frequency;         // Wave frequency (affects how many cycles shown)
    float amplitude;         // Wave amplitude (0.0 - 1.0)
    float phase;             // Phase offset for animation
    uint8_t decay;           // Amplitude decay counter
    bool active;             // Currently triggered
};

// Oscilloscope state
#define OSC_CHANNELS 6       // Match FM channels
OscChannel osc_channels[OSC_CHANNELS];
bool oscilloscope_initialized = false;
unsigned long oscilloscope_last_update = 0;
float global_phase = 0;      // Global animation phase
uint8_t waveform_buffer[128]; // Pre-calculated waveform for display

const float OSC_SCREEN_WIDTH = 128.0;
const float OSC_SCREEN_HEIGHT = 32.0;
const float OSC_CENTER_Y = 16.0;  // Vertical center

void initOscilloscopeDemo() {
    // Initialize all channels
    for (int i = 0; i < OSC_CHANNELS; i++) {
        osc_channels[i].waveform = i % 5;  // Distribute waveform types
        osc_channels[i].frequency = 1.0 + (i * 0.5);  // Slightly different frequencies
        osc_channels[i].amplitude = 0.0;
        osc_channels[i].phase = i * 0.5;  // Phase offset per channel
        osc_channels[i].decay = 0;
        osc_channels[i].active = false;
    }

    global_phase = 0;
    oscilloscope_initialized = true;
    oscilloscope_last_update = millis();
}

// Calculate waveform value at position (0.0 to 1.0 returns -1.0 to 1.0)
float getWaveValue(uint8_t waveform, float pos) {
    // Wrap position to 0-1 range
    pos = pos - floor(pos);

    switch (waveform) {
        case WAVE_SINE:
            return sin(pos * 2.0 * PI);

        case WAVE_SQUARE:
            return (pos < 0.5) ? 1.0 : -1.0;

        case WAVE_SAW:
            return 2.0 * pos - 1.0;

        case WAVE_TRIANGLE:
            if (pos < 0.25) return pos * 4.0;
            else if (pos < 0.75) return 2.0 - pos * 4.0;
            else return pos * 4.0 - 4.0;

        case WAVE_NOISE:
            return (random(-100, 101) / 100.0);

        default:
            return 0;
    }
}

void updateOscilloscopePhysics() {
    unsigned long now = millis();

    // Rate limit updates (~40fps)
    if ((now - oscilloscope_last_update) < 25) return;
    oscilloscope_last_update = now;

    // Advance global phase for animation
    global_phase += 0.08;
    if (global_phase > 1000.0) global_phase = 0;

    // Update each channel
    for (int i = 0; i < OSC_CHANNELS; i++) {
        if (osc_channels[i].active) {
            // Decay amplitude over time
            if (osc_channels[i].decay > 0) {
                osc_channels[i].decay--;
                osc_channels[i].amplitude = osc_channels[i].decay / 80.0;
                if (osc_channels[i].amplitude > 1.0) osc_channels[i].amplitude = 1.0;
            } else {
                osc_channels[i].amplitude *= 0.95;  // Smooth decay
                if (osc_channels[i].amplitude < 0.02) {
                    osc_channels[i].amplitude = 0;
                    osc_channels[i].active = false;
                }
            }
        }
    }
}

void handleMidiNoteForOscilloscope(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (channel >= OSC_CHANNELS) return;

    // Trigger the channel
    osc_channels[channel].active = true;
    osc_channels[channel].amplitude = velocity / 127.0;
    osc_channels[channel].decay = 60 + (velocity / 3);  // Longer decay for louder notes

    // Map note to frequency (higher notes = more cycles visible)
    osc_channels[channel].frequency = 0.5 + (note / 40.0);

    // Vary waveform based on note range
    if (note < 40) {
        osc_channels[channel].waveform = WAVE_SINE;  // Bass = smooth sine
    } else if (note < 60) {
        osc_channels[channel].waveform = WAVE_TRIANGLE;  // Mid = triangle
    } else if (note < 80) {
        osc_channels[channel].waveform = WAVE_SAW;  // Upper mid = saw
    } else if (note < 100) {
        osc_channels[channel].waveform = WAVE_SQUARE;  // High = square
    } else {
        osc_channels[channel].waveform = WAVE_NOISE;  // Very high = noise
    }
}

void drawOscilloscope() {
    // Draw center line (zero crossing reference)
    for (int x = 0; x < 128; x += 4) {
        display.drawPixel(x, OSC_CENTER_Y, SSD1306_WHITE);
    }

    // Draw grid lines
    for (int x = 0; x < 128; x += 32) {
        for (int y = 4; y < 28; y += 4) {
            display.drawPixel(x, y, SSD1306_WHITE);
        }
    }

    // Calculate combined waveform from all active channels
    float prev_y = OSC_CENTER_Y;
    bool first_point = true;

    for (int x = 0; x < 128; x++) {
        float combined = 0;
        float total_amp = 0;

        // Sum all active channel waveforms
        for (int ch = 0; ch < OSC_CHANNELS; ch++) {
            if (osc_channels[ch].amplitude > 0.01) {
                float pos = (x / OSC_SCREEN_WIDTH) * osc_channels[ch].frequency +
                           global_phase + osc_channels[ch].phase;
                float wave = getWaveValue(osc_channels[ch].waveform, pos);
                combined += wave * osc_channels[ch].amplitude;
                total_amp += osc_channels[ch].amplitude;
            }
        }

        // Normalize if multiple channels active
        if (total_amp > 1.0) {
            combined /= total_amp;
        }

        // If no channels active, show a quiet baseline with subtle noise
        if (total_amp < 0.01) {
            combined = (random(-5, 6) / 100.0);  // Subtle noise floor
        }

        // Scale to screen coordinates
        float y = OSC_CENTER_Y - (combined * 14.0);  // 14 pixels amplitude (28 total height)

        // Clamp to screen
        if (y < 1) y = 1;
        if (y > 30) y = 30;

        // Draw connected line for smooth waveform
        if (!first_point) {
            display.drawLine(x - 1, (int)prev_y, x, (int)y, SSD1306_WHITE);
        }
        prev_y = y;
        first_point = false;
    }
}

void oscilloscopeDemo() {
    if (!oscilloscope_initialized) {
        initOscilloscopeDemo();
    }

    // Update physics
    updateOscilloscopePhysics();

    // Render
    display.clearDisplay();
    drawOscilloscope();
    display.display();
}
