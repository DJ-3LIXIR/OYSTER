/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class BioluminescentKnobLAF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override
    {
        const float radius   = juce::jmin (width, height) * 0.5f - 6.0f;
        const float centreX  = x + width  * 0.5f;
        const float centreY  = y + height * 0.5f;
        const float angle    = rotaryStartAngle
                             + sliderPosProportional
                             * (rotaryEndAngle - rotaryStartAngle);

        // --- Outer glow ring ---
        const float glowRadius = radius + 6.0f;
        juce::ColourGradient glowGrad (
            juce::Colour (0xff00e5ff).withAlpha (0.25f),
            centreX, centreY,
            juce::Colours::transparentBlack,
            centreX + glowRadius, centreY,
            true);
        g.setGradientFill (glowGrad);
        g.fillEllipse (centreX - glowRadius, centreY - glowRadius,
                       glowRadius * 2.0f, glowRadius * 2.0f);

        // --- Dark body ---
        juce::ColourGradient bodyGrad (
            juce::Colour (0xff0d2233),
            centreX, centreY - radius,
            juce::Colour (0xff050d14),
            centreX, centreY + radius,
            false);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (centreX - radius, centreY - radius,
                       radius * 2.0f, radius * 2.0f);

        // --- Thin outer ring ---
        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.6f));
        g.drawEllipse (centreX - radius, centreY - radius,
                       radius * 2.0f, radius * 2.0f, 1.2f);

        // --- Arc track (background) ---
        juce::Path trackArc;
        trackArc.addCentredArc (centreX, centreY, radius - 5.0f, radius - 5.0f,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.strokePath (trackArc, juce::PathStrokeType (2.5f));

        // --- Glowing value arc ---
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY,
                                radius - 5.0f, radius - 5.0f,
                                0.0f, rotaryStartAngle, angle, true);

        juce::ColourGradient arcGrad (
            juce::Colour (0xff00e5ff),
            centreX - radius, centreY,
            juce::Colour (0xff0077aa),
            centreX + radius, centreY,
            false);
        g.setGradientFill (arcGrad);
        g.strokePath (valueArc, juce::PathStrokeType (
            2.8f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

        // --- Pointer line ---
        const float pointerLength = radius * 0.55f;
        const float pointerX = centreX + pointerLength * std::sin (angle);
        const float pointerY = centreY - pointerLength * std::cos (angle);

        // Glowing pointer dot
        juce::ColourGradient dotGrad (
            juce::Colours::white,
            pointerX, pointerY,
            juce::Colour (0xff00e5ff).withAlpha (0.0f),
            pointerX + 6.0f, pointerY + 6.0f,
            true);
        g.setGradientFill (dotGrad);
        g.fillEllipse (pointerX - 4.0f, pointerY - 4.0f, 8.0f, 8.0f);

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (pointerX - 2.5f, pointerY - 2.5f, 5.0f, 5.0f);

        // --- Inner reflection highlight ---
        juce::ColourGradient shineGrad (
            juce::Colours::white.withAlpha (0.07f),
            centreX - radius * 0.3f, centreY - radius * 0.5f,
            juce::Colours::transparentBlack,
            centreX, centreY,
            false);
        g.setGradientFill (shineGrad);
        g.fillEllipse (centreX - radius * 0.7f, centreY - radius * 0.8f,
                       radius * 1.0f, radius * 0.9f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox&) override
    {
        const auto box = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

        juce::ColourGradient bgGrad (
            juce::Colour (0xff0b1f2f), box.getCentreX(), box.getY(),
            juce::Colour (0xff05111a), box.getCentreX(), box.getBottom(),
            false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (box, 5.0f);

        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.6f));
        g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float) width - 14.0f;
        const float cy = (float) height * 0.5f;
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 3.5f);
        arrow.closeSubPath();
        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.8f));
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (1, 1, box.getWidth() - 20, box.getHeight() - 2);
        label.setFont (juce::FontOptions (11.0f));
    }
};

