// Neural Network Visualization - MIDI-Reactive Brain Network
// Part of GenaJam Pico Project

struct Neuron {
    float x, y;              // Position
    uint8_t brightness;      // Current brightness (0-255)
    uint8_t base_brightness; // Resting brightness
    unsigned long activation_time; // When neuron was activated
    bool is_active;          // Currently firing
    uint8_t connections[4];  // Connected neuron indices (up to 4 connections)
    uint8_t connection_count; // Number of active connections
    uint8_t note_channel;    // Which MIDI channel/note range this responds to
};

struct NeuralConnection {
    uint8_t from_neuron;     // Source neuron index
    uint8_t to_neuron;       // Target neuron index
    float pulse_position;    // Position of data pulse (0.0 to 1.0)
    bool is_pulsing;         // Currently transmitting data
    unsigned long pulse_start_time;
    uint8_t pulse_brightness; // Brightness of the pulse
};

// Neural Network demo state - REDUCED for memory optimization
#define MAX_NEURONS 12
#define MAX_CONNECTIONS 16
Neuron nn_neurons[MAX_NEURONS];
NeuralConnection nn_connections[MAX_CONNECTIONS];
bool neuralnet_initialized = false;
unsigned long neuralnet_last_update = 0;
const float NN_SCREEN_WIDTH = 128.0;
const float NN_SCREEN_HEIGHT = 32.0;
const uint16_t NEURON_ACTIVATION_DURATION = 800;  // How long neurons stay bright (ms)
const uint16_t PULSE_TRAVEL_TIME = 600;           // Time for pulse to travel connection (ms)

void initNeuralNetDemo() {
    // Initialize 12-neuron hexagonal grid neural network
    // Hexagonal grid positions

    // Center hexagon (1 neuron)
    nn_neurons[0].x = 64;  // Screen center
    nn_neurons[0].y = 16;  // Screen center
    nn_neurons[0].brightness = 80;
    nn_neurons[0].base_brightness = 80;
    nn_neurons[0].is_active = false;
    nn_neurons[0].activation_time = 0;
    nn_neurons[0].connection_count = 0;
    nn_neurons[0].note_channel = 0;  // Center responds to all channels

    // Inner ring (6 neurons) - perfect hexagon around center
    float inner_radius = 20;
    for (int i = 1; i < 7; i++) {
        float angle = (i - 1) * 60.0 * PI / 180.0;  // 60 degrees apart
        nn_neurons[i].x = 64 + cos(angle) * inner_radius;
        nn_neurons[i].y = 16 + sin(angle) * inner_radius * 0.6;  // Compressed for 32px height
        nn_neurons[i].brightness = 50;
        nn_neurons[i].base_brightness = 50;
        nn_neurons[i].is_active = false;
        nn_neurons[i].activation_time = 0;
        nn_neurons[i].connection_count = 0;
        nn_neurons[i].note_channel = (i - 1) % 6;  // Each responds to different FM channel
    }

    // Outer ring (5 neurons) - partial outer ring to fit screen
    float outer_radius = 35;
    for (int i = 7; i < 12; i++) {
        float angle = ((i - 7) * 72.0 - 36.0) * PI / 180.0;  // 72 degrees apart, offset
        nn_neurons[i].x = 64 + cos(angle) * outer_radius;
        nn_neurons[i].y = 16 + sin(angle) * outer_radius * 0.5;  // More compressed for screen
        nn_neurons[i].brightness = 30;
        nn_neurons[i].base_brightness = 30;
        nn_neurons[i].is_active = false;
        nn_neurons[i].activation_time = 0;
        nn_neurons[i].connection_count = 0;
        nn_neurons[i].note_channel = (i - 7) % 5;  // Distributed across channels
    }

    // Create beautiful hexagonal connections
    int connection_index = 0;

    // Center (0) connects to all inner ring neurons (1-6)
    for (int inner = 1; inner < 7; inner++) {
        if (connection_index < MAX_CONNECTIONS) {
            nn_connections[connection_index].from_neuron = 0;
            nn_connections[connection_index].to_neuron = inner;
            nn_connections[connection_index].is_pulsing = false;
            nn_connections[connection_index].pulse_position = 0;

            // Add to center neuron's connection list
            if (nn_neurons[0].connection_count < 4) {
                nn_neurons[0].connections[nn_neurons[0].connection_count] = connection_index;
                nn_neurons[0].connection_count++;
            }
            connection_index++;
        }
    }

    // Inner ring (1-6) connects to adjacent inner neurons (hexagon sides)
    for (int i = 1; i < 7; i++) {
        int next = (i == 6) ? 1 : (i + 1);  // Wrap around for last neuron
        if (connection_index < MAX_CONNECTIONS) {
            nn_connections[connection_index].from_neuron = i;
            nn_connections[connection_index].to_neuron = next;
            nn_connections[connection_index].is_pulsing = false;
            nn_connections[connection_index].pulse_position = 0;

            // Add to neuron's connection list
            if (nn_neurons[i].connection_count < 4) {
                nn_neurons[i].connections[nn_neurons[i].connection_count] = connection_index;
                nn_neurons[i].connection_count++;
            }
            connection_index++;
        }
    }

    // Inner ring connects to outer ring (radial connections)
    for (int inner = 1; inner < 7; inner++) {
        int outer = 7 + ((inner - 1) % 5);  // Map to outer ring neurons
        if (outer < MAX_NEURONS && connection_index < MAX_CONNECTIONS) {
            nn_connections[connection_index].from_neuron = inner;
            nn_connections[connection_index].to_neuron = outer;
            nn_connections[connection_index].is_pulsing = false;
            nn_connections[connection_index].pulse_position = 0;

            // Add to inner neuron's connection list
            if (nn_neurons[inner].connection_count < 4) {
                nn_neurons[inner].connections[nn_neurons[inner].connection_count] = connection_index;
                nn_neurons[inner].connection_count++;
            }
            connection_index++;
        }
    }

    neuralnet_initialized = true;
    neuralnet_last_update = millis();
}

