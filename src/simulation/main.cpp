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
#include "CursesInput.h"
#include "IClock.h"
#include "NonRealtimeSequencer.h"
#include "ControlMessage.h"
#include "ConsoleDebugOutput.h"
// New Input Layer Abstraction includes
#include "InputController.h"
#include "CursesInputLayer.h"
#include "GestureDetector.h"
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
    SimulationApp(bool useRealtimeMode = true) 
        : useRealtimeMode_(useRealtimeMode),
          clock_(std::make_unique<SystemClock>()),
          display_(std::make_unique<CursesDisplay>()),
          input_(std::make_unique<CursesInput>(clock_.get())),
          debugOutput_(nullptr),
          inputController_(nullptr),
          running_(false) {
        
        if (useRealtimeMode_) {
            // Create debug output for console
            debugOutput_ = std::make_unique<ConsoleDebugOutput>(static_cast<CursesDisplay*>(display_.get()));
            
            // Set up Input Layer Abstraction architecture
            setupInputController();
            
            // Initialize step sequencer with dependencies for realtime mode
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
        } else {
            // Initialize non-realtime sequencer
            nonRealtimeSequencer_ = std::make_unique<NonRealtimeSequencer>();
            // nonRealtimeSequencer_->setLogStream(&std::cout);
            nonRealtimeSequencer_->setVerbose(false);
        }
    }
    
    ~SimulationApp() {
        shutdown();
    }
    
    void init() {
        // CRITICAL: Initialize display first to set up ncurses properly
        display_->init();
        
        if (useRealtimeMode_) {
            // Initialize InputController (which handles CursesInputLayer internally)
            if (!inputController_->initialize()) {
                throw std::runtime_error("Failed to initialize InputController");
            }
            
            // Initialize sequencer with 120 BPM, 8 steps
            sequencer_->init(120, 8);
            
            // Create a simple demo pattern
            setupDemoPattern();
        } else {
            // For non-realtime mode, still use legacy CursesInput
            input_->init();
            
            // Initialize non-realtime sequencer
            nonRealtimeSequencer_->init(120, 8);
            
            // Create demo pattern using non-realtime commands
            setupNonRealtimeDemoPattern();
            
            // Show instructions for non-realtime mode
            showNonRealtimeInstructions();
        }
        
        running_ = true;
    }
    
    void shutdown() {
        running_ = false;
        
        if (useRealtimeMode_ && inputController_) {
            inputController_->shutdown();
        } else if (input_) {
            input_->shutdown();
        }
        
        if (display_) display_->shutdown();
    }
    
    void run() {
        init();
        
        if (useRealtimeMode_) {
            runRealtime();
        } else {
            runNonRealtime();
        }
        
        shutdown();
    }