class GrainPositionDisplay : public juce::Component
{
public:
    void setPosition (float p) noexcept;
    void setSpray (float s) noexcept;
    void setGrainSize (float g) noexcept;
    void setDrawingEnabled (bool e) noexcept { drawingEnabled = e; repaint(); }

    void paint (juce::Graphics& g) override;

private:
    float position { 0.5f }, spray { 0.1f }, grainSize { 0.1f };
    bool drawingEnabled { true };
};

class WaveformPreview : public juce::Component
{
public:
    void setWaveformIndex (int idx) noexcept;
    void setWaveBIndex (int idx) noexcept;
    void setMorphAmount (float m) noexcept;
    void setVertical (bool v) noexcept { isVertical = v; repaint(); }
    // 0 = WavetableEngine (grain), 1 = SubOscEngine, 2 = Osc2Engine
    void setPreviewMode (int mode) noexcept { previewMode = mode; repaint(); }
    void setDrawingEnabled (bool e) noexcept { drawingEnabled = e; repaint(); }

    void paint (juce::Graphics& g) override;

private:
    int waveIndex { 0 };
    int waveIndexB { 1 };
    float morphAmt { 0.0f };
    bool isVertical { false };
    int previewMode { 0 };
    bool drawingEnabled { true };
};

class FilterResponseDisplay : public juce::Component
{
public:
    void setFilterType (int t) noexcept { filterType = t; repaint(); }
    void setCutoff (float c) noexcept { cutoffNorm = c; repaint(); }
    void setResonance (float r) noexcept { resonance = r; repaint(); }
    void setDrawingEnabled (bool e) noexcept { drawingEnabled = e; repaint(); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float w = b.getWidth();
        const float h = b.getHeight();

        g.setColour (juce::Colour (0xff020e18));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.3f));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

        if (! drawingEnabled) return;

        g.setColour (juce::Colours::white.withAlpha (0.04f));
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine ((int) (b.getY() + h * (float) i / 4.0f), b.getX() + 4.0f, b.getRight() - 4.0f);
        for (int i = 1; i < 6; ++i)
            g.drawVerticalLine ((int) (b.getX() + w * (float) i / 6.0f), b.getY() + 4.0f, b.getBottom() - 4.0f);

        const float cutoffX = 0.05f + cutoffNorm * 0.88f;
        const float q = 0.5f + resonance * 6.0f;

        juce::Path curve;
        const int steps = (int) w;

        for (int i = 0; i <= steps; ++i)
        {
            const float nx = (float) i / (float) steps;
            float gain = 0.0f;

            switch (filterType)
            {
                case 0:
                    if (nx < cutoffX)
                        gain = 0.85f;
                    else
                    {
                        const float dist = (nx - cutoffX) / (1.0f - cutoffX + 0.001f);
                        gain = 0.85f * std::exp (-dist * 4.5f);
                    }
                    {
                        const float peak = std::exp (-std::pow ((nx - cutoffX) * q * 8.0f, 2.0f));
                        gain += peak * (q - 0.5f) * 0.18f;
                    }
                    break;
                case 1:
                    if (nx > cutoffX)
                        gain = 0.85f;
                    else
                    {
                        const float dist = (cutoffX - nx) / (cutoffX + 0.001f);
                        gain = 0.85f * std::exp (-dist * 4.5f);
                    }
                    {
                        const float peak = std::exp (-std::pow ((nx - cutoffX) * q * 8.0f, 2.0f));
                        gain += peak * (q - 0.5f) * 0.18f;
                    }
                    break;
                case 2:
                {
                    const float width = 0.08f + (1.0f - resonance) * 0.25f;
                    gain = 0.85f * std::exp (-std::pow ((nx - cutoffX) / width, 2.0f));
                    break;
                }
                case 3:
                {
                    const float width = 0.04f + (1.0f - resonance) * 0.12f;
                    const float notch = std::exp (-std::pow ((nx - cutoffX) / width, 2.0f));
                    gain = 0.85f * (1.0f - notch);
                    break;
                }
                default:
                    break;
            }

            gain = juce::jlimit (0.0f, 1.0f, gain);
            const float px = b.getX() + nx * w;
            const float py = b.getBottom() - 4.0f - gain * (h - 8.0f);

            if (i == 0)
                curve.startNewSubPath (px, py);
            else
                curve.lineTo (px, py);
        }

        juce::Path filled = curve;
        filled.lineTo (b.getRight() - 1.0f, b.getBottom() - 4.0f);
        filled.lineTo (b.getX() + 1.0f, b.getBottom() - 4.0f);
        filled.closeSubPath();

        juce::ColourGradient fillGrad (juce::Colour (0xff00e5ff).withAlpha (0.12f), 0.0f, b.getY(),
                                       juce::Colour (0xff00e5ff).withAlpha (0.02f), 0.0f, b.getBottom(), false);
        g.setGradientFill (fillGrad);
        g.fillPath (filled);

        juce::ColourGradient lineGrad (juce::Colour (0xff00e5ff), b.getX(), 0.0f,
                                       juce::Colour (0xff0077aa), b.getRight(), 0.0f, false);
        g.setGradientFill (lineGrad);
        g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float markerX = b.getX() + cutoffX * w;
        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.3f));
        g.drawLine (markerX, b.getY() + 4.0f, markerX, b.getBottom() - 4.0f, 1.0f);

        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.4f));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("RESPONSE", (int) b.getX() + 6, (int) b.getY() + 4, 70, 11, juce::Justification::centredLeft);
    }

