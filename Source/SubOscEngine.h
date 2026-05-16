#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

/*  SubOscEngine
    ============
    8 bass-optimised wavetables purpose-built for sub / foundation roles.

    Index  Name          Character
    -----  ------------  --------------------------------------------------
    0      Deep Sine     Pure fundamental – maximum sub-bass, zero harmonics
    1      Sub Octave    Sine + soft sub-octave doubling (one octave below)
    2      Warm Bass     Fundamental + gentle 2nd harmonic body
    3      Fat Saw       Band-limited saw with highs rolled off (~6 partials)
    4      Hollow Sq     Odd-only harmonics (square character), limited to 5
    5      Growl         Asymmetric, 2nd+3rd harmonics – classic bass growl
    6      Tube Drive    Tanh-saturated sine – warm, slightly even-harmonic
    7      Thud          Attack-body shape: squared half-cycle + smooth tail
*/

class SubOscEngine
{
public:
    static constexpr int tableSize    = 2048;
    static constexpr int numWavetables = 8;

    SubOscEngine() { buildWavetables(); }

    void buildWavetables()
    {
        for (int i = 0; i < tableSize; ++i)
        {
            const float p = static_cast<float> (i) / static_cast<float> (tableSize);
            const float a = juce::MathConstants<float>::twoPi * p;

            // 0 — Deep Sine
            wavetables[0][i] = std::sin (a);

            // 1 — Sub Octave: fundamental + -12 semitone sine at -12 dB
            wavetables[1][i] = 0.85f * std::sin (a)
                             + 0.25f * std::sin (a * 0.5f);  // half freq = sub octave

            // 2 — Warm Bass: fundamental + soft 2nd harmonic
            wavetables[2][i] = 0.80f * std::sin (a)
                             + 0.16f * std::sin (2.0f * a)
                             + 0.04f * std::sin (3.0f * a);

            // 3 — Fat Saw: bandlimited additive (6 partials), rolled off
            {
                float s = 0.0f;
                const float amps[6] = { 1.0f, 0.55f, 0.30f, 0.15f, 0.07f, 0.03f };
                for (int h = 0; h < 6; ++h)
                    s += amps[h] * std::sin ((float)(h + 1) * a)
                         * (h % 2 == 0 ? 1.0f : -1.0f);   // saw sign pattern
                wavetables[3][i] = s * 0.62f;  // normalise
            }

            // 4 — Hollow Square: odd harmonics only (1,3,5,7,9), limited
            {
                float s = std::sin (a)
                        + (1.0f / 3.0f) * std::sin (3.0f * a)
                        + (1.0f / 5.0f) * std::sin (5.0f * a)
                        + (1.0f / 7.0f) * std::sin (7.0f * a)
                        + (1.0f / 9.0f) * std::sin (9.0f * a);
                wavetables[4][i] = s * 0.62f;
            }

            // 5 — Growl: asymmetric waveform (2nd + 3rd dominate)
            wavetables[5][i] = 0.60f * std::sin (a)
                             + 0.28f * std::sin (2.0f * a)
                             + 0.12f * std::sin (3.0f * a - 0.4f);  // slight phase offset

            // 6 — Tube Drive: tanh-shaped sine (soft even-harmonic saturation)
            {
                const float drive = 2.5f;
                const float raw   = std::sin (a);
                wavetables[6][i]  = std::tanh (raw * drive) / std::tanh (drive);
            }

            // 7 — Thud: squared positive half-cycle + smooth negative tail
            {
                float v;
                if (p < 0.5f)
                    v = std::sin (a) * std::sin (a);     // squared = percussive attack body
                else
                    v = -0.55f * std::sin (a - juce::MathConstants<float>::pi * 0.1f);
                wavetables[7][i] = v;
            }
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
            "Deep Sine", "Sub Octave", "Warm Bass", "Fat Saw",
            "Hollow Sq", "Growl", "Tube Drive", "Thud"
        };
        return (i >= 0 && i < numWavetables) ? names[i] : "?";
    }

private:
    std::array<std::array<float, tableSize>, numWavetables> wavetables {};
};
