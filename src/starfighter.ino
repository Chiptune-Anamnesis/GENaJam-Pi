// Starfighter Visualization - MIDI-Reactive Space Flight
// Part of GenaJam Pico Project

struct Star {
    float x, y;              // Position
    float speed;             // Movement speed
    uint8_t brightness;      // Star brightness (0-255)
    uint8_t base_brightness; // Normal brightness level
    uint8_t size;            // Star size (1-3)
    unsigned long flash_time; // When star was triggered by MIDI
    bool midi_triggered;     // True if star is flashing from MIDI
    unsigned long twinkle_time; // For natural twinkling
};

struct StarfighterShip {
    float y;                 // Y position (oscillates)
    float y_offset;          // Oscillation offset
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

// Starfighter demo state
#define MAX_STARS 96
Star sf_stars[MAX_STARS];
StarfighterShip starfighter_ship;
EnvironmentalEffect env_effects;
bool starfighter_initialized = false;
unsigned long starfighter_last_update = 0;
const float SF_SCREEN_WIDTH = 128.0;
const float SF_SCREEN_HEIGHT = 32.0;
const uint16_t MIDI_FLASH_DURATION = 300;  // How long stars flash (ms)

void initStarfighterDemo() {
    // Initialize ship in center of screen (horizontally centered for side-scrolling)
    starfighter_ship.y = SF_SCREEN_WIDTH / 2;  // Now using y for horizontal position
    starfighter_ship.y_offset = 0;
    starfighter_ship.last_engine_pulse = 0;
    starfighter_ship.engine_on = false;

    // Manual distribution that we know works
    for (int i = 0; i < MAX_STARS; i++) {
        // Distribute X evenly across screen with some offset
        sf_stars[i].x = (i * 127) / MAX_STARS + (i % 7);  // Spread across 128 pixels

        // Distribute Y more evenly across height - simple modulo approach
        sf_stars[i].y = i % 32;   // Just cycle through 0-31

        // Clamp to display bounds
        if (sf_stars[i].x > 127) sf_stars[i].x = 127;
        if (sf_stars[i].y > 31) sf_stars[i].y = 31;

        // Vary speeds with slower background stars for better depth
        sf_stars[i].speed = 0.2 + (i % 10) * 0.18;  // Speeds from 0.2 to 1.82
        sf_stars[i].brightness = 255;
        sf_stars[i].base_brightness = 255;
        sf_stars[i].size = 1;
        sf_stars[i].flash_time = 0;
        sf_stars[i].midi_triggered = false;
        sf_stars[i].twinkle_time = 0;
    }

    // Initialize environmental effects
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

    // Limit update rate
    if (dt < 0.01) return;  // Min 10ms between updates
    if (dt > 0.1) dt = 0.1; // Max 100ms delta to prevent huge jumps

    starfighter_last_update = now;

    // Update ship gentle oscillation - using integer math to prevent float accumulation errors
    static int oscillation_counter = 0;
    oscillation_counter++;
    if (oscillation_counter > 1000) oscillation_counter = 0;  // Prevent overflow
    starfighter_ship.y = (SF_SCREEN_WIDTH / 2) + sin(oscillation_counter * 0.01) * 4;

    // Engine pulse effect
    if (now - starfighter_ship.last_engine_pulse > 150) {
        starfighter_ship.engine_on = !starfighter_ship.engine_on;
        starfighter_ship.last_engine_pulse = now;
    }

    // Update stars - move them leftward (ship flying rightward through space)
    for (int i = 0; i < MAX_STARS; i++) {
        // Bounds safety check
        if (i >= MAX_STARS) break;

        // Move stars leftward (ship moving rightward through space) with bounds protection
        sf_stars[i].x -= sf_stars[i].speed;

        // Safety: prevent stars from getting corrupted positions
        if (sf_stars[i].x < -100 || sf_stars[i].x > 300) {
            sf_stars[i].x = 128 + (i % 20);  // Reset to safe position
        }
        if (sf_stars[i].y < 0 || sf_stars[i].y > 31) {
            sf_stars[i].y = i % 32;  // Reset Y to safe position
        }

        // Wrap stars to right side when they exit left - DON'T change Y!
        if (sf_stars[i].x < -5) {  // Increased threshold to prevent rapid wrapping
            sf_stars[i].x = 128 + (i % 20);  // Smaller modulo for better distribution
            // DON'T touch sf_stars[i].y - keep original Y position!
        }

        // Handle MIDI flash decay and size reset
        if (sf_stars[i].midi_triggered) {
            if (now - sf_stars[i].flash_time > MIDI_FLASH_DURATION) {
                // Flash ended, return to normal brightness and size
                sf_stars[i].midi_triggered = false;
                sf_stars[i].brightness = 255;

                // Reset star size to original value based on position
                if (i < 30) {
                    sf_stars[i].size = 1;  // Far stars
                } else if (i < 55) {
                    sf_stars[i].size = (i % 2) + 1;  // Mid-far stars (1-2)
                } else if (i < 75) {
                    sf_stars[i].size = (i % 3) + 1;  // Mid stars (1-3)
                } else {
                    sf_stars[i].size = (i % 2) + 2;  // Near stars (2-3)
                }
            } else {
                // Enhanced twinkle brightness during flash
                float flash_progress = (float)(now - sf_stars[i].flash_time) / MIDI_FLASH_DURATION;
                float twinkle = sin((now - sf_stars[i].flash_time) * 0.03) * 30;
                sf_stars[i].brightness = constrain(255 - (flash_progress * 50) + twinkle, 200, 255);
            }
        } else {
            // No automatic twinkling - keep stars steady at full brightness
            sf_stars[i].brightness = 255;
        }
    }

    // Update environmental effects
    if (env_effects.hyperspace_active && now > env_effects.hyperspace_end_time) {
        env_effects.hyperspace_active = false;

        // Reset all star sizes when exiting hyperspace to prevent buffer corruption
        for (int i = 0; i < MAX_STARS; i++) {
            if (i >= MAX_STARS) break;

            // Reset star size to original value based on position
            if (i < 30) {
                sf_stars[i].size = 1;  // Far stars
            } else if (i < 55) {
                sf_stars[i].size = (i % 2) + 1;  // Mid-far stars (1-2)
            } else if (i < 75) {
                sf_stars[i].size = (i % 3) + 1;  // Mid stars (1-3)
            } else {
                sf_stars[i].size = (i % 2) + 2;  // Near stars (2-3)
            }

            // Clear any stuck MIDI triggers
            sf_stars[i].midi_triggered = false;
            sf_stars[i].brightness = 255;
        }
    }

    // Nebula effects removed

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
            env_effects.ripple_radius = progress * 50;  // Max radius of 50 pixels
        }
    }

    // Natural environmental effects removed with nebula clouds
}

