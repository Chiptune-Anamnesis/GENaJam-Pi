// Matrix Rain Visualization - MIDI-Reactive Digital Rain
// Part of GenaJam Pico Project

struct RainDrop {
    float y;              // Current Y position
    float speed;          // Fall speed (pixels per frame)
    uint8_t length;       // Trail length (3-8 characters)
    uint8_t char_offset;  // Character animation offset
    bool active;          // Currently falling
    unsigned long spawn_time;
    uint8_t brightness;   // For MIDI-triggered brightness boost
};

// Matrix Rain state
#define RAIN_COLUMNS 21      // 128 pixels / 6 pixels per char = ~21 columns
#define MAX_TRAIL_LENGTH 6   // Maximum trail length for 32px height
RainDrop rain_drops[RAIN_COLUMNS];
bool matrixrain_initialized = false;
unsigned long matrixrain_last_update = 0;
const float RAIN_SCREEN_WIDTH = 128.0;
const float RAIN_SCREEN_HEIGHT = 32.0;

// Character set for matrix effect (simplified for OLED)
// Using numbers, letters, and symbols that look good at 5x7 font
const char matrix_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@#$%&*<>[]{}";
const uint8_t matrix_char_count = sizeof(matrix_chars) - 1;

void initMatrixRainDemo() {
    // Initialize all columns with staggered starting positions for full coverage
    for (int i = 0; i < RAIN_COLUMNS; i++) {
        rain_drops[i].active = true;  // Start all columns active
        rain_drops[i].y = random(-40, 32);  // Stagger across screen height
        rain_drops[i].speed = 0.3 + (random(0, 100) / 120.0);  // Varied speeds
        rain_drops[i].length = 3 + random(0, MAX_TRAIL_LENGTH - 2);
        // Spread char_offsets widely so adjacent columns look different
        rain_drops[i].char_offset = (i * 7 + random(0, 5)) % matrix_char_count;
        rain_drops[i].spawn_time = millis();
        rain_drops[i].brightness = 0;
    }

    matrixrain_initialized = true;
    matrixrain_last_update = millis();
}

void spawnRainDrop(int column) {
    if (column < 0 || column >= RAIN_COLUMNS) return;

    rain_drops[column].active = true;
    rain_drops[column].y = -6;  // Start just above screen
    rain_drops[column].speed = 0.3 + (random(0, 100) / 200.0);  // 0.3 to 0.8 pixels per update
    rain_drops[column].length = 3 + random(0, MAX_TRAIL_LENGTH - 2);  // 3 to MAX_TRAIL_LENGTH
    rain_drops[column].char_offset = random(0, matrix_char_count);
    rain_drops[column].spawn_time = millis();
    rain_drops[column].brightness = 0;
}

void boostRandomColumn(uint8_t velocity) {
    // Pick a random column to boost
    int column = random(0, RAIN_COLUMNS);

    // Boost this drop - reset to top with faster speed
    rain_drops[column].y = -6;  // Reset to top
    rain_drops[column].speed = 0.8 + (velocity / 150.0);  // Faster when MIDI triggered
    rain_drops[column].length = 4 + (velocity / 40);  // Velocity affects length
    if (rain_drops[column].length > MAX_TRAIL_LENGTH) {
        rain_drops[column].length = MAX_TRAIL_LENGTH;
    }
    rain_drops[column].char_offset = random(0, matrix_char_count);  // New random chars
    rain_drops[column].brightness = velocity;  // Bright flash for MIDI notes
}

void updateMatrixRainPhysics() {
    unsigned long now = millis();

    // Rate limit updates (aim for ~30fps)
    if ((now - matrixrain_last_update) < 33) return;
    matrixrain_last_update = now;

    // Update each column
    for (int i = 0; i < RAIN_COLUMNS; i++) {
        if (rain_drops[i].active) {
            // Move drop down
            rain_drops[i].y += rain_drops[i].speed;

            // Animate character offset - each column changes at different rates
            if (random(0, 15) < (1 + (i % 4))) {
                rain_drops[i].char_offset = (rain_drops[i].char_offset + 1 + (i % 3)) % matrix_char_count;
            }

            // Decay MIDI brightness boost
            if (rain_drops[i].brightness > 0) {
                rain_drops[i].brightness = (rain_drops[i].brightness > 15) ?
                    rain_drops[i].brightness - 15 : 0;
            }

            // Check if drop has fallen off screen (with trail) - immediately respawn
            if (rain_drops[i].y > RAIN_SCREEN_HEIGHT + (rain_drops[i].length * 6)) {
                // Respawn at top with new random properties for continuous rain
                rain_drops[i].y = random(-25, -5);  // Start above screen at varying heights
                rain_drops[i].speed = 0.3 + (random(0, 100) / 120.0);  // Randomize speed
                rain_drops[i].length = 3 + random(0, MAX_TRAIL_LENGTH - 2);
                rain_drops[i].char_offset = random(0, matrix_char_count);  // Fresh random chars
                rain_drops[i].brightness = 0;
            }
        }
    }
}

void handleMidiNoteForMatrixRain(uint8_t note, uint8_t velocity) {
    // Boost 1-3 random columns based on velocity
    boostRandomColumn(velocity);

    // Higher velocity boosts more columns
    if (velocity > 80) {
        boostRandomColumn(velocity);
    }
    if (velocity > 110) {
        boostRandomColumn(velocity);
    }
}

void drawMatrixRain() {
    display.setTextSize(1);

    for (int col = 0; col < RAIN_COLUMNS; col++) {
        if (!rain_drops[col].active) continue;

        int x = col * 6;  // 6 pixels per character column
        int head_y = (int)rain_drops[col].y;

        // Draw the trail (oldest to newest, dim to bright)
        for (int t = rain_drops[col].length - 1; t >= 0; t--) {
            int char_y = head_y - (t * 6);  // 6 pixels per character height

            // Skip if off-screen
            if (char_y < -5 || char_y >= (int)RAIN_SCREEN_HEIGHT) continue;

            // Get character for this position
            uint8_t char_idx = (rain_drops[col].char_offset + t) % matrix_char_count;
            char c = matrix_chars[char_idx];

            // Head of trail (t=0) is brightest
            if (t == 0) {
                // Lead character - always draw
                display.setCursor(x, char_y);
                display.print(c);

                // If MIDI-triggered, add extra brightness effect
                if (rain_drops[col].brightness > 50) {
                    // Draw a small highlight
                    display.drawPixel(x + 2, char_y + 3, SSD1306_WHITE);
                }
            } else if (t == 1) {
                // Second character - draw full
                display.setCursor(x, char_y);
                display.print(c);
            } else if (t == 2) {
                // Third character - still visible but we'll draw it
                display.setCursor(x, char_y);
                display.print(c);
            } else {
                // Trailing characters - draw as dimmer (fewer pixels)
                // Just draw a partial character or dots
                if (char_y >= 0 && char_y < 32) {
                    display.drawPixel(x + 2, char_y + 1, SSD1306_WHITE);
                    display.drawPixel(x + 2, char_y + 5, SSD1306_WHITE);
                }
            }
        }
    }
}

void matrixRainDemo() {
    if (!matrixrain_initialized) {
        initMatrixRainDemo();
    }

    // Update physics
    updateMatrixRainPhysics();

    // Render
    display.clearDisplay();
    drawMatrixRain();
    display.display();
}
