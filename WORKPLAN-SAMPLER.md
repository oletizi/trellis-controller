# Audio Sampler Implementation Work Plan

## Overview
Implement a professional audio sampler for the NeoTrellis M4 that supports SFZ and DecentSampler formats, with stereo DAC output, USB file transfer, and dual-mode operation (sequencer/sampler).

## Phase 1: Audio Infrastructure Foundation

### 1.1 I2S Audio Output Setup
- [ ] Configure SAMD51 I2S peripheral for stereo output
- [ ] Set up DMA for efficient audio streaming
- [ ] Implement double-buffered audio output at 44.1kHz/48kHz
- [ ] Create IAudioOutput interface for abstraction
- [ ] Test basic sine wave generation through DAC

### 1.2 Audio Engine Core
- [ ] Design sample playback engine with voice architecture
- [ ] Implement polyphonic voice allocation (8-16 voices)
- [ ] Add linear interpolation for sample rate conversion
- [ ] Create mixing bus for combining voices to stereo output
- [ ] Implement basic ADSR envelope per voice

### 1.3 Memory Management
- [ ] Design memory pool for sample data (use QSPI flash if available)
- [ ] Implement sample streaming from flash/SD card
- [ ] Create sample cache for frequently used samples
- [ ] Add memory-efficient sample format (16-bit, compressed)

## Phase 2: Sample Format Support

### 2.1 SFZ Parser Implementation
- [ ] Create SFZ file parser for basic opcodes
- [ ] Support key mapping (lokey, hikey, pitch_keycenter)
- [ ] Implement velocity layers (lovel, hivel)
- [ ] Add sample playback regions
- [ ] Support loop points and crossfades
- [ ] Parse filter and envelope settings

### 2.2 DecentSampler Format Support
- [ ] Implement DecentSampler XML parser
- [ ] Map DS groups to internal voice structure
- [ ] Support DS sample mapping and velocity layers
- [ ] Convert DS envelope/filter parameters
- [ ] Handle DS round-robin and random sample selection

### 2.3 Sample File Loading
- [ ] Support WAV file parsing (16/24-bit, mono/stereo)
- [ ] Add FLAC decoder for compressed samples
- [ ] Implement sample rate conversion on load
- [ ] Create sample metadata storage

## Phase 3: USB Communication & File Transfer

### 3.1 USB Mass Storage Class
- [ ] Implement USB MSC for direct file access
- [ ] Create virtual filesystem for sample storage
- [ ] Support drag-and-drop sample upload
- [ ] Add progress indication via LEDs

### 3.2 Custom USB Protocol (Alternative)
- [ ] Design efficient binary protocol for sample transfer
- [ ] Create host-side upload utility (Python/Node.js)
- [ ] Implement chunked transfer with checksums
- [ ] Add sample library management commands

### 3.3 Storage Management
- [ ] Implement SD card support if available
- [ ] Create sample library organization (banks/programs)
- [ ] Add preset save/load functionality
- [ ] Implement sample purge/cleanup

## Phase 4: Mode Switching & Control

### 4.1 Dual Mode Implementation
- [ ] Add mode state machine (Sequencer/Sampler)
- [ ] Implement shift+button combo for mode switching
- [ ] Create visual mode indication (LED colors/patterns)
- [ ] Preserve mode-specific settings

### 4.2 Sampler Mode Controls
- [ ] Map 32 buttons to MIDI notes (configurable)
- [ ] Implement velocity sensitivity (if pressure-sensitive)
- [ ] Add note-on/note-off handling
- [ ] Support sustain/hold functionality
- [ ] Create octave shift controls

### 4.3 Sequencer Integration
- [ ] Route sequencer MIDI output to sampler engine
- [ ] Implement internal MIDI bus
- [ ] Add per-track instrument selection
- [ ] Support pattern-based program changes
- [ ] Create mute/solo per voice

## Phase 5: Advanced Playback Features

### 5.1 Sample Manipulation
- [ ] Add pitch bend support
- [ ] Implement time-stretching algorithm
- [ ] Add basic filters (LP/HP/BP)
- [ ] Support reverse playback
- [ ] Implement sample start/end point adjustment

### 5.2 Effects Processing
- [ ] Add reverb send/return bus
- [ ] Implement basic delay effect
- [ ] Add compression/limiting on output
- [ ] Support per-voice panning
- [ ] Create simple EQ

### 5.3 Modulation
- [ ] Implement LFOs (pitch, filter, amplitude)
- [ ] Add modulation matrix
- [ ] Support MIDI CC mapping
- [ ] Create macro controls
- [ ] Add aftertouch support (if available)

## Phase 6: Recording & Sampling (Future)

### 6.1 ADC Input Configuration
- [ ] Set up SAMD51 ADC for audio input
- [ ] Configure appropriate sampling rate
- [ ] Add input gain staging
- [ ] Implement input monitoring

