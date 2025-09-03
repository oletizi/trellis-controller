#include "StepSequencer.h"
#include "CursesDisplay.h"
#include "CursesInput.h"
#include "IClock.h"
#include <memory>
#include <chrono>
#include <thread>

class SystemClock : public IClock {
public:
    uint32_t getCurrentTime() const override {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
    
    void delay(uint32_t milliseconds) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    void reset() override {
        // Not applicable for system clock
    }
};

class SimulationApp {
public:
    SimulationApp() 
        : clock_(std::make_unique<SystemClock>()),
          display_(std::make_unique<CursesDisplay>()),
          input_(std::make_unique<CursesInput>(clock_.get())),
          running_(false) {
        
        // Initialize step sequencer with dependencies
        StepSequencer::Dependencies deps;
        deps.clock = clock_.get();
        sequencer_ = std::make_unique<StepSequencer>(deps);
    }
    
    ~SimulationApp() {
        shutdown();
    }
    
    void init() {
        display_->init();
        input_->init();
        
        // Initialize sequencer with 120 BPM, 8 steps
        sequencer_->init(120, 8);
        
        // Create a simple demo pattern
        setupDemoPattern();
        
        running_ = true;
    }
    
    void shutdown() {
        running_ = false;
        
        if (input_) input_->shutdown();
        if (display_) display_->shutdown();
    }
    
    void run() {
        init();
        
        const auto targetFrameTime = std::chrono::milliseconds(16); // ~60 FPS
        
        while (running_) {
            auto frameStart = std::chrono::steady_clock::now();
            
            // Process input events
            handleInput();
            
            // Update sequencer
            sequencer_->tick();
            
            // Update display
            updateDisplay();
            
            // Check for quit condition
            if (!running_) break;
            
            // Frame rate limiting
            auto frameEnd = std::chrono::steady_clock::now();
            auto frameTime = frameEnd - frameStart;
            if (frameTime < targetFrameTime) {
                std::this_thread::sleep_for(targetFrameTime - frameTime);
            }
        }
        
        shutdown();
    }

private:
    std::unique_ptr<SystemClock> clock_;
    std::unique_ptr<CursesDisplay> display_;
    std::unique_ptr<CursesInput> input_;
    std::unique_ptr<StepSequencer> sequencer_;
    bool running_;
    uint32_t lastStepTime_;
    
    void setupDemoPattern() {
        // Create a simple demo pattern for testing
        // Track 0 (Red): steps 0, 2, 4, 6
        sequencer_->toggleStep(0, 0);
        sequencer_->toggleStep(0, 2); 
        sequencer_->toggleStep(0, 4);
        sequencer_->toggleStep(0, 6);
        
        // Track 1 (Green): steps 1, 3, 5, 7
        sequencer_->toggleStep(1, 1);
        sequencer_->toggleStep(1, 3);
        sequencer_->toggleStep(1, 5);
        sequencer_->toggleStep(1, 7);
        
        // Track 2 (Blue): steps 0, 4
        sequencer_->toggleStep(2, 0);
        sequencer_->toggleStep(2, 4);
        
        // Track 3 (Yellow): step 2, 6
        sequencer_->toggleStep(3, 2);
        sequencer_->toggleStep(3, 6);
        
        // Start the sequencer
        sequencer_->start();
        lastStepTime_ = 0;
    }
    
    void handleInput() {
        if (!input_->pollEvents()) return;
        
        ButtonEvent event;
        while (input_->getNextEvent(event)) {
            // Check for quit event
            if (event.row == 255 && event.col == 255) {
                running_ = false;
                return;
            }
            
            // Handle button presses to toggle steps
            if (event.pressed && event.row < 4 && event.col < 8) {
                sequencer_->toggleStep(event.row, event.col);
            }
        }
    }
    
    void updateDisplay() {
        // Clear display
        display_->clear();
        
        // Get current step for highlighting
        uint8_t currentStep = sequencer_->getCurrentStep();
        bool isPlaying = sequencer_->isPlaying();
        
        // Update LED states based on sequencer pattern
        for (uint8_t track = 0; track < 4; track++) {
            for (uint8_t step = 0; step < 8; step++) {
                bool stepActive = sequencer_->isStepActive(track, step);
                bool isCurrentStep = (step == currentStep && isPlaying);
                
                uint8_t r = 0, g = 0, b = 0;
                
                if (stepActive) {
                    // Different colors for different tracks
                    switch (track) {
                        case 0: r = isCurrentStep ? 255 : 128; break;  // Red
                        case 1: g = isCurrentStep ? 255 : 128; break;  // Green  
                        case 2: b = isCurrentStep ? 255 : 128; break;  // Blue
                        case 3: r = g = isCurrentStep ? 255 : 128; break; // Yellow
                    }
                } else if (isCurrentStep) {
                    // Dim white for current step when not active
                    r = g = b = 32;
                }
                
                display_->setLED(track, step, r, g, b);
            }
        }
        
        display_->refresh();
    }
};

int main() {
    try {
        SimulationApp app;
        app.run();
    } catch (const std::exception& e) {
        // Clean up ncurses in case of error
        endwin();
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}