private:
    int filterType { 0 };
    float cutoffNorm { 0.7f };
    float resonance { 0.1f };
    bool drawingEnabled { true };
};

class LfoDisplay : public juce::Component
{
public:
    void setShape (int s) noexcept { shape = s; repaint(); }
    void setRate (float r) noexcept { rate = r; repaint(); }
    void setDrawingEnabled (bool e) noexcept { drawingEnabled = e; repaint(); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float w = b.getWidth();
        const float h = b.getHeight();

        g.setColour (juce::Colour (0xff020e18));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.25f));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

        if (! drawingEnabled) return;

        juce::Path p;
        const int steps = (int) w - 8;
        const float cycles = 2.5f;

        for (int i = 0; i <= steps; ++i)
        {
            const float nx = (float) i / (float) steps;
            const float phase = nx * cycles;
            const float frac = phase - std::floor (phase);
            float y = 0.0f;

            switch (shape)
            {
                case 0: y = std::sin (frac * juce::MathConstants<float>::twoPi); break;
                case 1: y = (frac < 0.5f) ? (frac * 4.0f - 1.0f) : (3.0f - frac * 4.0f); break;
                case 2: y = frac * 2.0f - 1.0f; break;
                case 3: y = frac < 0.5f ? 1.0f : -1.0f; break;
                case 4:
                {
                    const int seg = (int) (phase * 2.0f);
                    y = (seg % 3 == 0) ? 0.7f : (seg % 3 == 1) ? -0.4f : 0.2f;
                    break;
                }
                default:
                    break;
            }

            const float px = b.getX() + 4.0f + (float) i;
            const float py = b.getY() + (0.5f - y * 0.38f) * h;
            if (i == 0)
                p.startNewSubPath (px, py);
            else
                p.lineTo (px, py);
        }

        juce::ColourGradient grad (juce::Colour (0xff00e5ff), b.getX(), 0.0f,
                                   juce::Colour (0xff0077aa), b.getRight(), 0.0f, false);
        g.setGradientFill (grad);
        g.strokePath (p, juce::PathStrokeType (1.5f));

        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.4f));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("LFO", (int) b.getX() + 5, (int) b.getY() + 3, 30, 11, juce::Justification::centredLeft);
    }

private:
    int shape { 0 };
    float rate { 1.0f };
    bool drawingEnabled { true };
};

