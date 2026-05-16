#include "GrainScheduler.h"


GrainScheduler::GrainScheduler (GrainPool& poolToUse)
    : grainPool (poolToUse)
{
}

void GrainScheduler::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = juce::jmax (1.0, newSampleRate);
}

void GrainScheduler::setDensity (float newDensity) noexcept
{
    density = juce::jmax (0.0f, newDensity);
}

void GrainScheduler::setGrainSizeSamples (int newGrainSizeSamples) noexcept
{
    grainSizeSamples = juce::jmax (1, newGrainSizeSamples);
}

void GrainScheduler::setBasePosition (float newBasePosition) noexcept
{
    basePosition = juce::jlimit (0.0f, 1.0f, newBasePosition);
}

void GrainScheduler::setSpray (float newSpray) noexcept
{
    spray = juce::jmax (0.0f, newSpray);
}

void GrainScheduler::setBasePitch (float newBasePitch) noexcept
{
    basePitch = juce::jmax (0.01f, newBasePitch);
}

void GrainScheduler::setPitchScatter (float newPitchScatter) noexcept
{
    pitchScatter = juce::jmax (0.0f, newPitchScatter);
}

void GrainScheduler::setPanSpread (float newPanSpread) noexcept
{
    panSpread = juce::jlimit (0.0f, 1.0f, newPanSpread);
}

void GrainScheduler::setGrainAmplitude (float newGrainAmplitude) noexcept
{
    grainAmplitude = juce::jmax (0.0f, newGrainAmplitude);
}

void GrainScheduler::setPitchMultiplier (float mult) noexcept
{
    pitchMultiplier = juce::jmax (0.01f, mult);
}

void GrainScheduler::process (int numSamples)
{
    if (density <= 0.0f)
        return;

    const auto samplesPerGrain = static_cast<float> (sampleRate / juce::jmax (0.001f, density));

    // Force immediate spawn on first call after reset
    if (samplesSinceLastSpawn < 0.0f)
    {
        spawnGrain();
        samplesSinceLastSpawn = 0.0f;
    }

    samplesSinceLastSpawn += static_cast<float> (numSamples);

    while (samplesSinceLastSpawn >= samplesPerGrain)
    {
        samplesSinceLastSpawn -= samplesPerGrain;
        spawnGrain();
    }
}

void GrainScheduler::spawnGrain()
{
    auto* grain = grainPool.getNextFreeGrain();

    if (grain == nullptr)
        return;

    const auto randomBiPolar = [this]() { return random.nextFloat() * 2.0f - 1.0f; };

    grain->position = juce::jlimit (0.0f, 1.0f, basePosition + spray * randomBiPolar());
    grain->size = juce::jmax (1, grainSizeSamples);
    grain->pitch = juce::jmax (0.01f, (basePitch * pitchMultiplier) + pitchScatter * randomBiPolar());
    grain->pan = juce::jlimit (0.0f, 1.0f, 0.5f + (panSpread * 0.5f * randomBiPolar()));
    grain->amplitude = grainAmplitude;
    grain->currentSample = 0;
    grain->isActive = true;
}
