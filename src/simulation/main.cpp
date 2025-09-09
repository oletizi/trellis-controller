/*
 * Trellis Controller Simulation
 * 
 * ARCHITECTURAL NOTE: Proper Dependency Injection Pattern (Phase 3.2)
 * 
 * The correct architecture uses InputController instead of direct IInput:
 * 
 * struct ApplicationDependencies {
 *     IClock* clock;
 *     IDisplay* display;
 *     InputController* inputController;  // Instead of IInput*
 *     IMidiOutput* midiOutput;
 *     IDebugOutput* debugOutput;
 * };
 * 
 * InputController construction pattern:
 * auto inputLayer = InputLayerFactory::createInputLayerForCurrentPlatform(config, inputDeps);
 * auto gestureDetector = std::make_unique<GestureDetector>(config, clock, debug);
 * InputController::Dependencies controllerDeps{
 *     .inputLayer = std::move(inputLayer),
 *     .gestureDetector = std::move(gestureDetector),
 *     .clock = clock,
 *     .debugOutput = debug
 * };
 * auto inputController = std::make_unique<InputController>(std::move(controllerDeps), config);
 */

#include "StepSequencer.h"
#include "ShiftControls.h"
#include "CursesDisplay.h"
#include "IClock.h"
#include "ControlMessage.h"
#include "ConsoleDebugOutput.h"
// New Input Layer Abstraction includes
#include "InputController.h"
#include "CursesInputLayer.h"
#include "GestureDetector.h"
#include "InputStateEncoder.h"
#include "InputStateProcessor.h"
#include "InputSystemConfiguration.h"
#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

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
          debugOutput_(nullptr),
          inputController_(nullptr),
          running_(false) {
        
        // Create debug output for console
        debugOutput_ = std::make_unique<ConsoleDebugOutput>(static_cast<CursesDisplay*>(display_.get()));
        
        // Set up Input Layer Abstraction architecture
        setupInputController();
        
        // Initialize step sequencer with dependencies
        StepSequencer::Dependencies deps;
        deps.clock = clock_.get();
        deps.display = display_.get();
        deps.debugOutput = debugOutput_.get();
        // Note: midiOutput and midiInput are nullptr for simulation
        // Note: input dependency removed - StepSequencer now uses semantic messages
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
        // CRITICAL: Initialize display first to set up ncurses properly
        display_->init();
        
        // Initialize InputController (which handles CursesInputLayer internally)
        if (!inputController_->initialize()) {
            throw std::runtime_error("Failed to initialize InputController");
        }
        
        // Initialize sequencer with 120 BPM, 8 steps
        sequencer_->init(120, 8);
        
        // Create a simple demo pattern
        setupDemoPattern();
        
        running_ = true;
    }
    
    void shutdown() {
        running_ = false;
        
        if (inputController_) {
            inputController_->shutdown();
        }
        
        if (display_) display_->shutdown();
    }
    
    void run() {
        init();
        runRealtime();
        shutdown();
    }

private:
    std::unique_ptr<SystemClock> clock_;
    std::unique_ptr<CursesDisplay> display_;
    std::unique_ptr<ConsoleDebugOutput> debugOutput_;
    std::unique_ptr<InputController> inputController_;
    std::unique_ptr<StepSequencer> sequencer_;
    std::unique_ptr<ShiftControls> shiftControls_;
    bool running_;
    uint32_t lastStepTime_;
    
    /**
     * @brief Set up the complete Input Layer Abstraction architecture
     * 
     * Creates the proper pipeline: CursesInputLayer → InputStateEncoder → InputStateProcessor → InputController
     * This uses the new bitwise state management system instead of the legacy GestureDetector.
     */
    void setupInputController() {
        // Create configuration optimized for simulation
        auto config = InputSystemConfiguration::forSimulation();
        
        // Create input layer dependencies
        InputLayerDependencies inputLayerDeps;
        inputLayerDeps.clock = clock_.get();
        inputLayerDeps.debugOutput = debugOutput_.get();
        
        // Create CursesInputLayer
        auto inputLayer = std::make_unique<CursesInputLayer>();
        
        // Create new bitwise state management components
        auto inputStateEncoder = std::make_unique<InputStateEncoder>(InputStateEncoder::Dependencies{
            .clock = clock_.get(),
            .debugOutput = debugOutput_.get()
        });
        
        auto inputStateProcessor = std::make_unique<InputStateProcessor>(InputStateProcessor::Dependencies{
            .clock = clock_.get(),
            .debugOutput = debugOutput_.get()
        });
        
        // Create InputController dependencies with new bitwise system
        InputController::Dependencies controllerDeps;
        controllerDeps.inputLayer = std::move(inputLayer);
        controllerDeps.gestureDetector = nullptr; // Remove legacy system
        controllerDeps.inputStateEncoder = std::move(inputStateEncoder);
        controllerDeps.inputStateProcessor = std::move(inputStateProcessor);
        controllerDeps.clock = clock_.get();
        controllerDeps.debugOutput = debugOutput_.get();
        
        // Create InputController
        inputController_ = std::make_unique<InputController>(std::move(controllerDeps), config);
    }
    
    void runRealtime() {
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
    }
    
    
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
        // NEW ARCHITECTURE: Use InputController for complete input processing
        
        // Poll InputController for new events and messages
        if (!inputController_->poll()) {
            // Handle polling error if needed
            return;
        }
        
        // Process all available control messages from InputController
        ControlMessage::Message controlMessage;
        while (inputController_->hasMessages() && inputController_->getNextMessage(controlMessage)) {
            
            // Check for system quit message
            if (controlMessage.type == ControlMessage::Type::SYSTEM_EVENT && 
                controlMessage.param1 == 255 && controlMessage.param2 == 255) {
                running_ = false;
                return;
            }
            
            // Process control message through sequencer
            sequencer_->processMessage(controlMessage);
            
            // Handle shift controls for legacy compatibility
            // Note: This is a bridge until shift controls are integrated into gesture detection
            handleLegacyShiftControls(controlMessage);
        }
    }
    
    /**
     * @brief Bridge function to handle legacy shift controls during transition
     * 
     * Eventually shift controls should be integrated into gesture detection,
     * but for now we translate ControlMessages back to ButtonEvents for compatibility.
     */
    void handleLegacyShiftControls(const ControlMessage::Message& controlMessage) {
        // Convert certain control messages to legacy ButtonEvent format for shift controls
        if (controlMessage.type == ControlMessage::Type::TOGGLE_STEP) {
            uint8_t track = controlMessage.param1;
            uint8_t step = controlMessage.param2;
            
            // Check if this corresponds to shift control positions
            if (shiftControls_->shouldHandleAsControl(track, step)) {
                // Create legacy ButtonEvent for shift controls
                ButtonEvent legacyEvent;
                legacyEvent.row = track;
                legacyEvent.col = step;
                legacyEvent.pressed = true;  // Assume press for toggle
                legacyEvent.timestamp = controlMessage.timestamp;
                
                shiftControls_->handleShiftInput(legacyEvent);
                
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