void handleMidiNoteForStarfighter(uint8_t note, uint8_t velocity) {
    unsigned long now = millis();

    // Simplified environmental effects with single cooldown system
    static unsigned long last_effect_time = 0;
    static uint8_t effect_cooldown_counter = 0;

    // Prevent too frequent effect processing
    if ((now - last_effect_time) < 100) return;  // Min 100ms between any effects

    effect_cooldown_counter++;
    if (effect_cooldown_counter > 250) effect_cooldown_counter = 0;  // Reset to prevent overflow

    // HYPERSPACE STREAKS - less frequent but longer lasting
    if (velocity > 118 && effect_cooldown_counter % 40 == 0) {  // Every 4 seconds max
        env_effects.hyperspace_active = true;
        env_effects.hyperspace_end_time = now + 1200;
        last_effect_time = now;
    }

    // SOLAR FLARES - more reasonable threshold
    else if (velocity > 110 && effect_cooldown_counter % 12 == 0) {  // Every 1.2 seconds max
        env_effects.solar_flare_active = true;
        env_effects.flare_progress = 0;
        env_effects.flare_end_time = now + 500;
        last_effect_time = now;
    }

    // WARP RIPPLES - EVERY bass note triggers ripples
    else if (note < 45 && velocity > 30 && effect_cooldown_counter % 3 == 0) {  // Every 300ms max
        env_effects.warp_ripples_active = true;
        env_effects.ripple_radius = 0;
        env_effects.ripple_end_time = now + 1000;
        last_effect_time = now;
    }

    // Enhanced star twinkling - ONLY when NOT in hyperspace mode to prevent buffer issues
    if (!env_effects.hyperspace_active) {
        int stars_to_twinkle = map(velocity, 0, 127, 3, 8);  // Reduced max to prevent too many updates

        for (int twinkle_count = 0; twinkle_count < stars_to_twinkle; twinkle_count++) {
            // Pick completely random stars for twinkling effect - with bounds safety
            int random_star = random(0, MAX_STARS);
            if (random_star >= MAX_STARS) random_star = MAX_STARS - 1;  // Safety check

            // Make the star temporarily bigger and brighter
            sf_stars[random_star].midi_triggered = true;
            sf_stars[random_star].flash_time = now;
            sf_stars[random_star].brightness = 255;

            // Dramatically increase star size for very noticeable twinkle effect
            sf_stars[random_star].size = 4;  // Make it extra large (biggest size) temporarily
        }
    }
}

