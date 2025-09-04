// NeoTrellis M4 Step Sequencer
// Full implementation with Arduino framework

#include <Adafruit_NeoTrellisM4.h>

// Create the trellis object
Adafruit_NeoTrellisM4 trellis;

// Step Sequencer Implementation (ported from core logic)
class StepSequencer {
private:
    static const int MAX_TRACKS = 4;
    static const int MAX_STEPS = 8;
    
    bool pattern_[MAX_TRACKS][MAX_STEPS];
    int bpm_;
    int stepCount_;
    int currentStep_;
    bool playing_;
    unsigned long lastStepTime_;
    unsigned long stepInterval_;
    
public:
    StepSequencer() 
        : bpm_(120)
        , stepCount_(MAX_STEPS)
        , currentStep_(0)
        , playing_(true)
        , lastStepTime_(0) {
        
        // Clear pattern
        for (int track = 0; track < MAX_TRACKS; track++) {
            for (int step = 0; step < MAX_STEPS; step++) {
                pattern_[track][step] = false;
            }
        }
        
        calculateStepInterval();
    }
    
    void init(int bpm, int stepCount) {
        bpm_ = bpm;
        stepCount_ = (stepCount > MAX_STEPS) ? MAX_STEPS : stepCount;
        calculateStepInterval();
        reset();
    }
    
    void calculateStepInterval() {
        // Convert BPM to milliseconds between steps
        // For 16th notes: 60000ms / (BPM * 4)
        stepInterval_ = 60000 / (bpm_ * 2); // 8th notes for 8-step pattern
    }
    
    void tick() {
        if (!playing_) return;
        
        unsigned long currentTime = millis();
        if (currentTime - lastStepTime_ >= stepInterval_) {
            advance();
            lastStepTime_ = currentTime;
        }
    }
    
    void advance() {
        currentStep_ = (currentStep_ + 1) % stepCount_;
    }
    
    void reset() {
        currentStep_ = 0;
        lastStepTime_ = millis();
    }
    
    void start() {
        playing_ = true;
        lastStepTime_ = millis();
    }
    
    void stop() {
        playing_ = false;
    }
    
    void togglePlayback() {
        if (playing_) {
            stop();
            Serial.println("Sequencer stopped");
        } else {
            start();
            Serial.println("Sequencer started");
        }
    }
    
    void toggleStep(int track, int step) {
        if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
            pattern_[track][step] = !pattern_[track][step];
        }
    }
    
    bool isStepActive(int track, int step) const {
        if (track >= 0 && track < MAX_TRACKS && step >= 0 && step < MAX_STEPS) {
            return pattern_[track][step];
        }
        return false;
    }
    
    int getCurrentStep() const { return currentStep_; }
    bool isPlaying() const { return playing_; }
    int getBPM() const { return bpm_; }
    
    void setBPM(int bpm) {
        bpm_ = bpm;
        calculateStepInterval();
    }
    
    // Setup demo pattern
    void setupDemoPattern() {
        // Track 0 (Red): Kick pattern - steps 0, 2, 4, 6
        pattern_[0][0] = pattern_[0][2] = pattern_[0][4] = pattern_[0][6] = true;
        
        // Track 1 (Green): Snare pattern - steps 1, 3, 5, 7  
        pattern_[1][1] = pattern_[1][3] = pattern_[1][5] = pattern_[1][7] = true;
        
        // Track 2 (Blue): Hi-hat pattern - steps 0, 4
        pattern_[2][0] = pattern_[2][4] = true;
        
        // Track 3 (Yellow): Accent pattern - steps 2, 6
        pattern_[3][2] = pattern_[3][6] = true;
    }
};

// Shift Controls Implementation (Arduino compatible)
class ShiftControls {
private:
    static const uint8_t SHIFT_ROW = 3;
    static const uint8_t SHIFT_COL = 0;  // Bottom-left key
    static const uint8_t CONTROL_ROW = 3;
    static const uint8_t CONTROL_COL = 7; // Bottom-right key
    
    bool shiftActive_;
    bool startStopTriggered_;
    
public:
    ShiftControls() : shiftActive_(false), startStopTriggered_(false) {}
    
    void handleInput(uint8_t row, uint8_t col, bool pressed) {
        if (isShiftKey(row, col)) {
            shiftActive_ = pressed;
            Serial.print("Shift key ");
            Serial.println(pressed ? "pressed" : "released");
        }
        else if (isControlKey(row, col) && pressed && shiftActive_) {
            startStopTriggered_ = true;
            Serial.println("Start/Stop triggered!");
        }
    }
    
    bool isShiftActive() const {
        return shiftActive_;
    }
    
    bool shouldHandleAsShiftControl(uint8_t row, uint8_t col) const {
        return isShiftKey(row, col) || isControlKey(row, col);
    }
    
    bool getAndClearStartStopTrigger() {
        bool triggered = startStopTriggered_;
        startStopTriggered_ = false;
        return triggered;
    }
    
private:
    bool isShiftKey(uint8_t row, uint8_t col) const {
        return (row == SHIFT_ROW && col == SHIFT_COL);
    }
    
    bool isControlKey(uint8_t row, uint8_t col) const {
        return (row == CONTROL_ROW && col == CONTROL_COL);
    }
};

// Global instances
ShiftControls shiftControls;
StepSequencer sequencer;