class EnvelopeDisplay : public juce::Component
{
public:
    void setAttack (float a) noexcept { attack = a; repaint(); }
    void setDecay (float d) noexcept { decay = d; repaint(); }
    void setSustain (float s) noexcept { sustain = s; repaint(); }
    void setRelease (float r) noexcept { release = r; repaint(); }
    void setDrawingEnabled (bool e) noexcept { drawingEnabled = e; repaint(); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float w = b.getWidth() - 8.0f;
        const float h = b.getHeight() - 8.0f;
        const float ox = b.getX() + 4.0f;
        const float oy = b.getY() + 4.0f;

        g.setColour (juce::Colour (0xff020e18));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.25f));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

        if (! drawingEnabled) return;

        const float total = attack + decay + 0.3f + release + 0.001f;
        const float atkW = (attack / total) * w;
        const float decW = (decay / total) * w;
        const float susW = (0.3f / total) * w;
        const float relW = (release / total) * w;
        const float susH = sustain * h;

        juce::Path env;
        env.startNewSubPath (ox, oy + h);
        env.lineTo (ox + atkW, oy);
        env.lineTo (ox + atkW + decW, oy + h - susH);
        env.lineTo (ox + atkW + decW + susW, oy + h - susH);
        env.lineTo (ox + atkW + decW + susW + relW, oy + h);

        juce::Path filled = env;
        filled.lineTo (ox, oy + h);
        filled.closeSubPath();
        juce::ColourGradient fillGrad (juce::Colour (0xff00e5ff).withAlpha (0.12f), 0.0f, oy,
                                       juce::Colour (0xff00e5ff).withAlpha (0.02f), 0.0f, oy + h, false);
        g.setGradientFill (fillGrad);
        g.fillPath (filled);

        juce::ColourGradient lineGrad (juce::Colour (0xff00e5ff), ox, 0.0f,
                                       juce::Colour (0xff0077aa), ox + w, 0.0f, false);
        g.setGradientFill (lineGrad);
        g.strokePath (env, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.4f));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("ENV", (int) b.getX() + 5, (int) b.getY() + 3, 30, 11, juce::Justification::centredLeft);
    }

private:
    float attack { 0.01f };
    float decay { 0.1f };
    float sustain { 0.8f };
    float release { 0.3f };
    bool drawingEnabled { true };
};

class OysterAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                    public juce::Timer
{
public:
    OysterAudioProcessorEditor (OysterAudioProcessor&);
    ~OysterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Component contentComponent;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OysterAudioProcessor& audioProcessor;
    // keyboard needs audioProcessor reference — initialised in cpp
    juce::MidiKeyboardComponent keyboard;
    BioluminescentKnobLAF bioluminescentLAF;

    // Navigation
    int currentPage { 0 };  // 0 = Main, 1 = Synth, 2 = Matrix, 3 = Settings
    int activeSlot  { 0 };
    void rebuildAllAttachments();

    // Nav bar buttons
    juce::TextButton mainPageButton   { "MAIN" };
    juce::TextButton synthPageButton  { "SYNTH" };
    juce::TextButton matrixPageButton { "MATRIX" };
    juce::TextButton settingsPageButton { "SETTINGS" };

    // Page panels — one component per page, swapped in/out
    juce::Component mainPanel;
    juce::Component synthPanel;
    juce::Component matrixPanel;
    juce::Component settingsPanel;

    // Helper
    void showPage (int pageIndex);
    void paintNavBar (juce::Graphics& g);
    void paintMainPage (juce::Graphics& g);
    void paintSynthPage (juce::Graphics& g);
    void paintMatrixPage (juce::Graphics& g);
    void paintSettingsPage (juce::Graphics& g);

    // Matrix page — modulation routing grid layout
    static constexpr int matNumSources = 2;   // LFO, ENV
    static constexpr int matNumDests   = 5;   // Cutoff, Position, Pitch, Density, Amp
    // Track which cells are valid (have an APVTS parameter)
    const bool matCellValid[2][5] = {
        { true, true, true, true, false },   // LFO: cutoff, position, pitch, density, (no amp)
        { true, true, true, false, true  }   // ENV: cutoff, position, pitch, (no density), amp
    };

