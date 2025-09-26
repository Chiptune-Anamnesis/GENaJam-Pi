// Asteroids Demo System - MIDI-Reactive Asteroids Visualizer
// Part of GenaJam Pico Project

// Asteroids Demo System
struct AsteroidShip {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float angle;          // Rotation angle
    bool thrusting;       // Thrust state
    unsigned long last_shot;
};

struct Bullet {
    float x, y;           // Position
    float vx, vy;         // Velocity
    unsigned long created;
    bool active;
};

struct Asteroid {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float angle;          // Rotation
    uint8_t size;         // 0=large, 1=medium, 2=small
    bool active;
    bool is_note;         // True if spawned from MIDI note
    uint8_t note_pitch;   // MIDI note that spawned this asteroid
    uint8_t velocity;     // MIDI velocity
    unsigned long spawn_time;  // When it was created
};

// Asteroids demo state
#define MAX_BULLETS 8
#define MAX_ASTEROIDS 16
AsteroidShip ship;
Bullet bullets[MAX_BULLETS];
Asteroid asteroids[MAX_ASTEROIDS];
unsigned long asteroids_last_update = 0;
unsigned long asteroids_next_action = 0;
bool asteroids_initialized = false;
unsigned long last_midi_note_time = 0;
uint8_t recent_notes[8] = {0};  // Track recent notes for rhythm patterns
uint8_t recent_note_count = 0;
const float SCREEN_WIDTH = 128.0;
const float SCREEN_HEIGHT = 32.0;

