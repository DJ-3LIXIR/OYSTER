#include "WavetableEngine.h"

#include <cmath>


WavetableEngine::WavetableEngine()
{
    buildWavetables();
}

void WavetableEngine::buildWavetables()
{
    for (int i = 0; i < tableSize; ++i)
    {
        const auto phase = static_cast<float> (i) / static_cast<float> (tableSize);
        const auto angle = juce::MathConstants<float>::twoPi * phase;

        const auto sine = std::sin (angle);
        const auto saw = (2.0f * phase) - 1.0f;
        const auto square = phase < 0.5f ? 1.0f : -1.0f;
        const auto triangle = 1.0f - 4.0f * std::abs (phase - 0.5f);
        const auto warmSine = 0.86f * sine
                            + 0.10f * std::sin (2.0f * angle)
                            + 0.04f * std::sin (3.0f * angle);
        const auto softSaw = 0.80f * saw
                           + 0.15f * std::sin (angle)
                           + 0.05f * std::sin (2.0f * angle);
        const auto noise = juce::jmap (juce::Random::getSystemRandom().nextFloat(), 0.0f, 1.0f, -1.0f, 1.0f);
        const auto hybrid = 0.50f * warmSine + 0.35f * softSaw + 0.15f * triangle;

        wavetables[0][i] = sine;
        wavetables[1][i] = saw;
        wavetables[2][i] = square;
        wavetables[3][i] = triangle;
        wavetables[4][i] = warmSine;
        wavetables[5][i] = softSaw;
        wavetables[6][i] = noise;
        wavetables[7][i] = hybrid;
    }
}

float WavetableEngine::getSample (int wavetableIndex, float position) const noexcept
{
    const auto index = juce::jlimit (0, numWavetables - 1, wavetableIndex);
    const auto wrappedPosition = position - std::floor (position);
    const auto tablePos = wrappedPosition * static_cast<float> (tableSize);

    const auto indexA = static_cast<int> (tablePos) % tableSize;
    const auto indexB = (indexA + 1) % tableSize;
    const auto frac = tablePos - static_cast<float> (indexA);

    const auto a = wavetables[static_cast<size_t> (index)][static_cast<size_t> (indexA)];
    const auto b = wavetables[static_cast<size_t> (index)][static_cast<size_t> (indexB)];
    return juce::jmap (frac, a, b);
}

float WavetableEngine::getMorphedSample (int waveA, int waveB, float morphAmount, float position) const noexcept
{
    const auto clampedMorph = juce::jlimit (0.0f, 1.0f, morphAmount);
    const auto sampleA = getSample (waveA, position);
    const auto sampleB = getSample (waveB, position);
    return juce::jmap (clampedMorph, sampleA, sampleB);
}