void updateNeuralNetPhysics() {
    unsigned long now = millis();

    // Safety: prevent too-fast updates
    if ((now - neuralnet_last_update) < 20) return;  // Min 20ms between updates
    neuralnet_last_update = now;

    // Update neuron states
    for (int i = 0; i < MAX_NEURONS; i++) {
        if (nn_neurons[i].is_active) {
            // Check if activation has expired
            if (now - nn_neurons[i].activation_time > NEURON_ACTIVATION_DURATION) {
                nn_neurons[i].is_active = false;
                nn_neurons[i].brightness = nn_neurons[i].base_brightness;
            } else {
                // Fade out gradually during activation
                float fade_progress = (float)(now - nn_neurons[i].activation_time) / NEURON_ACTIVATION_DURATION;
                nn_neurons[i].brightness = 255 - (fade_progress * (255 - nn_neurons[i].base_brightness));
            }
        } else {
            // Subtle breathing effect for inactive neurons
            float breath = sin(now * 0.003 + i * 0.5) * 10;
            nn_neurons[i].brightness = constrain(nn_neurons[i].base_brightness + breath, 20, 80);
        }
    }

    // Update connection pulses
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (nn_connections[i].is_pulsing) {
            float elapsed = now - nn_connections[i].pulse_start_time;
            nn_connections[i].pulse_position = elapsed / PULSE_TRAVEL_TIME;

            // Pulse brightness fades as it travels
            nn_connections[i].pulse_brightness = 255 - (nn_connections[i].pulse_position * 155);

            if (nn_connections[i].pulse_position >= 1.0) {
                // Pulse reached destination - activate target neuron
                uint8_t target = nn_connections[i].to_neuron;
                if (target < MAX_NEURONS) {
                    nn_neurons[target].is_active = true;
                    nn_neurons[target].activation_time = now;
                    nn_neurons[target].brightness = 255;

                    // Propagate pulse to target's connections
                    propagateNeuralPulse(target, now);
                }

                nn_connections[i].is_pulsing = false;
                nn_connections[i].pulse_position = 0;
            }
        }
    }
}

void propagateNeuralPulse(uint8_t neuron_index, unsigned long now) {
    if (neuron_index >= MAX_NEURONS) return;

    // Start pulses on all connections from this neuron
    for (int i = 0; i < nn_neurons[neuron_index].connection_count; i++) {
        uint8_t connection_idx = nn_neurons[neuron_index].connections[i];
        if (connection_idx < MAX_CONNECTIONS && !nn_connections[connection_idx].is_pulsing) {
            nn_connections[connection_idx].is_pulsing = true;
            nn_connections[connection_idx].pulse_position = 0;
            nn_connections[connection_idx].pulse_start_time = now;
            nn_connections[connection_idx].pulse_brightness = 255;
        }
    }
}

