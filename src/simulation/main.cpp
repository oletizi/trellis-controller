#include "StepSequencer.h"
#include "ShiftControls.h"
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
        deps.display = display_.get();
        deps.input = input_.get();
        // Note: midiOutput and midiInput are nullptr for simulation
        sequencer_ = std::make_unique<StepSequencer>(deps);
        
        // Initialize shift controls
        ShiftControls::Dependencies shiftDeps;
        shiftDeps.clock = clock_.get();
        shiftControls_ = std::make_unique<ShiftControls>(shiftDeps);
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
    std::unique_ptr<ShiftControls> shiftControls_;
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
            
            // Convert row/col to button index for parameter lock system
            if (event.row < 4 && event.col < 8) {
                uint8_t buttonIndex = event.row * 8 + event.col;
                uint32_t currentTime = clock_->getCurrentTime();
                
                // Use StepSequencer's parameter lock button handling
                sequencer_->handleButton(buttonIndex, event.pressed, currentTime);
            }
            
            // Handle shift controls first (for legacy compatibility)
            if (shiftControls_->shouldHandleAsControl(event.row, event.col)) {
                shiftControls_->handleShiftInput(event);
                
                // Check for triggered actions
                auto action = shiftControls_->getTriggeredAction();
                if (action == ShiftControls::ControlAction::StartStop) {
                    if (sequencer_->isPlaying()) {
                        sequencer_->stop();
                    } else {
                        sequencer_->start();
                    }
                    shiftControls_->clearTriggeredAction();
                }
                
                // Update shift key visual feedback
                updateShiftVisualFeedback();
            }
        }
    }
    
    void updateDisplay() {
        // Clear display
        display_->clear();
        
        // Let StepSequencer handle its own display updates (including parameter locks)
        sequencer_->updateDisplay();
        
        // Add shift-key visual feedback (legacy)
        updateShiftVisualFeedback();
        
        display_->refresh();
    }
    
    void updateShiftVisualFeedback() {
        if (shiftControls_->isShiftActive()) {
            // Highlight shift key (bottom-left) in white
            display_->setLED(3, 0, 128, 128, 128);
            
            // Highlight control key (bottom-right) in yellow when shift active
            display_->setLED(3, 7, 128, 128, 0);
        }
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