// Color definitions
#define COLOR_OFF     0x000000
#define COLOR_RED     0x400000  // Track 0 - Kick
#define COLOR_GREEN   0x004000  // Track 1 - Snare
#define COLOR_BLUE    0x000040  // Track 2 - Hi-hat
#define COLOR_YELLOW  0x404000  // Track 3 - Accent
#define COLOR_RED_BRIGHT    0xFF0000
#define COLOR_GREEN_BRIGHT  0x00FF00
#define COLOR_BLUE_BRIGHT   0x0000FF
#define COLOR_YELLOW_BRIGHT 0xFFFF00
#define COLOR_WHITE_DIM     0x101010

void setup() {
    Serial.begin(115200);
    Serial.println("=== NeoTrellis M4 Step Sequencer ===");
    
    // Initialize NeoTrellis M4
    trellis.begin();
    trellis.setBrightness(80);
    
    // Initialize sequencer
    sequencer.init(120, 8); // 120 BPM, 8 steps
    sequencer.setupDemoPattern();
    sequencer.start();
    
    // Clear all pixels initially
    trellis.fill(COLOR_OFF);
    trellis.show();
    
    Serial.println("Step Sequencer initialized:");
    Serial.print("- BPM: "); Serial.println(sequencer.getBPM());
    Serial.println("- Tracks: 4 (Red=Kick, Green=Snare, Blue=Hi-hat, Yellow=Accent)");
    Serial.println("- Steps: 8");
    Serial.println("- Controls: Press buttons to toggle steps");
    Serial.println("Ready!");
}

void loop() {
    // Update sequencer timing
    sequencer.tick();
    
    // Handle button input
    handleInput();
    
    // Update LED display
    updateDisplay();
    
    // Small delay for stability
    delay(10);
}

void handleInput() {
    // Check for button presses using keypad functionality
    trellis.tick();
    
    // Check each key individually since NeoTrellis M4 library 
    // doesn't have the same API as regular NeoTrellis
    static bool keyStates[32] = {false}; // Track previous states
    
    for (int i = 0; i < 32; i++) {
        bool currentPressed = trellis.justPressed(i);
        bool currentReleased = trellis.justReleased(i);
        
        if (currentPressed || currentReleased) {
            int row = i / 8;  // 4 rows
            int col = i % 8;  // 8 columns
            
            // Handle shift controls first
            if (shiftControls.shouldHandleAsShiftControl(row, col)) {
                shiftControls.handleInput(row, col, currentPressed);
                
                // Check for start/stop trigger
                if (shiftControls.getAndClearStartStopTrigger()) {
                    sequencer.togglePlayback();
                }
                continue; // Skip normal step processing
            }
            
            // Normal step sequencer input (only on press, only in sequencer area)
            if (currentPressed && row < 4 && col < 8) {
                sequencer.toggleStep(row, col);
                
                Serial.print("Toggled step [");
                Serial.print(row);
                Serial.print(",");
                Serial.print(col);
                Serial.print("] -> ");
                Serial.println(sequencer.isStepActive(row, col) ? "ON" : "OFF");
            }
        }
    }
}

void updateDisplay() {
    int currentStep = sequencer.getCurrentStep();
    bool isPlaying = sequencer.isPlaying();
    
    // Update all LEDs based on pattern and current step
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 8; col++) {
            int ledIndex = row * 8 + col;
            bool stepActive = sequencer.isStepActive(row, col);
            bool isCurrentStep = (col == currentStep && isPlaying);
            
            uint32_t color = COLOR_OFF;
            
            if (stepActive) {
                // Different colors for different tracks
                switch (row) {
                    case 0: color = isCurrentStep ? COLOR_RED_BRIGHT : COLOR_RED; break;
                    case 1: color = isCurrentStep ? COLOR_GREEN_BRIGHT : COLOR_GREEN; break;
                    case 2: color = isCurrentStep ? COLOR_BLUE_BRIGHT : COLOR_BLUE; break;
                    case 3: color = isCurrentStep ? COLOR_YELLOW_BRIGHT : COLOR_YELLOW; break;
                }
            } else if (isCurrentStep) {
                // Dim white for current step when not active
                color = COLOR_WHITE_DIM;
            }
            
            trellis.setPixelColor(ledIndex, color);
        }
    }
    
    // Add shift-key visual feedback
    if (shiftControls.isShiftActive()) {
        // Highlight shift key (bottom-left) in white
        trellis.setPixelColor(24, 0x404040);  // Row 3, Col 0 = index 24
        
        // Highlight control key (bottom-right) in dim yellow when shift active
        trellis.setPixelColor(31, 0x404000);  // Row 3, Col 7 = index 31
    }
    
    trellis.show();
}

// Debug function to print current pattern
void printPattern() {
    Serial.println("=== Current Pattern ===");
    for (int row = 0; row < 4; row++) {
        Serial.print("Track "); Serial.print(row); Serial.print(": ");
        for (int col = 0; col < 8; col++) {
            Serial.print(sequencer.isStepActive(row, col) ? "X" : "-");
        }
        Serial.println();
    }
    Serial.print("Current Step: "); Serial.println(sequencer.getCurrentStep());
    Serial.print("Playing: "); Serial.println(sequencer.isPlaying() ? "YES" : "NO");
}