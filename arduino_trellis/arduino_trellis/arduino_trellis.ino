// NeoTrellis M4 Step Sequencer - Thin Hardware Interface
// This file contains ONLY device-specific hardware translation
// All business logic is in src/core/

#include <Adafruit_NeoTrellisM4.h>
#include <MIDIUSB.h>

// Parameter lock functionality (forward declaration)
struct ParameterLock {
    uint8_t noteOffset;      // -12 to +12 semitone offset
    uint8_t velocityOffset;  // 0-127 velocity override
    bool active;
    
    ParameterLock() : noteOffset(0), velocityOffset(127), active(false) {}
};

// Hardware objects
Adafruit_NeoTrellisM4 trellis;

// Simple Arduino clock implementation for core
class ArduinoSystemClock {
public:
    uint32_t getCurrentTime() const {
        return millis();
    }
    
    void delay(uint32_t milliseconds) {
        ::delay(milliseconds);
    }
    
    void reset() {
        // No-op for Arduino millis()
    }
};

// Simple Arduino MIDI Output implementation
class ArduinoMidiOutput {
public:
    ArduinoMidiOutput() : connected_(true) {}
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!connected_) return;
        
        midiEventPacket_t noteOn = {0x09, static_cast<uint8_t>(0x90 | channel), note, velocity};
        MidiUSB.sendMIDI(noteOn);
        MidiUSB.flush();
        
        Serial.print("MIDI Note On: CH");
        Serial.print(channel + 1);
        Serial.print(" Note:");
        Serial.print(note);
        Serial.print(" Vel:");
        Serial.println(velocity);
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
        if (!connected_) return;
        
        midiEventPacket_t noteOff = {0x08, static_cast<uint8_t>(0x80 | channel), note, velocity};
        MidiUSB.sendMIDI(noteOff);
        MidiUSB.flush();
    }
    
    void sendStart() {
        if (!connected_) return;
        
        midiEventPacket_t startMsg = {0x0F, 0xFA, 0, 0};
        MidiUSB.sendMIDI(startMsg);
        MidiUSB.flush();
        Serial.println("MIDI: Start sent");
    }
    
    void sendStop() {
        if (!connected_) return;
        
        midiEventPacket_t stopMsg = {0x0F, 0xFC, 0, 0};
        MidiUSB.sendMIDI(stopMsg);
        MidiUSB.flush();
        Serial.println("MIDI: Stop sent");
    }
    
    bool isConnected() const {
        return connected_;
    }
    
    void flush() {
        MidiUSB.flush();
    }

private:
    bool connected_;
};

// Simple core application logic embedded directly
class StepSequencer {
private:
    static const int MAX_TRACKS = 4;
    static const int MAX_STEPS = 8;
    
    bool pattern_[MAX_TRACKS][MAX_STEPS];
    uint8_t trackMidiNotes_[MAX_TRACKS];
    uint8_t trackMidiChannels_[MAX_TRACKS];
    uint8_t trackVolumes_[MAX_TRACKS];
    
    uint16_t bpm_;
    uint8_t stepCount_;
    uint8_t currentStep_;
    bool playing_;
    unsigned long lastStepTime_;
    unsigned long stepInterval_;
    
    ArduinoMidiOutput* midiOutput_;
    
    // Parameter lock storage
    ParameterLock parameterLocks_[MAX_TRACKS][MAX_STEPS];
    
    // Parameter lock state
    bool parameterLockMode_;
    uint8_t parameterLockTrack_;
    uint8_t parameterLockStep_;
    
public:
    StepSequencer(ArduinoMidiOutput* midi = nullptr) 
        : bpm_(120)
        , stepCount_(MAX_STEPS)
        , currentStep_(0)
        , playing_(true)
        , lastStepTime_(0)
        , midiOutput_(midi)
        , parameterLockMode_(false)
        , parameterLockTrack_(0)
        , parameterLockStep_(0) {
        
        // Clear pattern
        for (int track = 0; track < MAX_TRACKS; track++) {
            for (int step = 0; step < MAX_STEPS; step++) {
                pattern_[track][step] = false;
            }
            trackMidiNotes_[track] = 36 + track;  // C2, C#2, D2, D#2
            trackMidiChannels_[track] = 9;        // MIDI channel 10 (0-based)
            trackVolumes_[track] = 127;
        }
        
        calculateStepInterval();
    }
    
