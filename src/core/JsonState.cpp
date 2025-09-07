#include "JsonState.h"
#include "StepSequencer.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

using json = nlohmann::json;
using namespace JsonState;

std::string Snapshot::toJson() const {
    json j = *this; // Use implicit conversion
    return j.dump(2); // Pretty print with 2-space indent
}

bool Snapshot::fromJson(const std::string& jsonStr) {
    try {
        json j = json::parse(jsonStr);
        *this = j.get<Snapshot>(); // Use implicit conversion
        return validate();
    } catch (const std::exception& e) {
        return false;
    }
}

bool Snapshot::saveToFile(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << toJson();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool Snapshot::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return fromJson(buffer.str());
    } catch (const std::exception& e) {
        return false;
    }
}

bool Snapshot::validate() const {
    // Basic validation
    if (sequencer.bpm < 60 || sequencer.bpm > 200) return false;
    if (sequencer.stepCount < 1 || sequencer.stepCount > 8) return false;
    if (sequencer.currentStep >= 8) return false;
    
    // Validate parameter locks
    for (int i = 0; i < 64; ++i) {
        const auto& lock = parameterLocks[i];
        if (lock.inUse) {
            if (lock.stepIndex >= 8 || lock.trackIndex >= 4) return false;
            if (lock.noteOffset < -64 || lock.noteOffset > 63) return false;
            if (lock.velocity > 127) return false;
            if (lock.length < 1 || lock.length > 255) return false;
        }
    }
    
    // Validate tracks
    for (int i = 0; i < 4; ++i) {
        const auto& track = tracks[i];
        if (track.volume > 127) return false;
        if (track.note > 127) return false;
        if (track.channel > 15) return false;
    }
    
    // Validate parameter lock mode
    if (parameterLockMode.active) {
        if (parameterLockMode.heldTrack >= 4 || parameterLockMode.heldStep >= 8) return false;
    }
    
    return true;
}

bool Snapshot::equals(const Snapshot& other) const {
    // Use JSON comparison for deep equality
    return toJson() == other.toJson();
}

std::string Snapshot::getDiff(const Snapshot& other) const {
    std::stringstream diff;
    
    if (sequencer.bpm != other.sequencer.bpm) {
        diff << "BPM: " << sequencer.bpm << " -> " << other.sequencer.bpm << "\n";
    }
    if (sequencer.currentStep != other.sequencer.currentStep) {
        diff << "Current Step: " << (int)sequencer.currentStep << " -> " << (int)other.sequencer.currentStep << "\n";
    }
    if (sequencer.playing != other.sequencer.playing) {
        diff << "Playing: " << (sequencer.playing ? "true" : "false") << " -> " << (other.sequencer.playing ? "true" : "false") << "\n";
    }
    
    // Check pattern differences
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            if (pattern[track][step].active != other.pattern[track][step].active) {
                diff << "Pattern[" << track << "][" << step << "]: " 
                     << (pattern[track][step].active ? "active" : "inactive") << " -> "
                     << (other.pattern[track][step].active ? "active" : "inactive") << "\n";
            }
        }
    }
    
    // Check parameter lock mode
    if (parameterLockMode.active != other.parameterLockMode.active) {
        diff << "Parameter Lock Mode: " << (parameterLockMode.active ? "active" : "inactive") 
             << " -> " << (other.parameterLockMode.active ? "active" : "inactive") << "\n";
    }
    
    if (diff.str().empty()) {
        return "No differences found";
    }
    
    return diff.str();
}

std::string Snapshot::getSummary() const {
    std::stringstream summary;
    summary << "Sequencer State Summary:\n";
    summary << "  BPM: " << sequencer.bpm << "\n";
    summary << "  Current Step: " << (int)sequencer.currentStep << "\n";
    summary << "  Playing: " << (sequencer.playing ? "Yes" : "No") << "\n";
    summary << "  Parameter Lock Mode: " << (parameterLockMode.active ? "Active" : "Inactive") << "\n";
    
    if (parameterLockMode.active) {
        summary << "    Held: Track " << (int)parameterLockMode.heldTrack 
                << ", Step " << (int)parameterLockMode.heldStep << "\n";
    }
    
    // Count active steps
    int activeSteps[4] = {0};
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            if (pattern[track][step].active) {
                activeSteps[track]++;
            }
        }
    }
    
    summary << "  Active Steps: ";
    for (int i = 0; i < 4; ++i) {
        summary << "T" << i << "=" << activeSteps[i];
        if (i < 3) summary << ", ";
    }
    summary << "\n";
    
    // Count parameter locks
    int activeLocks = 0;
    for (int i = 0; i < 64; ++i) {
        if (parameterLocks[i].inUse) {
            activeLocks++;
        }
    }
    summary << "  Parameter Locks: " << activeLocks << "/64\n";
    
    return summary.str();
}