void drawStarfighterShip() {
    int ship_x = (int)starfighter_ship.y;  // Ship position along X-axis (horizontal movement)
    int ship_y = (int)(SF_SCREEN_HEIGHT / 2);  // Center vertically

    // Cool space jet with really long wings - top-down view pointing right
    // Main fuselage (sleek body)
    display.drawLine(ship_x - 10, ship_y, ship_x + 6, ship_y - 1, SSD1306_WHITE);  // Top fuselage
    display.drawLine(ship_x - 10, ship_y, ship_x + 6, ship_y + 1, SSD1306_WHITE);  // Bottom fuselage
    display.drawLine(ship_x + 6, ship_y - 1, ship_x + 8, ship_y, SSD1306_WHITE);   // Nose top
    display.drawLine(ship_x + 6, ship_y + 1, ship_x + 8, ship_y, SSD1306_WHITE);   // Nose bottom

    // REALLY LONG WINGS - with triangular tips at the far ends
    // Top wing - extends from fuselage out to triangular tip
    display.drawLine(ship_x - 4, ship_y - 1, ship_x - 6, ship_y - 10, SSD1306_WHITE);  // Wing leading edge to tip front
    display.drawLine(ship_x - 4, ship_y - 1, ship_x - 2, ship_y - 10, SSD1306_WHITE);  // Wing trailing edge to tip rear
    display.drawLine(ship_x - 6, ship_y - 10, ship_x - 4, ship_y - 12, SSD1306_WHITE); // Tip front to point
    display.drawLine(ship_x - 2, ship_y - 10, ship_x - 4, ship_y - 12, SSD1306_WHITE); // Tip rear to point (triangular tip)

    // Bottom wing - extends from fuselage out to triangular tip
    display.drawLine(ship_x - 4, ship_y + 1, ship_x - 6, ship_y + 10, SSD1306_WHITE);  // Wing leading edge to tip front
    display.drawLine(ship_x - 4, ship_y + 1, ship_x - 2, ship_y + 10, SSD1306_WHITE);  // Wing trailing edge to tip rear
    display.drawLine(ship_x - 6, ship_y + 10, ship_x - 4, ship_y + 12, SSD1306_WHITE); // Tip front to point
    display.drawLine(ship_x - 2, ship_y + 10, ship_x - 4, ship_y + 12, SSD1306_WHITE); // Tip rear to point (triangular tip)

    // Wing details and weapon hardpoints
    display.drawPixel(ship_x - 3, ship_y - 8, SSD1306_WHITE);  // Top wing detail
    display.drawPixel(ship_x - 3, ship_y + 8, SSD1306_WHITE);  // Bottom wing detail
    display.drawPixel(ship_x - 1, ship_y - 6, SSD1306_WHITE);  // Weapon mount
    display.drawPixel(ship_x - 1, ship_y + 6, SSD1306_WHITE);  // Weapon mount

    // Cockpit canopy (larger and more detailed)
    display.fillRect(ship_x - 2, ship_y - 1, 6, 3, SSD1306_WHITE);
    display.drawPixel(ship_x + 1, ship_y, SSD1306_BLACK);  // Cockpit window

    // Twin engine exhausts with afterburners
    if (starfighter_ship.engine_on) {
        // Bright afterburner flames
        display.drawPixel(ship_x - 11, ship_y - 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 11, ship_y + 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 12, ship_y - 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 12, ship_y + 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 13, ship_y, SSD1306_WHITE);
        display.drawPixel(ship_x - 14, ship_y, SSD1306_WHITE);  // Extended flame
    } else {
        // Normal exhaust
        display.drawPixel(ship_x - 11, ship_y - 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 11, ship_y + 1, SSD1306_WHITE);
        display.drawPixel(ship_x - 12, ship_y, SSD1306_WHITE);
    }

    // Additional fuselage details
    display.drawPixel(ship_x - 6, ship_y, SSD1306_WHITE);   // Center line detail
    display.drawPixel(ship_x + 3, ship_y, SSD1306_WHITE);   // Nose detail
}

