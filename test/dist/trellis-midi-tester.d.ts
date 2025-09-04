/**
 * Trellis M4 MIDI Integration Tester
 *
 * Validates bidirectional MIDI communication with the NeoTrellis M4 step sequencer
 */
import { AnalysisResult, TestResult } from './midi-types';
export declare class TrellisMidiTester {
    private midiIn;
    private midiOut;
    private receivedMessages;
    private listening;
    private trellisInPort;
    private trellisOutPort;
    private readonly config;
    constructor();
    private setupMidiCallback;
    findTrellisPorts(): boolean;
    private isTrellisPort;
    connect(): Promise<boolean>;
    disconnect(): void;
    startListening(): void;
    stopListening(): void;
    private parseMidiMessage;
    sendNoteOn(channel: number, note: number, velocity: number): void;
    sendNoteOff(channel: number, note: number, velocity?: number): void;
    sendControlChange(channel: number, control: number, value: number): void;
    sendTransportMessage(msgType: 'start' | 'stop' | 'continue' | 'clock'): void;
    clearReceivedMessages(): void;
    analyzeSequencerOutput(durationSeconds: number): Promise<AnalysisResult>;
    validateDrumPattern(analysis: AnalysisResult): string[];
    testBidirectionalCommunication(): Promise<TestResult[]>;
    testTransportControl(): Promise<TestResult>;
    runFullTestSuite(): Promise<boolean>;
    private sleep;
}
//# sourceMappingURL=trellis-midi-tester.d.ts.map