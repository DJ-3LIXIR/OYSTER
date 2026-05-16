/*
  ==============================================================================
    OysterAudioProcessor.h
    Granular Synthesizer — JUCE Plugin Processor
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include "GrainPool.h"
#include "GrainScheduler.h"
#include "WavetableEngine.h"
#include "GranularVoice.h"
#include "SubOscEngine.h"
#include "Osc2Engine.h"
#include "SlotState.h"
#include "PresetManager.h"

//==============================================================================
class OysterAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    OysterAudioProcessor();
    ~OysterAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void pushSampleToVisualizer (float sample);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter tree — public so Editor can attach sliders
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::MidiKeyboardState keyboardState;

    // Multi-slot
    static constexpr int numSlots = 4;
    void switchSlot (int newSlot);
    int getActiveSlot() const { return activeSlot; }

private:
    //==============================================================================
    // Synthesis engine — one per slot
    juce::Synthesiser synths[numSlots];
    WavetableEngine   wavetableEngine;
    SubOscEngine      subOscEngine;
    Osc2Engine        osc2Engine;

    // DSP — per-slot filter, global FX
    juce::dsp::StateVariableTPTFilter<float> filters[numSlots];
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
    juce::dsp::Chorus<float> chorus;

    // Parameter pointers — cached for processBlock performance
    std::atomic<float>* positionParam      { nullptr };
    std::atomic<float>* sprayParam         { nullptr };
    std::atomic<float>* grainSizeParam     { nullptr };
    std::atomic<float>* densityParam       { nullptr };
    std::atomic<float>* pitchScatterParam  { nullptr };
    std::atomic<float>* panSpreadParam     { nullptr };

    std::atomic<float>* waveAParam         { nullptr };
    std::atomic<float>* waveBParam         { nullptr };
    std::atomic<float>* morphAmountParam   { nullptr };

    std::atomic<float>* filterCutoffParam  { nullptr };
    std::atomic<float>* filterResParam     { nullptr };
    std::atomic<float>* filterTypeParam    { nullptr };
    std::atomic<float>* filterDriveParam   { nullptr };
    std::atomic<float>* filterEnvAmtParam  { nullptr };
    std::atomic<float>* filterKeyTrackParam { nullptr };

    std::atomic<float>* glideParam        { nullptr };
    std::atomic<float>* bendUpParam       { nullptr };
    std::atomic<float>* bendDownParam     { nullptr };
    std::atomic<float>* velSensParam      { nullptr };
    std::atomic<float>* transposeParam    { nullptr };
    std::atomic<float>* octaveShiftParam  { nullptr };
    std::atomic<float>* stereoWidthParam  { nullptr };
    std::atomic<float>* masterTuneParam   { nullptr };
    std::atomic<float>* lfoToCutoffParam   { nullptr };
    std::atomic<float>* lfoToPositionParam { nullptr };
    std::atomic<float>* lfoToPitchParam    { nullptr };
    std::atomic<float>* lfoToDensityParam  { nullptr };
    std::atomic<float>* envToCutoffParam   { nullptr };
    std::atomic<float>* envToPositionParam { nullptr };
    std::atomic<float>* envToPitchParam    { nullptr };
    std::atomic<float>* envToAmpParam      { nullptr };

    std::atomic<float>* masterVolumeParam  { nullptr };
    std::atomic<float>* masterPanParam     { nullptr };

    std::atomic<float>* reverbSizeParam    { nullptr };
    std::atomic<float>* reverbMixParam     { nullptr };
    std::atomic<float>* chorusMixParam     { nullptr };

    // On/off enables
    std::atomic<float>* subOscEnabledParam { nullptr };
    std::atomic<float>* osc2EnabledParam   { nullptr };
    std::atomic<float>* grainEnabledParam  { nullptr };

    // Sub OSC
    std::atomic<float>* subWaveParam   { nullptr };
    std::atomic<float>* subOctaveParam { nullptr };
    std::atomic<float>* subSemiParam   { nullptr };
    std::atomic<float>* subTuneParam   { nullptr };
    std::atomic<float>* subMixParam    { nullptr };
    std::atomic<float>* subPanParam    { nullptr };
    std::atomic<float>* subPhaseParam  { nullptr };

    // OSC 2 param pointers
    std::atomic<float>* osc2WaveParam   { nullptr };
    std::atomic<float>* osc2OctaveParam { nullptr };
    std::atomic<float>* osc2SemiParam   { nullptr };
    std::atomic<float>* osc2FineParam   { nullptr };
    std::atomic<float>* osc2PhaseParam  { nullptr };
    std::atomic<float>* osc2MixParam    { nullptr };
    std::atomic<float>* osc2DetuneParam { nullptr };
    std::atomic<float>* osc2PanParam    { nullptr };

    // Internal helpers
    void updateParameters();
    void initialiseSynth();
    void updateSlotFromState (int slotIndex);
    void tickLfo (SlotState& state, int numSamples);

    // Multi-slot state
    SlotState slotStates[numSlots];
    int activeSlot { 0 };
    juce::AudioBuffer<float> slotBuffer;

    // Slot-level envelopes for modulation (ENV 2 = filter, ENV 3 = position, ENV 4 = free)
    juce::ADSR slotEnvs[numSlots][3];
    int heldNoteCount[numSlots] {};

    std::atomic<float>* lfoEnabledParams[4] { nullptr, nullptr, nullptr, nullptr };
    std::atomic<float>* env4DestParam { nullptr };

    double currentSampleRate { 44100.0 };
    int currentBlockSize     { 512 };

    // Track what prepareToPlay was last called with, so we only reset
    // filter/reverb/chorus state when the spec actually changes.
    double lastPreparedSampleRate { 0.0 };
    int    lastPreparedBlockSize  { 0 };

    // SpinLock to protect DSP state (filter/reverb/chorus) from concurrent
    // access.  Logic calls prepareToPlay on the message thread while the
    // audio thread is still running (e.g. when Musical Typing opens).
    // processBlock uses ScopedTryLockType (non-blocking) — if the lock is
    // held by prepareToPlay it outputs silence instead of corrupting state.
    juce::SpinLock dspLock;

public:
    // Lock-free FIFO for visualizer — processor writes, editor reads
    static constexpr int vizFifoSize = 4096;
    juce::AbstractFifo vizFifo { vizFifoSize };
    std::array<float, vizFifoSize> vizBuffer {};

    // Preset management — public so Editor can call save/load
    PresetManager presetManager { apvts, slotStates, numSlots, activeSlot };

    // Per-slot preset names — stored in processor so they survive editor open/close
    juce::String presetSlotNames[numSlots] { "Initial", "[ empty ]", "[ empty ]", "[ empty ]" };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OysterAudioProcessor)
};