private:
    bool useRealtimeMode_;
    std::unique_ptr<SystemClock> clock_;
    std::unique_ptr<CursesDisplay> display_;
    std::unique_ptr<CursesInput> input_;  // Keep legacy for non-realtime mode
    std::unique_ptr<ConsoleDebugOutput> debugOutput_;
    std::unique_ptr<InputController> inputController_;  // New Input Controller
    std::unique_ptr<StepSequencer> sequencer_;
    std::unique_ptr<ShiftControls> shiftControls_;
    std::unique_ptr<NonRealtimeSequencer> nonRealtimeSequencer_;
    bool running_;
    uint32_t lastStepTime_;
    
    /**
     * @brief Set up the complete Input Layer Abstraction architecture
     * 
     * Creates the proper pipeline: CursesInputLayer → GestureDetector → InputController
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
        
        // Create GestureDetector
        auto gestureDetector = std::make_unique<GestureDetector>(config, clock_.get(), debugOutput_.get());
        
        // Create InputController dependencies
        InputController::Dependencies controllerDeps;
        controllerDeps.inputLayer = std::move(inputLayer);
        controllerDeps.gestureDetector = std::move(gestureDetector);
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
    
    void runNonRealtime() {
        // Non-realtime mode: process input commands and execute them discretely
        while (running_) {
            // Process input events
            if (!input_->pollEvents()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            
            ButtonEvent event;
            while (input_->getNextEvent(event)) {
                // Check for quit event
                if (event.row == 255 && event.col == 255) {
                    running_ = false;
                    return;
                }
                
                // Convert button events to control messages and process them
                if (event.row < 4 && event.col < 8) {
                    uint8_t buttonIndex = event.row * 8 + event.col;
                    
                    ControlMessage::Message message;
                    message.type = event.pressed ? ControlMessage::Type::KEY_PRESS : ControlMessage::Type::KEY_RELEASE;
                    message.timestamp = event.timestamp;
                    message.param1 = buttonIndex;
                    
                    auto result = nonRealtimeSequencer_->processMessage(message);
                    
                    // Commented out debug output to avoid corrupting ncurses display
                    // if (!result.success) {
                    //     std::cout << "Error: " << result.errorMessage << std::endl;
                    // } else if (!result.output.empty()) {
                    //     std::cout << "Action: " << result.output << std::endl;
                    // }
                }
                
                // Handle special commands
                handleNonRealtimeCommands(event);
            }
            
            // Update display using the sequencer's current state
            updateNonRealtimeDisplay();
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
    
    void setupNonRealtimeDemoPattern() {
        // Create demo pattern using non-realtime messages
        std::vector<ControlMessage::Message> setupMessages = {
            {ControlMessage::Type::KEY_PRESS, 100, 0, 0, ""}, // Track 0, step 0
            {ControlMessage::Type::KEY_RELEASE, 200, 0, 0, ""},
            {ControlMessage::Type::KEY_PRESS, 300, 2, 0, ""}, // Track 0, step 2
            {ControlMessage::Type::KEY_RELEASE, 400, 2, 0, ""},
            {ControlMessage::Type::KEY_PRESS, 500, 4, 0, ""}, // Track 0, step 4
            {ControlMessage::Type::KEY_RELEASE, 600, 4, 0, ""},
            {ControlMessage::Type::KEY_PRESS, 700, 6, 0, ""}, // Track 0, step 6
            {ControlMessage::Type::KEY_RELEASE, 800, 6, 0, ""},
            
            {ControlMessage::Type::KEY_PRESS, 900, 9, 0, ""}, // Track 1, step 1
            {ControlMessage::Type::KEY_RELEASE, 1000, 9, 0, ""},
            {ControlMessage::Type::KEY_PRESS, 1100, 11, 0, ""}, // Track 1, step 3
            {ControlMessage::Type::KEY_RELEASE, 1200, 11, 0, ""},
            
            {ControlMessage::Type::START, 1500, 0, 0, ""} // Start sequencer
        };
        
        auto result = nonRealtimeSequencer_->processBatch(setupMessages);
        // Commented out debug output to avoid corrupting ncurses display
        // if (!result.allSucceeded) {
        //     std::cout << "Warning: Some demo pattern setup failed" << std::endl;
        // }
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
    
    void handleNonRealtimeCommands(const ButtonEvent& event) {
        // Handle special non-realtime commands
        if (event.pressed && event.row == 3) {
            switch (event.col) {
                case 0: // 'z' - Start/Stop toggle
                    {
                        auto state = nonRealtimeSequencer_->getCurrentState();
                        ControlMessage::Message msg;
                        msg.type = state.playing ? ControlMessage::Type::STOP : ControlMessage::Type::START;
                        msg.timestamp = event.timestamp;
                        
                        auto result = nonRealtimeSequencer_->processMessage(msg);
                        // Commented out debug output to avoid corrupting ncurses display
                        // if (!result.success) {
                        //     std::cout << "Error: " << result.errorMessage << std::endl;
                        // } else {
                        //     std::cout << result.output << std::endl;
                        // }
                    }
                    break;
                    
                case 1: // 'x' - Step forward
                    {
                        ControlMessage::Message msg;
                        msg.type = ControlMessage::Type::CLOCK_TICK;
                        msg.timestamp = event.timestamp;
                        msg.param1 = 1;
                        
                        auto result = nonRealtimeSequencer_->processMessage(msg);
                        // Commented out debug output to avoid corrupting ncurses display
                        // if (!result.success) {
                        //     std::cout << "Error: " << result.errorMessage << std::endl;
                        // } else {
                        //     std::cout << "Manual tick: " << result.output << std::endl;
                        // }
                    }
                    break;
                    
                case 2: // 'c' - Reset
                    {
                        ControlMessage::Message msg;
                        msg.type = ControlMessage::Type::RESET;
                        msg.timestamp = event.timestamp;
                        
                        auto result = nonRealtimeSequencer_->processMessage(msg);
                        // Commented out debug output to avoid corrupting ncurses display
                        // if (!result.success) {
                        //     std::cout << "Error: " << result.errorMessage << std::endl;
                        // } else {
                        //     std::cout << "Reset: " << result.output << std::endl;
                        // }
                    }
                    break;
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
    
    void updateNonRealtimeDisplay() {
        // Clear display
        display_->clear();
        
        // Get current state from non-realtime sequencer
        auto state = nonRealtimeSequencer_->getCurrentState();
        
        // Manually draw the sequencer state
        drawSequencerState(state);
        
        display_->refresh();
    }
    
    void drawSequencerState(const SequencerState::Snapshot& state) {
        // Track colors
        const uint8_t trackColors[4][3] = {
            {255, 0, 0},   // Red
            {0, 255, 0},   // Green
            {0, 0, 255},   // Blue
            {255, 255, 0}  // Yellow
        };
        
        // Draw pattern steps
        for (int track = 0; track < 4; ++track) {
            for (int step = 0; step < 8; ++step) {
                uint8_t row = static_cast<uint8_t>(track);
                uint8_t col = static_cast<uint8_t>(step);
                
                if (state.pattern[track][step].active) {
                    // Active step - use full brightness
                    uint8_t brightness = (state.playing && state.currentStep == step) ? 255 : 128;
                    display_->setLED(row, col, 
                        (trackColors[track][0] * brightness) / 255,
                        (trackColors[track][1] * brightness) / 255,
                        (trackColors[track][2] * brightness) / 255);
                } else {
                    // Inactive step - dim or off
                    uint8_t brightness = (state.playing && state.currentStep == step) ? 64 : 0;
                    display_->setLED(row, col, brightness, brightness, brightness);
                }
            }
        }
        
        // Show parameter lock status
        if (state.inParameterLockMode && state.heldTrack < 4 && state.heldStep < 8) {
            // Highlight the held button in white
            display_->setLED(state.heldTrack, state.heldStep, 255, 255, 255);
        }
    }
    
    void showNonRealtimeInstructions() {
        // Commented out debug output to avoid corrupting ncurses display
        // Instructions are shown in the ncurses display instead
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

int main(int argc, char* argv[]) {
    try {
        bool useRealtimeMode = true;
        
        // Check for non-realtime mode flag
        if (argc > 1 && std::string(argv[1]) == "--non-realtime") {
            useRealtimeMode = false;
        }
        
        // Commented out debug output to avoid corrupting ncurses display
        // if (!useRealtimeMode) {
        //     std::cout << "Starting simulation in NON-REALTIME mode..." << std::endl;
        // } else {
        //     std::cout << "Starting simulation in REALTIME mode..." << std::endl;
        // }
        
        SimulationApp app(useRealtimeMode);
        app.run();
    } catch (const std::exception& e) {
        // Clean up ncurses in case of error
        endwin();
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}