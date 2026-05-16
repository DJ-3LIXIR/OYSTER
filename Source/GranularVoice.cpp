#include "GranularVoice.h"

#include <cmath>


GranularVoice::GranularVoice (WavetableEngine& wavetableEngineToUse,
                              SubOscEngine& subOscEngineToUse,
                              Osc2Engine& osc2EngineToUse)
    : wavetableEngine (wavetableEngineToUse),
      subOscEngine (subOscEngineToUse),
      osc2Engine (osc2EngineToUse),
      scheduler (grainPool)
{
    scheduler.setDensity (density);
    scheduler.setGrainSizeSamples (grainSizeSamples);
    scheduler.setSpray (spray);
    scheduler.setPitchScatter (pitchScatter);
    scheduler.setPanSpread (panSpread);

    // Initialize ADSR with default parameters
    juce::ADSR::Parameters adsrParams;
    adsrParams.attack  = 0.01f;
    adsrParams.decay   = 0.1f;
    adsrParams.sustain = 0.8f;
    adsrParams.release = 0.3f;
    adsr.setParameters (adsrParams);
}

bool GranularVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<GranularSound*> (sound) != nullptr;
}

void GranularVoice::startNote (int midiNoteNumber, float velocity,
                               juce::SynthesiserSound* sound,
                               int currentPitchWheelPosition)
{
    juce::ignoreUnused (sound);

    scheduler.setSampleRate (getSampleRate());
    adsr.setSampleRate (getSampleRate());
    noteIsActive = true;
    currentMidiNote = midiNoteNumber;
    scheduler.setDensity (density); // restore density on new note
    scheduler.reset();
    adsr.noteOn();

    // Reset per-voice oscillator phases
    subOscPhase = 0.0f;
    osc2PhaseL  = 0.0f;
    osc2PhaseR  = 0.0f;

    targetGlidePitch = std::pow (2.0f, static_cast<float> (midiNoteNumber - 60) / 12.0f);

    if (glideSeconds <= 0.001f || currentGlidePitch <= 0.01f)
        currentGlidePitch = targetGlidePitch;

    // Scale velocity by sensitivity: sens=1 -> full velocity, sens=0 -> always full
    const float scaledVelocity = 1.0f - velSensitivity + velSensitivity * velocity;
    voiceVelocity = scaledVelocity;
    scheduler.setGrainAmplitude (scaledVelocity);
    scheduler.setBasePosition (position);

    // Apply initial pitch wheel
    pitchWheelMoved (currentPitchWheelPosition);
}

void GranularVoice::stopNote (float velocity, bool allowTailOff)
{
    juce::ignoreUnused (velocity);

    if (allowTailOff)
    {
        // Stop spawning new grains but let existing ones finish
        scheduler.setDensity (0.0f);
        noteIsActive = false;
        adsr.noteOff();
    }
    else
    {
        // Hard stop — voice is being stolen for a new note.
        // Kill all grains immediately and free the voice.
        for (auto& grain : grainPool.getGrains())
            grain.isActive = false;
        scheduler.setDensity (0.0f);
        noteIsActive = false;
        adsr.reset();
        clearCurrentNote();
    }
}

void GranularVoice::pitchWheelMoved (int newPitchWheelValue)
{
    const float norm = (newPitchWheelValue - 8192) / 8192.0f;
    const float semis = norm >= 0.0f
                      ? norm * (float) bendRangeUp
                      : norm * (float) bendRangeDown;
    currentPitchBend = std::pow (2.0f, semis / 12.0f);
    scheduler.setPitchMultiplier (pitchMultiplier * currentPitchBend);
}

void GranularVoice::controllerMoved (int controllerNumber, int newControllerValue)
{
    juce::ignoreUnused (newControllerValue);

    // CC 120 = All Sound Off, CC 123 = All Notes Off
    if (controllerNumber == 120 || controllerNumber == 123)
    {
        for (auto& grain : grainPool.getGrains())
            grain.isActive = false;
        scheduler.setDensity (0.0f);
        noteIsActive = false;
        adsr.reset();
        clearCurrentNote();
    }
}

void GranularVoice::setGrainParameters (float newPosition,
                                        float newSpray,
                                        float newGrainSizeMs,
                                        float newDensity,
                                        float newPitchScatter,
                                        float newPanSpread) noexcept
{
    position = juce::jlimit (0.0f, 1.0f, newPosition);
    spray = juce::jmax (0.0f, newSpray);
    density = juce::jmax (0.0f, newDensity);
    pitchScatter = juce::jmax (0.0f, newPitchScatter);
    panSpread = juce::jlimit (0.0f, 1.0f, newPanSpread);

    const auto grainSamples = static_cast<int> (juce::roundToInt (newGrainSizeMs * 0.001f * static_cast<float> (getSampleRate())));
    grainSizeSamples = juce::jmax (1, grainSamples);

    scheduler.setBasePosition (position);
    scheduler.setSpray (spray);
    if (noteIsActive)
        scheduler.setDensity (density);
    scheduler.setPitchScatter (pitchScatter);
    scheduler.setPanSpread (panSpread);
    scheduler.setGrainSizeSamples (grainSizeSamples);
}