// Custom JSON serialization for the complete Snapshot
void nlohmann::adl_serializer<JsonState::Snapshot>::to_json(json& j, const JsonState::Snapshot& s) {
    // Create timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    
    j = json{
        {"version", s.version},
        {"timestamp", ss.str()},
        {"sequencer", s.sequencer},
        {"parameterLockMode", s.parameterLockMode}
    };
    
    // Serialize pattern array
    j["pattern"] = json::array();
    for (int track = 0; track < 4; ++track) {
        json trackArray = json::array();
        for (int step = 0; step < 8; ++step) {
            trackArray.push_back(s.pattern[track][step]);
        }
        j["pattern"].push_back(trackArray);
    }
    
    // Serialize parameter locks array
    j["parameterLocks"] = json::array();
    for (int i = 0; i < 64; ++i) {
        j["parameterLocks"].push_back(s.parameterLocks[i]);
    }
    
    // Serialize buttons array
    j["buttons"] = json::array();
    for (int i = 0; i < 32; ++i) {
        j["buttons"].push_back(s.buttons[i]);
    }
    
    // Serialize tracks array
    j["tracks"] = json::array();
    for (int i = 0; i < 4; ++i) {
        j["tracks"].push_back(s.tracks[i]);
    }
}

void nlohmann::adl_serializer<JsonState::Snapshot>::from_json(const json& j, JsonState::Snapshot& s) {
    s.version = j.at("version").get<std::string>();
    if (j.contains("timestamp")) {
        s.timestamp = j.at("timestamp").get<std::string>();
    }
    
    s.sequencer = j.at("sequencer").get<JsonState::SequencerCore>();
    s.parameterLockMode = j.at("parameterLockMode").get<JsonState::ParameterLockMode>();
    
    // Deserialize pattern array
    const auto& patternJson = j.at("pattern");
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            s.pattern[track][step] = patternJson[track][step].get<JsonState::StepData>();
        }
    }
    
    // Deserialize parameter locks array
    const auto& locksJson = j.at("parameterLocks");
    for (int i = 0; i < 64; ++i) {
        s.parameterLocks[i] = locksJson[i].get<JsonState::ParameterLock>();
    }
    
    // Deserialize buttons array
    const auto& buttonsJson = j.at("buttons");
    for (int i = 0; i < 32; ++i) {
        s.buttons[i] = buttonsJson[i].get<JsonState::ButtonState>();
    }
    
    // Deserialize tracks array
    const auto& tracksJson = j.at("tracks");
    for (int i = 0; i < 4; ++i) {
        s.tracks[i] = tracksJson[i].get<JsonState::TrackSettings>();
    }
}

// Implementation for state capture/restore
JsonState::Snapshot JsonState::captureState(const StepSequencer& sequencer) {
    Snapshot snapshot;
    
    // TODO: Need to add getter methods to StepSequencer to access internal state
    // For now, create a basic snapshot structure
    
    snapshot.sequencer.bpm = sequencer.getTempo();
    snapshot.sequencer.currentStep = sequencer.getCurrentStep();
    snapshot.sequencer.playing = sequencer.isPlaying();
    // snapshot.sequencer.currentTime = sequencer.getCurrentTime(); // Need getter
    // snapshot.sequencer.tickCounter = sequencer.getTickCounter(); // Available
    
    // TODO: Capture pattern state
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            snapshot.pattern[track][step].active = sequencer.isStepActive(track, step);
            // snapshot.pattern[track][step].hasLock = ; // Need getter
            // snapshot.pattern[track][step].lockIndex = ; // Need getter
        }
    }
    
    // TODO: Capture parameter lock state, button state, etc.
    // This requires adding getter methods to StepSequencer or making fields accessible
    
    return snapshot;
}

bool JsonState::restoreState(StepSequencer& sequencer, const Snapshot& snapshot) {
    // TODO: Implement state restoration
    // This requires adding setter methods to StepSequencer or making fields accessible
    
    // Basic restoration for now
    sequencer.setTempo(snapshot.sequencer.bpm);
    
    // TODO: Restore pattern state, parameter locks, etc.
    
    return true;
}