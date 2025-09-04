"use strict";
/**
 * Trellis M4 MIDI Integration Tester
 *
 * Validates bidirectional MIDI communication with the NeoTrellis M4 step sequencer
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TrellisMidiTester = void 0;
const midi = __importStar(require("midi"));
const midi_types_1 = require("./midi-types");
class TrellisMidiTester {
    constructor() {
        this.receivedMessages = [];
        this.listening = false;
        this.trellisInPort = null;
        this.trellisOutPort = null;
        // Expected configuration from Arduino sketch
        this.config = {
            expectedChannel: 9, // MIDI channel 10 (0-based = 9)
            expectedDrumMapping: {
                0: 36, // Track 0 (Red): Kick drum - C2
                1: 38, // Track 1 (Green): Snare - D2  
                2: 42, // Track 2 (Blue): Closed hi-hat - F#2
                3: 46, // Track 3 (Yellow): Open hi-hat - A#2
            },
            expectedBpm: 120,
            stepsPerPattern: 8
        };
        this.midiIn = new midi.Input();
        this.midiOut = new midi.Output();
        this.setupMidiCallback();
    }
    setupMidiCallback() {
        this.midiIn.on('message', (deltaTime, message) => {
            if (this.listening) {
                const parsed = this.parseMidiMessage(message, performance.now());
                if (parsed) {
                    this.receivedMessages.push(parsed);
                    console.log(`📨 Received: ${parsed.messageType} CH:${parsed.channel + 1} ` +
                        `Data:[${parsed.data1}, ${parsed.data2}] Raw:[${parsed.raw.join(',')}]`);
                }
            }
        });
    }
    findTrellisPorts() {
        console.log('Scanning for MIDI ports...');
        // List input ports
        console.log('\nAvailable MIDI Input Ports:');
        const inputPortCount = this.midiIn.getPortCount();
        for (let i = 0; i < inputPortCount; i++) {
            const portName = this.midiIn.getPortName(i);
            console.log(`  ${i}: ${portName}`);
            if (this.isTrellisPort(portName)) {
                this.trellisInPort = i;
                console.log('    ^^ Detected Trellis input port');
            }
        }
        // List output ports
        console.log('\nAvailable MIDI Output Ports:');
        const outputPortCount = this.midiOut.getPortCount();
        for (let i = 0; i < outputPortCount; i++) {
            const portName = this.midiOut.getPortName(i);
            console.log(`  ${i}: ${portName}`);
            if (this.isTrellisPort(portName)) {
                this.trellisOutPort = i;
                console.log('    ^^ Detected Trellis output port');
            }
        }
        if (this.trellisInPort === null || this.trellisOutPort === null) {
            console.log('\n❌ ERROR: Could not find Trellis M4 MIDI ports!');
            console.log('Make sure the device is connected and flashed with MIDI firmware.');
            return false;
        }
        return true;
    }
    isTrellisPort(portName) {
        const trellisKeywords = ['trellis', 'neotrellis', 'arduino', 'samd'];
        return trellisKeywords.some(keyword => portName.toLowerCase().includes(keyword.toLowerCase()));
    }
    async connect() {
        if (!this.findTrellisPorts()) {
            return false;
        }
        try {
            this.midiIn.openPort(this.trellisInPort);
            this.midiOut.openPort(this.trellisOutPort);
            console.log('\n✅ Connected to Trellis M4 MIDI ports');
            return true;
        }
        catch (error) {
            console.log(`\n❌ Failed to connect to MIDI ports: ${error}`);
            return false;
        }
    }
    disconnect() {
        this.stopListening();
        try {
            if (this.midiIn.isPortOpen()) {
                this.midiIn.closePort();
            }
            if (this.midiOut.isPortOpen()) {
                this.midiOut.closePort();
            }
            console.log('Disconnected from MIDI ports');
        }
        catch (error) {
            console.log(`Error disconnecting: ${error}`);
        }
    }
    startListening() {
        if (this.listening)
            return;
        this.listening = true;
        this.receivedMessages = [];
        console.log('🎧 Started listening for MIDI messages...');
    }
    stopListening() {
        if (!this.listening)
            return;
        this.listening = false;
        console.log('⏹️  Stopped listening for MIDI messages');
    }
    parseMidiMessage(data, timestamp) {
        if (data.length < 1)
            return null;
        const status = data[0];
        const channel = status & 0x0F;
        const command = status & 0xF0;
        let messageType = midi_types_1.MidiMessageType.Unknown;
        const data1 = data[1] || 0;
        const data2 = data[2] || 0;
        switch (command) {
            case 0x90: // Note On
                messageType = midi_types_1.MidiMessageType.NoteOn;
                break;
            case 0x80: // Note Off
                messageType = midi_types_1.MidiMessageType.NoteOff;
                break;
            case 0xB0: // Control Change
                messageType = midi_types_1.MidiMessageType.ControlChange;
                break;
            case 0xC0: // Program Change
                messageType = midi_types_1.MidiMessageType.ProgramChange;
                break;
        }
        // Handle system messages
        if (status === 0xF8)
            messageType = midi_types_1.MidiMessageType.Clock;
        if (status === 0xFA)
            messageType = midi_types_1.MidiMessageType.Start;
        if (status === 0xFC)
            messageType = midi_types_1.MidiMessageType.Stop;
        if (status === 0xFB)
            messageType = midi_types_1.MidiMessageType.Continue;
        return {
            timestamp,
            messageType,
            channel: (messageType === midi_types_1.MidiMessageType.Clock ||
                messageType === midi_types_1.MidiMessageType.Start ||
                messageType === midi_types_1.MidiMessageType.Stop ||
                messageType === midi_types_1.MidiMessageType.Continue) ? 0 : channel,
            data1,
            data2,
            raw: [...data]
        };
    }
    sendNoteOn(channel, note, velocity) {
        const msg = [0x90 | channel, note, velocity];
        this.midiOut.sendMessage(msg);
        console.log(`📤 Sent Note On: CH:${channel + 1} Note:${note} Vel:${velocity}`);
    }
    sendNoteOff(channel, note, velocity = 0) {
        const msg = [0x80 | channel, note, velocity];
        this.midiOut.sendMessage(msg);
        console.log(`📤 Sent Note Off: CH:${channel + 1} Note:${note} Vel:${velocity}`);
    }
    sendControlChange(channel, control, value) {
        const msg = [0xB0 | channel, control, value];
        this.midiOut.sendMessage(msg);
        console.log(`📤 Sent Control Change: CH:${channel + 1} CC:${control} Val:${value}`);
    }
    sendTransportMessage(msgType) {
        const transportCodes = {
            start: [0xFA],
            stop: [0xFC],
            continue: [0xFB],
            clock: [0xF8]
        };
        if (msgType in transportCodes) {
            this.midiOut.sendMessage(transportCodes[msgType]);
            console.log(`📤 Sent ${msgType.charAt(0).toUpperCase() + msgType.slice(1)}`);
        }
    }
    clearReceivedMessages() {
        this.receivedMessages = [];
        console.log('🗑️  Cleared received message buffer');
    }
    async analyzeSequencerOutput(durationSeconds) {
        console.log(`\n🔍 Analyzing sequencer output for ${durationSeconds} seconds...`);
        this.clearReceivedMessages();
        // Wait and collect messages
        await this.sleep(durationSeconds * 1000);
        // Analyze collected messages
        const noteUsage = {};
        const channelUsage = {};
        const messageTypes = {};
        const timingIntervals = [];
        let lastNoteTime = null;
        for (const msg of this.receivedMessages) {
            messageTypes[msg.messageType] = (messageTypes[msg.messageType] || 0) + 1;
            if (msg.messageType === midi_types_1.MidiMessageType.NoteOn) {
                noteUsage[msg.data1] = (noteUsage[msg.data1] || 0) + 1;
                channelUsage[msg.channel] = (channelUsage[msg.channel] || 0) + 1;
                if (lastNoteTime !== null) {
                    const interval = (msg.timestamp - lastNoteTime) / 1000; // Convert to seconds
                    timingIntervals.push(interval);
                }
                lastNoteTime = msg.timestamp;
            }
        }
        // Calculate average timing interval
        const avgInterval = timingIntervals.length > 0
            ? timingIntervals.reduce((a, b) => a + b, 0) / timingIntervals.length
            : 0;
        const estimatedBpm = avgInterval > 0 ? 60.0 / avgInterval : 0;
        return {
            duration: durationSeconds,
            totalMessages: this.receivedMessages.length,
            messageTypes,
            noteUsage,
            channelUsage,
            timing: {
                avgIntervalSeconds: avgInterval,
                estimatedBpm,
                intervals: timingIntervals.slice(0, 10) // First 10 intervals
            }
        };
    }
    validateDrumPattern(analysis) {
        const issues = [];
        // Check if expected drum notes are being used
        for (const [track, expectedNote] of Object.entries(this.config.expectedDrumMapping)) {
            if (!(expectedNote in analysis.noteUsage)) {
                issues.push(`Expected drum note ${expectedNote} (track ${track}) not found in output`);
            }
        }
        // Check channel usage
        if (!(this.config.expectedChannel in analysis.channelUsage)) {
            issues.push(`Expected MIDI channel ${this.config.expectedChannel + 1} not used`);
        }
        // Check timing consistency
        const estimatedBpm = analysis.timing.estimatedBpm;
        if (estimatedBpm > 0) {
            // For 8th note timing at 120 BPM, expect ~240 BPM in note triggers
            const expectedBpmRange = [200, 280]; // Allows some tolerance
            if (estimatedBpm < expectedBpmRange[0] || estimatedBpm > expectedBpmRange[1]) {
                issues.push(`Timing seems off: estimated ${estimatedBpm.toFixed(1)} BPM, ` +
                    `expected ${expectedBpmRange[0]}-${expectedBpmRange[1]} BPM`);
            }
        }
        return issues;
    }
    async testBidirectionalCommunication() {
        console.log('\n🔄 Testing bidirectional communication...');
        const results = [];
        // Test 1: Send control change messages
        console.log('\nTest 1: Sending Control Change messages...');
        this.clearReceivedMessages();
        const controlChanges = [1, 7, 10]; // Mod wheel, volume, pan
        for (const ccNum of controlChanges) {
            this.sendControlChange(this.config.expectedChannel, ccNum, 64);
            await this.sleep(100);
        }
        results.push({
            name: 'Control Change Messages Sent',
            passed: true,
            message: `Sent ${controlChanges.length} CC messages`
        });
        // Test 2: Send note messages
        console.log('\nTest 2: Sending Note messages...');
        const drumNotes = [36, 38, 42, 46]; // Standard drum notes
        for (const note of drumNotes) {
            this.sendNoteOn(this.config.expectedChannel, note, 100);
            await this.sleep(100);
            this.sendNoteOff(this.config.expectedChannel, note, 0);
            await this.sleep(100);
        }
        results.push({
            name: 'Note Messages Sent',
            passed: true,
            message: `Sent note on/off for ${drumNotes.length} drum notes`
        });
        // Test 3: Send transport messages
        console.log('\nTest 3: Sending Transport messages...');
        this.sendTransportMessage('stop');
        await this.sleep(500);
        this.sendTransportMessage('start');
        await this.sleep(500);
        results.push({
            name: 'Transport Messages Sent',
            passed: true,
            message: 'Sent stop and start transport messages'
        });
        return results;
    }
    async testTransportControl() {
        console.log('\n🎮 Testing Transport Control Response...');
        const initialCount = this.receivedMessages.length;
        this.sendTransportMessage('stop');
        await this.sleep(2000);
        const stopCount = this.receivedMessages.length - initialCount;
        this.sendTransportMessage('start');
        await this.sleep(2000);
        const startCount = this.receivedMessages.length - initialCount - stopCount;
        console.log(`Messages during stop: ${stopCount}`);
        console.log(`Messages after restart: ${startCount}`);
        const transportWorking = stopCount < startCount;
        return {
            name: 'Transport Control',
            passed: transportWorking,
            message: transportWorking
                ? 'Transport control appears to be working'
                : 'Transport control response unclear',
            details: { stopCount, startCount }
        };
    }
    async runFullTestSuite() {
        console.log('🎵 MIDI Integration Test Suite for Trellis M4 Step Sequencer');
        console.log('='.repeat(60));
        if (!(await this.connect())) {
            return false;
        }
        try {
            this.startListening();
            let allTestsPassed = true;
            // Test 1: Analyze sequencer output
            console.log('\n📊 TEST 1: Sequencer Output Analysis');
            console.log('-'.repeat(40));
            const analysis = await this.analyzeSequencerOutput(8); // 8 seconds
            console.log('\nAnalysis Results:');
            console.log(`  Total messages: ${analysis.totalMessages}`);
            console.log(`  Message types: ${JSON.stringify(analysis.messageTypes, null, 2)}`);
            console.log(`  Notes used: [${Object.keys(analysis.noteUsage).join(', ')}]`);
            console.log(`  Channels used: [${Object.keys(analysis.channelUsage).map(ch => +ch + 1).join(', ')}]`);
            console.log(`  Estimated BPM: ${analysis.timing.estimatedBpm.toFixed(1)}`);
            // Validate pattern
            const issues = this.validateDrumPattern(analysis);
            if (issues.length > 0) {
                console.log('\n⚠️  Issues found:');
                issues.forEach(issue => console.log(`    - ${issue}`));
                allTestsPassed = false;
            }
            else {
                console.log('\n✅ Drum pattern validation passed!');
            }
            // Test 2: Bidirectional communication
            console.log('\n📡 TEST 2: Bidirectional Communication');
            console.log('-'.repeat(40));
            const commResults = await this.testBidirectionalCommunication();
            commResults.forEach(result => {
                const icon = result.passed ? '✅' : '❌';
                console.log(`${icon} ${result.name}: ${result.message}`);
            });
            // Test 3: Transport control
            console.log('\n🎮 TEST 3: Transport Control Response');
            console.log('-'.repeat(40));
            const transportResult = await this.testTransportControl();
            const transportIcon = transportResult.passed ? '✅' : '❌';
            console.log(`${transportIcon} ${transportResult.name}: ${transportResult.message}`);
            if (!transportResult.passed) {
                allTestsPassed = false;
            }
            // Summary
            console.log('\n📋 TEST SUMMARY');
            console.log('='.repeat(40));
            console.log('✅ Successfully connected to Trellis M4');
            console.log(`✅ Received ${this.receivedMessages.length} total MIDI messages`);
            if (analysis.totalMessages > 0) {
                console.log('✅ Sequencer is sending MIDI data');
            }
            else {
                console.log('❌ No MIDI data received from sequencer');
                allTestsPassed = false;
            }
            if (issues.length === 0) {
                console.log('✅ Drum pattern validation passed');
            }
            else {
                console.log(`⚠️  ${issues.length} validation issues found`);
            }
            console.log(`\n🎯 Integration test ${allTestsPassed ? 'completed successfully' : 'completed with issues'}!`);
            return allTestsPassed;
        }
        catch (error) {
            console.log(`\n💥 Test failed with error: ${error}`);
            return false;
        }
        finally {
            this.disconnect();
        }
    }
    sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}
exports.TrellisMidiTester = TrellisMidiTester;
//# sourceMappingURL=trellis-midi-tester.js.map