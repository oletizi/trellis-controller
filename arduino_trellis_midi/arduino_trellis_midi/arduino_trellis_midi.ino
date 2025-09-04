// NeoTrellis M4 Step Sequencer with MIDI Support
// Arduino implementation with USB MIDI class-compliant functionality

#include <Adafruit_NeoTrellisM4.h>
#include <MIDIUSB.h>
#include "IMidiOutput.h"
#include "IMidiInput.h"
#include "ArduinoMidiOutput.h"
#include "ArduinoMidiInput.h"

// Create the trellis object
Adafruit_NeoTrellisM4 trellis;

// Create MIDI objects
ArduinoMidiOutput midiOutput;
ArduinoMidiInput midiInput;

// Step Sequencer Implementation with MIDI support
class StepSequencer {
private:
    static const int MAX_TRACKS = 4;
    static const int MAX_STEPS = 8;
    
    bool pattern_[MAX_TRACKS][MAX_STEPS];
    uint8_t trackVolumes_[MAX_TRACKS];
    bool trackMutes_[MAX_TRACKS];
    uint8_t trackMidiNotes_[MAX_TRACKS];
    uint8_t trackMidiChannels_[MAX_TRACKS];
    
    int bpm_;
    int stepCount_;
    int currentStep_;
    bool playing_;
    bool midiSyncEnabled_;
    unsigned long lastStepTime_;
    unsigned long stepInterval_;
    
