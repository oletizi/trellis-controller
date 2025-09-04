#!/usr/bin/env node
"use strict";
/**
 * Main entry point for MIDI Integration Test Suite
 *
 * Usage:
 *   npm test                    # Run full test suite
 *   npm run test:watch          # Run with continuous monitoring
 *   node dist/midi-integration-test.js --help
 */
Object.defineProperty(exports, "__esModule", { value: true });
const trellis_midi_tester_1 = require("./trellis-midi-tester");
function parseArgs(args) {
    const options = {
        help: false,
        watch: false,
        duration: 8,
        verbose: false
    };
    for (let i = 0; i < args.length; i++) {
        const arg = args[i];
        switch (arg) {
            case '--help':
            case '-h':
                options.help = true;
                break;
            case '--watch':
            case '-w':
                options.watch = true;
                break;
            case '--duration':
            case '-d':
                const duration = parseInt(args[++i]);
                if (!isNaN(duration) && duration > 0) {
                    options.duration = duration;
                }
                break;
            case '--verbose':
            case '-v':
                options.verbose = true;
                break;
        }
    }
    return options;
}
function showHelp() {
    console.log(`
🎵 Trellis M4 MIDI Integration Test Suite

This tool validates bidirectional MIDI communication with the NeoTrellis M4
step sequencer to ensure proper operation of the MIDI implementation.

USAGE:
  npm test                           # Run full test suite
  npm run test:watch                 # Run with continuous monitoring
  node dist/midi-integration-test.js [OPTIONS]

OPTIONS:
  -h, --help              Show this help message
  -w, --watch             Run continuous monitoring mode
  -d, --duration <secs>   Analysis duration in seconds (default: 8)
  -v, --verbose           Enable verbose output

REQUIREMENTS:
  1. Trellis M4 connected via USB
  2. Device flashed with arduino_trellis_midi firmware
  3. Node.js with midi package installed

TESTS PERFORMED:
  📊 Sequencer Output Analysis
     - Validates MIDI message types and timing
     - Checks drum note mapping (C2=36, D2=38, F#2=42, A#2=46)
     - Verifies MIDI channel 10 usage
     - Measures BPM accuracy (~240 BPM for 8th notes at 120 BPM)

  📡 Bidirectional Communication
     - Sends control change messages to device
     - Sends note on/off messages
     - Tests transport control (start/stop)

  🎮 Transport Control Response
     - Verifies device responds to MIDI start/stop commands
     - Measures message flow changes during transport control

EXPECTED DEVICE BEHAVIOR:
  - Sends MIDI notes on channel 10 for active sequencer steps
  - Default drum mapping: Track 0→36, Track 1→38, Track 2→42, Track 3→46
  - Responds to transport control messages
  - Triggers callbacks for incoming MIDI messages

TROUBLESHOOTING:
  - If no MIDI ports found: Check USB connection and device power
  - If no messages received: Verify device is running and sequencer is active
  - If timing issues: Check for USB interference or system latency
`);
}
async function runContinuousMonitoring(tester, options) {
    console.log('🔄 Starting continuous monitoring mode (Press Ctrl+C to exit)...\n');
    if (!(await tester.connect())) {
        process.exit(1);
    }
    tester.startListening();
    // Set up graceful shutdown
    process.on('SIGINT', () => {
        console.log('\n\n⏸️  Shutting down monitoring...');
        tester.disconnect();
        process.exit(0);
    });
    let cycleCount = 0;
    while (true) {
        cycleCount++;
        console.log(`\n📊 Analysis Cycle ${cycleCount} (${options.duration}s)`);
        console.log('-'.repeat(50));
        try {
            const analysis = await tester.analyzeSequencerOutput(options.duration);
            console.log(`Messages: ${analysis.totalMessages}, ` +
                `BPM: ${analysis.timing.estimatedBpm.toFixed(1)}, ` +
                `Types: ${Object.keys(analysis.messageTypes).join(', ')}`);
            if (options.verbose) {
                console.log(`Notes: [${Object.keys(analysis.noteUsage).join(', ')}]`);
                console.log(`Channels: [${Object.keys(analysis.channelUsage).map(ch => +ch + 1).join(', ')}]`);
            }
            const issues = tester.validateDrumPattern(analysis);
            if (issues.length > 0) {
                console.log(`⚠️  Issues: ${issues.length}`);
                if (options.verbose) {
                    issues.forEach(issue => console.log(`  - ${issue}`));
                }
            }
            else {
                console.log('✅ All validations passed');
            }
        }
        catch (error) {
            console.log(`❌ Analysis error: ${error}`);
        }
        // Wait before next cycle
        await new Promise(resolve => setTimeout(resolve, 2000));
    }
}
async function main() {
    const options = parseArgs(process.argv.slice(2));
    if (options.help) {
        showHelp();
        return;
    }
    console.log('Initializing MIDI Integration Test...');
    try {
        const tester = new trellis_midi_tester_1.TrellisMidiTester();
        if (options.watch) {
            await runContinuousMonitoring(tester, options);
        }
        else {
            const success = await tester.runFullTestSuite();
            if (success) {
                console.log('\n🎉 All tests completed successfully!');
                process.exit(0);
            }
            else {
                console.log('\n❌ Tests completed with issues!');
                process.exit(1);
            }
        }
    }
    catch (error) {
        console.log(`\n💥 Test failed with error: ${error}`);
        if (error instanceof Error) {
            console.log(error.stack);
        }
        process.exit(1);
    }
}
// Handle unhandled promise rejections
process.on('unhandledRejection', (reason, promise) => {
    console.log('💥 Unhandled Rejection at:', promise, 'reason:', reason);
    process.exit(1);
});
if (require.main === module) {
    main();
}
//# sourceMappingURL=midi-integration-test.js.map