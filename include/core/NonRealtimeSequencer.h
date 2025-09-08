#ifndef NON_REALTIME_SEQUENCER_H
#define NON_REALTIME_SEQUENCER_H

#include "StepSequencer.h"
#include "ControlMessage.h"
#include "SequencerState.h"
#include "IClock.h"
#include "JsonState.h"
#include <vector>
#include <queue>
#include <memory>
#include <fstream>

/**
 * Non-realtime wrapper for StepSequencer that processes discrete control messages.
 * Enables deterministic testing by removing real-time dependencies.
 */
class NonRealtimeSequencer {
public:
    /**
     * Execution result for a single message
     */
    struct ExecutionResult {
        bool success = true;
        std::string errorMessage;
        SequencerState::Snapshot stateBefore;
        SequencerState::Snapshot stateAfter;
        std::string output;  // Any debug/log output
        
        ExecutionResult(bool s = true, const std::string& err = "") 
            : success(s), errorMessage(err) {}
    };
    
    /**
     * Batch execution result
     */
    struct BatchResult {
        std::vector<ExecutionResult> messageResults;
        bool allSucceeded = true;
        uint32_t totalMessages = 0;
        uint32_t successfulMessages = 0;
        std::string summary;
        
        void addResult(const ExecutionResult& result) {
            messageResults.push_back(result);
            totalMessages++;
            if (result.success) {
                successfulMessages++;
            } else {
                allSucceeded = false;
            }
        }
    };
    
public:
    /**
     * Constructor
     */
    NonRealtimeSequencer();
    
    /**
     * Destructor
     */
    ~NonRealtimeSequencer();
    
    /**
     * Initialize sequencer
     * @param bpm Initial BPM
     * @param steps Number of steps
     */
    void init(uint16_t bpm = 120, uint8_t steps = 8);
    
    /**
     * Process a single control message
     * @param message Message to process
     * @return Execution result
     */
    ExecutionResult processMessage(const ControlMessage::Message& message);
    
    /**
     * Process a batch of control messages
     * @param messages Vector of messages to process
     * @return Batch execution result
     */
    BatchResult processBatch(const std::vector<ControlMessage::Message>& messages);
    
    /**
     * Load and execute messages from file
     * @param filename File containing control messages (one per line)
     * @return Batch execution result
     */
    BatchResult executeFromFile(const std::string& filename);
    
    /**
     * Get current sequencer state snapshot
     * @return Current state
     */
    SequencerState::Snapshot getCurrentState() const;
    
    /**
     * Set sequencer state
     * @param snapshot State to restore
     * @return true if successful
     */
    bool setState(const SequencerState::Snapshot& snapshot);
    
    /**
     * Save current state to file
     * @param filename Output file path
     * @return true if successful
     */
    bool saveState(const std::string& filename) const;
    
    /**
     * Load state from file
     * @param filename Input file path
     * @return true if successful
     */
    bool loadState(const std::string& filename);
    
    /**
     * Get the underlying sequencer (for advanced access)
     * @return Reference to wrapped sequencer
     */
    StepSequencer& getSequencer() { return *sequencer_; }
    const StepSequencer& getSequencer() const { return *sequencer_; }
    
    /**
     * Set logging output stream
     * @param stream Output stream for logging (nullptr to disable)
     */
    void setLogStream(std::ostream* stream) { logStream_ = stream; }
    
    /**
     * Enable/disable verbose logging
     * @param verbose Whether to log all operations
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
    /**
     * Reset to initial state
     */
    void reset();
    
private:
    std::unique_ptr<StepSequencer> sequencer_;
    std::unique_ptr<IClock> clock_;
    
    // Mock interfaces for non-realtime operation
    class NullMidiOutput;
    class NullDisplay;
    
    std::unique_ptr<NullMidiOutput> nullMidi_;
    std::unique_ptr<NullDisplay> nullDisplay_;
    
    // Logging
    std::ostream* logStream_;
    bool verbose_;
    
    // State tracking
    uint32_t virtualTime_;  // Virtual time in milliseconds
    std::string stateDirectory_;  // Directory for saving states
    
    /**
     * Log a message if logging is enabled
     */
    void log(const std::string& message) const;
    
    /**
     * Process semantic control message (replaces legacy key press/release)
     */
    ExecutionResult processSemanticMessage(const ControlMessage::Message& message);
    
    /**
     * Process clock tick message
     */
    ExecutionResult processClockTick(const ControlMessage::Message& message);
    
    /**
     * Process time advance message
     */
    ExecutionResult processTimeAdvance(const ControlMessage::Message& message);
    
    /**
     * Process sequencer control message (start/stop/reset)
     */
    ExecutionResult processSequencerControl(const ControlMessage::Message& message);
    
    /**
     * Process state management message (save/load/verify)
     */
    ExecutionResult processStateMessage(const ControlMessage::Message& message);
    
    /**
     * Validate message parameters
     */
    bool validateMessage(const ControlMessage::Message& message, std::string& error);
    
    /**
     * Create state directory if it doesn't exist
     */
    void ensureStateDirectory() const;
    
    /**
     * Convert JsonState::Snapshot to SequencerState::Snapshot
     */
    SequencerState::Snapshot convertJsonStateToSequencerState(const JsonState::Snapshot& jsonSnapshot) const;
    
private:
    // Null implementations for non-realtime operation
    class NullMidiOutput : public IMidiOutput {
    public:
        void sendNoteOn(uint8_t, uint8_t, uint8_t) override {}
        void sendNoteOff(uint8_t, uint8_t, uint8_t) override {}
        void sendControlChange(uint8_t, uint8_t, uint8_t) override {}
        void sendProgramChange(uint8_t, uint8_t) override {}
        void sendClock() override {}
        void sendStart() override {}
        void sendStop() override {}
        void sendContinue() override {}
        bool isConnected() const override { return true; }
        void flush() override {}
    };
    
    class NullDisplay : public IDisplay {
    public:
        void init() override {}
        void shutdown() override {}
        void setLED(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) override {}
        void clear() override {}
        void refresh() override {}
        uint8_t getRows() const override { return 4; }
        uint8_t getCols() const override { return 8; }
    };
    
    // NullInput removed - StepSequencer no longer requires IInput dependency
};

#endif // NON_REALTIME_SEQUENCER_H