    void init(uint16_t bpm, uint8_t stepCount) {
        bpm_ = bpm;
        stepCount_ = (stepCount > MAX_STEPS) ? MAX_STEPS : stepCount;
        calculateStepInterval();
        reset();
    }
    
    void calculateStepInterval() {
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
        sendMidiTriggers();
    }
    
    void sendMidiTriggers() {
        if (!midiOutput_ || !midiOutput_->isConnected()) {
            return;
        }
        
        for (uint8_t track = 0; track < MAX_TRACKS; track++) {
            if (pattern_[track][currentStep_]) {
                // Check for parameter locks on this step
                uint8_t midiNote = trackMidiNotes_[track];
                uint8_t velocity = trackVolumes_[track];
                
                ParameterLock& lock = parameterLocks_[track][currentStep_];
                if (lock.active) {
                    // Apply parameter lock overrides
                    midiNote += lock.noteOffset;
                    velocity = lock.velocityOffset;
                    
                    // Clamp values to valid MIDI range
                    if (midiNote > 127) midiNote = 127;
                    if (midiNote < 0) midiNote = 0;
                }
                
                midiOutput_->sendNoteOn(
                    trackMidiChannels_[track],
                    midiNote,
                    velocity
                );
            }
        }
    }
    
    void reset() {
        currentStep_ = 0;
        lastStepTime_ = millis();
    }
    
    void start() {
        playing_ = true;
        lastStepTime_ = millis();
        if (midiOutput_) midiOutput_->sendStart();
    }
    
