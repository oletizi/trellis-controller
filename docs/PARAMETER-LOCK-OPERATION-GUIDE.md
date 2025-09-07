# Parameter Lock Feature - Operation Guide

## Overview
The parameter lock system allows per-step parameter customization in the NeoTrellis M4 step sequencer, enabling rich musical variation through an intuitive hold-and-adjust interface.

## User Experience

### 1. Entering Parameter Lock Mode
- **Action**: Press and hold any sequencer step button for 500-700ms (adaptive threshold)
- **Visual Feedback**: The opposite 4×4 grid lights up with parameter control mapping
- **Audio**: No disruption to playback - sequencer continues running

**Example**: Hold button `Q` (track 1, step 0) → Control grid appears on buttons `5678`, `TYUI`, `GHJK`, `VBNM`

### 2. Parameter Control Grid Layout
When holding a step, the **opposite 4×4 grid** becomes parameter controls:

```
If holding LEFT side (columns 0-3):   If holding RIGHT side (columns 4-7):
  Control grid = RIGHT (columns 4-7)    Control grid = LEFT (columns 0-3)

  5  6  7  8                           1  2  3  4
  T  Y  U  I                           Q  W  E  R  
  G  H  J  K                           A  S  D  F
  V  B  N  M                           Z  X  C  V
```

### 3. Parameter Adjustment
While in parameter lock mode:
- **Note Up**: `T` (or `C` if holding right side)
- **Note Down**: `G` (or `D` if holding right side)  
- **Velocity Up/Down**: Other mapped buttons
- **Gate Length**: Additional mapped buttons
- **Real-time Updates**: Parameter changes apply immediately to that step

### 4. Exiting Parameter Lock Mode
- **Action**: Release the held step button
- **Result**: Control grid disappears, return to normal sequencer operation
- **Parameter Persistence**: All parameter lock changes are saved to that step

### 5. Parameter Lock Storage
- **Per-Step Parameters**: Each step can have unique note, velocity, gate length
- **Memory Efficient**: Uses fixed-size parameter pool (64 total locks across all steps)
- **Default Fallback**: Steps without parameter locks use track default values

## Ergonomic Design Principles

### Hand Position Optimization
- **Hold with left hand** → Control with right hand (and vice versa)
- **No hand conflicts** → Never need same hand for hold + control
- **Muscle memory** → Consistent control layout regardless of held step

### Real-Time Performance
- **<10μs overhead** → No audible glitches during parameter changes
- **Pre-calculated parameters** → Cached for instant step triggering
- **Non-blocking** → Sequencer playback never interrupted

### Visual Feedback System
- **Normal Mode**: Step buttons show on/off state with track colors
- **Parameter Lock Mode**: 
  - Held step: Special "parameter lock" color/pattern
  - Control grid: Different colors for different parameter types
  - Non-control buttons: Dimmed to show they're inactive

## Advanced Features

### Adaptive Hold Detection
- **Learning System**: Adjusts 500-700ms threshold based on user's hold patterns
- **Prevents Accidents**: Won't trigger on quick taps or normal step editing
- **Consistent Feel**: Adapts to user's natural button press timing

### Parameter Inheritance
- **Track Defaults**: New steps inherit track default note/velocity/length
- **Parameter Locks**: Override defaults for specific steps only
- **Efficient Memory**: Only stores differences from defaults

### Integration with Playback
- **Live Parameter Changes**: Hear parameter adjustments immediately during playback
- **Step Triggers**: Parameter-locked steps play with their custom settings
- **MIDI Output**: Custom parameters sent as MIDI note/velocity/timing data

## Expected User Workflow

1. **Create Pattern**: Set up basic step pattern with step on/off
2. **Add Variation**: Hold a step button to enter parameter lock mode
3. **Adjust Parameters**: Use control grid to change note/velocity/timing
4. **Hear Results**: Parameter changes applied immediately to playback
5. **Refine**: Release and hold different steps to add more variation
6. **Play**: Resulting pattern has rich per-step parameter variation

## Key Benefits

- **Musical Expression**: Each step can have unique character
- **Workflow Efficiency**: No menu diving - direct hardware control
- **Performance Ready**: Real-time parameter tweaking during live performance  
- **Memory Efficient**: Works within embedded hardware constraints
- **Intuitive**: Visual grid mapping makes parameter control obvious

## Technical Implementation Notes

### State Transitions
- **Normal Mode** → Hold step button 500ms+ → **Parameter Lock Mode**
- **Parameter Lock Mode** → Release held button → **Normal Mode**
- All other transitions blocked while in parameter lock mode

### Memory Architecture
- **Parameter Lock Pool**: 64 pre-allocated parameter lock slots
- **O(1) Operations**: Constant-time allocation/deallocation
- **Cache-Friendly**: Optimized for embedded processor cache lines
- **No Dynamic Allocation**: All memory statically allocated at startup

### Performance Guarantees
- **Parameter Calculation**: <10μs per step trigger
- **Mode Transition**: <100μs for enter/exit operations  
- **Button Response**: <1ms from press to visual feedback
- **MIDI Latency**: <5ms from step trigger to MIDI output

## Button Mapping Reference

### Control Grid Functions (When in Parameter Lock Mode)

#### Left-Hand Hold → Right-Hand Controls
```
Column 4 (5,T,G,V):
- 5: Velocity down
- T: Note up ⬆️
- G: Note down ⬇️  
- V: Gate length down

Column 5 (6,Y,H,B):
- 6: Velocity up
- Y: Octave up
- H: Octave down
- B: Gate length up

Column 6-7: Additional parameters (future expansion)
```

#### Right-Hand Hold → Left-Hand Controls
```
Column 0 (1,Q,A,Z):
- 1: Velocity down
- Q: Note up ⬆️
- A: Note down ⬇️
- Z: Gate length down

Column 1 (2,W,S,X):
- 2: Velocity up
- W: Octave up
- S: Octave down
- X: Gate length up

Column 2-3: Additional parameters (future expansion)
```

## Troubleshooting

### Parameter Lock Not Entering
- Ensure button is held for full 500ms+ duration
- Check that sequencer is in normal mode (not pattern select or settings)
- Verify button is a valid step button (32 buttons total)

### Parameters Not Saving
- Confirm parameter lock pool has free slots (64 max)
- Check that step is active (parameter locks only apply to active steps)
- Verify MIDI output is connected for hearing changes

### Visual Feedback Issues
- Control grid should light up immediately on mode entry
- Different colors indicate different parameter types
- Dimmed buttons show inactive areas during parameter lock mode

## Future Enhancements (Roadmap)

- **Parameter Copy/Paste**: Copy parameters between steps
- **Parameter Slides**: Smooth parameter transitions between steps
- **Conditional Locks**: Parameters that activate based on conditions
- **Parameter LFOs**: Automated parameter modulation
- **Pattern-Level Locks**: Apply parameter locks to entire patterns
- **MIDI CC Locks**: Lock MIDI CC values per step

---

This design creates a **powerful but approachable** system for adding musical variation to step sequences, with the ergonomics and performance needed for both studio work and live performance.