    // FFT
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder; // 2048
    static constexpr int numBars  = 64;

    juce::dsp::FFT forwardFFT { fftOrder };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, numBars>     barHeights {};
    std::array<float, numBars>     smoothedBars {};

    juce::Timer* visualTimer { nullptr };

    // For feeding audio data to the visualizer
    std::array<float, fftSize> audioFifo {};
    int fifoIndex { 0 };
    bool nextFFTBlockReady { false };

    void pushSampleToFifo (float sample) noexcept;
    void drawOceanDisplay (juce::Graphics& g);
    void drawPresetSlots (juce::Graphics& g);

    // Ocean display state
    float wavePhase1  { 0.0f };
    float wavePhase2  { 0.0f };
    float wavePhase3  { 0.0f };
    float waveEnergy  { 0.0f };   // 0..1 smoothed audio energy

    struct FoamParticle
    {
        float x { 0.0f }, y { 0.0f };
        float life  { 0.0f };   // 0..1 remaining life
        float size  { 2.0f };
    };
    std::vector<FoamParticle> foamParticles;
    void drawSynthPage (juce::Graphics& g);
    void updateFFT();
    void timerCallback() override;

    // Preset slot buttons
    static constexpr int numPresetSlots = 4;
    int slotAreaY    { 0 };
    int mainDispY    { 46 };
    int mainDispH    { 300 };
    int presetStripX { 420 };
    std::array<juce::TextButton, numPresetSlots> presetSlotButtons;
    // presetSlotNames now lives in audioProcessor to survive editor open/close

    // Main page macro knobs
    juce::Slider reverbKnob;
    juce::Slider shimmerKnob;
    juce::Slider warmthKnob;
    juce::Slider driftKnob;
    juce::Slider bloomKnob;
    juce::Slider mixKnob;

    juce::Label reverbLabel;
    juce::Label shimmerLabel;
    juce::Label warmthLabel;
    juce::Label driftLabel;
    juce::Label bloomLabel;
    juce::Label mixLabel;

    void setupMacroKnob (juce::Slider& knob, juce::Label& label,
                         const juce::String& name);

    void setupSynthKnob (juce::Slider& knob, juce::Label& label,
                         const juce::String& name);
    void layoutKnobRow (juce::Rectangle<int> area,
                        std::vector<std::pair<juce::Slider*, juce::Label*>> knobs,
                        int knobSize = 58);

    WaveformPreview osc2Preview;
    WaveformPreview subOscPreview;
    GrainPositionDisplay grainDisplay;
    juce::Slider positionKnob, sprayKnob, grainSizeKnob,
                 densityKnob, pitchScatterKnob, panSpreadKnob;
    juce::Label positionLabel, sprayLabel, grainSizeLabel,
                densityLabel, pitchScatterLabel, panSpreadLabel;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAtt> positionAtt, sprayAtt, grainSizeAtt,
        densityAtt, pitchScatterAtt, panSpreadAtt;

    WaveformPreview wavePreview;
    juce::ComboBox waveABox, waveBBox;
    juce::Slider morphKnob;
    juce::Label waveALabel, waveBLabel, morphLabel;

    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAtt> waveAAtt, waveBAtt;
    std::unique_ptr<SliderAtt> morphAtt;

    juce::Slider wtOctaveKnob, wtSemitoneKnob, wtFineKnob;
    juce::Label wtOctaveLabel, wtSemitoneLabel, wtFineLabel;
    std::unique_ptr<SliderAtt> wtOctaveAtt, wtSemitoneAtt, wtFineAtt;

    juce::Slider wtPhaseKnob, wtTiltKnob;
    juce::Label wtPhaseLabel, wtTiltLabel;
    std::unique_ptr<SliderAtt> wtPhaseAtt, wtTiltAtt;

    juce::Slider unisonVoicesKnob, unisonDetuneKnob, unisonSpreadKnob;
    juce::Label unisonVoicesLabel, unisonDetuneLabel, unisonSpreadLabel;
    std::unique_ptr<SliderAtt> unisonVoicesAtt, unisonDetuneAtt, unisonSpreadAtt;

