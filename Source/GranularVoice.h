#pragma once

#include <JuceHeader.h>

#include "GrainPool.h"
#include "GrainScheduler.h"
#include "WavetableEngine.h"
#include "SubOscEngine.h"
#include "Osc2Engine.h"

class GranularSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class GranularVoice : public juce::SynthesiserVoice
{
public:
    GranularVoice (WavetableEngine& wavetableEngineToUse,
                   SubOscEngine& subOscEngineToUse,
                   Osc2Engine& osc2EngineToUse);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void setGrainParameters (float newPosition,
                             float newSpray,
                             float newGrainSizeMs,
                             float newDensity,
                             float newPitchScatter,
                             float newPanSpread) noexcept;

    void setWavetableParameters (int newWaveA, int newWaveB, float newMorphAmount) noexcept;
    void setWavetableSourceParams (int newWtOctave,
                                   int newWtSemitone,
                                   float newWtFine,
                                   float newWtPhase,
                                   float newWtTilt,
                                   int newUnisonVoices,
                                   float newUnisonDetune,
                                   float newUnisonSpread) noexcept;
    void setPitchMultiplier (float mult) noexcept;
    void setGlide (float glideSeconds) noexcept;
    void setVelocitySensitivity (float sens) noexcept;
    void setBendRange (int semisUp, int semisDown) noexcept;

    void setDensity (float newDensity) noexcept { density = newDensity; scheduler.setDensity (newDensity); }
    void setGrainSizeSamples (int newGrainSizeSamples) noexcept { grainSizeSamples = newGrainSizeSamples; scheduler.setGrainSizeSamples (newGrainSizeSamples); }
    void setSpray (float newSpray) noexcept { spray = newSpray; scheduler.setSpray (newSpray); }
    void setPitchScatter (float newPitchScatter) noexcept { pitchScatter = newPitchScatter; scheduler.setPitchScatter (newPitchScatter); }
    void setPanSpread (float newPanSpread) noexcept { panSpread = newPanSpread; scheduler.setPanSpread (newPanSpread); }
    void setWaveA (int newWaveA) noexcept { waveA = newWaveA; }
    void setWaveB (int newWaveB) noexcept { waveB = newWaveB; }
    void setMorphAmount (float newMorphAmount) noexcept { morphAmount = juce::jlimit (0.0f, 1.0f, newMorphAmount); }
    void setAdsrParameters (float attack, float decay, float sustain, float release) noexcept
    {
        juce::ADSR::Parameters params;
        params.attack  = attack;
        params.decay   = decay;
        params.sustain = sustain;
        params.release = release;
        adsr.setParameters (params);
    }

    // Sub Osc parameter setters
    void setSubOscParams (int wave, float octave, float semi, float tune,
                          float mix, float pan, float phase, bool enabled) noexcept
    {
        subWave = wave; subOctave = octave; subSemi = semi; subTune = tune;
        subMix = mix; subPan = pan; subPhase = phase; subEnabled = enabled;
    }

    // OSC 2 parameter setters
    void setOsc2Params (int wave, float octave, float semi, float fine,
                        float phase, float mix, float detune, float pan, bool enabled) noexcept
    {
        o2Wave = wave; o2Octave = octave; o2Semi = semi; o2Fine = fine;
        o2Phase = phase; o2Mix = mix; o2Detune = detune; o2Pan = pan; o2Enabled = enabled;
    }

    void setGrainEnabled (bool enabled) noexcept { grainEnabled = enabled; }

private:
    WavetableEngine& wavetableEngine;
    SubOscEngine&    subOscEngine;
    Osc2Engine&      osc2Engine;
    GrainPool grainPool;
    GrainScheduler scheduler;

    float density = 20.0f;
    int grainSizeSamples = 2048;
    float spray = 0.1f;
    float pitchScatter = 0.1f;
    float panSpread = 1.0f;
    float position = 0.0f;
    float morphAmount = 0.0f;
    int waveA = 0;
    int waveB = 1;
    int   wtOctave = 0;
    int   wtSemitone = 0;
    float wtFine = 0.0f;
    float wtPhase = 0.0f;
    float wtTilt = 0.0f;
    int   unisonVoices = 1;
    float unisonDetune = 0.0f;
    float unisonSpread = 0.5f;
    float pitchMultiplier   { 1.0f };
    float glideSeconds      { 0.0f };
    float currentGlidePitch { 1.0f };
    float targetGlidePitch  { 1.0f };
    float velSensitivity    { 1.0f };
    int   bendRangeUp       { 2 };
    int   bendRangeDown     { 2 };
    float currentPitchBend  { 1.0f };
    bool noteIsActive { false };
    bool grainEnabled { true };

    // Per-voice polyphony support
    juce::ADSR adsr;
    int currentMidiNote { 60 };
    float voiceVelocity { 1.0f };

    // Per-voice Sub Osc state
    float subOscPhase { 0.0f };
    int   subWave     { 0 };
    float subOctave   { 0.0f };
    float subSemi     { 0.0f };
    float subTune     { 0.0f };
    float subMix      { 0.5f };
    float subPan      { 0.5f };
    float subPhase    { 0.0f };
    bool  subEnabled  { false };

    // Per-voice OSC 2 state
    float osc2PhaseL  { 0.0f };
    float osc2PhaseR  { 0.0f };
    int   o2Wave      { 0 };
    float o2Octave    { 0.0f };
    float o2Semi      { 0.0f };
    float o2Fine      { 0.0f };
    float o2Phase     { 0.0f };
    float o2Mix       { 0.5f };
    float o2Detune    { 0.0f };
    float o2Pan       { 0.5f };
    bool  o2Enabled   { false };
};
