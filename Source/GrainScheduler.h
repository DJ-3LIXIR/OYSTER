#pragma once

#include <JuceHeader.h>

#include "GrainPool.h"

class GrainScheduler
{
public:
    explicit GrainScheduler (GrainPool& poolToUse);

    void setSampleRate (double newSampleRate) noexcept;

    void setDensity (float newDensity) noexcept;
    void setGrainSizeSamples (int newGrainSizeSamples) noexcept;
    void setBasePosition (float newBasePosition) noexcept;
    void setSpray (float newSpray) noexcept;
    void setBasePitch (float newBasePitch) noexcept;
    void setPitchMultiplier (float mult) noexcept;
    void setPitchScatter (float newPitchScatter) noexcept;
    void setPanSpread (float newPanSpread) noexcept;
    void setGrainAmplitude (float newGrainAmplitude) noexcept;
    void reset() noexcept { samplesSinceLastSpawn = -1.0f; }

    void process (int numSamples);
    void spawnGrain();

private:
    GrainPool& grainPool;
    juce::Random random;

    double sampleRate = 44100.0;
    float samplesSinceLastSpawn = 0.0f;

    float density = 20.0f;       // grains per second
    int grainSizeSamples = 2048;
    float basePosition = 0.0f;   // normalized 0..1
    float spray = 0.1f;          // normalized +- offset
    float basePitch = 1.0f;      // playback multiplier
    float pitchMultiplier { 1.0f };
    float pitchScatter = 0.0f;   // +- pitch offset
    float panSpread = 1.0f;      // 0..1 around center
    float grainAmplitude = 0.2f;
};
