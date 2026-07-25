/*
  ==============================================================================
    OysterAudioProcessor.cpp
    Granular Synthesizer — JUCE Plugin Processor
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetManager.cpp"
#include <cmath>

//==============================================================================
OysterAudioProcessor::OysterAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                     ),
      apvts (*this, nullptr, "Parameters", createParameters())
{
    // Cache parameter pointers — do this once, not every block
    positionParam     = apvts.getRawParameterValue ("position");
    sprayParam        = apvts.getRawParameterValue ("spray");
    grainSizeParam    = apvts.getRawParameterValue ("grainSize");
    densityParam      = apvts.getRawParameterValue ("density");
    pitchScatterParam = apvts.getRawParameterValue ("pitchScatter");
    panSpreadParam    = apvts.getRawParameterValue ("panSpread");

    waveAParam        = apvts.getRawParameterValue ("waveA");
    waveBParam        = apvts.getRawParameterValue ("waveB");
    morphAmountParam  = apvts.getRawParameterValue ("morphAmount");

    filterCutoffParam = apvts.getRawParameterValue ("filterCutoff");
    filterResParam    = apvts.getRawParameterValue ("filterRes");
    filterTypeParam   = apvts.getRawParameterValue ("filterType");
    filterDriveParam  = apvts.getRawParameterValue ("filterDrive");
    filterEnvAmtParam = apvts.getRawParameterValue ("filterEnvAmt");
    filterKeyTrackParam = apvts.getRawParameterValue ("filterKeyTrack");

    glideParam       = apvts.getRawParameterValue ("glide");
    bendUpParam      = apvts.getRawParameterValue ("bendUp");
    bendDownParam    = apvts.getRawParameterValue ("bendDown");
    velSensParam     = apvts.getRawParameterValue ("velSens");
    transposeParam   = apvts.getRawParameterValue ("transpose");
    octaveShiftParam = apvts.getRawParameterValue ("octaveShift");
    stereoWidthParam = apvts.getRawParameterValue ("stereoWidth");
    masterTuneParam  = apvts.getRawParameterValue ("masterTune");
    masterVolumeParam = apvts.getRawParameterValue ("masterVolume");
    masterPanParam    = apvts.getRawParameterValue ("masterPan");
    lfoToCutoffParam   = apvts.getRawParameterValue ("lfoToCutoff");
    lfoToPositionParam = apvts.getRawParameterValue ("lfoToPosition");
    lfoToPitchParam    = apvts.getRawParameterValue ("lfoToPitch");
    lfoToDensityParam  = apvts.getRawParameterValue ("lfoToDensity");
    envToCutoffParam   = apvts.getRawParameterValue ("envToCutoff");
    envToPositionParam = apvts.getRawParameterValue ("envToPosition");
    envToPitchParam    = apvts.getRawParameterValue ("envToPitch");
    envToAmpParam      = apvts.getRawParameterValue ("envToAmp");

    reverbSizeParam   = apvts.getRawParameterValue ("reverbSize");
    reverbMixParam    = apvts.getRawParameterValue ("reverbMix");
    chorusMixParam    = apvts.getRawParameterValue ("chorusMix");

    subOscEnabledParam = apvts.getRawParameterValue ("subOscEnabled");
    osc2EnabledParam   = apvts.getRawParameterValue ("osc2Enabled");
    grainEnabledParam  = apvts.getRawParameterValue ("grainEnabled");

    osc2WaveParam   = apvts.getRawParameterValue ("osc2Wave");
    osc2OctaveParam = apvts.getRawParameterValue ("osc2Octave");
    osc2SemiParam   = apvts.getRawParameterValue ("osc2Semi");
    osc2FineParam   = apvts.getRawParameterValue ("osc2Fine");
    osc2PhaseParam  = apvts.getRawParameterValue ("osc2Phase");
    osc2MixParam    = apvts.getRawParameterValue ("osc2Mix");
    osc2DetuneParam = apvts.getRawParameterValue ("osc2Detune");
    osc2PanParam    = apvts.getRawParameterValue ("osc2Pan");
    subWaveParam   = apvts.getRawParameterValue ("subWave");
    subOctaveParam = apvts.getRawParameterValue ("subOctave");
    subSemiParam   = apvts.getRawParameterValue ("subSemi");
    subTuneParam   = apvts.getRawParameterValue ("subTune");
    subMixParam    = apvts.getRawParameterValue ("subMix");
    subPanParam    = apvts.getRawParameterValue ("subPan");
    subPhaseParam  = apvts.getRawParameterValue ("subPhase");

    for (int n = 0; n < 4; ++n)
        lfoEnabledParams[n] = apvts.getRawParameterValue ("lfo" + juce::String (n + 1) + "Enabled");
    env4DestParam = apvts.getRawParameterValue ("env4Dest");

    initialiseSynth();

    // Load the built-in Initial preset into all slots
    presetManager.loadInitialPresetIntoAllSlots();

   #if JucePlugin_Build_Standalone
    // Splash screen should only run in standalone app mode.
    // Creating UI in plugin/AU validation context can destabilize host validation.
    auto splashFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                          .getParentDirectory().getChildFile ("OysterSplash.png");
    if (! splashFile.existsAsFile())
        splashFile = juce::File ("/Users/jaredfrazier/Synth Projects/Oyster/Source/OysterSplash.png");

    auto img = juce::ImageFileFormat::loadFrom (splashFile);
    if (img.isValid())
    {
        auto* splash = new juce::SplashScreen ("Oyster", img, true);
        splash->deleteAfterDelay (juce::RelativeTime::seconds (3), false);
    }
   #endif
}

OysterAudioProcessor::~OysterAudioProcessor() {}

//==============================================================================
void OysterAudioProcessor::initialiseSynth()
{
    for (int s = 0; s < numSlots; ++s)
    {
        synths[s].clearVoices();
        synths[s].clearSounds();

        for (int i = 0; i < 8; ++i)
            synths[s].addVoice (new GranularVoice (wavetableEngine, subOscEngine, osc2Engine));

        synths[s].addSound (new GranularSound());
    }
}

//==============================================================================
void OysterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Acquire the DSP lock so processBlock outputs silence while we
    // reinitialise.  This eliminates the race where processBlock reads
    // half-reset filter/reverb/chorus state and produces permanent crackling.
    const juce::SpinLock::ScopedLockType scopedLock (dspLock);

    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    for (int s = 0; s < numSlots; ++s)
    {
        synths[s].setCurrentPlaybackSampleRate (sampleRate);
        for (int e = 0; e < 3; ++e)
            slotEnvs[s][e].setSampleRate (sampleRate);
    }

    // Prepare DSP chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;

    // Only re-prepare (which resets internal state) when the sample rate
    // or block size actually change.  Logic calls prepareToPlay when Musical
    // Typing opens/closes — if we reset the filter every time, notes that are
    // playing get a sudden discontinuity in the filter state → crackling.
    const bool specChanged = (sampleRate != lastPreparedSampleRate)
                          || (samplesPerBlock != lastPreparedBlockSize);

    if (specChanged)
    {
        for (int s = 0; s < numSlots; ++s)
        {
            filters[s].prepare (spec);
            filters[s].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        }

        reverb.prepare (spec);

        chorus.prepare (spec);
        chorus.setRate (0.3f);
        chorus.setDepth (0.15f);
        chorus.setCentreDelay (7.0f);
        chorus.setFeedback (0.1f);

        lastPreparedSampleRate = sampleRate;
        lastPreparedBlockSize  = samplesPerBlock;
    }

    slotBuffer.setSize (2, samplesPerBlock);
}

void OysterAudioProcessor::releaseResources()
{
    const juce::SpinLock::ScopedLockType scopedLock (dspLock);
    for (int s = 0; s < numSlots; ++s)
        filters[s].reset();
    reverb.reset();
    chorus.reset();
}

//==============================================================================
void OysterAudioProcessor::updateParameters()
{
    // Sync current APVTS to active slot
    slotStates[activeSlot].loadFromAPVTS (apvts);

    // Update all 4 slots from their respective states
    for (int s = 0; s < numSlots; ++s)
        updateSlotFromState (s);

    // Global FX
    reverbParams.roomSize = reverbSizeParam->load();
    reverbParams.wetLevel = reverbMixParam->load();
    reverbParams.dryLevel = 1.0f - (reverbMixParam->load() * 0.5f);
    reverbParams.damping  = 0.5f;
    reverbParams.width    = 1.0f;
    reverb.setParameters (reverbParams);
}

//==============================================================================
void OysterAudioProcessor::updateSlotFromState (int s)
{
    auto& state = slotStates[s];
    auto& filt  = filters[s];
    auto& syn   = synths[s];

    // Update slot envelope ADSR params (ENV 2, 3, 4)
    slotEnvs[s][0].setParameters ({ state.envAttack[1], state.envDecay[1], state.envSustain[1], state.envRelease[1] });
    slotEnvs[s][1].setParameters ({ state.envAttack[2], state.envDecay[2], state.envSustain[2], state.envRelease[2] });
    slotEnvs[s][2].setParameters ({ state.envAttack[3], state.envDecay[3], state.envSustain[3], state.envRelease[3] });

    // Pre-compute ENV 4 destination contributions
    const float env4Amt       = state.envToAmp * state.currentEnv4Value;
    const float env4CutoffAdd = (state.env4Dest == 1) ? env4Amt * 8000.0f  : 0.0f;
    const float env4PosAdd    = (state.env4Dest == 2) ? env4Amt * 0.3f     : 0.0f;
    const float env4PitchAdd  = (state.env4Dest == 3) ? env4Amt * 0.5f     : 0.0f;
    const float env4DensMult  = (state.env4Dest == 4) ? (1.0f + env4Amt * 2.0f) : 1.0f;

    // Filter type
    switch (state.filterType)
    {
        case 0: filt.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
        case 1: filt.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
        case 2: filt.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: filt.setType (juce::dsp::StateVariableTPTFilterType::lowpass); break;
    }
    {
        const float safeRes = std::isfinite (state.filterRes)
            ? juce::jlimit (0.1f, 10.0f, state.filterRes)
            : 0.7f;
        filt.setResonance (safeRes);
    }

    // Modulated cutoff — guard against NaN from modulation sources
    // (juce::jlimit does NOT catch NaN; comparisons with NaN are always false)
    const float keyTrackOffset = state.filterKeyTrack
        * ((float)(state.lastMidiNote - 60) / 60.0f) * 8000.0f;
    const float envCutoffAmt = state.envToCutoff * state.currentEnv2Value * 8000.0f;
    const float lfoCutoffAmt = state.lfoToCutoff * state.currentLfoValue * 4000.0f;
    const float envAmtDirect = state.filterEnvAmt * state.currentEnv2Value * 10000.0f;
    const float lfoAmtDirect = state.filterLfoAmt * state.currentLfoValue * 10000.0f;
    const float rawCutoff = state.filterCutoff + keyTrackOffset + envCutoffAmt
                          + lfoCutoffAmt + envAmtDirect + lfoAmtDirect + env4CutoffAdd;
    const float modulatedCutoff = std::isfinite (rawCutoff)
        ? juce::jlimit (20.0f, 20000.0f, rawCutoff)
        : 8000.0f;   // safe fallback if any mod source is NaN
    filt.setCutoffFrequency (modulatedCutoff);

    for (int i = 0; i < syn.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<GranularVoice*> (syn.getVoice (i)))
        {
            const float lfoPosMod = state.lfoToPosition * state.currentLfoValue * 0.3f;
            const float envPosMod = state.envToPosition * state.currentEnv3Value * 0.3f;
            const float modulatedPosition = juce::jlimit (0.0f, 1.0f,
                state.position + lfoPosMod + envPosMod + env4PosAdd);

            const float env4Density = state.density * env4DensMult;
            const float lfoDensityMod = state.lfoToDensity * state.currentLfoValue * 50.0f;
            voice->setGrainParameters (modulatedPosition, state.spray, state.grainSize,
                                       juce::jlimit (1.0f, 100.0f, env4Density + lfoDensityMod),
                                       state.pitchScatter, state.panSpread);

            voice->setWavetableParameters (state.waveA, state.waveB, state.morphAmount);
            voice->setWavetableSourceParams (state.wtOctave, state.wtSemitone, state.wtFine,
                                             state.wtPhase, state.wtTilt, state.unisonVoices,
                                             state.unisonDetune, state.unisonSpread);

            const float semis = (float) state.transpose
                              + (float) state.octaveShift * 12.0f
                              + state.masterTune / 100.0f;
            const float lfoPitchMod = state.lfoToPitch * state.currentLfoValue * 0.5f;
            const float envPitchMod = state.envToPitch * state.currentEnv3Value * 0.5f;
            const float pitchMult = std::pow (2.0f, (semis + lfoPitchMod + envPitchMod + env4PitchAdd) / 12.0f);

            voice->setPitchMultiplier (pitchMult);
            voice->setGlide (state.glide);
            voice->setVelocitySensitivity (state.velSens);
            voice->setBendRange (state.bendUp, state.bendDown);
            voice->setGrainEnabled (state.grainEnabled);

            voice->setSubOscParams (
                state.subWave, (float) state.subOctave, (float) state.subSemi,
                state.subTune, state.subMix, state.subPan, state.subPhase,
                state.subOscEnabled);

            voice->setOsc2Params (
                state.osc2Wave, (float) state.osc2Octave, (float) state.osc2Semi,
                state.osc2Fine, state.osc2Phase, state.osc2Mix, state.osc2Detune,
                state.osc2Pan, state.osc2Enabled);

            voice->setAdsrParameters (state.envAttack[0], state.envDecay[0],
                                      state.envSustain[0], state.envRelease[0]);
        }
    }
}

//==============================================================================
void OysterAudioProcessor::tickLfo (SlotState& state, int numSamples)
{
    if (! state.lfoEnabled[0])
    {
        state.currentLfoValue = 0.0f;
        return;
    }

    state.lfoPhaseRuntime += (state.lfoRate[0] / (float) currentSampleRate) * (float) numSamples;
    if (state.lfoPhaseRuntime >= 1.0f) state.lfoPhaseRuntime -= 1.0f;

    const float angle = state.lfoPhaseRuntime * juce::MathConstants<float>::twoPi;
    switch (state.lfoShape[0])
    {
        case 0: state.currentLfoValue = std::sin (angle); break;
        case 1: state.currentLfoValue = state.lfoPhaseRuntime < 0.5f
                    ? state.lfoPhaseRuntime * 4.0f - 1.0f
                    : 3.0f - state.lfoPhaseRuntime * 4.0f; break;
        case 2: state.currentLfoValue = state.lfoPhaseRuntime * 2.0f - 1.0f; break;
        case 3: state.currentLfoValue = state.lfoPhaseRuntime < 0.5f ? 1.0f : -1.0f; break;
        default: state.currentLfoValue = std::sin (angle); break;
    }
    state.currentLfoValue *= state.lfoDepth[0];

    // Guard against NaN from bad phase arithmetic
    if (! std::isfinite (state.currentLfoValue))
        state.currentLfoValue = 0.0f;
}

//==============================================================================
void OysterAudioProcessor::switchSlot (int newSlot)
{
    if (newSlot < 0 || newSlot >= numSlots || newSlot == activeSlot) return;
    slotStates[activeSlot].loadFromAPVTS (apvts);
    activeSlot = newSlot;
    slotStates[activeSlot].writeToAPVTS (apvts);
}

//==============================================================================
void OysterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Try to acquire the DSP lock.  If prepareToPlay holds it (e.g. Logic
    // reopened Musical Typing), output silence rather than touching
    // half-initialised filter/reverb/chorus state.
    const juce::SpinLock::ScopedTryLockType tryLock (dspLock);
    if (! tryLock.isLocked())
    {
        buffer.clear();
        return;
    }

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Always inject the on-screen keyboard's MIDI events so the built-in
    // keyboard works in both standalone and plugin modes.
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // Track last MIDI note + trigger slot-level envelopes (ENV 2, 3, 4)
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            for (int s = 0; s < numSlots; ++s)
            {
                slotStates[s].lastMidiNote = msg.getNoteNumber();
                if (heldNoteCount[s] == 0)
                    for (int e = 0; e < 3; ++e)
                        slotEnvs[s][e].noteOn();
                heldNoteCount[s]++;
            }
        }
        else if (msg.isNoteOff())
        {
            for (int s = 0; s < numSlots; ++s)
            {
                heldNoteCount[s] = juce::jmax (0, heldNoteCount[s] - 1);
                if (heldNoteCount[s] == 0)
                    for (int e = 0; e < 3; ++e)
                        slotEnvs[s][e].noteOff();
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            // Musical Typing (and other controllers) send CC 123/120
            // when opening/closing — reset all slot envelope state so
            // nothing gets stuck.
            for (int s = 0; s < numSlots; ++s)
            {
                heldNoteCount[s] = 0;
                for (int e = 0; e < 3; ++e)
                {
                    slotEnvs[s][e].noteOff();
                    slotEnvs[s][e].reset();
                }
            }
        }
    }

    // Tick LFO and slot envelopes for each slot
    for (int s = 0; s < numSlots; ++s)
    {
        tickLfo (slotStates[s], buffer.getNumSamples());

        float env2Val = 0.0f, env3Val = 0.0f, env4Val = 0.0f;
        const int n = buffer.getNumSamples();
        for (int samp = 0; samp < n; ++samp)
        {
            env2Val = slotEnvs[s][0].getNextSample();
            env3Val = slotEnvs[s][1].getNextSample();
            env4Val = slotEnvs[s][2].getNextSample();
        }
        slotStates[s].currentEnv2Value = std::isfinite (env2Val) ? env2Val : 0.0f;
        slotStates[s].currentEnv3Value = std::isfinite (env3Val) ? env3Val : 0.0f;
        slotStates[s].currentEnv4Value = std::isfinite (env4Val) ? env4Val : 0.0f;
    }

    updateParameters();

    // Clear output and render all slots
    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    for (int s = 0; s < numSlots; ++s)
    {
        slotBuffer.clear();
        synths[s].renderNextBlock (slotBuffer, midiMessages, 0, numSamples);

        // Sanitise: if any voice produced NaN/Inf, clear the slot buffer
        // rather than letting it poison the filter/reverb/chorus permanently.
        {
            bool bad = false;
            for (int ch = 0; ch < slotBuffer.getNumChannels() && !bad; ++ch)
            {
                const auto* data = slotBuffer.getReadPointer (ch);
                for (int samp = 0; samp < numSamples; ++samp)
                {
                    if (! std::isfinite (data[samp]))
                    { bad = true; break; }
                }
            }
            if (bad)
            {
                slotBuffer.clear();
                // Also reset the filter so it doesn't stay stuck on NaN
                filters[s].reset();
            }
        }

        // Per-slot filter — use exactly numSamples, not the full slotBuffer
        {
            juce::dsp::AudioBlock<float> slotBlock (slotBuffer.getArrayOfWritePointers(),
                                                     (size_t) slotBuffer.getNumChannels(),
                                                     (size_t) numSamples);
            juce::dsp::ProcessContextReplacing<float> slotCtx (slotBlock);
            filters[s].process (slotCtx);

            // If the filter's internal state has been corrupted (NaN/Inf),
            // clear the output and reset the filter so it recovers.
            bool filterBad = false;
            for (int ch = 0; ch < slotBuffer.getNumChannels() && !filterBad; ++ch)
            {
                const auto* data = slotBuffer.getReadPointer (ch);
                for (int samp = 0; samp < numSamples; ++samp)
                {
                    if (! std::isfinite (data[samp]))
                    { filterBad = true; break; }
                }
            }
            if (filterBad)
            {
                slotBuffer.clear();
                filters[s].reset();
            }
        }

        // Per-slot drive
        {
            const float drive = 1.0f + slotStates[s].filterDrive * 8.0f;
            for (int ch = 0; ch < slotBuffer.getNumChannels(); ++ch)
            {
                auto* data = slotBuffer.getWritePointer (ch);
                for (int samp = 0; samp < numSamples; ++samp)
                    data[samp] = std::tanh (data[samp] * drive) / std::tanh (drive);
            }
        }

        // Per-slot stereo width
        if (slotBuffer.getNumChannels() >= 2)
        {
            const float width = slotStates[s].stereoWidth;
            const float mid   = (2.0f - width) * 0.5f;
            const float side  = width * 0.5f;
            auto* L = slotBuffer.getWritePointer (0);
            auto* R = slotBuffer.getWritePointer (1);
            for (int samp = 0; samp < numSamples; ++samp)
            {
                const float m  = (L[samp] + R[samp]) * mid;
                const float sd = (L[samp] - R[samp]) * side;
                L[samp] = m + sd;
                R[samp] = m - sd;
            }
        }

        // Per-slot volume + pan
        {
            const float vol = slotStates[s].masterVolume;
            const float pan = slotStates[s].masterPan;
            const float gainL = vol * std::min (1.0f, 1.0f - pan);
            const float gainR = vol * std::min (1.0f, 1.0f + pan);

            if (slotBuffer.getNumChannels() >= 2)
            {
                slotBuffer.applyGain (0, 0, numSamples, gainL);
                slotBuffer.applyGain (1, 0, numSamples, gainR);
            }
            else
            {
                slotBuffer.applyGain (vol);
            }
        }

        // Accumulate into main buffer
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom (ch, 0, slotBuffer, ch, 0, numSamples);
    }

    // Global FX — reverb
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);

    // Chorus — wet/dry blend via chorusMix param
    if (chorusMixParam->load() > 0.001f)
    {
        juce::AudioBuffer<float> chorusBuffer;
        chorusBuffer.makeCopyOf (buffer);
        juce::dsp::AudioBlock<float> chorusBlock (chorusBuffer);
        juce::dsp::ProcessContextReplacing<float> chorusContext (chorusBlock);
        chorus.process (chorusContext);

        const float mix = chorusMixParam->load();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom (ch, 0, chorusBuffer, ch, 0, numSamples, mix);
    }

    // Feed left channel samples to visualizer
    auto* leftChannel = buffer.getReadPointer (0);
    for (int samp = 0; samp < numSamples; ++samp)
        pushSampleToVisualizer (leftChannel[samp]);
}

void OysterAudioProcessor::pushSampleToVisualizer (float sample)
{
    const auto scope = vizFifo.write (1);
    if (scope.blockSize1 > 0)
        vizBuffer[(size_t) scope.startIndex1] = sample;
    else if (scope.blockSize2 > 0)
        vizBuffer[(size_t) scope.startIndex2] = sample;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OysterAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Grain
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("position",     "Position",     0.0f,    1.0f,    0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("spray",        "Spray",        0.0f,    1.0f,    0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("grainSize",    "Grain Size",   20.0f,   500.0f,  200.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("density",      "Density",      1.0f,    100.0f,  40.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("pitchScatter", "Pitch Scatter",0.0f,    100.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("panSpread",    "Pan Spread",   0.0f,    1.0f,    0.5f));

    // Wavetable
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("waveA",        "Wave A",       0,       7,       0));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("waveB",        "Wave B",       0,       7,       1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("morphAmount",  "Morph",        0.0f,    1.0f,    0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("wtOctave",     "Octave",      -3,       3,       0));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("wtSemitone",   "Semitone",   -12,      12,       0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("wtFine",       "Fine",      -100.0f,  100.0f,    0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("wtPhase",      "Phase",        0.0f,    1.0f,    0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("wtTilt",       "Tilt",        -1.0f,    1.0f,    0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("unisonVoices", "Unison Voices", 1,       8,       1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("unisonDetune", "Unison Detune", 0.0f, 100.0f,    0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("unisonSpread", "Unison Spread", 0.0f,   1.0f,    0.5f));

    // Filter
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterCutoff", "Cutoff",       20.0f,   20000.0f, 8000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterRes",    "Resonance",    0.1f,    10.0f,   0.7f));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("filterType", "Filter Type", 0, 3, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterDrive", "Filter Drive", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterEnvAmt",  "Filter Env Amount", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterLfoAmt",  "Filter LFO Amount", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterKeyTrack", "Filter Key Track",  0.0f, 1.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo1Rate",      "LFO 1 Rate",      0.01f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo1Depth",     "LFO 1 Depth",     0.0f,  1.0f,  0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("lfo1Shape",     "LFO 1 Shape",     0, 4, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo1Attack",    "LFO 1 Attack",    0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo1Decay",     "LFO 1 Decay",     0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo1Sync",      "LFO 1 Sync",      false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo1Retrigger", "LFO 1 Retrigger", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo1Phase",     "LFO 1 Phase",     false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo2Rate",      "LFO 2 Rate",      0.01f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo2Depth",     "LFO 2 Depth",     0.0f,  1.0f,  0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("lfo2Shape",     "LFO 2 Shape",     0, 4, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo2Attack",    "LFO 2 Attack",    0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo2Decay",     "LFO 2 Decay",     0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo2Sync",      "LFO 2 Sync",      false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo2Retrigger", "LFO 2 Retrigger", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo2Phase",     "LFO 2 Phase",     false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo3Rate",      "LFO 3 Rate",      0.01f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo3Depth",     "LFO 3 Depth",     0.0f,  1.0f,  0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("lfo3Shape",     "LFO 3 Shape",     0, 4, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo3Attack",    "LFO 3 Attack",    0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo3Decay",     "LFO 3 Decay",     0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo3Sync",      "LFO 3 Sync",      false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo3Retrigger", "LFO 3 Retrigger", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo3Phase",     "LFO 3 Phase",     false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo4Rate",      "LFO 4 Rate",      0.01f, 20.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo4Depth",     "LFO 4 Depth",     0.0f,  1.0f,  0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("lfo4Shape",     "LFO 4 Shape",     0, 4, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo4Attack",    "LFO 4 Attack",    0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfo4Decay",     "LFO 4 Decay",     0.0f,  4.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo4Sync",      "LFO 4 Sync",      false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo4Retrigger", "LFO 4 Retrigger", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo4Phase",     "LFO 4 Phase",     false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo1Enabled",   "LFO 1 On",        true));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo2Enabled",   "LFO 2 On",        true));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo3Enabled",   "LFO 3 On",        true));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("lfo4Enabled",   "LFO 4 On",        true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envAttack", "Attack", 0.001f, 4.0f, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envDecay", "Decay", 0.001f, 4.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envSustain", "Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envRelease", "Release", 0.001f, 8.0f, 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env2Attack",  "Env 2 Attack",  0.001f, 4.0f, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env2Decay",   "Env 2 Decay",   0.001f, 4.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env2Sustain", "Env 2 Sustain", 0.0f,   1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env2Release", "Env 2 Release", 0.001f, 8.0f, 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env3Attack",  "Env 3 Attack",  0.001f, 4.0f, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env3Decay",   "Env 3 Decay",   0.001f, 4.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env3Sustain", "Env 3 Sustain", 0.0f,   1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env3Release", "Env 3 Release", 0.001f, 8.0f, 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env4Attack",  "Env 4 Attack",  0.001f, 4.0f, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env4Decay",   "Env 4 Decay",   0.001f, 4.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env4Sustain", "Env 4 Sustain", 0.0f,   1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("env4Release", "Env 4 Release", 0.001f, 8.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("env4Dest",    "Env 4 Dest",    0, 4, 0));

    // OSC 2
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("osc2Wave",    "OSC2 Wave",    0,      7,      1));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("osc2Octave",  "OSC2 Octave", -3,      3,      0));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("osc2Semi",    "OSC2 Semi",  -12,     12,      0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Fine",    "OSC2 Fine", -100.0f, 100.0f,   0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Phase",   "OSC2 Phase",  0.0f,   1.0f,   0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Mix",     "OSC2 Mix",    0.0f,   1.0f,   0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Detune",  "OSC2 Detune", 0.0f, 100.0f,   0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Pan",     "OSC2 Pan",    0.0f,   1.0f,   0.5f));

    // Sub OSC
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("subOscEnabled","Sub OSC On",   true));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("osc2Enabled",  "OSC 2 On",     true));
    params.push_back (std::make_unique<juce::AudioParameterBool>  ("grainEnabled", "Grain On",     true));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("subWave",     "Sub Wave",    0,      7,      0));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("subOctave",   "Sub Octave", -3,     -1,     -1));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("subSemi",     "Sub Semi",  -12,     12,      0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("subTune",     "Sub Tune", -100.0f, 100.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("subMix",      "Sub Mix",    0.0f,   1.0f,   0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("subPan",      "Sub Pan",    0.0f,   1.0f,   0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("subPhase",    "Sub Phase",  0.0f,   1.0f,   0.0f));

    // Control
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("glide",       "Glide",        0.0f,   2.0f,  0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("bendUp",      "Bend Up",      0,      24,    2));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("bendDown",    "Bend Down",    0,      24,    2));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("velSens",     "Velocity",     0.0f,   1.0f,  1.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("transpose",   "Transpose",   -24,     24,    0));
    params.push_back (std::make_unique<juce::AudioParameterInt>   ("octaveShift", "Octave",      -3,      3,     0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("stereoWidth", "Width",        0.0f,   1.0f,  1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("masterTune",  "Tune",        -100.0f, 100.0f, 0.0f));

    // Modulation — Row 1: LFO targets
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoToCutoff",    "LFO→Cutoff",    0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoToPosition",  "LFO→Position",  0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoToPitch",     "LFO→Pitch",     0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoToDensity",   "LFO→Density",   0.0f, 1.0f, 0.0f));

    // Modulation — Row 2: ENV targets
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envToCutoff",    "ENV→Cutoff",    0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envToPosition",  "ENV→Position",  0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envToPitch",     "ENV→Pitch",     0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("envToAmp",       "ENV→Amp",       0.0f, 1.0f, 0.0f));

    // Master
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("masterVolume", "Volume",       0.0f,    1.0f,    0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("masterPan",    "Pan",         -1.0f,    1.0f,    0.0f));

    // FX
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbSize",   "Reverb Size",  0.0f,    1.0f,    0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbMix",    "Reverb Mix",   0.0f,    1.0f,    0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("chorusMix",    "Chorus Mix",   0.0f,    1.0f,    0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
// Boilerplate below — unchanged from template

bool OysterAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OysterAudioProcessor::createEditor()
{
    return new OysterAudioProcessorEditor (*this);
}

const juce::String OysterAudioProcessor::getName() const { return JucePlugin_Name; }
bool OysterAudioProcessor::acceptsMidi() const  { return true; }
bool OysterAudioProcessor::producesMidi() const { return false; }
bool OysterAudioProcessor::isMidiEffect() const { return false; }
double OysterAudioProcessor::getTailLengthSeconds() const { return 2.0; }

int OysterAudioProcessor::getNumPrograms()                              { return 1; }
int OysterAudioProcessor::getCurrentProgram()                           { return 0; }
void OysterAudioProcessor::setCurrentProgram (int)                      {}
const juce::String OysterAudioProcessor::getProgramName (int)           { return {}; }
void OysterAudioProcessor::changeProgramName (int, const juce::String&) {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OysterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::stereo())
        return false;

    const auto mainIn = layouts.getMainInputChannelSet();
    if (! mainIn.isDisabled() && mainIn != mainOut)
        return false;

    return true;
}
#endif

void OysterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Ensure active slot is up to date
    slotStates[activeSlot].loadFromAPVTS (apvts);

    auto state = apvts.copyState();
    state.setProperty ("activeSlot", activeSlot, nullptr);
    state.setProperty ("currentPresetName", presetManager.getCurrentPresetName(), nullptr);

    for (int s = 0; s < numSlots; ++s)
    {
        state.setProperty ("presetSlotName" + juce::String (s), presetSlotNames[s], nullptr);

        auto slotTree = slotStates[s].toValueTree();
        slotTree.setProperty ("slotIndex", s, nullptr);
        state.addChild (slotTree, -1, nullptr);
    }

    auto xml = state.createXml();
    copyXmlToBinary (*xml, destData);
}

void OysterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xmlState = getXmlFromBinary (data, sizeInBytes);
    if (xmlState != nullptr)
    {
        auto tree = juce::ValueTree::fromXml (*xmlState);
        if (tree.isValid())
        {
            // Restore slot states
            for (int s = 0; s < numSlots; ++s)
            {
                for (int c = 0; c < tree.getNumChildren(); ++c)
                {
                    auto child = tree.getChild (c);
                    if (child.hasType ("SlotState") && (int) child.getProperty ("slotIndex") == s)
                    {
                        slotStates[s].fromValueTree (child);
                        break;
                    }
                }
            }

            activeSlot = (int) tree.getProperty ("activeSlot", 0);

            // Restore per-slot preset names
            for (int s = 0; s < numSlots; ++s)
            {
                juce::String defaultName = (s == 0) ? "Initial" : "[ empty ]";
                presetSlotNames[s] = tree.getProperty ("presetSlotName" + juce::String (s), defaultName).toString();
            }
            presetManager.setCurrentPresetName (
                tree.getProperty ("currentPresetName", presetSlotNames[activeSlot]).toString());

            // Remove slot children before restoring APVTS
            for (int c = tree.getNumChildren() - 1; c >= 0; --c)
            {
                if (tree.getChild (c).hasType ("SlotState"))
                    tree.removeChild (c, nullptr);
            }

            apvts.replaceState (tree);

            // Write active slot's state to APVTS
            slotStates[activeSlot].writeToAPVTS (apvts);
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OysterAudioProcessor();
}
