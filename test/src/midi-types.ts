/**
 * MIDI message types and interfaces for Trellis integration testing
 */

export interface MidiMessage {
  timestamp: number;
  messageType: MidiMessageType;
  channel: number;
  data1: number;
  data2: number;
  raw: number[];
}

export enum MidiMessageType {
  NoteOn = 'Note On',
  NoteOff = 'Note Off',
  ControlChange = 'Control Change',
  ProgramChange = 'Program Change',
  Clock = 'Clock',
  Start = 'Start',
  Stop = 'Stop',
  Continue = 'Continue',
  Unknown = 'Unknown'
}

export interface DrumMapping {
  [track: number]: number; // track -> MIDI note
}

export interface SequencerConfig {
  expectedChannel: number; // 0-based MIDI channel
  expectedDrumMapping: DrumMapping;
  expectedBpm: number;
  stepsPerPattern: number;
}

export interface AnalysisResult {
  duration: number;
  totalMessages: number;
  messageTypes: Record<string, number>;
  noteUsage: Record<number, number>;
  channelUsage: Record<number, number>;
  timing: {
    avgIntervalSeconds: number;
    estimatedBpm: number;
    intervals: number[];
  };
}

export interface TestResult {
  name: string;
  passed: boolean;
  message: string;
  details?: any;
}