    IMidiOutput* midiOutput_;
    IMidiInput* midiInput_;
    
public:
    StepSequencer(IMidiOutput* midiOut = nullptr, IMidiInput* midiIn = nullptr) 
        : bpm_(120)
        , stepCount_(MAX_STEPS)
        , currentStep_(0)
        , playing_(true)
        , midiSyncEnabled_(false)
        , lastStepTime_(0)
        , midiOutput_(midiOut)
        , midiInput_(midiIn) {
        
        // Clear pattern
        for (int track = 0; track < MAX_TRACKS; track++) {
            for (int step = 0; step < MAX_STEPS; step++) {
                pattern_[track][step] = false;
            }
            trackVolumes_[track] = 127;
            trackMutes_[track] = false;
            trackMidiNotes_[track] = 36 + track;  // C2, C#2, D2, D#2 (drum notes)
            trackMidiChannels_[track] = 9;        // MIDI channel 10 (0-based = 9)
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
        
        // Process MIDI input first
        if (midiInput_) {
            midiInput_->processMidiInput();
        }
        
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
        
        // Send note on for active steps
        for (int track = 0; track < MAX_TRACKS; track++) {
            if (!trackMutes_[track] && pattern_[track][currentStep_]) {
                midiOutput_->sendNoteOn(
                    trackMidiChannels_[track],
                    trackMidiNotes_[track],
                    trackVolumes_[track]
                );
                
                Serial.print("MIDI: Track ");
                Serial.print(track);
                Serial.print(" Note On - CH:");
                Serial.print(trackMidiChannels_[track] + 1);
                Serial.print(" Note:");
                Serial.print(trackMidiNotes_[track]);
                Serial.print(" Vel:");
                Serial.println(trackVolumes_[track]);
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
        
        if (midiOutput_ && midiOutput_->isConnected()) {
            midiOutput_->sendStart();
            Serial.println("MIDI: Start sent");
        }
    }
    
    void stop() {
        playing_ = false;
        
        if (midiOutput_ && midiOutput_->isConnected()) {
            midiOutput_->sendStop();
            Serial.println("MIDI: Stop sent");
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
    
    // MIDI configuration methods
    void setTrackMidiNote(int track, uint8_t note) {
        if (track >= 0 && track < MAX_TRACKS) {
            trackMidiNotes_[track] = note;
        }
    }
    
    void setTrackMidiChannel(int track, uint8_t channel) {
        if (track >= 0 && track < MAX_TRACKS) {
            trackMidiChannels_[track] = channel;
        }
    }
    
    uint8_t getTrackMidiNote(int track) const {
        return (track >= 0 && track < MAX_TRACKS) ? trackMidiNotes_[track] : 0;
    }
    
    uint8_t getTrackMidiChannel(int track) const {
        return (track >= 0 && track < MAX_TRACKS) ? trackMidiChannels_[track] : 0;
    }
    
    void setTrackVolume(int track, uint8_t volume) {
        if (track >= 0 && track < MAX_TRACKS) {
            trackVolumes_[track] = volume;
        }
    }
    
    void setTrackMute(int track, bool mute) {
        if (track >= 0 && track < MAX_TRACKS) {
            trackMutes_[track] = mute;
        }
    }
    
    void setMidiSync(bool enabled) {
        midiSyncEnabled_ = enabled;
    }
    
    // Getters
    int getCurrentStep() const { return currentStep_; }
    bool isPlaying() const { return playing_; }
    int getBPM() const { return bpm_; }
    bool isMidiSync() const { return midiSyncEnabled_; }
    
    void setBPM(int bpm) {
        bpm_ = bpm;
        calculateStepInterval();
    }
    
    // Setup demo pattern with MIDI-optimized drum mapping
    void setupDemoPattern() {
        // Track 0 (Red): Kick pattern - MIDI note 36 (C2)
        trackMidiNotes_[0] = 36;  // Kick drum
        pattern_[0][0] = pattern_[0][4] = true;  // Steps 0, 4
        
        // Track 1 (Green): Snare pattern - MIDI note 38 (D2)  
        trackMidiNotes_[1] = 38;  // Snare drum
        pattern_[1][2] = pattern_[1][6] = true;  // Steps 2, 6
        
        // Track 2 (Blue): Hi-hat pattern - MIDI note 42 (F#2)
        trackMidiNotes_[2] = 42;  // Closed hi-hat
        pattern_[2][1] = pattern_[2][3] = pattern_[2][5] = pattern_[2][7] = true;  // Offbeats
        
        // Track 3 (Yellow): Open hi-hat - MIDI note 46 (A#2)
        trackMidiNotes_[3] = 46;  // Open hi-hat
        pattern_[3][3] = true;  // Step 3
        
        Serial.println("Demo pattern loaded:");
        Serial.println("- Track 0 (Red): Kick drum (C2) - steps 0,4");
        Serial.println("- Track 1 (Green): Snare (D2) - steps 2,6");
        Serial.println("- Track 2 (Blue): Closed hi-hat (F#2) - steps 1,3,5,7");
        Serial.println("- Track 3 (Yellow): Open hi-hat (A#2) - step 3");
    }
};

// Global sequencer instance with MIDI
StepSequencer sequencer(&midiOutput, &midiInput);

// Color definitions
#define COLOR_OFF     0x000000
#define COLOR_RED     0x400000  // Track 0 - Kick
#define COLOR_GREEN   0x004000  // Track 1 - Snare
#define COLOR_BLUE    0x000040  // Track 2 - Hi-hat
#define COLOR_YELLOW  0x404000  // Track 3 - Open hi-hat
#define COLOR_RED_BRIGHT    0xFF0000
#define COLOR_GREEN_BRIGHT  0x00FF00
#define COLOR_BLUE_BRIGHT   0x0000FF
#define COLOR_YELLOW_BRIGHT 0xFFFF00
#define COLOR_WHITE_DIM     0x101010

void setup() {
    Serial.begin(115200);
    
    // Wait for USB serial connection (for debugging)
    while (!Serial && millis() < 5000) {
        delay(100);
    }
    
    Serial.println("=== NeoTrellis M4 Step Sequencer with MIDI ===");
    
    // Initialize NeoTrellis M4
    trellis.begin();
    Serial.println("NeoTrellis M4 initialized successfully!");
    trellis.setBrightness(80);
    
    // Initialize MIDI
    Serial.println("Initializing USB MIDI...");
    
    // Setup MIDI callbacks for incoming data (optional)
    midiInput.setNoteOnCallback([](uint8_t channel, uint8_t note, uint8_t velocity) {
        Serial.print("MIDI In - Note On: CH:");
        Serial.print(channel + 1);
        Serial.print(" Note:");
        Serial.print(note);
        Serial.print(" Vel:");
        Serial.println(velocity);
    });
    
    // Initialize sequencer
    sequencer.init(120, 8); // 120 BPM, 8 steps
    sequencer.setupDemoPattern();
    sequencer.start();
    
    // Clear all pixels initially
    trellis.fill(COLOR_OFF);
    trellis.show();
    
    Serial.println("Step Sequencer initialized:");
    Serial.print("- BPM: "); Serial.println(sequencer.getBPM());
    Serial.println("- Tracks: 4 (Red=Kick, Green=Snare, Blue=Closed HH, Yellow=Open HH)");
    Serial.println("- Steps: 8");
    Serial.println("- MIDI: USB Class-Compliant Device (Channel 10)");
    Serial.println("- Controls: Press buttons to toggle steps");
    Serial.println("Ready!");
    
    // Send MIDI start message
    if (midiOutput.isConnected()) {
        Serial.println("USB MIDI connected and ready!");
    } else {
        Serial.println("USB MIDI not detected (will work when connected to host)");
    }
}

void loop() {
    // Update sequencer timing and MIDI
    sequencer.tick();
    
    // Handle button input
    handleInput();
    
    // Update LED display
    updateDisplay();
    
    // Small delay for stability
    delay(5);
}

void handleInput() {
    // Check for button presses
    trellis.tick();
    
    for (int i = 0; i < 32; i++) {
        if (trellis.justPressed(i)) {
            int row = i / 8;  // 4 rows
            int col = i % 8;  // 8 columns
            
            if (row < 4 && col < 8) {
                sequencer.toggleStep(row, col);
                
                Serial.print("Toggled step [");
                Serial.print(row);
                Serial.print(",");
                Serial.print(col);
                Serial.print("] -> ");
                Serial.print(sequencer.isStepActive(row, col) ? "ON" : "OFF");
                Serial.print(" (MIDI Note: ");
                Serial.print(sequencer.getTrackMidiNote(row));
                Serial.println(")");
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
    
    trellis.show();
}