    void stop() {
        playing_ = false;
        currentStep_ = 0;
        if (midiOutput_) midiOutput_->sendStop();
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
    
    void toggleStep(uint8_t track, uint8_t step) {
        if (track < MAX_TRACKS && step < MAX_STEPS) {
            pattern_[track][step] = !pattern_[track][step];
        }
    }
    
    bool isStepActive(uint8_t track, uint8_t step) const {
        if (track < MAX_TRACKS && step < MAX_STEPS) {
            return pattern_[track][step];
        }
        return false;
    }
    
    void setTrackMidiNote(uint8_t track, uint8_t note) {
        if (track < MAX_TRACKS) {
            trackMidiNotes_[track] = note;
        }
    }
    
    uint8_t getTrackMidiNote(uint8_t track) const {
        return (track < MAX_TRACKS) ? trackMidiNotes_[track] : 0;
    }
    
    uint8_t getCurrentStep() const { return currentStep_; }
    bool isPlaying() const { return playing_; }
    uint16_t getBPM() const { return bpm_; }
    
    void setBPM(uint16_t bpm) {
        bpm_ = bpm;
        calculateStepInterval();
    }
    
    // Setup demo pattern with proper MIDI notes
    void setupDemoPattern() {
        // Configure MIDI notes for drum sounds
        trackMidiNotes_[0] = 36;  // Kick drum (C2)
        trackMidiNotes_[1] = 38;  // Snare (D2)
        trackMidiNotes_[2] = 42;  // Closed hi-hat (F#2)
        trackMidiNotes_[3] = 46;  // Open hi-hat (A#2)
        
        // Create basic drum pattern
        pattern_[0][0] = pattern_[0][4] = true;  // Kick on 1, 5
        pattern_[1][2] = pattern_[1][6] = true;  // Snare on 3, 7
        pattern_[2][1] = pattern_[2][3] = pattern_[2][5] = pattern_[2][7] = true;  // Hi-hat offbeats
        pattern_[3][3] = true;  // Open hi-hat on 4
    }
    
    // Parameter lock functionality
    bool enterParameterLockMode(uint8_t track, uint8_t step) {
        if (track >= MAX_TRACKS || step >= MAX_STEPS) return false;
        
        parameterLockMode_ = true;
        parameterLockTrack_ = track;
        parameterLockStep_ = step;
        
        Serial.print("PARAMETER LOCK: Entered mode for track ");
        Serial.print(track);
        Serial.print(", step ");
        Serial.println(step);
        
        return true;
    }
    
    void exitParameterLockMode() {
        parameterLockMode_ = false;
        Serial.println("PARAMETER LOCK: Exited mode");
    }
    
    bool isInParameterLockMode() const {
        return parameterLockMode_;
    }
    
    void adjustParameter(uint8_t controlType, int8_t adjustment) {
        if (!parameterLockMode_) return;
        
        ParameterLock& lock = parameterLocks_[parameterLockTrack_][parameterLockStep_];
        lock.active = true;
        
        switch (controlType) {
            case 0: // Note up
                lock.noteOffset += adjustment;
                if (lock.noteOffset > 12) lock.noteOffset = 12;
                Serial.print("NOTE OFFSET: ");
                Serial.println((int8_t)lock.noteOffset);
                break;
                
            case 1: // Note down  
                lock.noteOffset += adjustment;
                if (lock.noteOffset < -12) lock.noteOffset = -12;
                Serial.print("NOTE OFFSET: ");
                Serial.println((int8_t)lock.noteOffset);
                break;
                
            case 2: // Velocity up
                lock.velocityOffset += adjustment * 8;
                if (lock.velocityOffset > 127) lock.velocityOffset = 127;
                Serial.print("VELOCITY: ");
                Serial.println(lock.velocityOffset);
                break;
                
            case 3: // Velocity down
                lock.velocityOffset += adjustment * 8; 
                if (lock.velocityOffset < 1) lock.velocityOffset = 1;
                Serial.print("VELOCITY: ");
                Serial.println(lock.velocityOffset);
                break;
        }
    }
};

// Hold detection for parameter locks
class HoldDetector {
private:
    struct ButtonState {
        unsigned long pressTime;
        bool pressed;
        bool holdProcessed;
        
        ButtonState() : pressTime(0), pressed(false), holdProcessed(false) {}
    };
    
    static const unsigned long HOLD_THRESHOLD_MS = 500;
    ButtonState states_[32];
    
public:
    void buttonPressed(uint8_t button) {
        if (button >= 32) return;
        states_[button].pressed = true;
        states_[button].pressTime = millis();
        states_[button].holdProcessed = false;
    }
    
    void buttonReleased(uint8_t button) {
        if (button >= 32) return;
        states_[button].pressed = false;
        states_[button].holdProcessed = false;
    }
    
    bool checkForHold(uint8_t button) {
        if (button >= 32) return false;
        
        ButtonState& state = states_[button];
        if (!state.pressed || state.holdProcessed) return false;
        
        unsigned long currentTime = millis();
        if (currentTime - state.pressTime >= HOLD_THRESHOLD_MS) {
            state.holdProcessed = true;
            return true;
        }
        return false;
    }
    
