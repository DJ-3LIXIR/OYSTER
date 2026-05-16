#pragma once

#include <array>

#include <JuceHeader.h>

class WavetableEngine
{
public:
    static constexpr int numWavetables = 8;
    static constexpr int tableSize = 2048;

    WavetableEngine();

    void buildWavetables();

    float getSample (int wavetableIndex, float position) const noexcept;
    float getMorphedSample (int waveA, int waveB, float morphAmount, float position) const noexcept;

private:
    using Wavetable = std::array<float, tableSize>;

    std::array<Wavetable, numWavetables> wavetables {};
};
