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

class SimpleSimulationApp {
public:
    SimpleSimulationApp() : running_(false) {
        clock_ = std::make_unique<SystemClock>();
        display_ = std::make_unique<CursesDisplay>();
        input_ = std::make_unique<CursesInput>(clock_.get());
        
        // Initialize step sequencer with dependencies
        StepSequencer::Dependencies deps;
        deps.clock = clock_.get();
        deps.display = display_.get();
        deps.input = input_.get();
        // midiOutput and midiInput are nullptr for simulation
        sequencer_ = std::make_unique<StepSequencer>(deps);
        
        // Initialize shift controls
        ShiftControls::Dependencies shiftDeps;
        shiftDeps.clock = clock_.get();
        shiftControls_ = std::make_unique<ShiftControls>(shiftDeps);
    }
    
    ~SimpleSimulationApp() {
        shutdown();
    }
    
    void init() {
        // Initialize display first (sets up ncurses)
        display_->init();
        
        // Then initialize input after ncurses is ready
        input_->init();
        
        // Initialize sequencer with 120 BPM, 8 steps
        sequencer_->init(120, 8);
        
        running_ = true;
    }
    
    void shutdown() {
        running_ = false;
        
        if (input_) input_->shutdown();
        if (display_) display_->shutdown();
    }
    
    void run() {
        init();
        runSimulation();
        shutdown();
    }

private:
    std::unique_ptr<SystemClock> clock_;
    std::unique_ptr<CursesDisplay> display_;
    std::unique_ptr<CursesInput> input_;
    std::unique_ptr<StepSequencer> sequencer_;
    std::unique_ptr<ShiftControls> shiftControls_;
    bool running_;
    
    void runSimulation() {
        const auto targetFrameTime = std::chrono::milliseconds(16); // ~60 FPS
        
        while (running_) {
            auto frameStart = std::chrono::steady_clock::now();
            
            // Process input events
            processInput();
            
            // Update sequencer (handles timing, parameter locks, etc.)
            sequencer_->tick();
            
            // Shift controls are event-driven, no tick needed
            
            // Update display
            display_->show();
            
            // Check for quit condition
            if (!running_) break;
            
            // Frame rate limiting
            auto frameEnd = std::chrono::steady_clock::now();
            auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
            auto sleepTime = targetFrameTime - frameDuration;
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }
    
    void processInput() {
        if (!input_->pollEvents()) {
            return;
        }
        
        ButtonEvent event;
        while (input_->getNextEvent(event)) {
            // Check for quit event
            if (event.row == 255 && event.col == 255) {
                running_ = false;
                return;
            }
            
            uint8_t buttonIndex = event.row * 8 + event.col;
            uint32_t currentTime = clock_->getCurrentTime();
            
            // Handle shift controls first
            ButtonEvent buttonEvent = {event.row, event.col, event.pressed, currentTime};
            if (shiftControls_->shouldHandleAsControl(event.row, event.col)) {
                shiftControls_->handleShiftInput(buttonEvent);
                
                // If it's start/stop and shift is held, handle it
                if (event.row == 3 && event.col == 7 && event.pressed && shiftControls_->isShiftActive()) {
                    if (sequencer_->isPlaying()) {
                        sequencer_->stop();
                    } else {
                        sequencer_->start();
                    }
                }
            } else {
                // Let sequencer handle normal button events
                sequencer_->handleButton(buttonIndex, event.pressed, currentTime);
            }
        }
    }
};

int main(int argc, char* argv[]) {
    SimpleSimulationApp app;
    app.run();
    return 0;
}