#include "StepSequencer.h"
#include "NeoTrellisDisplay.h"
#include "NeoTrellisInput.h"
#include "IClock.h"

// Embedded clock implementation
class EmbeddedClock : public IClock {
public:
    uint32_t getCurrentTime() const override {
        // TODO: Implement with SAMD51 system timer
        static uint32_t time = 0;
        return time++;
    }
    
    void delay(uint32_t milliseconds) override {
        // TODO: Implement with SAMD51 delay
        for (volatile uint32_t i = 0; i < milliseconds * 1000; i++) {
            // Simple busy wait for now
        }
    }
    
    void reset() override {
        // TODO: Reset system timer if needed
    }
    
    // Non-virtual destructor for embedded
    ~EmbeddedClock() {}
};

// Global instances - embedded style
static EmbeddedClock g_clock;
static NeoTrellisDisplay g_display;
static NeoTrellisInput g_input(&g_clock);
static StepSequencer g_sequencer;

// Application class following dependency injection pattern
class EmbeddedApp {
public:
    EmbeddedApp() : lastTick_(0) {}
    
    void init() {
        // Initialize hardware
        g_display.init();
        g_input.init();
        
        // Configure sequencer with dependencies
        StepSequencer::Dependencies deps;
        deps.clock = &g_clock;
        g_sequencer = StepSequencer(deps);
        
        // Initialize sequencer with 120 BPM, 8 steps
        g_sequencer.init(120, 8);
        
        // Create demo pattern
        setupDemoPattern();
        
        // Start sequencer
        g_sequencer.start();
    }
    
    void run() {
        init();
        
        // Main application loop
        while (true) {
            // Process input events
            handleInput();
            
            // Update sequencer timing
            g_sequencer.tick();
            
            // Update display
            updateDisplay();
            
            // Small delay to prevent overwhelming the system
            g_clock.delay(10);
        }
    }

private:
    uint32_t lastTick_;
    
    void setupDemoPattern() {
        // Track 0: steps 0, 2, 4, 6
        g_sequencer.toggleStep(0, 0);
        g_sequencer.toggleStep(0, 2);
        g_sequencer.toggleStep(0, 4);
        g_sequencer.toggleStep(0, 6);
        
        // Track 1: steps 1, 3, 5, 7
        g_sequencer.toggleStep(1, 1);
        g_sequencer.toggleStep(1, 3);
        g_sequencer.toggleStep(1, 5);
        g_sequencer.toggleStep(1, 7);
        
        // Track 2: steps 0, 4
        g_sequencer.toggleStep(2, 0);
        g_sequencer.toggleStep(2, 4);
        
        // Track 3: steps 2, 6
        g_sequencer.toggleStep(3, 2);
        g_sequencer.toggleStep(3, 6);
    }
    
    void handleInput() {
        if (!g_input.pollEvents()) return;
        
        ButtonEvent event;
        while (g_input.getNextEvent(event)) {
            // Handle button presses to toggle steps
            if (event.pressed && event.row < 4 && event.col < 8) {
                g_sequencer.toggleStep(event.row, event.col);
            }
        }
    }
    
    void updateDisplay() {
        // Clear display
        g_display.clear();
        
        // Get current step for highlighting
        uint8_t currentStep = g_sequencer.getCurrentStep();
        bool isPlaying = g_sequencer.isPlaying();
        
        // Update LED states based on sequencer pattern
        for (uint8_t track = 0; track < 4; track++) {
            for (uint8_t step = 0; step < 8; step++) {
                bool stepActive = g_sequencer.isStepActive(track, step);
                bool isCurrentStep = (step == currentStep && isPlaying);
                
                uint8_t r = 0, g = 0, b = 0;
                
                if (stepActive) {
                    // Different colors for different tracks
                    switch (track) {
                        case 0: r = isCurrentStep ? 255 : 64; break;  // Red
                        case 1: g = isCurrentStep ? 255 : 64; break;  // Green  
                        case 2: b = isCurrentStep ? 255 : 64; break;  // Blue
                        case 3: r = g = isCurrentStep ? 255 : 64; break; // Yellow
                    }
                } else if (isCurrentStep) {
                    // Dim white for current step when not active
                    r = g = b = 16;
                }
                
                g_display.setLED(track, step, r, g, b);
            }
        }
        
        g_display.refresh();
    }
};

// Application entry point
int main() {
    EmbeddedApp app;
    app.run();
    return 0;
}