void handleMidiNoteForNeuralNet(uint8_t note, uint8_t velocity) {
    unsigned long now = millis();

    // Map MIDI notes to hexagonal neurons
    // Different note ranges trigger different parts of the hex grid

    // Low notes (bass) - activate center neuron (spreads to all)
    if (note < 48) {  // Below C3
        nn_neurons[0].is_active = true;  // Center neuron
        nn_neurons[0].activation_time = now;
        nn_neurons[0].brightness = constrain(velocity * 2, 150, 255);
        propagateNeuralPulse(0, now);
    }

    // Mid notes - activate inner ring neurons (1-6)
    else if (note >= 48 && note < 84) {  // C3 to C6
        int ring_neuron = 1 + ((note - 48) % 6);  // Map to neurons 1-6
        nn_neurons[ring_neuron].is_active = true;
        nn_neurons[ring_neuron].activation_time = now;
        nn_neurons[ring_neuron].brightness = constrain(velocity * 2, 100, 255);
        propagateNeuralPulse(ring_neuron, now);
    }

    // High notes - activate outer ring neurons (7-11)
    else if (note >= 84) {  // Above C6
        int outer_neuron = 7 + ((note - 84) % 5);  // Map to neurons 7-11
        nn_neurons[outer_neuron].is_active = true;
        nn_neurons[outer_neuron].activation_time = now;
        nn_neurons[outer_neuron].brightness = constrain(velocity * 2, 100, 255);
        propagateNeuralPulse(outer_neuron, now);
    }

    // High velocity notes cause "neural cascades" - activate multiple neurons
    if (velocity > 110) {
        // Activate a random inner ring neuron for cascade effect
        int cascade_neuron = 1 + random(0, 6);  // Random inner ring neuron
        nn_neurons[cascade_neuron].is_active = true;
        nn_neurons[cascade_neuron].activation_time = now + 100;  // Slight delay
        nn_neurons[cascade_neuron].brightness = 255;
        propagateNeuralPulse(cascade_neuron, now + 100);
    }

    // Very low bass notes - activate entire inner ring simultaneously
    if (note < 36 && velocity > 80) {  // Very low bass
        for (int i = 1; i < 4; i++) {  // Activate first 3 inner ring neurons
            nn_neurons[i].is_active = true;
            nn_neurons[i].activation_time = now + (i * 50);  // Staggered
            nn_neurons[i].brightness = 255;
            propagateNeuralPulse(i, now + (i * 50));
        }
    }
}

void drawNeuralNetwork() {
    // Draw connections first (so neurons appear on top)
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        uint8_t from_idx = nn_connections[i].from_neuron;
        uint8_t to_idx = nn_connections[i].to_neuron;

        if (from_idx < MAX_NEURONS && to_idx < MAX_NEURONS) {
            int x1 = (int)nn_neurons[from_idx].x;
            int y1 = (int)nn_neurons[from_idx].y;
            int x2 = (int)nn_neurons[to_idx].x;
            int y2 = (int)nn_neurons[to_idx].y;

            // Draw connection line (dimmed)
            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);

            // Draw pulse if active
            if (nn_connections[i].is_pulsing) {
                float pos = nn_connections[i].pulse_position;
                int pulse_x = x1 + (pos * (x2 - x1));
                int pulse_y = y1 + (pos * (y2 - y1));

                // Draw pulse as a bright dot with trailing effect
                if (pulse_x >= 0 && pulse_x < 128 && pulse_y >= 0 && pulse_y < 32) {
                    display.fillRect(pulse_x - 1, pulse_y - 1, 3, 3, SSD1306_WHITE);
                    display.drawPixel(pulse_x, pulse_y, SSD1306_WHITE);
                }
            }
        }
    }

    // Draw neurons
    for (int i = 0; i < MAX_NEURONS; i++) {
        int x = (int)nn_neurons[i].x;
        int y = (int)nn_neurons[i].y;

        if (x >= 0 && x < 128 && y >= 0 && y < 32) {
            if (nn_neurons[i].is_active) {
                // Active neuron - filled circle
                display.fillRect(x - 2, y - 2, 5, 5, SSD1306_WHITE);
                display.drawPixel(x, y, SSD1306_WHITE);  // Center dot

                // Add "spark" effect for very active neurons
                if (nn_neurons[i].brightness > 200) {
                    display.drawPixel(x - 3, y, SSD1306_WHITE);
                    display.drawPixel(x + 3, y, SSD1306_WHITE);
                    display.drawPixel(x, y - 3, SSD1306_WHITE);
                    display.drawPixel(x, y + 3, SSD1306_WHITE);
                }
            } else {
                // Inactive neuron - small dot
                display.drawRect(x - 1, y - 1, 3, 3, SSD1306_WHITE);
                display.drawPixel(x, y, SSD1306_WHITE);
            }
        }
    }
}

void neuralNetDemo() {
    if (!neuralnet_initialized) {
        initNeuralNetDemo();
    }

    // Update physics
    updateNeuralNetPhysics();

    // Render everything
    display.clearDisplay();

    // Only show title briefly when first entering the mode
    static unsigned long title_start_time = 0;
    static bool title_shown = false;

    if (!title_shown) {
        title_start_time = millis();
        title_shown = true;
    }

    // Show title for first 2 seconds only
    if ((millis() - title_start_time) < 2000) {
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("NEURAL NET");
    }

    // Draw the network
    drawNeuralNetwork();

    display.display();
}