    juce::Label unisonSectionLabel;
    juce::Label pitchSectionLabel;

    juce::Slider filterCutoffKnob, filterResKnob;
    juce::Label filterCutoffLabel, filterResLabel;
    std::unique_ptr<SliderAtt> filterCutoffAtt, filterResAtt;
    FilterResponseDisplay filterDisplay;
    juce::TextButton filterLPButton { "LP" };
    juce::TextButton filterHPButton { "HP" };
    juce::TextButton filterBPButton { "BP" };
    juce::TextButton filterNTButton { "NOTCH" };
    juce::Slider filterDriveKnob;
    juce::Label filterDriveLabel;
    std::unique_ptr<SliderAtt> filterDriveAtt;
    juce::Slider filterEnvAmtKnob, filterLfoAmtKnob, filterKeyTrackKnob;
    juce::Label filterEnvAmtLabel, filterLfoAmtLabel, filterKeyTrackLabel;
    std::unique_ptr<SliderAtt> filterEnvAmtAtt, filterLfoAmtAtt, filterKeyTrackAtt;
    int currentFilterType { 0 };

    // OSC 2
    juce::ComboBox osc2WaveBox;
    juce::Label    osc2WaveLabel;
    std::unique_ptr<ComboAtt> osc2WaveAtt;

    juce::Slider osc2OctaveKnob, osc2SemiKnob, osc2FineKnob, osc2PhaseKnob;
    juce::Slider osc2MixKnob, osc2DetuneKnob, osc2PanKnob;
    juce::Label  osc2OctaveLabel, osc2SemiLabel, osc2FineLabel, osc2PhaseLabel;
    juce::Label  osc2MixLabel, osc2DetuneLabel, osc2PanLabel;
    std::unique_ptr<SliderAtt> osc2OctaveAtt, osc2SemiAtt, osc2FineAtt, osc2PhaseAtt;
    std::unique_ptr<SliderAtt> osc2MixAtt, osc2DetuneAtt, osc2PanAtt;

    // Sub OSC
    juce::ComboBox subWaveBox;
    juce::Label    subWaveLabel;
    std::unique_ptr<ComboAtt> subWaveAtt;
    juce::TextButton subOscPowerBtn;
    juce::TextButton osc2PowerBtn;
    juce::TextButton grainPowerBtn;

    juce::Slider subOctaveKnob, subMixKnob, subTuneKnob, subSemiKnob, subPhaseKnob, subPanKnob;
    juce::Label  subOctaveLabel, subMixLabel, subTuneLabel, subSemiLabel, subPhaseLabel, subPanLabel;
    std::unique_ptr<SliderAtt> subOctaveAtt, subMixAtt, subTuneAtt, subSemiAtt, subPhaseAtt, subPanAtt;

    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAtt> subOscEnabledAtt, osc2EnabledAtt, grainEnabledAtt;

    // Control knobs
    juce::Slider glideKnob, bendUpKnob, bendDownKnob, velSensKnob;
    juce::Label  glideLabel, bendUpLabel, bendDownLabel, velSensLabel;
    std::unique_ptr<SliderAtt> glideAtt, bendUpAtt, bendDownAtt, velSensAtt;

    // Master knobs (MAIN page)
    juce::Slider masterVolumeKnob, masterPanKnob, masterTransposeKnob, masterTuneKnob2;
    juce::Label  masterVolumeLabel, masterPanLabel, masterTransposeLabel, masterTuneLabel2;
    std::unique_ptr<SliderAtt> masterVolumeAtt, masterPanAtt, masterTransposeAtt, masterTuneAtt2;

    class TransparentImageComponent : public juce::Component
    {
    public:
        void setImage (const juce::Image& img) { image = img; }
        void paint (juce::Graphics& g) override
        {
            if (image.isValid())
                g.drawImageWithin (image, 0, 0, getWidth(), getHeight(),
                                   juce::RectanglePlacement::centred);
        }
    private:
        juce::Image image;
    };