void drawStarfield() {
    // Draw all stars (modified for hyperspace effect and varying sizes)
    for (int i = 0; i < MAX_STARS; i++) {
        // Bounds safety check
        if (i >= MAX_STARS) break;

        int x = (int)sf_stars[i].x;
        int y = (int)sf_stars[i].y;

        // Enhanced bounds checking
        if (x >= 0 && x < 128 && y >= 0 && y < 32 && x > -10 && y > -5) {
            // HYPERSPACE STREAKS - draw stars as horizontal lines during intense passages (optimized)
            if (env_effects.hyperspace_active) {
                // Draw limited streak to prevent hangs
                int streak_length = (sf_stars[i].speed * 4);  // Shorter streaks
                if (streak_length > 12) streak_length = 12;   // Max 12 pixels to prevent Core 1 hang
                for (int streak = 0; streak < streak_length && x + streak < 128; streak += 2) {
                    display.drawPixel(x + streak, y, SSD1306_WHITE);
                }
            } else {
                // Enhanced star rendering based on size - with bounds checking
                if (sf_stars[i].size == 1) {
                    // Small star - single pixel
                    if (x >= 0 && x < 128 && y >= 0 && y < 32)
                        display.drawPixel(x, y, SSD1306_WHITE);
                } else if (sf_stars[i].size == 2) {
                    // Medium star - 2x2 pattern
                    if (x >= 0 && x < 127 && y >= 0 && y < 31) {
                        display.drawPixel(x, y, SSD1306_WHITE);
                        display.drawPixel(x + 1, y, SSD1306_WHITE);
                        display.drawPixel(x, y + 1, SSD1306_WHITE);
                        display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
                    }
                } else if (sf_stars[i].size == 3) {
                    // Large star - plus pattern
                    if (x >= 1 && x < 127 && y >= 1 && y < 31) {
                        display.drawPixel(x, y, SSD1306_WHITE);
                        display.drawPixel(x + 1, y, SSD1306_WHITE);
                        display.drawPixel(x - 1, y, SSD1306_WHITE);
                        display.drawPixel(x, y + 1, SSD1306_WHITE);
                        display.drawPixel(x, y - 1, SSD1306_WHITE);
                    }
                } else { // size 4+ - extra large star
                    // Extra large star - filled plus with diagonals
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
    // NEBULA CLOUDS REMOVED - was causing device freezing

    // SOLAR FLARES - sweep across screen
    if (env_effects.solar_flare_active) {
        int flare_x = env_effects.flare_progress * 140;  // Sweep from left to right beyond screen
        // Draw vertical flare line
        for (int y = 0; y < 32; y++) {
            if (flare_x >= 0 && flare_x < 128) {
                display.drawPixel(flare_x, y, SSD1306_WHITE);
            }
            // Add trailing effect
            if (flare_x - 1 >= 0 && flare_x - 1 < 128) {
                display.drawPixel(flare_x - 1, y, SSD1306_WHITE);
            }
        }
    }

    // WARP RIPPLES - emanate from ship (optimized to prevent Core 1 hangs)
    if (env_effects.warp_ripples_active) {
        int ship_x = (int)starfighter_ship.y;
        int ship_y = 16;  // Center of screen
        int radius = (int)env_effects.ripple_radius;

        // Limit radius to prevent excessive drawing that causes hangs
        if (radius > 25) radius = 25;  // Max radius to prevent Core 1 lockup

        // Draw sparse ripple pattern (every 3rd pixel for performance)
        if (radius > 0) {
            // Horizontal ripple lines - sparse pattern
            for (int x = ship_x - radius; x <= ship_x + radius; x += 3) {
                if (x >= 0 && x < 128) {
                    if (ship_y - radius >= 0) display.drawPixel(x, ship_y - radius, SSD1306_WHITE);
                    if (ship_y + radius < 32) display.drawPixel(x, ship_y + radius, SSD1306_WHITE);
                }
            }
            // Vertical ripple lines - sparse pattern
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

    // Update physics
    updateStarfighterPhysics();

    // Render everything
    display.clearDisplay();
    drawStarfield();
    drawEnvironmentalEffects();
    drawStarfighterShip();
    display.display();
}