void initAsteroidsDemo() {
    // Initialize ship in center
    ship.x = SCREEN_WIDTH / 2;
    ship.y = SCREEN_HEIGHT / 2;
    ship.vx = 0;
    ship.vy = 0;
    ship.angle = random(0, 360);  // Random starting angle
    ship.thrusting = true;        // Start moving
    ship.last_shot = 0;

    // Clear bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    // Initialize some asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        asteroids[i].active = false;
    }

    // Create initial asteroids
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
            asteroids[i].vx = (random(-100, 101) / 100.0) * 0.15;  // Slower asteroids
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
    // First, count existing asteroids and remove oldest if too many
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

    // If we have 6 or more asteroids, remove the oldest one
    if (asteroid_count >= 6 && oldest_index >= 0) {
        asteroids[oldest_index].active = false;
    }

    // Now find a slot for the new asteroid
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!asteroids[i].active) {
            // Position based on note pitch (higher notes spawn higher on screen)
            float y_pos = map(note, 0, 127, SCREEN_HEIGHT - 4, 4);
            float x_pos = random(0, (int)SCREEN_WIDTH);

            // Velocity affects movement speed and size
            float speed_multiplier = map(velocity, 0, 127, 0.5, 2.0);
            uint8_t size = (velocity > 90) ? 0 : (velocity > 60) ? 1 : 2;  // Louder = bigger

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
            bullets[i].vx = cos(ship.angle * PI / 180.0) * 1.5;  // Slightly slower bullets
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

    // Update ship
    if (ship.thrusting) {
        ship.vx += cos(ship.angle * PI / 180.0) * 0.025;  // 30% slower again (0.035 * 0.7)
        ship.vy += sin(ship.angle * PI / 180.0) * 0.025;
    }

    // Apply drag
    ship.vx *= 0.994;  // More drag for slower movement
    ship.vy *= 0.994;

    // Update ship position with wrapping
    ship.x += ship.vx;
    ship.y += ship.vy;
    if (ship.x < 0) ship.x = SCREEN_WIDTH;
    if (ship.x > SCREEN_WIDTH) ship.x = 0;
    if (ship.y < 0) ship.y = SCREEN_HEIGHT;
    if (ship.y > SCREEN_HEIGHT) ship.y = 0;

    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].vx;
            bullets[i].y += bullets[i].vy;

            // Wrap bullets
            if (bullets[i].x < 0) bullets[i].x = SCREEN_WIDTH;
            if (bullets[i].x > SCREEN_WIDTH) bullets[i].x = 0;
            if (bullets[i].y < 0) bullets[i].y = SCREEN_HEIGHT;
            if (bullets[i].y > SCREEN_HEIGHT) bullets[i].y = 0;

            // Remove old bullets
            if (now - bullets[i].created > 3000) {
                bullets[i].active = false;
            }
        }
    }

    // Update asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            asteroids[i].x += asteroids[i].vx;
            asteroids[i].y += asteroids[i].vy;
            asteroids[i].angle += 0.3;  // Slower rotation

            // Wrap asteroids
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
    // Check bullet-asteroid collisions
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;

        for (int j = 0; j < MAX_ASTEROIDS; j++) {
            if (!asteroids[j].active) continue;

            float asteroid_radius = (asteroids[j].size == 0) ? 6 : (asteroids[j].size == 1) ? 4 : 2;

            if (asteroidsCheckCollision(bullets[i].x, bullets[i].y, 1, asteroids[j].x, asteroids[j].y, asteroid_radius)) {
                // Hit!
                if (asteroids[j].is_note) {
                    // Note asteroids die in one hit - no breaking up
                    asteroids[j].active = false;
                } else {
                    // Regular asteroids break up
                    if (asteroids[j].size < 2) {
                        // Create smaller asteroids
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

    // Check for immediate collision threats first
    bool danger_ahead = false;
    float danger_angle = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (asteroids[i].active) {
            float dx = asteroids[i].x - ship.x;
            float dy = asteroids[i].y - ship.y;
            float dist = sqrt(dx * dx + dy * dy);
            float asteroid_radius = (asteroids[i].size == 0) ? 8 : (asteroids[i].size == 1) ? 6 : 4;

            // Check if asteroid is too close (danger zone)
            if (dist < asteroid_radius + 12) {
                danger_ahead = true;
                danger_angle = atan2(dy, dx) * 180.0 / PI;
                break;
            }
        }
    }

    // Collision avoidance takes priority
    if (danger_ahead) {
        // Turn away from the dangerous asteroid
        float escape_angle = danger_angle + 180.0;  // Turn opposite direction
        if (escape_angle >= 360.0) escape_angle -= 360.0;
        ship.angle = escape_angle;
        ship.thrusting = true;  // Thrust away from danger
        asteroids_next_action = now + 300;  // Quick reaction time
    } else if (now > asteroids_next_action) {
        // Normal AI behavior when safe
        int action = random(0, 100);

        if (action < 25) {
            // 25% - Turn toward distant asteroid and fire (only if safe)
            float nearest_dist = 1000;
            bool found_safe_target = false;
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (asteroids[i].active) {
                    float dx = asteroids[i].x - ship.x;
                    float dy = asteroids[i].y - ship.y;
                    float dist = sqrt(dx * dx + dy * dy);
                    float asteroid_radius = (asteroids[i].size == 0) ? 8 : (asteroids[i].size == 1) ? 6 : 4;

                    // Only target asteroids that are far enough away
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
            // 20% - Small turn left
            ship.angle -= random(10, 30);
        } else if (action < 65) {
            // 20% - Small turn right
            ship.angle += random(10, 30);
        } else if (action < 80) {
            // 15% - Start thrusting
            ship.thrusting = true;
        } else if (action < 90) {
            // 10% - Stop thrusting
            ship.thrusting = false;
        } else {
            // 10% - Fire if ready
            if (now - ship.last_shot > 500) {
                fireBullet();
            }
        }

        asteroids_next_action = now + random(800, 1500);  // Normal decision timing
    }

    // Auto-fire occasionally regardless of AI decisions
    if (now - ship.last_shot > 1000 && random(0, 100) < 10) {
        fireBullet();
    }

    // Spawn new asteroids occasionally if too few
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

    // Create note asteroid
    createNoteAsteroid(note, velocity);

    // Track rhythm patterns
    if (now - last_midi_note_time < 300) {  // Fast notes (< 300ms apart)
        // Add to recent notes buffer
        if (recent_note_count < 8) {
            recent_notes[recent_note_count] = note;
            recent_note_count++;
        } else {
            // Shift buffer and add new note
            for (int i = 0; i < 7; i++) {
                recent_notes[i] = recent_notes[i + 1];
            }
            recent_notes[7] = note;
        }

        // Check for patterns - ascending/descending scales create formations
        if (recent_note_count >= 4) {
            bool ascending = true, descending = true;
            for (int i = 1; i < recent_note_count; i++) {
                if (recent_notes[i] <= recent_notes[i-1]) ascending = false;
                if (recent_notes[i] >= recent_notes[i-1]) descending = false;
            }

            if (ascending || descending) {
                // Create formation of asteroids in a line
                for (int i = 0; i < 3; i++) {
                    float x = 10 + i * 35;
                    float y = ascending ? (5 + i * 5) : (25 - i * 5);
                    createNoteAsteroid(note + i * (ascending ? 12 : -12), velocity);
                }
            }
        }
    } else {
        // Reset pattern detection for isolated notes
        recent_note_count = 1;
        recent_notes[0] = note;
    }

    last_midi_note_time = now;
}

void drawAsteroidsShip() {
    // Simple triangle ship
    float cos_a = cos(ship.angle * PI / 180.0);
    float sin_a = sin(ship.angle * PI / 180.0);

    // Ship points (nose forward)
    int x1 = ship.x + cos_a * 4;
    int y1 = ship.y + sin_a * 4;
    int x2 = ship.x + cos((ship.angle + 135) * PI / 180.0) * 3;
    int y2 = ship.y + sin((ship.angle + 135) * PI / 180.0) * 3;
    int x3 = ship.x + cos((ship.angle - 135) * PI / 180.0) * 3;
    int y3 = ship.y + sin((ship.angle - 135) * PI / 180.0) * 3;

    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    display.drawLine(x2, y2, x3, y3, SSD1306_WHITE);
    display.drawLine(x3, y3, x1, y1, SSD1306_WHITE);

    // Thrust flame
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
                // Note asteroids are filled circles with pulsing effect
                unsigned long age = millis() - asteroids[i].spawn_time;
                bool pulse = (age / 200) % 2;  // Pulse every 200ms

                if (pulse) {
                    display.fillCircle(x, y, radius, SSD1306_WHITE);
                } else {
                    display.drawCircle(x, y, radius, SSD1306_WHITE);
                }

                // Add a small indicator dot in center
                display.drawPixel(x, y, pulse ? SSD1306_BLACK : SSD1306_WHITE);
            } else {
                // Regular asteroids are just circles
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

    // Update physics and AI
    updateAsteroidsPhysics();
    updateAsteroidsAI();
    checkAsteroidsCollisions();

    // Render everything
    display.clearDisplay();
    drawAsteroidsShip();
    drawAsteroidsField();
    drawAsteroidsBullets();
    display.display();
}