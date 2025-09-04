"use strict";
/**
 * MIDI message types and interfaces for Trellis integration testing
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.MidiMessageType = void 0;
var MidiMessageType;
(function (MidiMessageType) {
    MidiMessageType["NoteOn"] = "Note On";
    MidiMessageType["NoteOff"] = "Note Off";
    MidiMessageType["ControlChange"] = "Control Change";
    MidiMessageType["ProgramChange"] = "Program Change";
    MidiMessageType["Clock"] = "Clock";
    MidiMessageType["Start"] = "Start";
    MidiMessageType["Stop"] = "Stop";
    MidiMessageType["Continue"] = "Continue";
    MidiMessageType["Unknown"] = "Unknown";
})(MidiMessageType || (exports.MidiMessageType = MidiMessageType = {}));
//# sourceMappingURL=midi-types.js.map