    TransparentImageComponent oysterLogo;

    juce::Slider octaveShiftKnob, stereoWidthKnob, masterTuneKnob;
    juce::Label  octaveShiftLabel, stereoWidthLabel, masterTuneLabel;
    std::unique_ptr<SliderAtt> octaveShiftAtt, stereoWidthAtt, masterTuneAtt;

    // Modulation knobs
    juce::Slider lfoToCutoffKnob, lfoToPositionKnob, lfoToPitchKnob, lfoToDensityKnob;
    juce::Label  lfoToCutoffLabel, lfoToPositionLabel, lfoToPitchLabel, lfoToDensityLabel;
    std::unique_ptr<SliderAtt> lfoToCutoffAtt, lfoToPositionAtt, lfoToPitchAtt, lfoToDensityAtt;

    juce::Slider envToCutoffKnob, envToPositionKnob, envToPitchKnob, envToAmpKnob;
    juce::Label  envToCutoffLabel, envToPositionLabel, envToPitchLabel, envToAmpLabel;
    std::unique_ptr<SliderAtt> envToCutoffAtt, envToPositionAtt, envToPitchAtt, envToAmpAtt;

    juce::Slider reverbSizeKnob, reverbMixKnob, chorusMixKnob;
    juce::Label reverbSizeLabel, reverbMixLabel, chorusMixLabel;
    std::unique_ptr<SliderAtt> reverbSizeAtt, reverbMixAtt, chorusMixAtt;

    EnvelopeDisplay envDisplay;
    juce::Slider envAttackKnob, envDecayKnob, envSustainKnob, envReleaseKnob;
    juce::Label envAttackLabel, envDecayLabel, envSustainLabel, envReleaseLabel;
    std::unique_ptr<SliderAtt> envAttackAtt, envDecayAtt, envSustainAtt, envReleaseAtt;

    LfoDisplay lfoDisplay;
    juce::Slider lfoRateKnob, lfoDepthKnob, lfoAttackKnob, lfoDecayKnob;
    juce::Label lfoRateLabel, lfoDepthLabel, lfoAttackLabel, lfoDecayLabel;
    std::unique_ptr<SliderAtt> lfoRateAtt, lfoDepthAtt, lfoAttackAtt, lfoDecayAtt;
    juce::TextButton lfoSyncBtn { "SYNC" }, lfoRetriggerBtn { "RETRIG" }, lfoPhaseBtn { "PHASE" }, lfoEnabledBtn { "ON" };
    std::unique_ptr<ButtonAtt> lfoSyncAtt, lfoRetriggerAtt, lfoPhaseAtt, lfoEnabledAtt;

    juce::TextButton lfoSineBtn { "SIN" };
    juce::TextButton lfoTriBtn { "TRI" };
    juce::TextButton lfoSawBtn { "SAW" };
    juce::TextButton lfoSqBtn { "SQ" };
    juce::TextButton lfoSHBtn { "S&H" };
    int currentLfoShape { 0 };

    // Envelope / LFO tab selectors
    int currentEnvTab { 0 };
    int currentLfoTab { 0 };
    int lfoShapes[4]  { 0, 0, 0, 0 };
    juce::TextButton envTabBtns[4];
    juce::TextButton lfoTabBtns[4];

    juce::ComboBox env4DestBox;
    std::unique_ptr<ComboAtt> env4DestAtt;

    int wtPanelH    { 0 };
    int filterPanelH { 0 };

    // Preset strip (header bar)
    juce::Label presetSlotLabel, presetTypeLabel, presetNameLabel;
    juce::TextButton presetSaveBtn { "SAVE" };

    // Cached preset file lists for sidebar (refreshed when MAIN page is shown)
    juce::Array<juce::File> sidebarStockPresets;
    juce::Array<juce::File> sidebarUserPresets;

    void refreshPresetLists();
    void loadPresetFromSidebar (const juce::File& file);

    // ── Settings page section labels ───────────────────────────────────
    juce::Label settAudioPerfLabel, settMidiLabel, settPolyphonyLabel,
                settDisplayLabel,
                settTuningLabel, settMasterLabel, settEffectsLabel;

