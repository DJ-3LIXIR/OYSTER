/*
  ==============================================================================
    SlotState.h
    Multi-slot state snapshot for Oyster Granular Synthesizer
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct SlotState
{
    // ── Grain ────────────────────────────────────────────────────────────
    float position      = 0.5f;
    float spray         = 0.1f;
    float grainSize     = 200.0f;
    float density       = 40.0f;
    float pitchScatter  = 0.0f;
    float panSpread     = 0.5f;

    // ── Wavetable ────────────────────────────────────────────────────────
    int   waveA         = 0;
    int   waveB         = 1;
    float morphAmount   = 0.0f;
    int   wtOctave      = 0;
    int   wtSemitone    = 0;
    float wtFine        = 0.0f;
    float wtPhase       = 0.0f;
    float wtTilt        = 0.0f;
    int   unisonVoices  = 1;
    float unisonDetune  = 0.0f;
    float unisonSpread  = 0.5f;

    // ── Filter ───────────────────────────────────────────────────────────
    float filterCutoff   = 8000.0f;
    float filterRes      = 0.7f;
    int   filterType     = 0;
    float filterDrive    = 0.0f;
    float filterEnvAmt   = 0.0f;
    float filterLfoAmt   = 0.0f;
    float filterKeyTrack = 0.0f;

    // ── LFOs 1-4 (arrays indexed 0-3) ───────────────────────────────────
    float lfoRate[4]      = { 1.0f, 1.0f, 1.0f, 1.0f };
    float lfoDepth[4]     = { 0.5f, 0.5f, 0.5f, 0.5f };
    int   lfoShape[4]     = { 0, 0, 0, 0 };
    float lfoAttack[4]    = { 0.0f, 0.0f, 0.0f, 0.0f };
    float lfoDecay[4]     = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool  lfoSync[4]      = { false, false, false, false };
    bool  lfoRetrigger[4] = { false, false, false, false };
    bool  lfoPhase[4]     = { false, false, false, false };
    bool  lfoEnabled[4]   = { true,  true,  true,  true  };

    // ── Envelopes 1-4 (arrays indexed 0-3) ──────────────────────────────
    float envAttack[4]  = { 0.01f, 0.01f, 0.01f, 0.01f };
    float envDecay[4]   = { 0.1f,  0.1f,  0.1f,  0.1f  };
    float envSustain[4] = { 0.8f,  0.8f,  0.8f,  0.8f  };
    float envRelease[4] = { 0.3f,  0.3f,  0.3f,  0.3f  };
    int   env4Dest      = 0; // 0=Off 1=Cutoff 2=Position 3=Pitch 4=Density

    // ── OSC 2 ────────────────────────────────────────────────────────────
    int   osc2Wave   = 1;
    int   osc2Octave = 0;
    int   osc2Semi   = 0;
    float osc2Fine   = 0.0f;
    float osc2Phase  = 0.0f;
    float osc2Mix    = 0.5f;
    float osc2Detune = 0.0f;
    float osc2Pan    = 0.5f;

    // ── Sub OSC ──────────────────────────────────────────────────────────
    int   subWave   = 0;
    int   subOctave = -1;
    int   subSemi   = 0;
    float subTune   = 0.0f;
    float subMix    = 0.5f;
    float subPan    = 0.5f;
    float subPhase  = 0.0f;

    // ── Enables ──────────────────────────────────────────────────────────
    bool subOscEnabled = true;
    bool osc2Enabled   = true;
    bool grainEnabled  = true;

    // ── Control ──────────────────────────────────────────────────────────
    float glide        = 0.0f;
    int   bendUp       = 2;
    int   bendDown     = 2;
    float velSens      = 1.0f;
    int   transpose    = 0;
    int   octaveShift  = 0;
    float stereoWidth  = 1.0f;
    float masterTune   = 0.0f;

    // ── Modulation ───────────────────────────────────────────────────────
    float lfoToCutoff    = 0.0f;
    float lfoToPosition  = 0.0f;
    float lfoToPitch     = 0.0f;
    float lfoToDensity   = 0.0f;
    float envToCutoff    = 0.0f;
    float envToPosition  = 0.0f;
    float envToPitch     = 0.0f;
    float envToAmp       = 0.0f;

    // ── Per-slot master ──────────────────────────────────────────────────
    float masterVolume = 0.8f;
    float masterPan    = 0.0f;

    // ── Runtime state (not serialised as params) ─────────────────────────
    float lfoPhaseRuntime      = 0.0f;
    float currentLfoValue      = 0.0f;
    int   lastMidiNote         = 60;
    float currentEnvelopeValue = 0.0f; // kept for legacy; use currentEnv2/3/4Value
    float currentEnv2Value     = 0.0f; // ENV 2 → filter
    float currentEnv3Value     = 0.0f; // ENV 3 → position
    float currentEnv4Value     = 0.0f; // ENV 4 → free dest

    // ══════════════════════════════════════════════════════════════════════
    //  loadFromAPVTS — read every parameter from the APVTS
    // ══════════════════════════════════════════════════════════════════════
    inline void loadFromAPVTS (juce::AudioProcessorValueTreeState& apvts)
    {
        // Grain
        position     = apvts.getRawParameterValue ("position")->load();
        spray        = apvts.getRawParameterValue ("spray")->load();
        grainSize    = apvts.getRawParameterValue ("grainSize")->load();
        density      = apvts.getRawParameterValue ("density")->load();
        pitchScatter = apvts.getRawParameterValue ("pitchScatter")->load();
        panSpread    = apvts.getRawParameterValue ("panSpread")->load();

        // Wavetable
        waveA        = (int) apvts.getRawParameterValue ("waveA")->load();
        waveB        = (int) apvts.getRawParameterValue ("waveB")->load();
        morphAmount  = apvts.getRawParameterValue ("morphAmount")->load();
        wtOctave     = (int) apvts.getRawParameterValue ("wtOctave")->load();
        wtSemitone   = (int) apvts.getRawParameterValue ("wtSemitone")->load();
        wtFine       = apvts.getRawParameterValue ("wtFine")->load();
        wtPhase      = apvts.getRawParameterValue ("wtPhase")->load();
        wtTilt       = apvts.getRawParameterValue ("wtTilt")->load();
        unisonVoices = (int) apvts.getRawParameterValue ("unisonVoices")->load();
        unisonDetune = apvts.getRawParameterValue ("unisonDetune")->load();
        unisonSpread = apvts.getRawParameterValue ("unisonSpread")->load();

        // Filter
        filterCutoff   = apvts.getRawParameterValue ("filterCutoff")->load();
        filterRes      = apvts.getRawParameterValue ("filterRes")->load();
        filterType     = (int) apvts.getRawParameterValue ("filterType")->load();
        filterDrive    = apvts.getRawParameterValue ("filterDrive")->load();
        filterEnvAmt   = apvts.getRawParameterValue ("filterEnvAmt")->load();
        filterLfoAmt   = apvts.getRawParameterValue ("filterLfoAmt")->load();
        filterKeyTrack = apvts.getRawParameterValue ("filterKeyTrack")->load();

        // LFOs 1-4
        const juce::String lfoIds[4] = { "lfo1", "lfo2", "lfo3", "lfo4" };
        for (int n = 0; n < 4; ++n)
        {
            lfoRate[n]      = apvts.getRawParameterValue (lfoIds[n] + "Rate")->load();
            lfoDepth[n]     = apvts.getRawParameterValue (lfoIds[n] + "Depth")->load();
            lfoShape[n]     = (int) apvts.getRawParameterValue (lfoIds[n] + "Shape")->load();
            lfoAttack[n]    = apvts.getRawParameterValue (lfoIds[n] + "Attack")->load();
            lfoDecay[n]     = apvts.getRawParameterValue (lfoIds[n] + "Decay")->load();
            lfoSync[n]      = apvts.getRawParameterValue (lfoIds[n] + "Sync")->load() > 0.5f;
            lfoRetrigger[n] = apvts.getRawParameterValue (lfoIds[n] + "Retrigger")->load() > 0.5f;
            lfoPhase[n]     = apvts.getRawParameterValue (lfoIds[n] + "Phase")->load() > 0.5f;
            lfoEnabled[n]   = apvts.getRawParameterValue (lfoIds[n] + "Enabled")->load() > 0.5f;
        }

        // Envelopes 1-4
        const juce::String envAttackIds[4]  = { "envAttack",  "env2Attack",  "env3Attack",  "env4Attack"  };
        const juce::String envDecayIds[4]   = { "envDecay",   "env2Decay",   "env3Decay",   "env4Decay"   };
        const juce::String envSustainIds[4] = { "envSustain", "env2Sustain", "env3Sustain", "env4Sustain" };
        const juce::String envReleaseIds[4] = { "envRelease", "env2Release", "env3Release", "env4Release" };
        for (int n = 0; n < 4; ++n)
        {
            envAttack[n]  = apvts.getRawParameterValue (envAttackIds[n])->load();
            envDecay[n]   = apvts.getRawParameterValue (envDecayIds[n])->load();
            envSustain[n] = apvts.getRawParameterValue (envSustainIds[n])->load();
            envRelease[n] = apvts.getRawParameterValue (envReleaseIds[n])->load();
        }
        env4Dest = (int) apvts.getRawParameterValue ("env4Dest")->load();

        // OSC 2
        osc2Wave   = (int) apvts.getRawParameterValue ("osc2Wave")->load();
        osc2Octave = (int) apvts.getRawParameterValue ("osc2Octave")->load();
        osc2Semi   = (int) apvts.getRawParameterValue ("osc2Semi")->load();
        osc2Fine   = apvts.getRawParameterValue ("osc2Fine")->load();
        osc2Phase  = apvts.getRawParameterValue ("osc2Phase")->load();
        osc2Mix    = apvts.getRawParameterValue ("osc2Mix")->load();
        osc2Detune = apvts.getRawParameterValue ("osc2Detune")->load();
        osc2Pan    = apvts.getRawParameterValue ("osc2Pan")->load();

        // Sub OSC
        subWave   = (int) apvts.getRawParameterValue ("subWave")->load();
        subOctave = (int) apvts.getRawParameterValue ("subOctave")->load();
        subSemi   = (int) apvts.getRawParameterValue ("subSemi")->load();
        subTune   = apvts.getRawParameterValue ("subTune")->load();
        subMix    = apvts.getRawParameterValue ("subMix")->load();
        subPan    = apvts.getRawParameterValue ("subPan")->load();
        subPhase  = apvts.getRawParameterValue ("subPhase")->load();

        // Enables
        subOscEnabled = apvts.getRawParameterValue ("subOscEnabled")->load() > 0.5f;
        osc2Enabled   = apvts.getRawParameterValue ("osc2Enabled")->load() > 0.5f;
        grainEnabled  = apvts.getRawParameterValue ("grainEnabled")->load() > 0.5f;

        // Control
        glide       = apvts.getRawParameterValue ("glide")->load();
        bendUp      = (int) apvts.getRawParameterValue ("bendUp")->load();
        bendDown    = (int) apvts.getRawParameterValue ("bendDown")->load();
        velSens     = apvts.getRawParameterValue ("velSens")->load();
        transpose   = (int) apvts.getRawParameterValue ("transpose")->load();
        octaveShift = (int) apvts.getRawParameterValue ("octaveShift")->load();
        stereoWidth = apvts.getRawParameterValue ("stereoWidth")->load();
        masterTune  = apvts.getRawParameterValue ("masterTune")->load();

        // Modulation
        lfoToCutoff   = apvts.getRawParameterValue ("lfoToCutoff")->load();
        lfoToPosition = apvts.getRawParameterValue ("lfoToPosition")->load();
        lfoToPitch    = apvts.getRawParameterValue ("lfoToPitch")->load();
        lfoToDensity  = apvts.getRawParameterValue ("lfoToDensity")->load();
        envToCutoff   = apvts.getRawParameterValue ("envToCutoff")->load();
        envToPosition = apvts.getRawParameterValue ("envToPosition")->load();
        envToPitch    = apvts.getRawParameterValue ("envToPitch")->load();
        envToAmp      = apvts.getRawParameterValue ("envToAmp")->load();

        // Per-slot master
        masterVolume = apvts.getRawParameterValue ("masterVolume")->load();
        masterPan    = apvts.getRawParameterValue ("masterPan")->load();
    }

    // ══════════════════════════════════════════════════════════════════════
    //  writeToAPVTS — push every value back into the APVTS
    // ══════════════════════════════════════════════════════════════════════
    inline void writeToAPVTS (juce::AudioProcessorValueTreeState& apvts)
    {
        auto setFloat = [&] (const juce::String& id, float value)
        {
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (value));
        };
        auto setInt = [&] (const juce::String& id, int value)
        {
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) value));
        };
        auto setBool = [&] (const juce::String& id, bool value)
        {
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (value ? 1.0f : 0.0f);
        };

        // Grain
        setFloat ("position",     position);
        setFloat ("spray",        spray);
        setFloat ("grainSize",    grainSize);
        setFloat ("density",      density);
        setFloat ("pitchScatter", pitchScatter);
        setFloat ("panSpread",    panSpread);

        // Wavetable
        setInt   ("waveA",        waveA);
        setInt   ("waveB",        waveB);
        setFloat ("morphAmount",  morphAmount);
        setInt   ("wtOctave",     wtOctave);
        setInt   ("wtSemitone",   wtSemitone);
        setFloat ("wtFine",       wtFine);
        setFloat ("wtPhase",      wtPhase);
        setFloat ("wtTilt",       wtTilt);
        setInt   ("unisonVoices", unisonVoices);
        setFloat ("unisonDetune", unisonDetune);
        setFloat ("unisonSpread", unisonSpread);

        // Filter
        setFloat ("filterCutoff",   filterCutoff);
        setFloat ("filterRes",      filterRes);
        setInt   ("filterType",     filterType);
        setFloat ("filterDrive",    filterDrive);
        setFloat ("filterEnvAmt",   filterEnvAmt);
        setFloat ("filterLfoAmt",   filterLfoAmt);
        setFloat ("filterKeyTrack", filterKeyTrack);

        // LFOs 1-4
        const juce::String lfoIds[4] = { "lfo1", "lfo2", "lfo3", "lfo4" };
        for (int n = 0; n < 4; ++n)
        {
            setFloat (lfoIds[n] + "Rate",      lfoRate[n]);
            setFloat (lfoIds[n] + "Depth",     lfoDepth[n]);
            setInt   (lfoIds[n] + "Shape",     lfoShape[n]);
            setFloat (lfoIds[n] + "Attack",    lfoAttack[n]);
            setFloat (lfoIds[n] + "Decay",     lfoDecay[n]);
            setBool  (lfoIds[n] + "Sync",      lfoSync[n]);
            setBool  (lfoIds[n] + "Retrigger", lfoRetrigger[n]);
            setBool  (lfoIds[n] + "Phase",     lfoPhase[n]);
            setBool  (lfoIds[n] + "Enabled",   lfoEnabled[n]);
        }

        // Envelopes 1-4
        const juce::String envAttackIds[4]  = { "envAttack",  "env2Attack",  "env3Attack",  "env4Attack"  };
        const juce::String envDecayIds[4]   = { "envDecay",   "env2Decay",   "env3Decay",   "env4Decay"   };
        const juce::String envSustainIds[4] = { "envSustain", "env2Sustain", "env3Sustain", "env4Sustain" };
        const juce::String envReleaseIds[4] = { "envRelease", "env2Release", "env3Release", "env4Release" };
        for (int n = 0; n < 4; ++n)
        {
            setFloat (envAttackIds[n],  envAttack[n]);
            setFloat (envDecayIds[n],   envDecay[n]);
            setFloat (envSustainIds[n], envSustain[n]);
            setFloat (envReleaseIds[n], envRelease[n]);
        }
        setInt ("env4Dest", env4Dest);

        // OSC 2
        setInt   ("osc2Wave",   osc2Wave);
        setInt   ("osc2Octave", osc2Octave);
        setInt   ("osc2Semi",   osc2Semi);
        setFloat ("osc2Fine",   osc2Fine);
        setFloat ("osc2Phase",  osc2Phase);
        setFloat ("osc2Mix",    osc2Mix);
        setFloat ("osc2Detune", osc2Detune);
        setFloat ("osc2Pan",    osc2Pan);

        // Sub OSC
        setInt   ("subWave",   subWave);
        setInt   ("subOctave", subOctave);
        setInt   ("subSemi",   subSemi);
        setFloat ("subTune",   subTune);
        setFloat ("subMix",    subMix);
        setFloat ("subPan",    subPan);
        setFloat ("subPhase",  subPhase);

        // Enables
        setBool ("subOscEnabled", subOscEnabled);
        setBool ("osc2Enabled",   osc2Enabled);
        setBool ("grainEnabled",  grainEnabled);

        // Control
        setFloat ("glide",       glide);
        setInt   ("bendUp",      bendUp);
        setInt   ("bendDown",    bendDown);
        setFloat ("velSens",     velSens);
        setInt   ("transpose",   transpose);
        setInt   ("octaveShift", octaveShift);
        setFloat ("stereoWidth", stereoWidth);
        setFloat ("masterTune",  masterTune);

        // Modulation
        setFloat ("lfoToCutoff",    lfoToCutoff);
        setFloat ("lfoToPosition",  lfoToPosition);
        setFloat ("lfoToPitch",     lfoToPitch);
        setFloat ("lfoToDensity",   lfoToDensity);
        setFloat ("envToCutoff",    envToCutoff);
        setFloat ("envToPosition",  envToPosition);
        setFloat ("envToPitch",     envToPitch);
        setFloat ("envToAmp",       envToAmp);

        // Per-slot master
        setFloat ("masterVolume", masterVolume);
        setFloat ("masterPan",    masterPan);
    }

    // ══════════════════════════════════════════════════════════════════════
    //  toValueTree — serialize to ValueTree for state save
    // ══════════════════════════════════════════════════════════════════════
    inline juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree ("SlotState");

        // Grain
        tree.setProperty ("position",     position,     nullptr);
        tree.setProperty ("spray",        spray,        nullptr);
        tree.setProperty ("grainSize",    grainSize,    nullptr);
        tree.setProperty ("density",      density,      nullptr);
        tree.setProperty ("pitchScatter", pitchScatter, nullptr);
        tree.setProperty ("panSpread",    panSpread,    nullptr);

        // Wavetable
        tree.setProperty ("waveA",        waveA,        nullptr);
        tree.setProperty ("waveB",        waveB,        nullptr);
        tree.setProperty ("morphAmount",  morphAmount,  nullptr);
        tree.setProperty ("wtOctave",     wtOctave,     nullptr);
        tree.setProperty ("wtSemitone",   wtSemitone,   nullptr);
        tree.setProperty ("wtFine",       wtFine,       nullptr);
        tree.setProperty ("wtPhase",      wtPhase,      nullptr);
        tree.setProperty ("wtTilt",       wtTilt,       nullptr);
        tree.setProperty ("unisonVoices", unisonVoices, nullptr);
        tree.setProperty ("unisonDetune", unisonDetune, nullptr);
        tree.setProperty ("unisonSpread", unisonSpread, nullptr);

        // Filter
        tree.setProperty ("filterCutoff",   filterCutoff,   nullptr);
        tree.setProperty ("filterRes",      filterRes,      nullptr);
        tree.setProperty ("filterType",     filterType,     nullptr);
        tree.setProperty ("filterDrive",    filterDrive,    nullptr);
        tree.setProperty ("filterEnvAmt",   filterEnvAmt,   nullptr);
        tree.setProperty ("filterLfoAmt",   filterLfoAmt,   nullptr);
        tree.setProperty ("filterKeyTrack", filterKeyTrack, nullptr);

        // LFOs 1-4
        for (int n = 0; n < 4; ++n)
        {
            juce::String prefix = "lfo" + juce::String (n + 1);
            tree.setProperty (prefix + "Rate",      lfoRate[n],      nullptr);
            tree.setProperty (prefix + "Depth",     lfoDepth[n],     nullptr);
            tree.setProperty (prefix + "Shape",     lfoShape[n],     nullptr);
            tree.setProperty (prefix + "Attack",    lfoAttack[n],    nullptr);
            tree.setProperty (prefix + "Decay",     lfoDecay[n],     nullptr);
            tree.setProperty (prefix + "Sync",      lfoSync[n],      nullptr);
            tree.setProperty (prefix + "Retrigger", lfoRetrigger[n], nullptr);
            tree.setProperty (prefix + "Phase",     lfoPhase[n],     nullptr);
            tree.setProperty (prefix + "Enabled",   lfoEnabled[n],   nullptr);
        }

        // Envelopes 1-4
        const juce::String envAttackIds[4]  = { "envAttack",  "env2Attack",  "env3Attack",  "env4Attack"  };
        const juce::String envDecayIds[4]   = { "envDecay",   "env2Decay",   "env3Decay",   "env4Decay"   };
        const juce::String envSustainIds[4] = { "envSustain", "env2Sustain", "env3Sustain", "env4Sustain" };
        const juce::String envReleaseIds[4] = { "envRelease", "env2Release", "env3Release", "env4Release" };
        for (int n = 0; n < 4; ++n)
        {
            tree.setProperty (envAttackIds[n],  envAttack[n],  nullptr);
            tree.setProperty (envDecayIds[n],   envDecay[n],   nullptr);
            tree.setProperty (envSustainIds[n], envSustain[n], nullptr);
            tree.setProperty (envReleaseIds[n], envRelease[n], nullptr);
        }
        tree.setProperty ("env4Dest", env4Dest, nullptr);

        // OSC 2
        tree.setProperty ("osc2Wave",   osc2Wave,   nullptr);
        tree.setProperty ("osc2Octave", osc2Octave, nullptr);
        tree.setProperty ("osc2Semi",   osc2Semi,   nullptr);
        tree.setProperty ("osc2Fine",   osc2Fine,   nullptr);
        tree.setProperty ("osc2Phase",  osc2Phase,  nullptr);
        tree.setProperty ("osc2Mix",    osc2Mix,    nullptr);
        tree.setProperty ("osc2Detune", osc2Detune, nullptr);
        tree.setProperty ("osc2Pan",    osc2Pan,    nullptr);

        // Sub OSC
        tree.setProperty ("subWave",   subWave,   nullptr);
        tree.setProperty ("subOctave", subOctave, nullptr);
        tree.setProperty ("subSemi",   subSemi,   nullptr);
        tree.setProperty ("subTune",   subTune,   nullptr);
        tree.setProperty ("subMix",    subMix,    nullptr);
        tree.setProperty ("subPan",    subPan,    nullptr);
        tree.setProperty ("subPhase",  subPhase,  nullptr);

        // Enables
        tree.setProperty ("subOscEnabled", subOscEnabled, nullptr);
        tree.setProperty ("osc2Enabled",   osc2Enabled,   nullptr);
        tree.setProperty ("grainEnabled",  grainEnabled,  nullptr);

        // Control
        tree.setProperty ("glide",       glide,       nullptr);
        tree.setProperty ("bendUp",      bendUp,      nullptr);
        tree.setProperty ("bendDown",    bendDown,    nullptr);
        tree.setProperty ("velSens",     velSens,     nullptr);
        tree.setProperty ("transpose",   transpose,   nullptr);
        tree.setProperty ("octaveShift", octaveShift, nullptr);
        tree.setProperty ("stereoWidth", stereoWidth, nullptr);
        tree.setProperty ("masterTune",  masterTune,  nullptr);

        // Modulation
        tree.setProperty ("lfoToCutoff",   lfoToCutoff,   nullptr);
        tree.setProperty ("lfoToPosition", lfoToPosition, nullptr);
        tree.setProperty ("lfoToPitch",    lfoToPitch,    nullptr);
        tree.setProperty ("lfoToDensity",  lfoToDensity,  nullptr);
        tree.setProperty ("envToCutoff",   envToCutoff,   nullptr);
        tree.setProperty ("envToPosition", envToPosition, nullptr);
        tree.setProperty ("envToPitch",    envToPitch,    nullptr);
        tree.setProperty ("envToAmp",      envToAmp,      nullptr);

        // Per-slot master
        tree.setProperty ("masterVolume", masterVolume, nullptr);
        tree.setProperty ("masterPan",    masterPan,    nullptr);

        // Runtime state
        tree.setProperty ("lfoPhaseRuntime",      lfoPhaseRuntime,      nullptr);
        tree.setProperty ("currentLfoValue",      currentLfoValue,      nullptr);
        tree.setProperty ("lastMidiNote",         lastMidiNote,         nullptr);
        tree.setProperty ("currentEnvelopeValue", currentEnvelopeValue, nullptr);

        return tree;
    }

    // ══════════════════════════════════════════════════════════════════════
    //  fromValueTree — deserialize from ValueTree
    // ══════════════════════════════════════════════════════════════════════
    inline void fromValueTree (const juce::ValueTree& tree)
    {
        // Grain
        position     = (float) tree.getProperty ("position",     0.5f);
        spray        = (float) tree.getProperty ("spray",        0.1f);
        grainSize    = (float) tree.getProperty ("grainSize",    200.0f);
        density      = (float) tree.getProperty ("density",      40.0f);
        pitchScatter = (float) tree.getProperty ("pitchScatter", 0.0f);
        panSpread    = (float) tree.getProperty ("panSpread",    0.5f);

        // Wavetable
        waveA        = (int) tree.getProperty ("waveA",        0);
        waveB        = (int) tree.getProperty ("waveB",        1);
        morphAmount  = (float) tree.getProperty ("morphAmount",  0.0f);
        wtOctave     = (int) tree.getProperty ("wtOctave",     0);
        wtSemitone   = (int) tree.getProperty ("wtSemitone",   0);
        wtFine       = (float) tree.getProperty ("wtFine",       0.0f);
        wtPhase      = (float) tree.getProperty ("wtPhase",      0.0f);
        wtTilt       = (float) tree.getProperty ("wtTilt",       0.0f);
        unisonVoices = (int) tree.getProperty ("unisonVoices", 1);
        unisonDetune = (float) tree.getProperty ("unisonDetune", 0.0f);
        unisonSpread = (float) tree.getProperty ("unisonSpread", 0.5f);

        // Filter
        filterCutoff   = (float) tree.getProperty ("filterCutoff",   8000.0f);
        filterRes      = (float) tree.getProperty ("filterRes",      0.7f);
        filterType     = (int) tree.getProperty ("filterType",     0);
        filterDrive    = (float) tree.getProperty ("filterDrive",    0.0f);
        filterEnvAmt   = (float) tree.getProperty ("filterEnvAmt",   0.0f);
        filterLfoAmt   = (float) tree.getProperty ("filterLfoAmt",   0.0f);
        filterKeyTrack = (float) tree.getProperty ("filterKeyTrack", 0.0f);

        // LFOs 1-4
        for (int n = 0; n < 4; ++n)
        {
            juce::String prefix = "lfo" + juce::String (n + 1);
            lfoRate[n]      = (float) tree.getProperty (prefix + "Rate",      1.0f);
            lfoDepth[n]     = (float) tree.getProperty (prefix + "Depth",     0.5f);
            lfoShape[n]     = (int)   tree.getProperty (prefix + "Shape",     0);
            lfoAttack[n]    = (float) tree.getProperty (prefix + "Attack",    0.0f);
            lfoDecay[n]     = (float) tree.getProperty (prefix + "Decay",     0.0f);
            lfoSync[n]      = (bool)  tree.getProperty (prefix + "Sync",      false);
            lfoRetrigger[n] = (bool)  tree.getProperty (prefix + "Retrigger", false);
            lfoPhase[n]     = (bool)  tree.getProperty (prefix + "Phase",     false);
            lfoEnabled[n]   = (bool)  tree.getProperty (prefix + "Enabled",   true);
        }

        // Envelopes 1-4
        const juce::String envAttackIds[4]  = { "envAttack",  "env2Attack",  "env3Attack",  "env4Attack"  };
        const juce::String envDecayIds[4]   = { "envDecay",   "env2Decay",   "env3Decay",   "env4Decay"   };
        const juce::String envSustainIds[4] = { "envSustain", "env2Sustain", "env3Sustain", "env4Sustain" };
        const juce::String envReleaseIds[4] = { "envRelease", "env2Release", "env3Release", "env4Release" };
        for (int n = 0; n < 4; ++n)
        {
            envAttack[n]  = (float) tree.getProperty (envAttackIds[n],  0.01f);
            envDecay[n]   = (float) tree.getProperty (envDecayIds[n],   0.1f);
            envSustain[n] = (float) tree.getProperty (envSustainIds[n], 0.8f);
            envRelease[n] = (float) tree.getProperty (envReleaseIds[n], 0.3f);
        }
        env4Dest = (int) tree.getProperty ("env4Dest", 0);

        // OSC 2
        osc2Wave   = (int) tree.getProperty ("osc2Wave",   1);
        osc2Octave = (int) tree.getProperty ("osc2Octave", 0);
        osc2Semi   = (int) tree.getProperty ("osc2Semi",   0);
        osc2Fine   = (float) tree.getProperty ("osc2Fine",   0.0f);
        osc2Phase  = (float) tree.getProperty ("osc2Phase",  0.0f);
        osc2Mix    = (float) tree.getProperty ("osc2Mix",    0.5f);
        osc2Detune = (float) tree.getProperty ("osc2Detune", 0.0f);
        osc2Pan    = (float) tree.getProperty ("osc2Pan",    0.5f);

        // Sub OSC
        subWave   = (int) tree.getProperty ("subWave",   0);
        subOctave = (int) tree.getProperty ("subOctave", -1);
        subSemi   = (int) tree.getProperty ("subSemi",   0);
        subTune   = (float) tree.getProperty ("subTune",   0.0f);
        subMix    = (float) tree.getProperty ("subMix",    0.5f);
        subPan    = (float) tree.getProperty ("subPan",    0.5f);
        subPhase  = (float) tree.getProperty ("subPhase",  0.0f);

        // Enables
        subOscEnabled = (bool) tree.getProperty ("subOscEnabled", true);
        osc2Enabled   = (bool) tree.getProperty ("osc2Enabled",   true);
        grainEnabled  = (bool) tree.getProperty ("grainEnabled",  true);

        // Control
        glide       = (float) tree.getProperty ("glide",       0.0f);
        bendUp      = (int) tree.getProperty ("bendUp",      2);
        bendDown    = (int) tree.getProperty ("bendDown",    2);
        velSens     = (float) tree.getProperty ("velSens",     1.0f);
        transpose   = (int) tree.getProperty ("transpose",   0);
        octaveShift = (int) tree.getProperty ("octaveShift", 0);
        stereoWidth = (float) tree.getProperty ("stereoWidth", 1.0f);
        masterTune  = (float) tree.getProperty ("masterTune",  0.0f);

        // Modulation
        lfoToCutoff   = (float) tree.getProperty ("lfoToCutoff",   0.0f);
        lfoToPosition = (float) tree.getProperty ("lfoToPosition", 0.0f);
        lfoToPitch    = (float) tree.getProperty ("lfoToPitch",    0.0f);
        lfoToDensity  = (float) tree.getProperty ("lfoToDensity",  0.0f);
        envToCutoff   = (float) tree.getProperty ("envToCutoff",   0.0f);
        envToPosition = (float) tree.getProperty ("envToPosition", 0.0f);
        envToPitch    = (float) tree.getProperty ("envToPitch",    0.0f);
        envToAmp      = (float) tree.getProperty ("envToAmp",      0.0f);

        // Per-slot master
        masterVolume = (float) tree.getProperty ("masterVolume", 0.8f);
        masterPan    = (float) tree.getProperty ("masterPan",    0.0f);

        // Runtime state
        lfoPhaseRuntime      = (float) tree.getProperty ("lfoPhaseRuntime",      0.0f);
        currentLfoValue      = (float) tree.getProperty ("currentLfoValue",      0.0f);
        lastMidiNote         = (int) tree.getProperty ("lastMidiNote",         60);
        currentEnvelopeValue = (float) tree.getProperty ("currentEnvelopeValue", 0.0f);
    }
};
