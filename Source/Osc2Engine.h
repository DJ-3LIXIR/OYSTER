#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

/*  Osc2Engine
    ==========
    8 harmonically rich wavetables purpose-built for OSC 2 layering roles.

    Index  Name          Character
    -----  ------------  --------------------------------------------------
    0      Sine          Clean sine for smooth, transparent layering
    1      Bright Saw    Full harmonic sawtooth – cutting and present
    2      Pulse 33%     Narrow pulse – buzzy, reedy character
    3      Overtone      Sine + accented 3rd & 5th – organ-like presence
    4      Formant       Vowel-like – mid-frequency emphasis peak
    5      Pluck         Decaying harmonic series – string/pluck body
    6      Bell          Inharmonic partials (×2.76, ×5.40) – metallic
    7      Shimmer       Dense stacked partials with slight detuning – pad sheen
*/

class Osc2Engine
{
public:
    static constexpr int tableSize     = 2048;
    static constexpr int numWavetables = 8;

    Osc2Engine() { buildWavetables(); }

    void buildWavetables()
    {
        for (int i = 0; i < tableSize; ++i)
        {
            const float p = static_cast<float> (i) / static_cast<float> (tableSize);
            const float a = juce::MathConstants<float>::twoPi * p;

            // 0 — Sine: clean fundamental
            wavetables[0][i] = std::sin (a);

            // 1 — Bright Saw: full harmonic series up to 16 partials
            {
                float s = 0.0f;
                for (int h = 1; h <= 16; ++h)
                    s += (1.0f / (float)h) * std::sin ((float)h * a) * (h % 2 == 0 ? -1.0f : 1.0f);
                wavetables[1][i] = s * 0.55f;
            }

            // 2 — Pulse 33%: narrow duty cycle via additive synthesis
            {
                const float duty  = 0.33f;
                float s = 2.0f * duty - 1.0f;
                for (int h = 1; h <= 14; ++h)
                    s += (2.0f / (float)(h * juce::MathConstants<float>::pi))
                         * std::sin ((float)h * juce::MathConstants<float>::pi * duty)
                         * std::cos ((float)h * a);
                wavetables[2][i] = juce::jlimit (-1.0f, 1.0f, s * 0.72f);
            }

            // 3 — Overtone: fundamental + accented 3rd, 5th, 7th (organ character)
            wavetables[3][i] = 0.55f * std::sin (a)
                             + 0.28f * std::sin (3.0f * a)
                             + 0.13f * std::sin (5.0f * a)
                             + 0.04f * std::sin (7.0f * a);

            // 4 — Formant: mid-band emphasis — resonant peak around 3rd–5th harmonics
            wavetables[4][i] = 0.35f * std::sin (a)
                             + 0.20f * std::sin (2.0f * a)
                             + 0.30f * std::sin (3.0f * a)    // peak
                             + 0.25f * std::sin (4.0f * a)    // peak
                             + 0.15f * std::sin (5.0f * a)
                             + 0.06f * std::sin (6.0f * a)
                             + 0.02f * std::sin (7.0f * a);

            // normalise formant
            wavetables[4][i] *= 0.75f;

            // 5 — Pluck: decaying harmonic series (amplitude ∝ 1/h²)
            {
                float s = 0.0f;
                for (int h = 1; h <= 12; ++h)
                    s += (1.0f / (float)(h * h)) * std::sin ((float)h * a);
                wavetables[5][i] = s * 1.15f;
            }

            // 6 — Bell: inharmonic partials (characteristic ratios ×1, ×2.76, ×5.40, ×8.93)
            wavetables[6][i] = 0.55f * std::sin (a)
                             + 0.28f * std::sin (2.76f * a)
                             + 0.13f * std::sin (5.40f * a)
                             + 0.04f * std::sin (8.93f * a);

            // 7 — Shimmer: dense stacked fifths + octaves with slight detuning
            wavetables[7][i] = 0.40f * std::sin (a)
                             + 0.22f * std::sin (2.003f * a)   // slight detune of octave
                             + 0.18f * std::sin (1.498f * a)   // slight detune of perfect fifth
                             + 0.12f * std::sin (3.007f * a)   // detune of 2nd octave
                             + 0.08f * std::sin (4.012f * a);  // detune of 3rd octave
        }
    }

    float getSample (int waveIndex, float position) const noexcept
    {
        const int idx        = juce::jlimit (0, numWavetables - 1, waveIndex);
        const float wrapped  = position - std::floor (position);
        const float tablePos = wrapped * static_cast<float> (tableSize);
        const int   ia       = static_cast<int> (tablePos) % tableSize;
        const int   ib       = (ia + 1) % tableSize;
        const float frac     = tablePos - static_cast<float> (ia);
        const float a        = wavetables[static_cast<size_t>(idx)][static_cast<size_t>(ia)];
        const float b        = wavetables[static_cast<size_t>(idx)][static_cast<size_t>(ib)];
        return a + frac * (b - a);
    }

    // Human-readable names for the combo box
    static const char* getWaveName (int i)
    {
        static constexpr const char* names[] = {
            "Sine", "Bright Saw", "Pulse 33%", "Overtone",
            "Formant", "Pluck", "Bell", "Shimmer"
        };
        return (i >= 0 && i < numWavetables) ? names[i] : "?";
    }

private:
    std::array<std::array<float, tableSize>, numWavetables> wavetables {};
};
