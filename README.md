# OYSTER — Granular Wavetable Synthesizer
### A professional polyphonic layered synthesizer plugin by 3LIXIR MUSIC

OYSTER is a polyphonic, 4-slot granular wavetable synthesizer built in C++ using the JUCE framework. Each slot runs an independent synthesis engine with its own grain scheduler, wavetable source, sub oscillator, harmonic OSC2 layer, filter, modulation state, and signal chain — all mixed into a single stereo output with global reverb and chorus.

Available as **AU**, **VST3**, and **Standalone** for macOS and Windows.

---

## What Makes Oyster Different

- **4-slot layered architecture** — not just one patch engine. Each slot behaves like an independent mini-instrument that can be programmed, modulated, and mixed independently, then summed to a master output.
- **Hybrid sound source model** — three parallel synthesis sources per voice: a granular wavetable engine, a dedicated sub oscillator, and a harmonic OSC2 layer.
- **Custom wavetable design** — curated waveform personalities beyond basic analog shapes (Warm Sine, Soft Saw, Hybrid, Formant, Shimmer, and more).
- **Allocation-free grain lifecycle** — fixed 128-grain pool with event-style scheduling. No dynamic memory allocation in the audio render path.
- **Real-time safety engineering** — NaN/Inf sanitization at every stage, SpinLock try-lock strategy during host re-prepare, per-slot buffer isolation.

---

## Core Architecture

### 4-Slot Engine
Oyster runs 4 independent synthesis slots. Each slot contains:
- Its own `juce::Synthesiser` with 8 polyphonic voices
- Its own `SlotState` snapshot (waveform, modulation, envelope, filter, OSC2, sub settings)
- Its own filter, drive, stereo width, volume, and pan processing

The active slot can be switched in the UI, and APVTS parameter values are swapped in and out of `SlotState` snapshots — effectively giving each slot the behavior of a standalone instrument layer.

### Voice Structure (per slot)
Each of the 8 `GranularVoice` instances per slot renders three sound sources in parallel:

| Source | Description |
|--------|-------------|
| **Grain Engine** | Wavetable grains with scheduling, spray, pitch scatter, pan spread |
| **Sub Oscillator** | Bass-focused independent oscillator layer |
| **OSC2 Layer** | Harmonic overlay with stereo detune mode |

Each voice maintains per-voice ADSR, glide, pitch bend, velocity scaling, and phase state independently.

---

## Synthesis Sources

### A) Granular Wavetable Engine
- 8 base wavetables: Sine, Saw, Square, Triangle, Warm Sine, Soft Saw, Noise, Hybrid
- Per-grain waveform position advances by pitch increment
- Morphing between Wave A and Wave B
- Hann-like windowing per grain to eliminate clicks
- Unison: 1–8 voices with detune, spread, phase offset
- **Tilt shaping:**
  - Positive tilt → `tanh` drive / high-end brightening
  - Negative tilt → cubic soft darkening

### B) Sub Oscillator
- 8 bass-focused tables: Deep Sine, Sub Octave, Warm Bass, Fat Saw, and more
- Per-voice phase, pitch offsets, pan, and mix level
- Designed as a foundational low-end layer under the grain engine

### C) OSC2 Harmonic Layer
- 8 harmonic tables: Bright Saw, Pulse 33%, Formant, Bell, Shimmer, and more
- **Stereo detune mode** — left and right frequencies offset independently around base pitch
- Per-voice phase, pan, detune, octave/semi/fine tuning

---

## Grain Scheduling

Grain generation is event-driven, not sample-by-sample random generation:

- `GrainScheduler` spawns grains based on **density** (grains/sec), spray, pitch scatter, pan spread, and grain size
- `GrainPool` is a fixed 128-grain pool that reuses grain structs — zero dynamic allocation in the render path
- Each grain applies a Hann-like amplitude window for clean transient behavior

---

## Modulation System

### LFOs & Envelopes
- 4 LFO parameter sets per slot
- LFO1 advances per audio block (`tickLfo`) with per-slot runtime value storage
- 4 envelope parameter sets:
  - **ENV1** — per-voice amplitude ADSR (inside `GranularVoice`)
  - **ENV2/ENV3/ENV4** — slot-level ADSRs evaluated in the processor

### Routing
LFO and envelope amounts can target: `cutoff`, `position`, `pitch`, `density`

ENV4 includes a destination selector:
- Off / Cutoff / Position / Pitch / Density

Filter also supports direct ENV and LFO amount routing plus keyboard tracking.

---

## Signal Flow

```
Per Slot:
  GranularVoice × 8
  (Grain Engine + Sub OSC + OSC2)
        ↓
  Slot Filter (SVF)
        ↓
  NaN/Inf Sanitization
        ↓
  Nonlinear Drive (tanh)
        ↓
  Stereo Width (M/S)
        ↓
  Volume / Pan
        ↓
━━━━━━━━━━━━━━━━━━━━━
All 4 Slots → Sum
        ↓
  Global Reverb
        ↓
  Global Chorus
        ↓
  Visualizer FIFO
        ↓
     Output
```

---

## Preset System

Two layers of persistence:

**Session state** (`getStateInformation` / `setStateInformation`)
- Saves full APVTS + active slot index + all 4 slot states + per-slot preset names

**Preset files** (`.oyster`)
- `PresetManager::savePreset` stores the active slot into a preset file
- `loadInitialPresetIntoAllSlots` sets a default sound across all slots on first load, with oscillators disabled on slots 2–4 so slot 1 is immediately audible

---

## Plugin Formats

| Format | Status |
|--------|--------|
| AU (Audio Unit) | ✅ macOS |
| VST3 | ✅ macOS / Windows |
| Standalone | ✅ macOS / Windows |

**Manufacturer:** 3LIXIR MUSIC  
**Product Name:** OYSTER  

---

## Tech Stack

- **C++** — core DSP and synthesis engine
- **JUCE Framework** — plugin architecture, UI, audio processing
- **Xcode** — macOS build
- **Visual Studio** — Windows build

---

## Key Source Files

| File | Description |
|------|-------------|
| `PluginProcessor.cpp/h` | Main audio engine, signal chain, slot management |
| `GranularVoice.cpp/h` | Per-voice synthesis with all three source layers |
| `GrainScheduler.cpp/h` | Event-driven grain spawning and parameter control |
| `GrainPool.cpp/h` | Fixed-size allocation-free grain pool |
| `WavetableEngine.cpp/h` | 8-table wavetable engine with morphing and tilt |
| `SubOscEngine.h` | Dedicated sub oscillator engine |
| `Osc2Engine.h` | Harmonic OSC2 layer with stereo detune |
| `SlotState.h` | Per-slot parameter snapshot and state model |
| `PresetManager.cpp/h` | Preset save/load and initialization logic |
| `PluginEditor.cpp` | UI and slot switching |

---

## About

OYSTER is developed and maintained by **Jared Frazier** under the **3LIXIR MUSIC** brand.  
Website: [3lixirmusic.com](https://3lixirmusic.com)  
Spotify: [3LIXIR MUSIC on Spotify](https://open.spotify.com/artist/2HTOPh4lH3ThbFmSrxeal7)  
YouTube: [3LIXIR MUSIC on YouTube](https://www.youtube.com/channel/UC6XPAtZH9cEUUIe0Qt3CjiQ)