    // Settings page bounds (computed in resized, used in paint)
    juce::Rectangle<int> settAudioPerfBounds, settMidiBounds, settPolyphonyBounds,
                         settDisplayBounds,
                         settTuningBounds, settMasterBounds, settEffectsBounds;

    void paintSettingsPanels (juce::Graphics& g);

    // ── Settings: Audio / Performance controls ─────────────────────────
    juce::ComboBox settOversamplingBox, settQualityBox;
    juce::Label    settOversamplingLabel, settQualityLabel;
    juce::TextButton settCpuOptBtn { "CPU OPT" };
    juce::Label    settBufferSizeVal, settSampleRateVal;
    juce::Label    settBufferSizeLabel, settSampleRateLabel;

    // ── Settings: Display controls ──────────────────────────────────────
    juce::ComboBox settWindowSizeBox;
    juce::Label    settWindowSizeLabel;
    juce::TextButton settShowKeyboardBtn   { "Show Keyboard: ON" };
    juce::TextButton settShowWaveformsBtn  { "Show Waveforms: ON" };
    juce::TextButton settShowOceanBtn      { "Show FFT Ocean: ON" };
    juce::TextButton settShowLabelsBtn     { "Show Labels: ON" };

    // ── Per-slot MIDI channel selectors ──────────────────────────────────
    std::array<juce::ComboBox, 4>  slotMidiChannelBox;
    std::array<juce::Label, 4>     slotMidiChannelLabel;

    // ── Settings: MIDI controls ─────────────────────────────────────────
    juce::TextButton settSustainPedalBtn   { "Sustain Pedal: ON" };
    juce::TextButton settLegatoModeBtn     { "Legato Mode: OFF" };
    juce::TextButton settProgramChangeBtn  { "Program Change: ON" };
    juce::TextButton settMidiLearnBtn      { "MIDI CC Learn: OFF" };

    // ── Settings: Polyphony / Voicing controls ──────────────────────────
    juce::ComboBox   settVoiceCountBox;
    juce::Label      settVoiceCountLabel;
    juce::ComboBox   settVoiceStealBox;
    juce::Label      settVoiceStealLabel;
    juce::ComboBox   settUnisonVoicesKnob;
    juce::Label      settUnisonVoicesLabel;
    juce::ComboBox   settUnisonDetuneKnob;
    juce::Label      settUnisonDetuneLabel;

    // ── Settings: Tuning controls ───────────────────────────────────────
    juce::ComboBox   settMasterTuneKnob2;
    juce::Label      settMasterTuneLabel2;
    juce::ComboBox   settTuningSystemBox;
    juce::Label      settTuningSystemLabel;
    juce::ComboBox   settRefPitchBox;
    juce::Label      settRefPitchLabel;
    juce::ComboBox   settTransposeBox;
    juce::Label      settTransposeLabel;
    juce::ComboBox   settOctaveShiftBox;
    juce::Label      settOctaveShiftLabel;
    juce::ComboBox   settScaleBox;
    juce::Label      settScaleLabel;

    // ── Settings: Master controls ───────────────────────────────────────
    juce::ComboBox   settMasterVolBox;
    juce::Label      settMasterVolLabel;
    juce::ComboBox   settLimiterBox;
    juce::Label      settLimiterLabel;
    juce::TextButton settMeteringBtn      { "Metering: ON" };
    juce::ComboBox   settOutputPanBox;
    juce::Label      settOutputPanLabel;

    // ── Settings: Effects Defaults controls ──────────────────────────────
    juce::ComboBox   settDryWetBox;
    juce::Label      settDryWetLabel;
    juce::ComboBox   settDitherBox;
    juce::Label      settDitherLabel;
    juce::TextButton settSoftClipBtn      { "Soft Clipping: OFF" };
    juce::TextButton settPhaseInvertBtn   { "Phase Invert: OFF" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OysterAudioProcessorEditor)
};