### 6.2 Sampling Engine
- [ ] Create recording buffer management
- [ ] Add threshold-based auto-sampling
- [ ] Implement sample trimming
- [ ] Support loop point detection
- [ ] Add normalize function

### 6.3 Sample Editing
- [ ] Create basic waveform editor
- [ ] Add slice-to-MIDI function
- [ ] Implement auto-chop for drum loops
- [ ] Support resample/bounce
- [ ] Add sample layering

## Phase 7: Performance & Optimization

### 7.1 CPU Optimization
- [ ] Profile audio callback performance
- [ ] Optimize mixing routines (SIMD if available)
- [ ] Implement voice stealing algorithm
- [ ] Add quality settings (interpolation, filters)
- [ ] Optimize file I/O

### 7.2 Memory Optimization
- [ ] Implement sample compression
- [ ] Add streaming for large samples
- [ ] Optimize voice allocation
- [ ] Create memory usage monitoring
- [ ] Add automatic quality reduction

### 7.3 Stability & Reliability
- [ ] Add watchdog timer
- [ ] Implement error recovery
- [ ] Create diagnostic mode
- [ ] Add factory reset function
- [ ] Implement auto-save

## Technical Specifications

### Hardware Constraints
- **CPU**: SAMD51J19A ARM Cortex-M4F @ 120MHz
- **RAM**: 192KB (need efficient memory management)
- **Flash**: 512KB (may need external storage)
- **Audio**: I2S stereo DAC output
- **Storage**: SD card slot (optional), QSPI flash (if available)

### Audio Specifications
- **Sample Rate**: 44.1kHz or 48kHz
- **Bit Depth**: 16-bit output (24-bit internal processing)
- **Polyphony**: 8-16 voices (depending on CPU load)
- **Latency Target**: <10ms

### File Format Support
- **Sample Formats**: WAV (16/24-bit), FLAC
- **Instrument Formats**: SFZ, DecentSampler
- **Maximum Sample Size**: Limited by available storage
- **Maximum Instruments**: 16 banks x 128 programs

## Implementation Priority

1. **Core Audio** (Phase 1): Essential foundation
2. **Basic Sampling** (Phase 2.3, 4): Minimum viable sampler
3. **Mode Switching** (Phase 4): Core UX feature
4. **SFZ Support** (Phase 2.1): Industry standard format
5. **USB Transfer** (Phase 3): Critical for usability
6. **Advanced Features** (Phase 5): Enhancement
7. **Recording** (Phase 6): Future expansion

## Development Approach

### Architecture Principles
- Maintain platform abstraction (follow existing pattern)
- Use dependency injection for all components
- Keep audio callback lightweight (no allocation, no blocking)
- Separate UI from audio engine
- Design for testability

### Testing Strategy
- Unit tests for format parsers
- Integration tests for audio pipeline
- Performance benchmarks for audio callback
- Hardware-in-loop testing for I2S output
- Stress testing with maximum polyphony

### Risk Mitigation
- **Memory Constraints**: Use external storage, streaming
- **CPU Limitations**: Optimize critical paths, reduce polyphony
- **Audio Glitches**: DMA double-buffering, priority interrupts
- **File Transfer Speed**: Chunked transfers, compression
- **Format Compatibility**: Start with subset, expand gradually

## Milestones

### Milestone 1: Audio Output Working
- Sine wave playing through I2S DAC
- Basic voice allocation
- Button-triggered playback

### Milestone 2: Sample Playback
- WAV file loading and playback
- Multiple samples mapped to buttons
- Basic envelope support

### Milestone 3: Format Support
- SFZ files loading and playing
- Velocity layers working
- Multi-sample instruments

### Milestone 4: Full Integration
- Mode switching operational
- Sequencer driving sampler
- USB file transfer working

### Milestone 5: Production Ready
- Stable performance
- Full feature set
- Documentation complete

## Resources Required

### Libraries/Dependencies
- ARM CMSIS DSP library (for optimized audio processing)
- TinyUSB or similar for USB functionality
- FatFS for SD card support (if used)
- Mini-XML or similar for XML parsing

### Tools
- Audio file converters
- SFZ editors for testing
- USB protocol analyzer
- Audio test equipment

### Documentation
- SAMD51 I2S peripheral documentation
- SFZ format specification
- DecentSampler format documentation
- USB MSC class specification

## Success Criteria

- [ ] Plays 16-bit stereo samples at 44.1kHz without glitches
- [ ] Supports at least 8-voice polyphony
- [ ] Loads SFZ instruments with basic features
- [ ] Switches seamlessly between sequencer and sampler modes
- [ ] Transfers files via USB in under 30 seconds for typical instruments
- [ ] Maintains <10ms latency for button-to-sound response
- [ ] Uses <80% CPU at maximum polyphony
- [ ] Operates reliably for extended periods

## Notes

This implementation will transform the NeoTrellis M4 from a MIDI sequencer into a full-featured hardware sampler, significantly expanding its capabilities while maintaining the existing sequencer functionality. The modular approach allows for incremental development and testing of each component.