void GranularVoice::setWavetableParameters (int newWaveA, int newWaveB, float newMorphAmount) noexcept
{
    waveA = newWaveA;
    waveB = newWaveB;
    morphAmount = juce::jlimit (0.0f, 1.0f, newMorphAmount);
}

void GranularVoice::setWavetableSourceParams (int newWtOctave,
                                              int newWtSemitone,
                                              float newWtFine,
                                              float newWtPhase,
                                              float newWtTilt,
                                              int newUnisonVoices,
                                              float newUnisonDetune,
                                              float newUnisonSpread) noexcept
{
    wtOctave = juce::jlimit (-3, 3, newWtOctave);
    wtSemitone = juce::jlimit (-12, 12, newWtSemitone);
    wtFine = juce::jlimit (-100.0f, 100.0f, newWtFine);
    wtPhase = juce::jlimit (0.0f, 1.0f, newWtPhase);
    wtTilt = juce::jlimit (-1.0f, 1.0f, newWtTilt);
    unisonVoices = juce::jlimit (1, 8, newUnisonVoices);
    unisonDetune = juce::jmax (0.0f, newUnisonDetune);
    unisonSpread = juce::jlimit (0.0f, 1.0f, newUnisonSpread);
}

void GranularVoice::setPitchMultiplier (float mult) noexcept
{
    pitchMultiplier = juce::jmax (0.01f, mult);
    scheduler.setPitchMultiplier (pitchMultiplier * currentPitchBend);
}

void GranularVoice::setGlide (float glideSeconds_) noexcept
{
    glideSeconds = juce::jmax (0.0f, glideSeconds_);
}

void GranularVoice::setVelocitySensitivity (float sens) noexcept
{
    velSensitivity = juce::jlimit (0.0f, 1.0f, sens);
}

void GranularVoice::setBendRange (int semisUp, int semisDown) noexcept
{
    bendRangeUp   = semisUp;
    bendRangeDown = semisDown;
}

void GranularVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Glide — smoothly move currentGlidePitch toward targetGlidePitch
    if (glideSeconds > 0.001f)
    {
        const float sr = (float) getSampleRate();
        const float smoothingCoeff = 1.0f - std::exp (-1.0f / (glideSeconds * sr / (float) numSamples));
        currentGlidePitch += smoothingCoeff * (targetGlidePitch - currentGlidePitch);
        scheduler.setBasePitch (currentGlidePitch);
    }
    else
    {
        scheduler.setBasePitch (targetGlidePitch);
    }

    scheduler.process (numSamples);

    // Pre-compute ADSR values for the block (one per sample)
    constexpr int maxBlockSize = 4096;
    float adsrBuffer[maxBlockSize];
    const int safeSamples = juce::jmin (numSamples, maxBlockSize);
    for (int s = 0; s < safeSamples; ++s)
        adsrBuffer[s] = adsr.getNextSample();

    const auto numChannels = outputBuffer.getNumChannels();

    // ── Grain rendering ─────────────────────────────────────────────────
    if (grainEnabled)
    {
        auto& grains = grainPool.getGrains();

        for (auto& grain : grains)
        {
            if (! grain.isActive)
                continue;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                if (grain.currentSample >= grain.size)
                {
                    grainPool.returnGrain (&grain);
                    break;
                }

                const auto grainPhase = static_cast<float> (grain.currentSample) / static_cast<float> (juce::jmax (1, grain.size));
                const float refFreq = 261.63f; // C4 reference
                const float wtPitchSemis = (float) wtOctave * 12.0f + (float) wtSemitone + wtFine / 100.0f;
                const float wtPitchMult = std::pow (2.0f, wtPitchSemis / 12.0f);
                const float phaseInc = grain.pitch * refFreq * wtPitchMult / static_cast<float> (getSampleRate());
                const auto window = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * grainPhase));
                const float tilt = wtTilt;
                float sampleL = 0.0f;
                float sampleR = 0.0f;

                for (int v = 0; v < unisonVoices; ++v)
                {
                    const float voiceNorm = (unisonVoices > 1)
                        ? ((2.0f * (float) v / (float) (unisonVoices - 1)) - 1.0f)
                        : 0.0f;
                    const float detuneRatio = std::pow (2.0f, (voiceNorm * unisonDetune) / 1200.0f);
                    const float waveformPosition = grain.position + wtPhase + grain.currentSample * phaseInc * detuneRatio;
                    float src = wavetableEngine.getMorphedSample (waveA, waveB, morphAmount, waveformPosition);

                    // Tilt: positive brightens (gentle drive), negative darkens (soft cubic smoothing).
                    if (tilt >= 0.0f)
                    {
                        const float drive = 1.0f + 3.0f * tilt;
                        const float norm = std::tanh (drive);
                        src = std::tanh (src * drive) / (norm > 0.0f ? norm : 1.0f);
                    }
                    else
                    {
                        const float a = -tilt;
                        src = (1.0f - a) * src + a * (src * src * src);
                    }

                    const float uniPan = juce::jlimit (0.0f, 1.0f, grain.pan + voiceNorm * unisonSpread * 0.5f);
                    sampleL += src * (1.0f - uniPan);
                    sampleR += src * uniPan;
                }

                const float uniNorm = 1.0f / (float) unisonVoices;
                const float amp = window * grain.amplitude * adsrBuffer[sample] * uniNorm;
                if (numChannels > 0) outputBuffer.addSample (0, startSample + sample, sampleL * amp);
                if (numChannels > 1) outputBuffer.addSample (1, startSample + sample, sampleR * amp);

                grain.currentSample++;
            }
        }
    }

    // ── Sub Oscillator (per-voice) ──────────────────────────────────────
    if (subEnabled && subMix > 0.0001f)
    {
        const int   waveIdx      = juce::jlimit (0, 7, subWave);
        const float octaveOffset = subOctave * 12.0f;
        const float semiOffset   = subSemi;
        const float tuneOffset   = subTune / 100.0f;
        const float pan          = juce::jlimit (0.0f, 1.0f, subPan);

        const float midiBase  = 60.0f + 12.0f * std::log2 (juce::jmax (0.001f, currentGlidePitch));
        const float midiNoteF = midiBase + octaveOffset + semiOffset + tuneOffset;
        const float freq      = 440.0f * std::pow (2.0f, (midiNoteF - 69.0f) / 12.0f)
                                * pitchMultiplier * currentPitchBend;
        const float phaseInc  = freq / (float) getSampleRate();

        const float leftGain  = (1.0f - pan) * subMix;
        const float rightGain = pan           * subMix;

        for (int s = 0; s < numSamples; ++s)
        {
            const float sample = subOscEngine.getSample (waveIdx, subOscPhase + subPhase)
                                 * adsrBuffer[s] * voiceVelocity;
            subOscPhase += phaseInc;
            if (subOscPhase >= 1.0f) subOscPhase -= 1.0f;

            if (numChannels > 0) outputBuffer.addSample (0, startSample + s, sample * leftGain);
            if (numChannels > 1) outputBuffer.addSample (1, startSample + s, sample * rightGain);
        }
    }

    // ── OSC 2 (per-voice) ───────────────────────────────────────────────
    if (o2Enabled && o2Mix > 0.0001f)
    {
        const int   waveIdx      = juce::jlimit (0, 7, o2Wave);
        const float octaveOffset = o2Octave * 12.0f;
        const float semiOffset   = o2Semi;
        const float fineOffset   = o2Fine / 100.0f;
        const float pan          = juce::jlimit (0.0f, 1.0f, o2Pan);
        const float detuneCents  = o2Detune;

        const float midiBase     = 60.0f + 12.0f * std::log2 (juce::jmax (0.001f, currentGlidePitch));
        const float midiNoteF    = midiBase + octaveOffset + semiOffset + fineOffset;
        const float freqBase     = 440.0f * std::pow (2.0f, (midiNoteF - 69.0f) / 12.0f)
                                   * pitchMultiplier * currentPitchBend;

        // Detune: left voice flat, right voice sharp (stereo spread via detune)
        const float detuneRatio  = std::pow (2.0f, detuneCents / (12.0f * 100.0f));
        const float freqL        = freqBase / detuneRatio;
        const float freqR        = freqBase * detuneRatio;
        const float phaseIncL    = freqL / (float) getSampleRate();
        const float phaseIncR    = freqR / (float) getSampleRate();

        const float leftGain     = (1.0f - pan) * o2Mix;
        const float rightGain    = pan           * o2Mix;

        for (int s = 0; s < numSamples; ++s)
        {
            const float sampleL = osc2Engine.getSample (waveIdx, osc2PhaseL + o2Phase)
                                  * adsrBuffer[s] * voiceVelocity;
            const float sampleR = osc2Engine.getSample (waveIdx, osc2PhaseR + o2Phase)
                                  * adsrBuffer[s] * voiceVelocity;

            osc2PhaseL += phaseIncL;
            osc2PhaseR += phaseIncR;
            if (osc2PhaseL >= 1.0f) osc2PhaseL -= 1.0f;
            if (osc2PhaseR >= 1.0f) osc2PhaseR -= 1.0f;

            if (numChannels > 0) outputBuffer.addSample (0, startSample + s, sampleL * leftGain);
            if (numChannels > 1) outputBuffer.addSample (1, startSample + s, sampleR * rightGain);
        }
    }

    // Clear voice when ADSR is done and no grains are active
    if (!noteIsActive && !adsr.isActive())
    {
        bool anyActive = false;
        for (const auto& grain : grainPool.getGrains())
            if (grain.isActive) { anyActive = true; break; }

        if (! anyActive)
            clearCurrentNote();
    }
}