    bool isPressed(uint8_t button) {
        return (button < 32) ? states_[button].pressed : false;
    }
};

// Simple shift controls embedded
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
        }
        else if (isControlKey(row, col) && pressed && shiftActive_) {
            startStopTriggered_ = true;
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
ArduinoMidiOutput midiOutput;
StepSequencer sequencer(&midiOutput);
ShiftControls shiftControls;
HoldDetector holdDetector;

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
    while (!Serial && millis() < 5000) delay(100);
    
    Serial.println("=== NeoTrellis M4 Step Sequencer with MIDI ===");
    
    // Initialize hardware
    trellis.begin();
    
    trellis.setBrightness(80);
    trellis.fill(COLOR_OFF);
    trellis.show();
    
    // Initialize sequencer with MIDI support
    sequencer.init(120, 8); // 120 BPM, 8 steps
    sequencer.setupDemoPattern();
    sequencer.start();
    
    Serial.println("Hardware initialized successfully!");
    Serial.print("- BPM: "); Serial.println(sequencer.getBPM());
    Serial.println("- Tracks: 4 (Red=Kick, Green=Snare, Blue=Hi-hat, Yellow=Open HH)");
    Serial.println("- Steps: 8");
    Serial.println("- USB MIDI enabled on channel 10");
    Serial.println("- Shift (bottom-left) + bottom-right for play/stop");
    Serial.println("- Press buttons to toggle steps");
    Serial.println("- PARAMETER LOCKS: Hold any step button for 500ms");
    Serial.println("- In param lock mode: Right-side buttons adjust NOTE/VELOCITY");
    Serial.println("- Columns 4-5: Note up/down | Columns 6-7: Velocity up/down");
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
    
    // Check for holds first (continuous check)
    for (int i = 0; i < 32; i++) {
        if (holdDetector.checkForHold(i)) {
            int row = i / 8;
            int col = i % 8;
            
            // Only enter parameter lock mode for step buttons (not shift controls)
            if (row < 4 && col < 8 && !shiftControls.shouldHandleAsShiftControl(row, col)) {
                sequencer.enterParameterLockMode(row, col);
            }
        }
    }
    
    // Check each key individually since NeoTrellis M4 library 
    // doesn't have the same API as regular NeoTrellis
    for (int i = 0; i < 32; i++) {
        bool currentPressed = trellis.justPressed(i);
        bool currentReleased = trellis.justReleased(i);
        
        if (currentPressed || currentReleased) {
            int row = i / 8;  // 4 rows
            int col = i % 8;  // 8 columns
            
            // Update hold detector
            if (currentPressed) {
                holdDetector.buttonPressed(i);
            } else if (currentReleased) {
                holdDetector.buttonReleased(i);
                
                // Exit parameter lock mode when step button is released
                if (sequencer.isInParameterLockMode() && row < 4 && col < 8) {
                    sequencer.exitParameterLockMode();
                }
            }
            
            // Handle shift controls first
            if (shiftControls.shouldHandleAsShiftControl(row, col)) {
                shiftControls.handleInput(row, col, currentPressed);
                
                // Check for start/stop trigger
                if (shiftControls.getAndClearStartStopTrigger()) {
                    sequencer.togglePlayback();
                }
                continue; // Skip normal step processing
            }
            
            // Parameter lock control grid (only when in parameter lock mode)
            if (sequencer.isInParameterLockMode() && currentPressed) {
                // Simple control mapping for right-side buttons when in parameter lock mode
                if (col >= 4) { // Right side columns (4-7)
                    uint8_t controlType = (row * 2) + ((col >= 6) ? 1 : 0); // Map to 0-7 controls
                    int8_t adjustment = (col == 4 || col == 6) ? 1 : -1;     // Up/down based on column
                    
                    sequencer.adjustParameter(controlType % 4, adjustment);
                }
                continue; // Skip normal step processing when in parameter lock mode
            }
            
            // Normal step sequencer input (only on press, only in sequencer area)
            if (currentPressed && row < 4 && col < 8 && !sequencer.isInParameterLockMode()) {
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
    
    // Parameter lock visual feedback
    if (sequencer.isInParameterLockMode()) {
        // Flash all right-side buttons (control grid) in cyan to indicate parameter lock mode
        uint32_t paramLockColor = (millis() / 200) % 2 ? 0x004040 : 0x002020;
        
        for (int row = 0; row < 4; row++) {
            for (int col = 4; col < 8; col++) { // Right side columns
                int ledIndex = row * 8 + col;
                trellis.setPixelColor(ledIndex, paramLockColor);
            }
        }
        
        // Highlight the held step button in bright white
        // (This will be overridden by the held button logic above)
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