/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

void GrainPositionDisplay::setPosition (float p) noexcept
{
    position = juce::jlimit (0.0f, 1.0f, p);
    repaint();
}

void GrainPositionDisplay::setSpray (float s) noexcept
{
    spray = juce::jlimit (0.0f, 1.0f, s);
    repaint();
}

void GrainPositionDisplay::setGrainSize (float g) noexcept
{
    grainSize = juce::jlimit (0.0f, 1.0f, g);
    repaint();
}

void GrainPositionDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff020e18));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.3f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

    if (! drawingEnabled) return;

    const float w = b.getWidth();
    const float h = b.getHeight();

    juce::Path wave;
    for (int i = 0; i <= (int) w; ++i)
    {
        const float nx = (float) i / w;
        const float ny = 0.5f + 0.35f * std::sin (nx * juce::MathConstants<float>::twoPi * 3.0f);
        const float px = b.getX() + nx * w;
        const float py = b.getY() + ny * h;
        if (i == 0)
            wave.startNewSubPath (px, py);
        else
            wave.lineTo (px, py);
    }
    g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.2f));
    g.strokePath (wave, juce::PathStrokeType (1.0f));

    const float sprayW = juce::jlimit (4.0f, w, spray * w);
    const float posX = b.getX() + position * w;
    g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.08f));
    g.fillRoundedRectangle (posX - sprayW * 0.5f, b.getY() + 2.0f, sprayW, h - 4.0f, 4.0f);

    const float grainW = juce::jlimit (2.0f, w * 0.5f, grainSize * w * 0.4f);
    g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.15f));
    g.fillRoundedRectangle (posX - grainW * 0.5f, b.getY() + h * 0.2f, grainW, h * 0.6f, 3.0f);

    g.setColour (juce::Colour (0xff00e5ff));
    g.drawLine (posX, b.getY() + 3.0f, posX, b.getBottom() - 3.0f, 1.8f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (posX - 3.5f, b.getY() + h * 0.5f - 3.5f, 7.0f, 7.0f);

    g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.4f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("POSITION", (int) b.getX() + 5, (int) b.getY() + 3, 55, 11, juce::Justification::centredLeft);
}

void WaveformPreview::setWaveformIndex (int idx) noexcept
{
    waveIndex = juce::jlimit (0, 7, idx);
    repaint();
}

void WaveformPreview::setWaveBIndex (int idx) noexcept
{
    waveIndexB = juce::jlimit (0, 7, idx);
    repaint();
}

void WaveformPreview::setMorphAmount (float m) noexcept
{
    morphAmt = juce::jlimit (0.0f, 1.0f, m);
    repaint();
}

void WaveformPreview::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff020e18));
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.25f));
    g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

    if (! drawingEnabled) return;

    // Returns one sample for a given engine mode, wave index, and phase [0,1)
    auto getWaveSample = [this] (int wi, float phase) -> float
    {
        const float a = phase * juce::MathConstants<float>::twoPi;

        if (previewMode == 1)
        {
            // SubOscEngine waveforms
            switch (wi)
            {
                case 0: return std::sin (a);  // Deep Sine
                case 1: return 0.85f * std::sin (a) + 0.25f * std::sin (a * 0.5f);  // Sub Octave
                case 2: return 0.80f * std::sin (a) + 0.16f * std::sin (2.0f * a) + 0.04f * std::sin (3.0f * a);  // Warm Bass
                case 3: // Fat Saw
                {
                    float s = 0.0f;
                    const float amps[6] = { 1.0f, 0.55f, 0.30f, 0.15f, 0.07f, 0.03f };
                    for (int h = 0; h < 6; ++h)
                        s += amps[h] * std::sin ((float)(h + 1) * a) * (h % 2 == 0 ? 1.0f : -1.0f);
                    return s * 0.62f;
                }
                case 4: // Hollow Sq
                    return (std::sin (a) + (1.0f/3.0f)*std::sin(3.0f*a)
                          + (1.0f/5.0f)*std::sin(5.0f*a) + (1.0f/7.0f)*std::sin(7.0f*a)
                          + (1.0f/9.0f)*std::sin(9.0f*a)) * 0.62f;
                case 5: return 0.60f*std::sin(a) + 0.28f*std::sin(2.0f*a) + 0.12f*std::sin(3.0f*a - 0.4f);  // Growl
                case 6: { const float d = 2.5f; return std::tanh(std::sin(a)*d)/std::tanh(d); }  // Tube Drive
                case 7: return phase < 0.5f ? std::sin(a)*std::sin(a) : -0.55f*std::sin(a - juce::MathConstants<float>::pi*0.1f);  // Thud
                default: return 0.0f;
            }
        }
        else if (previewMode == 2)
        {
            // Osc2Engine waveforms
            switch (wi)
            {
                case 0: return std::sin (a);  // Sine
                case 1: // Bright Saw
                {
                    float s = 0.0f;
                    for (int h = 1; h <= 16; ++h)
                        s += (1.0f/(float)h) * std::sin((float)h*a) * (h%2==0 ? -1.0f : 1.0f);
                    return s * 0.55f;
                }
                case 2: // Pulse 33%
                {
                    const float duty = 0.33f;
                    float s = 2.0f*duty - 1.0f;
                    for (int h = 1; h <= 14; ++h)
                        s += (2.0f/(float)(h*juce::MathConstants<float>::pi))
                             * std::sin((float)h*juce::MathConstants<float>::pi*duty)
                             * std::cos((float)h*a);
                    return juce::jlimit(-1.0f, 1.0f, s*0.72f);
                }
                case 3: return 0.55f*std::sin(a)+0.28f*std::sin(3.0f*a)+0.13f*std::sin(5.0f*a)+0.04f*std::sin(7.0f*a);  // Overtone
                case 4: return 0.75f*(0.35f*std::sin(a)+0.20f*std::sin(2.0f*a)+0.30f*std::sin(3.0f*a)+0.25f*std::sin(4.0f*a)+0.15f*std::sin(5.0f*a));  // Formant
                case 5: // Pluck
                {
                    float s = 0.0f;
                    for (int h = 1; h <= 12; ++h) s += (1.0f/(float)(h*h))*std::sin((float)h*a);
                    return s * 1.15f;
                }
                case 6: return 0.55f*std::sin(a)+0.28f*std::sin(2.76f*a)+0.13f*std::sin(5.40f*a)+0.04f*std::sin(8.93f*a);  // Bell
                case 7: return 0.40f*std::sin(a)+0.22f*std::sin(2.003f*a)+0.18f*std::sin(1.498f*a)+0.12f*std::sin(3.007f*a)+0.08f*std::sin(4.012f*a);  // Shimmer
                default: return 0.0f;
            }
        }
        else
        {
            // WavetableEngine waveforms (grain / main)
            switch (wi)
            {
                case 0: return std::sin (a);
                case 1: return 2.0f * phase - 1.0f;
                case 2: return phase < 0.5f ? 1.0f : -1.0f;
                case 3: return 1.0f - 4.0f * std::abs (phase - 0.5f);
                case 4: return 0.86f*std::sin(a)+0.10f*std::sin(2.0f*a)+0.04f*std::sin(3.0f*a);
                case 5: return 0.80f*(2.0f*phase-1.0f)+0.15f*std::sin(a)+0.05f*std::sin(2.0f*a);
                case 6: return juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
                case 7:
                {
                    const float ws = std::sin (a);
                    const float saw = 2.0f * phase - 1.0f;
                    const float tri = 1.0f - 4.0f * std::abs (phase - 0.5f);
                    const float wm = 0.86f*ws + 0.10f*std::sin(2.0f*a) + 0.04f*std::sin(3.0f*a);
                    const float sm = 0.80f*saw + 0.15f*std::sin(a) + 0.05f*std::sin(2.0f*a);
                    return 0.5f * wm + 0.35f * sm + 0.15f * tri;
                }
                default: return 0.0f;
            }
        }
    };

    const float ox = b.getX() + 4.0f;
    const float oy = b.getY() + 4.0f;
    const float w = b.getWidth() - 8.0f;
    const float h = b.getHeight() - 8.0f;

    juce::Path path;
    const int steps = juce::jmax (1, isVertical ? (int)h : (int)w);

    for (int i = 0; i <= steps; ++i)
    {
        const float phase = (float)i / (float)steps;
        const float sA = getWaveSample (waveIndex, phase);
        const float sB = getWaveSample (waveIndexB, phase);
        const float s  = sA + morphAmt * (sB - sA);

        float px, py;
        if (isVertical)
        {
            px = ox + (0.5f - s * 0.42f) * w;
            py = oy + (float)i * (h / (float)steps);
        }
        else
        {
            px = ox + (float)i * (w / (float)steps);
            py = oy + (0.5f - s * 0.42f) * h;
        }

        if (i == 0) path.startNewSubPath (px, py);
        else        path.lineTo (px, py);
    }

    juce::ColourGradient grad (juce::Colour (0xff00e5ff), ox, oy,
                               juce::Colour (0xff0077aa), ox + w, oy, false);
    g.setGradientFill (grad);
    g.strokePath (path, juce::PathStrokeType (1.5f));

    g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.4f));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("PREVIEW", (int) b.getX() + 5, (int) b.getY() + 3, 50, 11, juce::Justification::centredLeft);
}

//==============================================================================
OysterAudioProcessorEditor::OysterAudioProcessorEditor (OysterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1260, 840);
    addAndMakeVisible (contentComponent);

    // Nav button setup
    const int navH = 36;
    const int btnW = 120;
    const int btnY = 4;

    mainPageButton.setBounds (20, btnY, btnW, navH - 8);
    synthPageButton.setBounds (145, btnY, btnW, navH - 8);
    matrixPageButton.setBounds (270, btnY, btnW, navH - 8);
    settingsPageButton.setBounds (395, btnY, btnW, navH - 8);

    // Style nav buttons — transparent background, white text
    auto styleNavButton = [](juce::TextButton& btn)
    {
        btn.setColour (juce::TextButton::buttonColourId,
                       juce::Colours::white.withAlpha (0.08f));
        btn.setColour (juce::TextButton::buttonOnColourId,
                       juce::Colours::white.withAlpha (0.22f));
        btn.setColour (juce::TextButton::textColourOffId,
                       juce::Colours::white.withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId,
                       juce::Colours::white);
        btn.setClickingTogglesState (false);
    };

    styleNavButton (mainPageButton);
    styleNavButton (synthPageButton);
    styleNavButton (matrixPageButton);
    styleNavButton (settingsPageButton);

    contentComponent.addAndMakeVisible (mainPageButton);
    contentComponent.addAndMakeVisible (synthPageButton);
    contentComponent.addAndMakeVisible (matrixPageButton);
    contentComponent.addAndMakeVisible (settingsPageButton);

    mainPageButton.onClick    = [this] { showPage (0); };
    synthPageButton.onClick   = [this] { showPage (1); };
    matrixPageButton.onClick  = [this] { showPage (2); };
    settingsPageButton.onClick = [this] { showPage (3); };

    showPage (0);

    // Preset strip in header bar
    auto stylePresetLabel = [this] (juce::Label& lbl, const juce::String& text,
                                     float alpha, float fontSize, juce::Justification just)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setFont (juce::FontOptions (fontSize));
        lbl.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (alpha));
        lbl.setJustificationType (just);
        contentComponent.addAndMakeVisible (lbl);
    };
    // Background/outline drawn manually in paint() as one unified bar
    presetSlotLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    presetSlotLabel.setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);
    presetTypeLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    presetTypeLabel.setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);
    // Sync slot number label from processor's active slot
    const int initSlot = audioProcessor.getActiveSlot();
    activeSlot = initSlot;

    stylePresetLabel (presetSlotLabel,  "SLOT " + juce::String (initSlot + 1),  1.0f, 11.0f, juce::Justification::centred);
    stylePresetLabel (presetTypeLabel,  "PRESET",  0.5f, 10.0f, juce::Justification::centredLeft);
    stylePresetLabel (presetNameLabel,  audioProcessor.presetManager.getCurrentPresetName(), 0.8f, 11.0f, juce::Justification::centred);

    // Preset strip positioning handled in resized() — just style + add here
    presetSaveBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff0a1520));
    presetSaveBtn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff00e5ff).withAlpha (0.5f));
    presetSaveBtn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff00e5ff));
    contentComponent.addAndMakeVisible (presetSaveBtn);

    presetSaveBtn.onClick = [this]
    {
        auto* dialog = new juce::AlertWindow ("Save Preset",
                                              "Enter a name for this preset:",
                                              juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor ("name",
                               audioProcessor.presetManager.getCurrentPresetName(),
                               "Preset name:");
        dialog->addButton ("Save",   1);
        dialog->addButton ("Cancel", 0);

        dialog->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, dialog] (int result)
            {
                if (result == 1)
                {
                    auto name = dialog->getTextEditorContents ("name").trim();
                    if (name.isEmpty()) name = "Untitled";

                    if (audioProcessor.presetManager.savePreset (name))
                    {
                        presetNameLabel.setText (name, juce::dontSendNotification);
                        audioProcessor.presetSlotNames[(size_t) activeSlot] = name;
                        refreshPresetLists();
                        repaint();
                    }
                }
                delete dialog;
            }), false);
    };

    smoothedBars.fill (0.0f);
    startTimerHz (60);  // 60fps repaint
    keyboard.setAvailableRange (36, 96);   // C2 to C7
    keyboard.setLowestVisibleKey (48);     // start display at C3
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,
                        juce::Colour (0xff0d2233));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,
                        juce::Colour (0xff020e18));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        juce::Colour (0xff00e5ff).withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        juce::Colour (0xff00b4cc).withAlpha (0.4f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId,
                        juce::Colour (0xff00e5ff));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId,
                        juce::Colour (0xff071825));
    contentComponent.addAndMakeVisible (keyboard);

    // Style and add preset slot buttons
    for (int i = 0; i < numPresetSlots; ++i)
    {
        auto& btn = presetSlotButtons[i];
        btn.setButtonText ("SLOT " + juce::String (i + 1));

        btn.setColour (juce::TextButton::buttonColourId,
                       juce::Colour (0xffb0b8c1));   // silver
        btn.setColour (juce::TextButton::buttonOnColourId,
                       juce::Colour (0xffd0d8e0));   // lighter silver when active
        btn.setColour (juce::TextButton::textColourOffId,
                       juce::Colour (0xff1a1a2e));   // dark navy text
        btn.setColour (juce::TextButton::textColourOnId,
                       juce::Colour (0xff000000));

        contentComponent.addAndMakeVisible (btn);

        btn.onClick = [this, i]
        {
            audioProcessor.switchSlot (i);
            activeSlot = i;
            rebuildAllAttachments();
            presetSlotLabel.setText ("SLOT " + juce::String (i + 1), juce::dontSendNotification);
            showPage (1);
            for (int j = 0; j < numPresetSlots; ++j)
                presetSlotButtons[j].setToggleState (j == i, juce::dontSendNotification);
        };
    }

    setupMacroKnob (reverbKnob,  reverbLabel,  "REVERB");
    setupMacroKnob (shimmerKnob, shimmerLabel, "SHIMMER");
    setupMacroKnob (warmthKnob,  warmthLabel,  "WARMTH");
    setupMacroKnob (driftKnob,   driftLabel,   "DRIFT");
    setupMacroKnob (bloomKnob,   bloomLabel,   "BLOOM");
    setupMacroKnob (mixKnob,     mixLabel,     "MIX");

    contentComponent.addAndMakeVisible (osc2Preview);
    contentComponent.addAndMakeVisible (subOscPreview);
    contentComponent.addAndMakeVisible (grainDisplay);
    setupSynthKnob (positionKnob,     positionLabel,     "POSITION");
    setupSynthKnob (sprayKnob,        sprayLabel,        "SPRAY");
    setupSynthKnob (grainSizeKnob,    grainSizeLabel,    "SIZE");
    setupSynthKnob (densityKnob,      densityLabel,      "DENSITY");
    setupSynthKnob (pitchScatterKnob, pitchScatterLabel, "PITCH");
    setupSynthKnob (panSpreadKnob,    panSpreadLabel,    "PAN");

    positionAtt     = std::make_unique<SliderAtt> (p.apvts, "position", positionKnob);
    sprayAtt        = std::make_unique<SliderAtt> (p.apvts, "spray", sprayKnob);
    grainSizeAtt    = std::make_unique<SliderAtt> (p.apvts, "grainSize", grainSizeKnob);
    densityAtt      = std::make_unique<SliderAtt> (p.apvts, "density", densityKnob);
    pitchScatterAtt = std::make_unique<SliderAtt> (p.apvts, "pitchScatter", pitchScatterKnob);
    panSpreadAtt    = std::make_unique<SliderAtt> (p.apvts, "panSpread", panSpreadKnob);

    auto updateDisplay = [this]()
    {
        grainDisplay.setPosition ((float) positionKnob.getValue());
        grainDisplay.setSpray ((float) sprayKnob.getValue());
        grainDisplay.setGrainSize ((float) grainSizeKnob.getValue() / 500.0f);
    };
    positionKnob.onValueChange = updateDisplay;
    sprayKnob.onValueChange = updateDisplay;
    grainSizeKnob.onValueChange = updateDisplay;

    contentComponent.addAndMakeVisible (wavePreview);
    waveABox.setLookAndFeel (&bioluminescentLAF);
    waveBBox.setLookAndFeel (&bioluminescentLAF);
    waveABox.setColour (juce::ComboBox::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.85f));
    waveBBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.85f));
    waveABox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    waveBBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    waveABox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    waveBBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);

    static constexpr const char* waveNames[] {
        "Sine", "Saw", "Square", "Triangle", "Warm Sine", "Soft Saw", "Noise", "Hybrid"
    };
    for (int i = 0; i < 8; ++i)
    {
        waveABox.addItem (waveNames[i], i + 1);
        waveBBox.addItem (waveNames[i], i + 1);
    }

    auto setupComboLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setFont (juce::FontOptions (9.5f));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.65f));
        contentComponent.addAndMakeVisible (label);
    };

    setupComboLabel (waveALabel, "WAVE A");
    setupComboLabel (waveBLabel, "WAVE B");
    contentComponent.addAndMakeVisible (waveABox);
    contentComponent.addAndMakeVisible (waveBBox);

    setupSynthKnob (morphKnob, morphLabel, "MORPH");
    waveAAtt = std::make_unique<ComboAtt> (p.apvts, "waveA", waveABox);
    waveBAtt = std::make_unique<ComboAtt> (p.apvts, "waveB", waveBBox);
    morphAtt = std::make_unique<SliderAtt> (p.apvts, "morphAmount", morphKnob);

    auto updateWavePreview = [this]()
    {
        wavePreview.setWaveformIndex (juce::jlimit (0, 7, waveABox.getSelectedItemIndex()));
        wavePreview.setWaveBIndex (juce::jlimit (0, 7, waveBBox.getSelectedItemIndex()));
        wavePreview.setMorphAmount ((float) morphKnob.getValue());
    };
    waveABox.onChange = updateWavePreview;
    waveBBox.onChange = updateWavePreview;
    morphKnob.onValueChange = updateWavePreview;

    setupSynthKnob (wtOctaveKnob, wtOctaveLabel, "OCTAVE");
    setupSynthKnob (wtSemitoneKnob, wtSemitoneLabel, "SEMI");
    setupSynthKnob (wtFineKnob, wtFineLabel, "FINE");
    wtOctaveAtt = std::make_unique<SliderAtt> (p.apvts, "wtOctave", wtOctaveKnob);
    wtSemitoneAtt = std::make_unique<SliderAtt> (p.apvts, "wtSemitone", wtSemitoneKnob);
    wtFineAtt = std::make_unique<SliderAtt> (p.apvts, "wtFine", wtFineKnob);

    setupSynthKnob (wtPhaseKnob, wtPhaseLabel, "PHASE");
    setupSynthKnob (wtTiltKnob, wtTiltLabel, "TILT");
    wtPhaseAtt = std::make_unique<SliderAtt> (p.apvts, "wtPhase", wtPhaseKnob);
    wtTiltAtt = std::make_unique<SliderAtt> (p.apvts, "wtTilt", wtTiltKnob);

    setupSynthKnob (unisonVoicesKnob, unisonVoicesLabel, "VOICES");
    setupSynthKnob (unisonDetuneKnob, unisonDetuneLabel, "DETUNE");
    setupSynthKnob (unisonSpreadKnob, unisonSpreadLabel, "SPREAD");
    unisonVoicesAtt = std::make_unique<SliderAtt> (p.apvts, "unisonVoices", unisonVoicesKnob);
    unisonDetuneAtt = std::make_unique<SliderAtt> (p.apvts, "unisonDetune", unisonDetuneKnob);
    unisonSpreadAtt = std::make_unique<SliderAtt> (p.apvts, "unisonSpread", unisonSpreadKnob);

    pitchSectionLabel.setText ("PITCH", juce::dontSendNotification);
    unisonSectionLabel.setText ("UNISON", juce::dontSendNotification);
    for (auto* lbl : { &pitchSectionLabel, &unisonSectionLabel })
    {
        lbl->setFont (juce::FontOptions (9.0f));
        lbl->setJustificationType (juce::Justification::centredLeft);
        lbl->setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.4f));
        contentComponent.addAndMakeVisible (*lbl);
    }

    contentComponent.addAndMakeVisible (filterDisplay);
    for (auto* btn : { &filterLPButton, &filterHPButton, &filterBPButton, &filterNTButton })
    {
        btn->setClickingTogglesState (false);
        btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff071825));
        btn->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn->setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn->setColour (juce::TextButton::textColourOnId, juce::Colour (0xff00e5ff));
        contentComponent.addAndMakeVisible (*btn);
    }
    filterLPButton.setToggleState (true, juce::dontSendNotification);

    auto setFilterType = [this] (int type)
    {
        currentFilterType = type;
        filterDisplay.setFilterType (type);
        filterLPButton.setToggleState (type == 0, juce::dontSendNotification);
        filterHPButton.setToggleState (type == 1, juce::dontSendNotification);
        filterBPButton.setToggleState (type == 2, juce::dontSendNotification);
        filterNTButton.setToggleState (type == 3, juce::dontSendNotification);
        if (auto* param = audioProcessor.apvts.getParameter ("filterType"))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) type));
    };
    filterLPButton.onClick = [setFilterType] { setFilterType (0); };
    filterHPButton.onClick = [setFilterType] { setFilterType (1); };
    filterBPButton.onClick = [setFilterType] { setFilterType (2); };
    filterNTButton.onClick = [setFilterType] { setFilterType (3); };

    setupSynthKnob (filterCutoffKnob, filterCutoffLabel, "CUTOFF");
    setupSynthKnob (filterResKnob, filterResLabel, "RESONANCE");
    setupSynthKnob (filterDriveKnob, filterDriveLabel, "DRIVE");
    setupSynthKnob (filterEnvAmtKnob,  filterEnvAmtLabel,  "ENV AMT");
    setupSynthKnob (filterLfoAmtKnob,  filterLfoAmtLabel,  "LFO AMT");
    setupSynthKnob (filterKeyTrackKnob, filterKeyTrackLabel, "KEY TRACK");
    filterCutoffAtt   = std::make_unique<SliderAtt> (p.apvts, "filterCutoff",  filterCutoffKnob);
    filterResAtt      = std::make_unique<SliderAtt> (p.apvts, "filterRes",     filterResKnob);
    filterDriveAtt    = std::make_unique<SliderAtt> (p.apvts, "filterDrive",   filterDriveKnob);
    filterEnvAmtAtt   = std::make_unique<SliderAtt> (p.apvts, "filterEnvAmt",  filterEnvAmtKnob);
    filterLfoAmtAtt   = std::make_unique<SliderAtt> (p.apvts, "filterLfoAmt",  filterLfoAmtKnob);
    filterKeyTrackAtt = std::make_unique<SliderAtt> (p.apvts, "filterKeyTrack", filterKeyTrackKnob);

    filterCutoffKnob.onValueChange = [this]
    {
        const float norm = (float) ((std::log (filterCutoffKnob.getValue()) - std::log (20.0))
                                  / (std::log (20000.0) - std::log (20.0)));
        filterDisplay.setCutoff (norm);
    };
    filterResKnob.onValueChange = [this]
    {
        filterDisplay.setResonance ((float) (filterResKnob.getValue() / 10.0));
    };

    if (auto* typeParam = audioProcessor.apvts.getRawParameterValue ("filterType"))
        setFilterType ((int) typeParam->load());
    else
        setFilterType (0);

    // OSC 2 dropdown
    osc2WaveBox.setLookAndFeel (&bioluminescentLAF);
    osc2WaveBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.85f));
    osc2WaveBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    osc2WaveBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    static constexpr const char* waveNames2[] {
        "Sine", "Bright Saw", "Pulse 33%", "Overtone",
        "Formant", "Pluck", "Bell", "Shimmer"
    };
    for (int i = 0; i < 8; ++i) osc2WaveBox.addItem (waveNames2[i], i + 1);
    contentComponent.addAndMakeVisible (osc2WaveBox);
    osc2WaveAtt = std::make_unique<ComboAtt> (p.apvts, "osc2Wave", osc2WaveBox);
    osc2WaveBox.onChange = [this]() { osc2Preview.setWaveformIndex (osc2WaveBox.getSelectedItemIndex()); };
    osc2Preview.setPreviewMode (2);  // Osc2Engine waveforms

    setupSynthKnob (osc2OctaveKnob, osc2OctaveLabel, "OCTAVE");
    setupSynthKnob (osc2SemiKnob,   osc2SemiLabel,   "SEMI");
    setupSynthKnob (osc2FineKnob,   osc2FineLabel,   "FINE");
    setupSynthKnob (osc2PhaseKnob,  osc2PhaseLabel,  "PHASE");
    setupSynthKnob (osc2MixKnob,    osc2MixLabel,    "MIX");
    setupSynthKnob (osc2DetuneKnob, osc2DetuneLabel, "DETUNE");
    setupSynthKnob (osc2PanKnob,    osc2PanLabel,    "PAN");
    osc2OctaveAtt = std::make_unique<SliderAtt> (p.apvts, "osc2Octave", osc2OctaveKnob);
    osc2SemiAtt   = std::make_unique<SliderAtt> (p.apvts, "osc2Semi",   osc2SemiKnob);
    osc2FineAtt   = std::make_unique<SliderAtt> (p.apvts, "osc2Fine",   osc2FineKnob);
    osc2PhaseAtt  = std::make_unique<SliderAtt> (p.apvts, "osc2Phase",  osc2PhaseKnob);
    osc2MixAtt    = std::make_unique<SliderAtt> (p.apvts, "osc2Mix",    osc2MixKnob);
    osc2DetuneAtt = std::make_unique<SliderAtt> (p.apvts, "osc2Detune", osc2DetuneKnob);
    osc2PanAtt    = std::make_unique<SliderAtt> (p.apvts, "osc2Pan",    osc2PanKnob);

    // Sub OSC dropdown
    subWaveBox.setLookAndFeel (&bioluminescentLAF);
    subWaveBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.85f));
    subWaveBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    subWaveBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    static constexpr const char* subWaveNames[] {
        "Deep Sine", "Sub Octave", "Warm Bass", "Fat Saw",
        "Hollow Sq", "Growl", "Tube Drive", "Thud"
    };
    for (int i = 0; i < 8; ++i) subWaveBox.addItem (subWaveNames[i], i + 1);
    contentComponent.addAndMakeVisible (subWaveBox);
    subWaveAtt = std::make_unique<ComboAtt> (p.apvts, "subWave", subWaveBox);
    subWaveBox.onChange = [this]() { subOscPreview.setWaveformIndex (subWaveBox.getSelectedItemIndex()); };
    subOscPreview.setPreviewMode (1);  // SubOscEngine waveforms

    // Power buttons — styled as glowing LED toggles
    auto setupPowerBtn = [&] (juce::TextButton& btn)
    {
        btn.setButtonText (juce::String::fromUTF8 ("\xe2\x8f\xbb")); // ⏻ power symbol
        btn.setClickingTogglesState (true);
        btn.setToggleState (true, juce::dontSendNotification);
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00e5ff).withAlpha (0.25f));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff00e5ff));
        contentComponent.addAndMakeVisible (btn);
    };
    setupPowerBtn (subOscPowerBtn);
    setupPowerBtn (osc2PowerBtn);
    setupPowerBtn (grainPowerBtn);
    subOscEnabledAtt = std::make_unique<ButtonAtt> (p.apvts, "subOscEnabled", subOscPowerBtn);
    osc2EnabledAtt   = std::make_unique<ButtonAtt> (p.apvts, "osc2Enabled",   osc2PowerBtn);
    grainEnabledAtt  = std::make_unique<ButtonAtt> (p.apvts, "grainEnabled",  grainPowerBtn);

    setupSynthKnob (subOctaveKnob, subOctaveLabel, "OCTAVE");
    setupSynthKnob (subSemiKnob,   subSemiLabel,   "SEMI");
    setupSynthKnob (subMixKnob,    subMixLabel,    "MIX");
    setupSynthKnob (subTuneKnob,   subTuneLabel,   "TUNE");
    setupSynthKnob (subPanKnob,    subPanLabel,    "PAN");
    setupSynthKnob (subPhaseKnob,  subPhaseLabel,  "PHASE");
    subOctaveAtt = std::make_unique<SliderAtt> (p.apvts, "subOctave", subOctaveKnob);
    subSemiAtt   = std::make_unique<SliderAtt> (p.apvts, "subSemi",   subSemiKnob);
    subMixAtt    = std::make_unique<SliderAtt> (p.apvts, "subMix",    subMixKnob);
    subTuneAtt   = std::make_unique<SliderAtt> (p.apvts, "subTune",   subTuneKnob);
    subPanAtt    = std::make_unique<SliderAtt> (p.apvts, "subPan",    subPanKnob);
    subPhaseAtt  = std::make_unique<SliderAtt> (p.apvts, "subPhase",  subPhaseKnob);

    // Control row 1 — keyboard/performance
    setupSynthKnob (glideKnob,    glideLabel,    "GLIDE");
    setupSynthKnob (bendUpKnob,   bendUpLabel,   "BEND UP");
    setupSynthKnob (bendDownKnob, bendDownLabel, "BEND DN");
    setupSynthKnob (velSensKnob,  velSensLabel,  "VELOCITY");
    glideAtt    = std::make_unique<SliderAtt> (p.apvts, "glide",    glideKnob);
    bendUpAtt   = std::make_unique<SliderAtt> (p.apvts, "bendUp",   bendUpKnob);
    bendDownAtt = std::make_unique<SliderAtt> (p.apvts, "bendDown", bendDownKnob);
    velSensAtt  = std::make_unique<SliderAtt> (p.apvts, "velSens",  velSensKnob);

    // Master knobs (MAIN page)
    setupMacroKnob (masterVolumeKnob,    masterVolumeLabel,    "VOLUME");
    setupMacroKnob (masterPanKnob,       masterPanLabel,       "PAN");
    setupMacroKnob (masterTransposeKnob, masterTransposeLabel, "TRANSP");
    setupMacroKnob (masterTuneKnob2,     masterTuneLabel2,     "TUNE");
    masterVolumeAtt    = std::make_unique<SliderAtt> (p.apvts, "masterVolume", masterVolumeKnob);
    masterPanAtt       = std::make_unique<SliderAtt> (p.apvts, "masterPan",    masterPanKnob);
    masterTransposeAtt = std::make_unique<SliderAtt> (p.apvts, "transpose",    masterTransposeKnob);
    masterTuneAtt2     = std::make_unique<SliderAtt> (p.apvts, "masterTune",   masterTuneKnob2);

    // Oyster logo between PAN and TRANSPOSE
    {
        auto logoImage = juce::ImageCache::getFromMemory (BinaryData::OysterLogo_png,
                                                          BinaryData::OysterLogo_pngSize);
        if (logoImage.isValid())
            oysterLogo.setImage (logoImage);
        else
            oysterLogo.setImage ({});
        contentComponent.addAndMakeVisible (oysterLogo);
    }


    // Control row 2 — voicing
    setupSynthKnob (octaveShiftKnob, octaveShiftLabel, "OCTAVE");
    setupSynthKnob (stereoWidthKnob, stereoWidthLabel, "WIDTH");
    setupSynthKnob (masterTuneKnob,  masterTuneLabel,  "TUNE");
    octaveShiftAtt = std::make_unique<SliderAtt> (p.apvts, "octaveShift", octaveShiftKnob);
    stereoWidthAtt = std::make_unique<SliderAtt> (p.apvts, "stereoWidth", stereoWidthKnob);
    masterTuneAtt  = std::make_unique<SliderAtt> (p.apvts, "masterTune",  masterTuneKnob);

    // Modulation row 1 — LFO targets
    setupSynthKnob (lfoToCutoffKnob,   lfoToCutoffLabel,   "LFO>CUT");
    setupSynthKnob (lfoToPositionKnob, lfoToPositionLabel, "LFO>POS");
    setupSynthKnob (lfoToPitchKnob,    lfoToPitchLabel,    "LFO>PCH");
    setupSynthKnob (lfoToDensityKnob,  lfoToDensityLabel,  "LFO>DEN");
    lfoToCutoffAtt   = std::make_unique<SliderAtt> (p.apvts, "lfoToCutoff",   lfoToCutoffKnob);
    lfoToPositionAtt = std::make_unique<SliderAtt> (p.apvts, "lfoToPosition", lfoToPositionKnob);
    lfoToPitchAtt    = std::make_unique<SliderAtt> (p.apvts, "lfTooPitch",    lfoToPitchKnob);
    lfoToDensityAtt  = std::make_unique<SliderAtt> (p.apvts, "lfoToDensity",  lfoToDensityKnob);

    // Modulation row 2 — ENV targets
    setupSynthKnob (envToCutoffKnob,   envToCutoffLabel,   "ENV>CUT");
    setupSynthKnob (envToPositionKnob, envToPositionLabel, "ENV>POS");
    setupSynthKnob (envToPitchKnob,    envToPitchLabel,    "ENV>PCH");
    setupSynthKnob (envToAmpKnob,      envToAmpLabel,      "ENV>AMP");
    envToCutoffAtt   = std::make_unique<SliderAtt> (p.apvts, "envToCutoff",   envToCutoffKnob);
    envToPositionAtt = std::make_unique<SliderAtt> (p.apvts, "envToPosition", envToPositionKnob);
    envToPitchAtt    = std::make_unique<SliderAtt> (p.apvts, "envToPitch",    envToPitchKnob);
    envToAmpAtt      = std::make_unique<SliderAtt> (p.apvts, "envToAmp",      envToAmpKnob);

    setupSynthKnob (reverbSizeKnob, reverbSizeLabel, "REV SIZE");
    setupSynthKnob (reverbMixKnob, reverbMixLabel, "REV MIX");
    setupSynthKnob (chorusMixKnob, chorusMixLabel, "CHORUS");
    reverbSizeAtt = std::make_unique<SliderAtt> (p.apvts, "reverbSize", reverbSizeKnob);
    reverbMixAtt = std::make_unique<SliderAtt> (p.apvts, "reverbMix", reverbMixKnob);
    chorusMixAtt = std::make_unique<SliderAtt> (p.apvts, "chorusMix", chorusMixKnob);

    contentComponent.addAndMakeVisible (envDisplay);
    setupSynthKnob (envAttackKnob, envAttackLabel, "ATTACK");
    setupSynthKnob (envDecayKnob, envDecayLabel, "DECAY");
    setupSynthKnob (envSustainKnob, envSustainLabel, "SUSTAIN");
    setupSynthKnob (envReleaseKnob, envReleaseLabel, "RELEASE");
    envAttackAtt = std::make_unique<SliderAtt> (p.apvts, "envAttack", envAttackKnob);
    envDecayAtt = std::make_unique<SliderAtt> (p.apvts, "envDecay", envDecayKnob);
    envSustainAtt = std::make_unique<SliderAtt> (p.apvts, "envSustain", envSustainKnob);
    envReleaseAtt = std::make_unique<SliderAtt> (p.apvts, "envRelease", envReleaseKnob);

    auto updateEnv = [this]()
    {
        envDisplay.setAttack ((float) envAttackKnob.getValue());
        envDisplay.setDecay ((float) envDecayKnob.getValue());
        envDisplay.setSustain ((float) envSustainKnob.getValue());
        envDisplay.setRelease ((float) envReleaseKnob.getValue());
    };
    envAttackKnob.onValueChange = updateEnv;
    envDecayKnob.onValueChange = updateEnv;
    envSustainKnob.onValueChange = updateEnv;
    envReleaseKnob.onValueChange = updateEnv;

    contentComponent.addAndMakeVisible (lfoDisplay);
    setupSynthKnob (lfoRateKnob, lfoRateLabel, "RATE");
    setupSynthKnob (lfoDepthKnob, lfoDepthLabel, "DEPTH");
    lfoRateAtt = std::make_unique<SliderAtt> (p.apvts, "lfo1Rate", lfoRateKnob);
    lfoDepthAtt = std::make_unique<SliderAtt> (p.apvts, "lfo1Depth", lfoDepthKnob);
    lfoRateKnob.onValueChange = [this]() { lfoDisplay.setRate ((float) lfoRateKnob.getValue()); };

    setupSynthKnob (lfoAttackKnob, lfoAttackLabel, "ATTACK");
    setupSynthKnob (lfoDecayKnob, lfoDecayLabel, "DECAY");
    lfoAttackAtt = std::make_unique<SliderAtt> (p.apvts, "lfo1Attack", lfoAttackKnob);
    lfoDecayAtt  = std::make_unique<SliderAtt> (p.apvts, "lfo1Decay",  lfoDecayKnob);

    auto styleToggleBtn = [this] (juce::TextButton& btn)
    {
        btn.setClickingTogglesState (true);
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff00e5ff));
        contentComponent.addAndMakeVisible (btn);
    };
    styleToggleBtn (lfoSyncBtn);
    styleToggleBtn (lfoRetriggerBtn);
    styleToggleBtn (lfoPhaseBtn);
    styleToggleBtn (lfoEnabledBtn);
    lfoSyncAtt      = std::make_unique<ButtonAtt> (p.apvts, "lfo1Sync",      lfoSyncBtn);
    lfoRetriggerAtt = std::make_unique<ButtonAtt> (p.apvts, "lfo1Retrigger", lfoRetriggerBtn);
    lfoPhaseAtt     = std::make_unique<ButtonAtt> (p.apvts, "lfo1Phase",     lfoPhaseBtn);
    lfoEnabledAtt   = std::make_unique<ButtonAtt> (p.apvts, "lfo1Enabled",   lfoEnabledBtn);

    auto styleLfoBtn = [this] (juce::TextButton& btn)
    {
        btn.setClickingTogglesState (false);
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff00e5ff));
        contentComponent.addAndMakeVisible (btn);
    };
    styleLfoBtn (lfoSineBtn);
    styleLfoBtn (lfoTriBtn);
    styleLfoBtn (lfoSawBtn);
    styleLfoBtn (lfoSqBtn);
    styleLfoBtn (lfoSHBtn);
    lfoSineBtn.setToggleState (true, juce::dontSendNotification);

    auto setLfoShape = [this] (int s)
    {
        currentLfoShape = s;
        lfoDisplay.setShape (s);
        lfoSineBtn.setToggleState (s == 0, juce::dontSendNotification);
        lfoTriBtn.setToggleState (s == 1, juce::dontSendNotification);
        lfoSawBtn.setToggleState (s == 2, juce::dontSendNotification);
        lfoSqBtn.setToggleState (s == 3, juce::dontSendNotification);
        lfoSHBtn.setToggleState (s == 4, juce::dontSendNotification);
    };
    lfoSineBtn.onClick = [setLfoShape] { setLfoShape (0); };
    lfoTriBtn.onClick = [setLfoShape] { setLfoShape (1); };
    lfoSawBtn.onClick = [setLfoShape] { setLfoShape (2); };
    lfoSqBtn.onClick = [setLfoShape] { setLfoShape (3); };
    lfoSHBtn.onClick = [setLfoShape] { setLfoShape (4); };

    // --- Envelope / LFO tab selectors ---

    auto styleTabBtn = [this] (juce::TextButton& btn, const juce::String& label)
    {
        btn.setButtonText (label);
        btn.setClickingTogglesState (false);
        btn.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff00e5ff).withAlpha (0.5f));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff00e5ff));
        contentComponent.addAndMakeVisible (btn);
    };
    for (int i = 0; i < 4; ++i)
    {
        styleTabBtn (envTabBtns[i], juce::String (i + 1));
        styleTabBtn (lfoTabBtns[i], juce::String (i + 1));
    }
    envTabBtns[0].setToggleState (true, juce::dontSendNotification);
    lfoTabBtns[0].setToggleState (true, juce::dontSendNotification);

    // ENV 4 destination combo box
    env4DestBox.addItem ("Off",      1);
    env4DestBox.addItem ("Cutoff",   2);
    env4DestBox.addItem ("Position", 3);
    env4DestBox.addItem ("Pitch",    4);
    env4DestBox.addItem ("Density",  5);
    env4DestBox.setLookAndFeel (&bioluminescentLAF);
    env4DestAtt = std::make_unique<ComboAtt> (p.apvts, "env4Dest", env4DestBox);
    addChildComponent (env4DestBox); // hidden until ENV tab 4

    auto switchEnvTab = [this, updateEnv] (int tab)
    {
        envAttackAtt.reset();
        envDecayAtt.reset();
        envSustainAtt.reset();
        envReleaseAtt.reset();
        auto pre = (tab == 0) ? juce::String ("env") : ("env" + juce::String (tab + 1));
        envAttackAtt  = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Attack",  envAttackKnob);
        envDecayAtt   = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Decay",   envDecayKnob);
        envSustainAtt = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Sustain", envSustainKnob);
        envReleaseAtt = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Release", envReleaseKnob);
        currentEnvTab = tab;
        env4DestBox.setVisible (tab == 3);
        updateEnv();
        for (int i = 0; i < 4; ++i)
            envTabBtns[i].setToggleState (i == tab, juce::dontSendNotification);
    };

    auto switchLfoTab = [this, setLfoShape] (int tab)
    {
        lfoShapes[currentLfoTab] = currentLfoShape;
        lfoRateAtt.reset();
        lfoDepthAtt.reset();
        lfoAttackAtt.reset();
        lfoDecayAtt.reset();
        lfoSyncAtt.reset();
        lfoRetriggerAtt.reset();
        lfoPhaseAtt.reset();
        lfoEnabledAtt.reset();
        auto pre = "lfo" + juce::String (tab + 1);
        lfoRateAtt      = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Rate",      lfoRateKnob);
        lfoDepthAtt     = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Depth",     lfoDepthKnob);
        lfoAttackAtt    = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Attack",    lfoAttackKnob);
        lfoDecayAtt     = std::make_unique<SliderAtt> (audioProcessor.apvts, pre + "Decay",     lfoDecayKnob);
        lfoSyncAtt      = std::make_unique<ButtonAtt> (audioProcessor.apvts, pre + "Sync",      lfoSyncBtn);
        lfoRetriggerAtt = std::make_unique<ButtonAtt> (audioProcessor.apvts, pre + "Retrigger", lfoRetriggerBtn);
        lfoPhaseAtt     = std::make_unique<ButtonAtt> (audioProcessor.apvts, pre + "Phase",     lfoPhaseBtn);
        lfoEnabledAtt   = std::make_unique<ButtonAtt> (audioProcessor.apvts, pre + "Enabled",   lfoEnabledBtn);
        currentLfoTab = tab;
        setLfoShape (lfoShapes[tab]);
        lfoDisplay.setRate ((float) lfoRateKnob.getValue());
        for (int i = 0; i < 4; ++i)
            lfoTabBtns[i].setToggleState (i == tab, juce::dontSendNotification);
    };

    for (int i = 0; i < 4; ++i)
    {
        envTabBtns[i].onClick = [switchEnvTab, i] { switchEnvTab (i); };
        lfoTabBtns[i].onClick = [switchLfoTab, i] { switchLfoTab (i); };
    }

    updateDisplay();
    updateWavePreview();
    if (filterCutoffKnob.onValueChange != nullptr)
        filterCutoffKnob.onValueChange();
    if (filterResKnob.onValueChange != nullptr)
        filterResKnob.onValueChange();
    updateEnv();
    lfoDisplay.setRate ((float) lfoRateKnob.getValue());
    lfoDisplay.setShape (currentLfoShape);

    // ── Settings page section headers ──────────────────────────────────
    auto setupSettingsLabel = [this] (juce::Label& lbl, const juce::String& text)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
        lbl.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff));
        lbl.setJustificationType (juce::Justification::centredLeft);
        contentComponent.addAndMakeVisible (lbl);
        lbl.setVisible (false);
    };

    // Left column
    setupSettingsLabel (settAudioPerfLabel,  "AUDIO / PERFORMANCE");
    setupSettingsLabel (settMidiLabel,       "MIDI");
    setupSettingsLabel (settPolyphonyLabel,  "VOICING");
    setupSettingsLabel (settDisplayLabel,    "DISPLAY");

    // Right column
    setupSettingsLabel (settTuningLabel,     "TUNING");
    setupSettingsLabel (settMasterLabel,     "MASTER");
    setupSettingsLabel (settEffectsLabel,    "EFFECTS DEFAULTS");

    // ── Audio / Performance controls ───────────────────────────────────
    auto setupSettingsCombo = [this] (juce::ComboBox& box, juce::Label& lbl, const juce::String& text)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setFont (juce::FontOptions (11.0f));
        lbl.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.6f));
        lbl.setJustificationType (juce::Justification::centredLeft);
        contentComponent.addAndMakeVisible (lbl);
        lbl.setVisible (false);

        box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff071825));
        box.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff00e5ff).withAlpha (0.3f));
        box.setColour (juce::ComboBox::textColourId,       juce::Colour (0xff00e5ff));
        box.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xff00e5ff).withAlpha (0.5f));
        contentComponent.addAndMakeVisible (box);
        box.setVisible (false);
    };

    auto setupSettingsInfoLabel = [this] (juce::Label& lbl, juce::Label& valLbl, const juce::String& text)
    {
        lbl.setText (text, juce::dontSendNotification);
        lbl.setFont (juce::FontOptions (11.0f));
        lbl.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.6f));
        lbl.setJustificationType (juce::Justification::centredLeft);
        contentComponent.addAndMakeVisible (lbl);
        lbl.setVisible (false);

        valLbl.setFont (juce::FontOptions (11.0f));
        valLbl.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff));
        valLbl.setJustificationType (juce::Justification::centredRight);
        contentComponent.addAndMakeVisible (valLbl);
        valLbl.setVisible (false);
    };

    // Oversampling dropdown
    setupSettingsCombo (settOversamplingBox, settOversamplingLabel, "Oversampling");
    settOversamplingBox.addItem ("None", 1);
    settOversamplingBox.addItem ("2x",   2);
    settOversamplingBox.addItem ("4x",   3);
    settOversamplingBox.setSelectedId (1, juce::dontSendNotification);

    // Quality Mode dropdown
    setupSettingsCombo (settQualityBox, settQualityLabel, "Quality Mode");
    settQualityBox.addItem ("Draft",  1);
    settQualityBox.addItem ("Normal", 2);
    settQualityBox.addItem ("High",   3);
    settQualityBox.setSelectedId (2, juce::dontSendNotification);

    // CPU Optimization toggle
    settCpuOptBtn.setClickingTogglesState (true);
    settCpuOptBtn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
    settCpuOptBtn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
    settCpuOptBtn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff00e5ff).withAlpha (0.6f));
    settCpuOptBtn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff00e5ff));
    settCpuOptBtn.setButtonText ("CPU Optimization: OFF");
    settCpuOptBtn.onClick = [this]
    {
        settCpuOptBtn.setButtonText (settCpuOptBtn.getToggleState()
            ? "CPU Optimization: ON" : "CPU Optimization: OFF");
    };
    contentComponent.addAndMakeVisible (settCpuOptBtn);
    settCpuOptBtn.setVisible (false);

    // Buffer Size (read-only info)
    setupSettingsInfoLabel (settBufferSizeLabel, settBufferSizeVal, "Buffer Size");
    settBufferSizeVal.setText (juce::String ((int) audioProcessor.getBlockSize()), juce::dontSendNotification);

    // Sample Rate (read-only info)
    setupSettingsInfoLabel (settSampleRateLabel, settSampleRateVal, "Sample Rate");
    settSampleRateVal.setText (juce::String ((int) audioProcessor.getSampleRate()) + " Hz", juce::dontSendNotification);

    // ── Display controls ───────────────────────────────────────────────
    // Window Size dropdown
    setupSettingsCombo (settWindowSizeBox, settWindowSizeLabel, "Window Size");
    settWindowSizeBox.addItem ("1024 x 800",  1);
    settWindowSizeBox.addItem ("1280 x 800",  2);
    settWindowSizeBox.addItem ("1440 x 900",  3);
    settWindowSizeBox.addItem ("1600 x 1000", 4);
    settWindowSizeBox.setSelectedId (2, juce::dontSendNotification);
    settWindowSizeBox.onChange = [this]
    {
        switch (settWindowSizeBox.getSelectedId())
        {
            case 1: setSize (1024, 800);  break;
            case 2: setSize (1280, 800);  break;
            case 3: setSize (1440, 900);  break;
            case 4: setSize (1600, 1000); break;
            default: break;
        }
    };

    // Display toggle buttons
    auto setupDisplayToggle = [this] (juce::TextButton& btn, const juce::String& name)
    {
        btn.setClickingTogglesState (true);
        btn.setToggleState (true, juce::dontSendNotification);
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff00e5ff));
        btn.onClick = [this, &btn, name]
        {
            btn.setButtonText (name + (btn.getToggleState() ? ": ON" : ": OFF"));
        };
        contentComponent.addAndMakeVisible (btn);
        btn.setVisible (false);
    };

    setupDisplayToggle (settShowKeyboardBtn,  "Show Keyboard");
    settShowKeyboardBtn.setToggleState (false, juce::dontSendNotification);
    settShowKeyboardBtn.setButtonText ("Show Keyboard: OFF");
    keyboard.setVisible (false);
    settShowKeyboardBtn.onClick = [this]
    {
        bool on = settShowKeyboardBtn.getToggleState();
        settShowKeyboardBtn.setButtonText (juce::String ("Show Keyboard") + (on ? ": ON" : ": OFF"));
        keyboard.setVisible (on);

        // Resize the window — shrink/grow from the bottom by keyboard height
        const int kbH = 90;
        int newH = on ? getHeight() + kbH : getHeight() - kbH;
        setSize (getWidth(), newH);
    };

    setupDisplayToggle (settShowWaveformsBtn, "Show Waveforms");
    settShowWaveformsBtn.onClick = [this]
    {
        bool on = settShowWaveformsBtn.getToggleState();
        settShowWaveformsBtn.setButtonText (juce::String ("Show Waveforms") + (on ? ": ON" : ": OFF"));
        grainDisplay.setDrawingEnabled (on);
        filterDisplay.setDrawingEnabled (on);
        envDisplay.setDrawingEnabled (on);
        lfoDisplay.setDrawingEnabled (on);
        wavePreview.setDrawingEnabled (on);
        osc2Preview.setDrawingEnabled (on);
        subOscPreview.setDrawingEnabled (on);
        repaint();
    };

    setupDisplayToggle (settShowOceanBtn, "Show FFT Ocean");
    settShowOceanBtn.onClick = [this]
    {
        bool on = settShowOceanBtn.getToggleState();
        settShowOceanBtn.setButtonText (juce::String ("Show FFT Ocean") + (on ? ": ON" : ": OFF"));
        repaint();
    };

    setupDisplayToggle (settShowLabelsBtn, "Show Labels");
    settShowLabelsBtn.onClick = [this]
    {
        bool on = settShowLabelsBtn.getToggleState();
        settShowLabelsBtn.setButtonText (juce::String ("Show Labels") + (on ? ": ON" : ": OFF"));
        repaint();
    };

    // ── Per-slot MIDI channel selectors ────────────────────────────────
    for (int slot = 0; slot < 4; ++slot)
    {
        auto& box = slotMidiChannelBox[slot];
        auto& lbl = slotMidiChannelLabel[slot];

        lbl.setText (juce::String ("S") + juce::String (slot + 1), juce::dontSendNotification);
        lbl.setJustificationType (juce::Justification::centred);
        lbl.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.7f));
        lbl.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        contentComponent.addAndMakeVisible (lbl);

        box.addItem ("Omni", 1);
        for (int ch = 1; ch <= 16; ++ch)
            box.addItem (juce::String (ch), ch + 1);
        box.setSelectedId (slot + 2, juce::dontSendNotification);  // default: slots use channels 1-4
        box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff071825));
        box.setColour (juce::ComboBox::textColourId, juce::Colour (0xff00e5ff));
        box.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff00b4cc).withAlpha (0.5f));
        contentComponent.addAndMakeVisible (box);
    }

    // Sustain Pedal toggle (on by default)
    auto setupMidiToggle = [this] (juce::TextButton& btn, const juce::String& name, bool defaultOn)
    {
        btn.setClickingTogglesState (true);
        btn.setToggleState (defaultOn, juce::dontSendNotification);
        btn.setButtonText (name + (defaultOn ? ": ON" : ": OFF"));
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff00e5ff));
        btn.onClick = [this, &btn, name]
        {
            btn.setButtonText (name + (btn.getToggleState() ? ": ON" : ": OFF"));
        };
        contentComponent.addAndMakeVisible (btn);
        btn.setVisible (false);
    };

    setupMidiToggle (settSustainPedalBtn,  "Sustain Pedal",  true);
    setupMidiToggle (settLegatoModeBtn,    "Legato Mode",    false);
    setupMidiToggle (settProgramChangeBtn, "Program Change",  true);
    setupMidiToggle (settMidiLearnBtn,     "MIDI CC Learn",   false);

    // ── Polyphony / Voicing controls ───────────────────────────────────
    // Number of Voices
    setupSettingsCombo (settVoiceCountBox, settVoiceCountLabel, "Voices");
    settVoiceCountBox.addItem ("4",  1);
    settVoiceCountBox.addItem ("8",  2);
    settVoiceCountBox.addItem ("16", 3);
    settVoiceCountBox.addItem ("32", 4);
    settVoiceCountBox.setSelectedId (2, juce::dontSendNotification);  // default 8

    // Voice Stealing Mode
    setupSettingsCombo (settVoiceStealBox, settVoiceStealLabel, "Voice Stealing");
    settVoiceStealBox.addItem ("Oldest",  1);
    settVoiceStealBox.addItem ("Newest",  2);
    settVoiceStealBox.addItem ("Lowest",  3);
    settVoiceStealBox.addItem ("Highest", 4);
    settVoiceStealBox.setSelectedId (1, juce::dontSendNotification);  // default Oldest

    // Unison Voices
    setupSettingsCombo (settUnisonVoicesKnob, settUnisonVoicesLabel, "Unison Voices");
    settUnisonVoicesKnob.addItem ("1 (Off)", 1);
    settUnisonVoicesKnob.addItem ("2", 2);
    settUnisonVoicesKnob.addItem ("4", 3);
    settUnisonVoicesKnob.addItem ("8", 4);
    settUnisonVoicesKnob.setSelectedId (1, juce::dontSendNotification);

    // Unison Detune
    setupSettingsCombo (settUnisonDetuneKnob, settUnisonDetuneLabel, "Unison Detune");
    settUnisonDetuneKnob.addItem ("Fine",   1);
    settUnisonDetuneKnob.addItem ("Medium", 2);
    settUnisonDetuneKnob.addItem ("Wide",   3);
    settUnisonDetuneKnob.setSelectedId (1, juce::dontSendNotification);

    // ── Tuning controls ────────────────────────────────────────────────
    // Master Tune (cents)
    setupSettingsCombo (settMasterTuneKnob2, settMasterTuneLabel2, "Master Tune");
    settMasterTuneKnob2.addItem ("-50 cents", 1);
    settMasterTuneKnob2.addItem ("-25 cents", 2);
    settMasterTuneKnob2.addItem ("0 (Default)", 3);
    settMasterTuneKnob2.addItem ("+25 cents", 4);
    settMasterTuneKnob2.addItem ("+50 cents", 5);
    settMasterTuneKnob2.setSelectedId (3, juce::dontSendNotification);

    // Tuning System
    setupSettingsCombo (settTuningSystemBox, settTuningSystemLabel, "Tuning System");
    settTuningSystemBox.addItem ("Equal Temperament", 1);
    settTuningSystemBox.addItem ("Just Intonation",   2);
    settTuningSystemBox.addItem ("Pythagorean",       3);
    settTuningSystemBox.addItem ("Meantone",          4);
    settTuningSystemBox.setSelectedId (1, juce::dontSendNotification);

    // Reference Pitch
    setupSettingsCombo (settRefPitchBox, settRefPitchLabel, "Reference Pitch");
    settRefPitchBox.addItem ("A = 432 Hz", 1);
    settRefPitchBox.addItem ("A = 440 Hz", 2);
    settRefPitchBox.addItem ("A = 442 Hz", 3);
    settRefPitchBox.addItem ("A = 444 Hz", 4);
    settRefPitchBox.setSelectedId (2, juce::dontSendNotification);

    // Transpose
    setupSettingsCombo (settTransposeBox, settTransposeLabel, "Transpose");
    for (int st = -12; st <= 12; ++st)
    {
        juce::String label = (st == 0) ? "0 (Default)" : ((st > 0 ? "+" : "") + juce::String (st) + " st");
        settTransposeBox.addItem (label, st + 13);  // IDs 1-25
    }
    settTransposeBox.setSelectedId (13, juce::dontSendNotification);  // 0 = default

    // Octave Shift
    setupSettingsCombo (settOctaveShiftBox, settOctaveShiftLabel, "Octave Shift");
    settOctaveShiftBox.addItem ("-3", 1);
    settOctaveShiftBox.addItem ("-2", 2);
    settOctaveShiftBox.addItem ("-1", 3);
    settOctaveShiftBox.addItem ("0 (Default)", 4);
    settOctaveShiftBox.addItem ("+1", 5);
    settOctaveShiftBox.addItem ("+2", 6);
    settOctaveShiftBox.addItem ("+3", 7);
    settOctaveShiftBox.setSelectedId (4, juce::dontSendNotification);

    // Scale
    setupSettingsCombo (settScaleBox, settScaleLabel, "Scale");
    settScaleBox.addItem ("Chromatic",        1);
    settScaleBox.addItem ("Major",            2);
    settScaleBox.addItem ("Minor (Natural)",  3);
    settScaleBox.addItem ("Minor (Harmonic)", 4);
    settScaleBox.addItem ("Pentatonic Major", 5);
    settScaleBox.addItem ("Pentatonic Minor", 6);
    settScaleBox.addItem ("Blues",            7);
    settScaleBox.addItem ("Dorian",           8);
    settScaleBox.addItem ("Mixolydian",       9);
    settScaleBox.addItem ("Whole Tone",      10);
    settScaleBox.setSelectedId (1, juce::dontSendNotification);

    // ── Master controls ────────────────────────────────────────────────
    // Master Volume
    setupSettingsCombo (settMasterVolBox, settMasterVolLabel, "Master Volume");
    settMasterVolBox.addItem ("-12 dB", 1);
    settMasterVolBox.addItem ("-6 dB",  2);
    settMasterVolBox.addItem ("0 dB",   3);
    settMasterVolBox.addItem ("+6 dB",  4);
    settMasterVolBox.addItem ("+12 dB", 5);
    settMasterVolBox.setSelectedId (3, juce::dontSendNotification);

    // Limiter
    setupSettingsCombo (settLimiterBox, settLimiterLabel, "Limiting");
    settLimiterBox.addItem ("Off",  1);
    settLimiterBox.addItem ("Soft", 2);
    settLimiterBox.addItem ("Hard", 3);
    settLimiterBox.setSelectedId (2, juce::dontSendNotification);

    // Metering toggle
    {
        auto& btn = settMeteringBtn;
        btn.setClickingTogglesState (true);
        btn.setToggleState (true, juce::dontSendNotification);
        btn.setButtonText ("Metering: ON");
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff00e5ff));
        btn.onClick = [this] { settMeteringBtn.setButtonText (settMeteringBtn.getToggleState() ? "Metering: ON" : "Metering: OFF"); };
        contentComponent.addAndMakeVisible (btn);
        btn.setVisible (false);
    }

    // Output Pan
    setupSettingsCombo (settOutputPanBox, settOutputPanLabel, "Output Pan");
    settOutputPanBox.addItem ("Left",   1);
    settOutputPanBox.addItem ("Center", 2);
    settOutputPanBox.addItem ("Right",  3);
    settOutputPanBox.setSelectedId (2, juce::dontSendNotification);

    // ── Effects Defaults controls ──────────────────────────────────────
    // Dry/Wet Mix
    setupSettingsCombo (settDryWetBox, settDryWetLabel, "Dry/Wet Mix");
    settDryWetBox.addItem ("0% (Dry)",   1);
    settDryWetBox.addItem ("25%",        2);
    settDryWetBox.addItem ("50%",        3);
    settDryWetBox.addItem ("75%",        4);
    settDryWetBox.addItem ("100% (Wet)", 5);
    settDryWetBox.setSelectedId (3, juce::dontSendNotification);

    // Dither
    setupSettingsCombo (settDitherBox, settDitherLabel, "Dither");
    settDitherBox.addItem ("Off",       1);
    settDitherBox.addItem ("Triangle",  2);
    settDitherBox.addItem ("Gaussian",  3);
    settDitherBox.setSelectedId (1, juce::dontSendNotification);

    // Soft Clipping toggle
    auto setupEffectToggle = [this] (juce::TextButton& btn, const juce::String& name, bool defaultOn)
    {
        btn.setClickingTogglesState (true);
        btn.setToggleState (defaultOn, juce::dontSendNotification);
        btn.setButtonText (name + (defaultOn ? ": ON" : ": OFF"));
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff071825));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff00b4cc).withAlpha (0.4f));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff00e5ff).withAlpha (0.6f));
        btn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff00e5ff));
        btn.onClick = [this, &btn, name] { btn.setButtonText (name + (btn.getToggleState() ? ": ON" : ": OFF")); };
        contentComponent.addAndMakeVisible (btn);
        btn.setVisible (false);
    };

    setupEffectToggle (settSoftClipBtn,    "Soft Clipping", false);
    setupEffectToggle (settPhaseInvertBtn,  "Phase Invert",  false);

    showPage (currentPage);
}

OysterAudioProcessorEditor::~OysterAudioProcessorEditor()
{
    stopTimer();

    reverbKnob.setLookAndFeel  (nullptr);
    shimmerKnob.setLookAndFeel (nullptr);
    warmthKnob.setLookAndFeel  (nullptr);
    driftKnob.setLookAndFeel   (nullptr);
    bloomKnob.setLookAndFeel   (nullptr);
    mixKnob.setLookAndFeel     (nullptr);
    positionKnob.setLookAndFeel (nullptr);
    sprayKnob.setLookAndFeel (nullptr);
    grainSizeKnob.setLookAndFeel (nullptr);
    densityKnob.setLookAndFeel (nullptr);
    pitchScatterKnob.setLookAndFeel (nullptr);
    panSpreadKnob.setLookAndFeel (nullptr);
    morphKnob.setLookAndFeel (nullptr);
    filterCutoffKnob.setLookAndFeel (nullptr);
    filterResKnob.setLookAndFeel (nullptr);
    filterDriveKnob.setLookAndFeel (nullptr);
    filterEnvAmtKnob.setLookAndFeel  (nullptr);
    filterLfoAmtKnob.setLookAndFeel  (nullptr);
    filterKeyTrackKnob.setLookAndFeel (nullptr);
    osc2WaveBox.setLookAndFeel   (nullptr);
    subWaveBox.setLookAndFeel    (nullptr);
    env4DestBox.setLookAndFeel   (nullptr);
    osc2OctaveKnob.setLookAndFeel  (nullptr);
    osc2SemiKnob.setLookAndFeel    (nullptr);
    osc2FineKnob.setLookAndFeel    (nullptr);
    osc2PhaseKnob.setLookAndFeel   (nullptr);
    osc2MixKnob.setLookAndFeel     (nullptr);
    osc2DetuneKnob.setLookAndFeel  (nullptr);
    osc2PanKnob.setLookAndFeel     (nullptr);
    subOctaveKnob.setLookAndFeel   (nullptr);
    subMixKnob.setLookAndFeel      (nullptr);
    subTuneKnob.setLookAndFeel     (nullptr);
    subSemiKnob.setLookAndFeel     (nullptr);
    subPanKnob.setLookAndFeel      (nullptr);
    subPhaseKnob.setLookAndFeel    (nullptr);
    masterVolumeKnob.setLookAndFeel    (nullptr);
    masterPanKnob.setLookAndFeel       (nullptr);
    masterTransposeKnob.setLookAndFeel (nullptr);
    masterTuneKnob2.setLookAndFeel     (nullptr);
    glideKnob.setLookAndFeel        (nullptr);
    bendUpKnob.setLookAndFeel       (nullptr);
    bendDownKnob.setLookAndFeel     (nullptr);
    velSensKnob.setLookAndFeel      (nullptr);
    octaveShiftKnob.setLookAndFeel  (nullptr);
    stereoWidthKnob.setLookAndFeel  (nullptr);
    masterTuneKnob.setLookAndFeel   (nullptr);
    lfoToCutoffKnob.setLookAndFeel   (nullptr);
    lfoToPositionKnob.setLookAndFeel (nullptr);
    lfoToPitchKnob.setLookAndFeel    (nullptr);
    lfoToDensityKnob.setLookAndFeel  (nullptr);
    envToCutoffKnob.setLookAndFeel   (nullptr);
    envToPositionKnob.setLookAndFeel (nullptr);
    envToPitchKnob.setLookAndFeel    (nullptr);
    envToAmpKnob.setLookAndFeel      (nullptr);
    reverbSizeKnob.setLookAndFeel (nullptr);
    reverbMixKnob.setLookAndFeel (nullptr);
    chorusMixKnob.setLookAndFeel (nullptr);
    wtOctaveKnob.setLookAndFeel (nullptr);
    wtSemitoneKnob.setLookAndFeel (nullptr);
    wtFineKnob.setLookAndFeel (nullptr);
    wtPhaseKnob.setLookAndFeel (nullptr);
    wtTiltKnob.setLookAndFeel (nullptr);
    unisonVoicesKnob.setLookAndFeel (nullptr);
    unisonDetuneKnob.setLookAndFeel (nullptr);
    unisonSpreadKnob.setLookAndFeel (nullptr);
    envAttackKnob.setLookAndFeel (nullptr);
    envDecayKnob.setLookAndFeel (nullptr);
    envSustainKnob.setLookAndFeel (nullptr);
    envReleaseKnob.setLookAndFeel (nullptr);
    lfoRateKnob.setLookAndFeel (nullptr);
    lfoDepthKnob.setLookAndFeel (nullptr);
    waveABox.setLookAndFeel (nullptr);
    waveBBox.setLookAndFeel (nullptr);
}

//==============================================================================
void OysterAudioProcessorEditor::showPage (int pageIndex)
{
    currentPage = pageIndex;
    if (pageIndex == 0)
        refreshPresetLists();

    mainPageButton.setToggleState     (pageIndex == 0, juce::dontSendNotification);
    synthPageButton.setToggleState    (pageIndex == 1, juce::dontSendNotification);
    matrixPageButton.setToggleState   (pageIndex == 2, juce::dontSendNotification);
    settingsPageButton.setToggleState (pageIndex == 3, juce::dontSendNotification);

    const bool isMainPage = (pageIndex == 0);

    // Preset slots
    for (auto& btn : presetSlotButtons)
        btn.setVisible (isMainPage);

    // Macro knobs and labels
    reverbKnob.setVisible   (isMainPage);
    shimmerKnob.setVisible  (isMainPage);
    warmthKnob.setVisible   (isMainPage);
    driftKnob.setVisible    (isMainPage);
    bloomKnob.setVisible    (isMainPage);
    mixKnob.setVisible      (isMainPage);

    reverbLabel.setVisible  (isMainPage);
    shimmerLabel.setVisible (isMainPage);
    warmthLabel.setVisible  (isMainPage);
    driftLabel.setVisible   (isMainPage);
    bloomLabel.setVisible   (isMainPage);
    mixLabel.setVisible     (isMainPage);

    // Master knobs
    masterVolumeKnob.setVisible    (isMainPage);
    masterPanKnob.setVisible       (isMainPage);
    masterTransposeKnob.setVisible (isMainPage);
    masterTuneKnob2.setVisible     (isMainPage);
    masterVolumeLabel.setVisible   (isMainPage);
    masterPanLabel.setVisible      (isMainPage);
    masterTransposeLabel.setVisible(isMainPage);
    masterTuneLabel2.setVisible    (isMainPage);
    oysterLogo.setVisible          (isMainPage);

    const bool isSynth = (pageIndex == 1);
    for (auto* c : { (juce::Component*) &osc2Preview,
                     (juce::Component*) &subOscPreview,
                     (juce::Component*) &osc2WaveBox,
                     (juce::Component*) &subWaveBox,
                     (juce::Component*) &osc2OctaveKnob,  (juce::Component*) &osc2OctaveLabel,
                     (juce::Component*) &osc2SemiKnob,    (juce::Component*) &osc2SemiLabel,
                     (juce::Component*) &osc2FineKnob,    (juce::Component*) &osc2FineLabel,
                     (juce::Component*) &osc2PhaseKnob,   (juce::Component*) &osc2PhaseLabel,
                     (juce::Component*) &osc2MixKnob,     (juce::Component*) &osc2MixLabel,
                     (juce::Component*) &osc2DetuneKnob,  (juce::Component*) &osc2DetuneLabel,
                     (juce::Component*) &osc2PanKnob,     (juce::Component*) &osc2PanLabel,
                     (juce::Component*) &subOscPowerBtn,
                     (juce::Component*) &osc2PowerBtn,
                     (juce::Component*) &grainPowerBtn,
                     (juce::Component*) &subOctaveKnob,   (juce::Component*) &subOctaveLabel,
                     (juce::Component*) &subMixKnob,      (juce::Component*) &subMixLabel,
                     (juce::Component*) &subTuneKnob,     (juce::Component*) &subTuneLabel,
                     (juce::Component*) &subSemiKnob,     (juce::Component*) &subSemiLabel,
                     (juce::Component*) &subPanKnob,      (juce::Component*) &subPanLabel,
                     (juce::Component*) &subPhaseKnob,    (juce::Component*) &subPhaseLabel,
                     (juce::Component*) &grainDisplay,
                     (juce::Component*) &positionKnob, (juce::Component*) &sprayKnob, (juce::Component*) &grainSizeKnob,
                     (juce::Component*) &densityKnob, (juce::Component*) &pitchScatterKnob, (juce::Component*) &panSpreadKnob,
                     (juce::Component*) &positionLabel, (juce::Component*) &sprayLabel, (juce::Component*) &grainSizeLabel,
                     (juce::Component*) &densityLabel, (juce::Component*) &pitchScatterLabel, (juce::Component*) &panSpreadLabel,
                     (juce::Component*) &wavePreview, (juce::Component*) &waveABox, (juce::Component*) &waveBBox, (juce::Component*) &morphKnob,
                     (juce::Component*) &waveALabel, (juce::Component*) &waveBLabel, (juce::Component*) &morphLabel,
                     (juce::Component*) &wtOctaveKnob, (juce::Component*) &wtOctaveLabel,
                     (juce::Component*) &wtSemitoneKnob, (juce::Component*) &wtSemitoneLabel,
                     (juce::Component*) &wtFineKnob, (juce::Component*) &wtFineLabel,
                     (juce::Component*) &wtPhaseKnob, (juce::Component*) &wtPhaseLabel,
                     (juce::Component*) &wtTiltKnob, (juce::Component*) &wtTiltLabel,
                     (juce::Component*) &unisonVoicesKnob, (juce::Component*) &unisonVoicesLabel,
                     (juce::Component*) &unisonDetuneKnob, (juce::Component*) &unisonDetuneLabel,
                     (juce::Component*) &unisonSpreadKnob, (juce::Component*) &unisonSpreadLabel,
                     (juce::Component*) &pitchSectionLabel, (juce::Component*) &unisonSectionLabel,
                     (juce::Component*) &filterDisplay,
                     (juce::Component*) &filterLPButton, (juce::Component*) &filterHPButton,
                     (juce::Component*) &filterBPButton, (juce::Component*) &filterNTButton,
                     (juce::Component*) &filterCutoffKnob, (juce::Component*) &filterResKnob,
                     (juce::Component*) &filterDriveKnob, (juce::Component*) &filterEnvAmtKnob, (juce::Component*) &filterLfoAmtKnob,
                     (juce::Component*) &filterKeyTrackKnob,
                     (juce::Component*) &filterCutoffLabel, (juce::Component*) &filterResLabel, (juce::Component*) &filterDriveLabel,
                     (juce::Component*) &filterEnvAmtLabel, (juce::Component*) &filterLfoAmtLabel, (juce::Component*) &filterKeyTrackLabel,
                     (juce::Component*) &envDisplay, (juce::Component*) &lfoDisplay,
                     (juce::Component*) &envAttackKnob, (juce::Component*) &envAttackLabel,
                     (juce::Component*) &envDecayKnob, (juce::Component*) &envDecayLabel,
                     (juce::Component*) &envSustainKnob, (juce::Component*) &envSustainLabel,
                     (juce::Component*) &envReleaseKnob, (juce::Component*) &envReleaseLabel,
                     (juce::Component*) &lfoRateKnob, (juce::Component*) &lfoRateLabel,
                     (juce::Component*) &lfoDepthKnob, (juce::Component*) &lfoDepthLabel,
                     (juce::Component*) &lfoSineBtn, (juce::Component*) &lfoTriBtn, (juce::Component*) &lfoSawBtn, (juce::Component*) &lfoSqBtn, (juce::Component*) &lfoSHBtn,
                     (juce::Component*) &lfoAttackKnob, (juce::Component*) &lfoAttackLabel,
                     (juce::Component*) &lfoDecayKnob,  (juce::Component*) &lfoDecayLabel,
                     (juce::Component*) &lfoSyncBtn, (juce::Component*) &lfoRetriggerBtn, (juce::Component*) &lfoPhaseBtn, (juce::Component*) &lfoEnabledBtn,
                     (juce::Component*) &envTabBtns[0], (juce::Component*) &envTabBtns[1], (juce::Component*) &envTabBtns[2], (juce::Component*) &envTabBtns[3],
                     (juce::Component*) &lfoTabBtns[0], (juce::Component*) &lfoTabBtns[1], (juce::Component*) &lfoTabBtns[2], (juce::Component*) &lfoTabBtns[3],
                     (juce::Component*) &chorusMixKnob,
                     (juce::Component*) &chorusMixLabel,
                     (juce::Component*) &glideKnob,        (juce::Component*) &glideLabel,
                     (juce::Component*) &bendUpKnob,       (juce::Component*) &bendUpLabel,
                     (juce::Component*) &bendDownKnob,     (juce::Component*) &bendDownLabel,
                     (juce::Component*) &velSensKnob,      (juce::Component*) &velSensLabel,
                     (juce::Component*) &octaveShiftKnob,  (juce::Component*) &octaveShiftLabel,
                     (juce::Component*) &stereoWidthKnob,  (juce::Component*) &stereoWidthLabel,
                     (juce::Component*) &masterTuneKnob,   (juce::Component*) &masterTuneLabel,
                     (juce::Component*) &lfoToCutoffKnob,   (juce::Component*) &lfoToCutoffLabel,
                     (juce::Component*) &lfoToPositionKnob, (juce::Component*) &lfoToPositionLabel,
                     (juce::Component*) &lfoToPitchKnob,    (juce::Component*) &lfoToPitchLabel,
                     (juce::Component*) &lfoToDensityKnob,  (juce::Component*) &lfoToDensityLabel,
                     (juce::Component*) &envToCutoffKnob,   (juce::Component*) &envToCutoffLabel,
                     (juce::Component*) &envToPositionKnob, (juce::Component*) &envToPositionLabel,
                     (juce::Component*) &envToPitchKnob,    (juce::Component*) &envToPitchLabel,
                     (juce::Component*) &envToAmpKnob,      (juce::Component*) &envToAmpLabel })
        c->setVisible (isSynth);

    env4DestBox.setVisible (isSynth && currentEnvTab == 3);

    // Matrix page
    const bool isMatrix = (pageIndex == 2);
    // (Matrix page visibility controls would go here)

    // Settings page labels + controls
    const bool isSettings = (pageIndex == 3);
    settAudioPerfLabel.setVisible  (isSettings);
    settMidiLabel.setVisible       (isSettings);
    settPolyphonyLabel.setVisible  (isSettings);
    settDisplayLabel.setVisible    (isSettings);
    settTuningLabel.setVisible     (isSettings);
    settMasterLabel.setVisible     (isSettings);
    settEffectsLabel.setVisible    (isSettings);

    // Audio / Performance controls
    settOversamplingBox.setVisible   (isSettings);
    settOversamplingLabel.setVisible (isSettings);
    settQualityBox.setVisible        (isSettings);
    settQualityLabel.setVisible      (isSettings);
    settCpuOptBtn.setVisible         (isSettings);
    settBufferSizeLabel.setVisible   (isSettings);
    settBufferSizeVal.setVisible     (isSettings);
    settSampleRateLabel.setVisible   (isSettings);
    settSampleRateVal.setVisible     (isSettings);

    // Display controls
    settWindowSizeBox.setVisible     (isSettings);
    settWindowSizeLabel.setVisible   (isSettings);
    settShowKeyboardBtn.setVisible   (isSettings);
    settShowWaveformsBtn.setVisible  (isSettings);
    settShowOceanBtn.setVisible      (isSettings);
    settShowLabelsBtn.setVisible     (isSettings);

    // MIDI settings panel controls
    settSustainPedalBtn.setVisible   (isSettings);
    settLegatoModeBtn.setVisible     (isSettings);
    settProgramChangeBtn.setVisible  (isSettings);
    settMidiLearnBtn.setVisible      (isSettings);
    for (int slot = 0; slot < 4; ++slot)
    {
        slotMidiChannelBox[slot].setVisible (isSettings);
        slotMidiChannelLabel[slot].setVisible (isSettings);
    }

    // Polyphony / Voicing controls
    settVoiceCountBox.setVisible     (isSettings);
    settVoiceCountLabel.setVisible   (isSettings);
    settVoiceStealBox.setVisible     (isSettings);
    settVoiceStealLabel.setVisible   (isSettings);
    settUnisonVoicesKnob.setVisible  (isSettings);
    settUnisonVoicesLabel.setVisible (isSettings);
    settUnisonDetuneKnob.setVisible  (isSettings);
    settUnisonDetuneLabel.setVisible (isSettings);

    // Tuning controls
    settMasterTuneKnob2.setVisible   (isSettings);
    settMasterTuneLabel2.setVisible  (isSettings);
    settTuningSystemBox.setVisible   (isSettings);
    settTuningSystemLabel.setVisible (isSettings);
    settRefPitchBox.setVisible       (isSettings);
    settRefPitchLabel.setVisible     (isSettings);
    settTransposeBox.setVisible      (isSettings);
    settTransposeLabel.setVisible    (isSettings);
    settOctaveShiftBox.setVisible    (isSettings);
    settOctaveShiftLabel.setVisible  (isSettings);
    settScaleBox.setVisible          (isSettings);
    settScaleLabel.setVisible        (isSettings);

    // Master controls
    settMasterVolBox.setVisible      (isSettings);
    settMasterVolLabel.setVisible    (isSettings);
    settLimiterBox.setVisible        (isSettings);
    settLimiterLabel.setVisible      (isSettings);
    settMeteringBtn.setVisible       (isSettings);
    settOutputPanBox.setVisible      (isSettings);
    settOutputPanLabel.setVisible    (isSettings);

    // Effects Defaults controls
    settDryWetBox.setVisible         (isSettings);
    settDryWetLabel.setVisible       (isSettings);
    settDitherBox.setVisible         (isSettings);
    settDitherLabel.setVisible       (isSettings);
    settSoftClipBtn.setVisible       (isSettings);
    settPhaseInvertBtn.setVisible    (isSettings);

    // Update info labels when entering settings
    if (isSettings)
    {
        settBufferSizeVal.setText (juce::String ((int) audioProcessor.getBlockSize()), juce::dontSendNotification);
        settSampleRateVal.setText (juce::String ((int) audioProcessor.getSampleRate()) + " Hz", juce::dontSendNotification);
    }

    keyboard.setVisible (settShowKeyboardBtn.getToggleState());
    repaint();
}

//==============================================================================
void OysterAudioProcessorEditor::rebuildAllAttachments()
{
    // ---- Reset all per-slot attachments ----
    // (Global FX attachments — reverbSizeAtt, reverbMixAtt, chorusMixAtt — are NOT reset
    //  because they are shared across all slots.)

    // Grain
    positionAtt.reset();
    sprayAtt.reset();
    grainSizeAtt.reset();
    densityAtt.reset();
    pitchScatterAtt.reset();
    panSpreadAtt.reset();

    // Wavetable
    waveAAtt.reset();
    waveBAtt.reset();
    morphAtt.reset();
    wtOctaveAtt.reset();
    wtSemitoneAtt.reset();
    wtFineAtt.reset();
    wtPhaseAtt.reset();
    wtTiltAtt.reset();
    unisonVoicesAtt.reset();
    unisonDetuneAtt.reset();
    unisonSpreadAtt.reset();

    // Filter
    filterCutoffAtt.reset();
    filterResAtt.reset();
    filterDriveAtt.reset();
    filterEnvAmtAtt.reset();
    filterLfoAmtAtt.reset();
    filterKeyTrackAtt.reset();

    // OSC 2
    osc2WaveAtt.reset();
    osc2OctaveAtt.reset();
    osc2SemiAtt.reset();
    osc2FineAtt.reset();
    osc2PhaseAtt.reset();
    osc2MixAtt.reset();
    osc2DetuneAtt.reset();
    osc2PanAtt.reset();

    // Sub OSC
    subWaveAtt.reset();
    subOctaveAtt.reset();
    subSemiAtt.reset();
    subMixAtt.reset();
    subTuneAtt.reset();
    subPanAtt.reset();
    subPhaseAtt.reset();

    // Enables
    subOscEnabledAtt.reset();
    osc2EnabledAtt.reset();
    grainEnabledAtt.reset();

    // Control
    glideAtt.reset();
    bendUpAtt.reset();
    bendDownAtt.reset();
    velSensAtt.reset();
    octaveShiftAtt.reset();
    stereoWidthAtt.reset();
    masterTuneAtt.reset();

    // Modulation
    lfoToCutoffAtt.reset();
    lfoToPositionAtt.reset();
    lfoToPitchAtt.reset();
    lfoToDensityAtt.reset();
    envToCutoffAtt.reset();
    envToPositionAtt.reset();
    envToPitchAtt.reset();
    envToAmpAtt.reset();

    // Envelope (tab-aware)
    envAttackAtt.reset();
    envDecayAtt.reset();
    envSustainAtt.reset();
    envReleaseAtt.reset();

    // LFO (tab-aware)
    lfoRateAtt.reset();
    lfoDepthAtt.reset();
    lfoAttackAtt.reset();
    lfoDecayAtt.reset();
    lfoSyncAtt.reset();
    lfoRetriggerAtt.reset();
    lfoPhaseAtt.reset();
    lfoEnabledAtt.reset();

    // ENV 4 dest
    env4DestAtt.reset();

    // ---- Recreate all attachments from APVTS (now holding the active slot's values) ----
    auto& apvts = audioProcessor.apvts;

    // Grain
    positionAtt     = std::make_unique<SliderAtt> (apvts, "position",     positionKnob);
    sprayAtt        = std::make_unique<SliderAtt> (apvts, "spray",        sprayKnob);
    grainSizeAtt    = std::make_unique<SliderAtt> (apvts, "grainSize",    grainSizeKnob);
    densityAtt      = std::make_unique<SliderAtt> (apvts, "density",      densityKnob);
    pitchScatterAtt = std::make_unique<SliderAtt> (apvts, "pitchScatter", pitchScatterKnob);
    panSpreadAtt    = std::make_unique<SliderAtt> (apvts, "panSpread",    panSpreadKnob);

    // Wavetable
    waveAAtt        = std::make_unique<ComboAtt>  (apvts, "waveA",        waveABox);
    waveBAtt        = std::make_unique<ComboAtt>  (apvts, "waveB",        waveBBox);
    morphAtt        = std::make_unique<SliderAtt> (apvts, "morphAmount",  morphKnob);
    wtOctaveAtt     = std::make_unique<SliderAtt> (apvts, "wtOctave",     wtOctaveKnob);
    wtSemitoneAtt   = std::make_unique<SliderAtt> (apvts, "wtSemitone",   wtSemitoneKnob);
    wtFineAtt       = std::make_unique<SliderAtt> (apvts, "wtFine",       wtFineKnob);
    wtPhaseAtt      = std::make_unique<SliderAtt> (apvts, "wtPhase",      wtPhaseKnob);
    wtTiltAtt       = std::make_unique<SliderAtt> (apvts, "wtTilt",       wtTiltKnob);
    unisonVoicesAtt = std::make_unique<SliderAtt> (apvts, "unisonVoices", unisonVoicesKnob);
    unisonDetuneAtt = std::make_unique<SliderAtt> (apvts, "unisonDetune", unisonDetuneKnob);
    unisonSpreadAtt = std::make_unique<SliderAtt> (apvts, "unisonSpread", unisonSpreadKnob);

    // Filter
    filterCutoffAtt   = std::make_unique<SliderAtt> (apvts, "filterCutoff",   filterCutoffKnob);
    filterResAtt      = std::make_unique<SliderAtt> (apvts, "filterRes",      filterResKnob);
    filterDriveAtt    = std::make_unique<SliderAtt> (apvts, "filterDrive",    filterDriveKnob);
    filterEnvAmtAtt   = std::make_unique<SliderAtt> (apvts, "filterEnvAmt",   filterEnvAmtKnob);
    filterLfoAmtAtt   = std::make_unique<SliderAtt> (apvts, "filterLfoAmt",   filterLfoAmtKnob);
    filterKeyTrackAtt = std::make_unique<SliderAtt> (apvts, "filterKeyTrack", filterKeyTrackKnob);

    // OSC 2
    osc2WaveAtt   = std::make_unique<ComboAtt>  (apvts, "osc2Wave",   osc2WaveBox);
    osc2OctaveAtt = std::make_unique<SliderAtt> (apvts, "osc2Octave", osc2OctaveKnob);
    osc2SemiAtt   = std::make_unique<SliderAtt> (apvts, "osc2Semi",   osc2SemiKnob);
    osc2FineAtt   = std::make_unique<SliderAtt> (apvts, "osc2Fine",   osc2FineKnob);
    osc2PhaseAtt  = std::make_unique<SliderAtt> (apvts, "osc2Phase",  osc2PhaseKnob);
    osc2MixAtt    = std::make_unique<SliderAtt> (apvts, "osc2Mix",    osc2MixKnob);
    osc2DetuneAtt = std::make_unique<SliderAtt> (apvts, "osc2Detune", osc2DetuneKnob);
    osc2PanAtt    = std::make_unique<SliderAtt> (apvts, "osc2Pan",    osc2PanKnob);

    // Sub OSC
    subWaveAtt   = std::make_unique<ComboAtt>  (apvts, "subWave",   subWaveBox);
    subOctaveAtt = std::make_unique<SliderAtt> (apvts, "subOctave", subOctaveKnob);
    subSemiAtt   = std::make_unique<SliderAtt> (apvts, "subSemi",   subSemiKnob);
    subMixAtt    = std::make_unique<SliderAtt> (apvts, "subMix",    subMixKnob);
    subTuneAtt   = std::make_unique<SliderAtt> (apvts, "subTune",   subTuneKnob);
    subPanAtt    = std::make_unique<SliderAtt> (apvts, "subPan",    subPanKnob);
    subPhaseAtt  = std::make_unique<SliderAtt> (apvts, "subPhase",  subPhaseKnob);

    // Enables
    subOscEnabledAtt = std::make_unique<ButtonAtt> (apvts, "subOscEnabled", subOscPowerBtn);
    osc2EnabledAtt   = std::make_unique<ButtonAtt> (apvts, "osc2Enabled",   osc2PowerBtn);
    grainEnabledAtt  = std::make_unique<ButtonAtt> (apvts, "grainEnabled",  grainPowerBtn);

    // Control
    glideAtt       = std::make_unique<SliderAtt> (apvts, "glide",       glideKnob);
    bendUpAtt      = std::make_unique<SliderAtt> (apvts, "bendUp",      bendUpKnob);
    bendDownAtt    = std::make_unique<SliderAtt> (apvts, "bendDown",    bendDownKnob);
    velSensAtt     = std::make_unique<SliderAtt> (apvts, "velSens",     velSensKnob);
    octaveShiftAtt = std::make_unique<SliderAtt> (apvts, "octaveShift", octaveShiftKnob);
    stereoWidthAtt = std::make_unique<SliderAtt> (apvts, "stereoWidth", stereoWidthKnob);
    masterTuneAtt  = std::make_unique<SliderAtt> (apvts, "masterTune",  masterTuneKnob);

    // Modulation
    lfoToCutoffAtt   = std::make_unique<SliderAtt> (apvts, "lfoToCutoff",   lfoToCutoffKnob);
    lfoToPositionAtt = std::make_unique<SliderAtt> (apvts, "lfoToPosition", lfoToPositionKnob);
    lfoToPitchAtt    = std::make_unique<SliderAtt> (apvts, "lfTooPitch",    lfoToPitchKnob);
    lfoToDensityAtt  = std::make_unique<SliderAtt> (apvts, "lfoToDensity",  lfoToDensityKnob);
    envToCutoffAtt   = std::make_unique<SliderAtt> (apvts, "envToCutoff",   envToCutoffKnob);
    envToPositionAtt = std::make_unique<SliderAtt> (apvts, "envToPosition", envToPositionKnob);
    envToPitchAtt    = std::make_unique<SliderAtt> (apvts, "envToPitch",    envToPitchKnob);
    envToAmpAtt      = std::make_unique<SliderAtt> (apvts, "envToAmp",      envToAmpKnob);

    // Envelope — use current tab index
    {
        auto pre = (currentEnvTab == 0) ? juce::String ("env") : ("env" + juce::String (currentEnvTab + 1));
        envAttackAtt  = std::make_unique<SliderAtt> (apvts, pre + "Attack",  envAttackKnob);
        envDecayAtt   = std::make_unique<SliderAtt> (apvts, pre + "Decay",   envDecayKnob);
        envSustainAtt = std::make_unique<SliderAtt> (apvts, pre + "Sustain", envSustainKnob);
        envReleaseAtt = std::make_unique<SliderAtt> (apvts, pre + "Release", envReleaseKnob);
    }

    // LFO — use current tab index
    {
        auto pre = "lfo" + juce::String (currentLfoTab + 1);
        lfoRateAtt      = std::make_unique<SliderAtt> (apvts, pre + "Rate",      lfoRateKnob);
        lfoDepthAtt     = std::make_unique<SliderAtt> (apvts, pre + "Depth",     lfoDepthKnob);
        lfoAttackAtt    = std::make_unique<SliderAtt> (apvts, pre + "Attack",    lfoAttackKnob);
        lfoDecayAtt     = std::make_unique<SliderAtt> (apvts, pre + "Decay",     lfoDecayKnob);
        lfoSyncAtt      = std::make_unique<ButtonAtt> (apvts, pre + "Sync",      lfoSyncBtn);
        lfoRetriggerAtt = std::make_unique<ButtonAtt> (apvts, pre + "Retrigger", lfoRetriggerBtn);
        lfoPhaseAtt     = std::make_unique<ButtonAtt> (apvts, pre + "Phase",     lfoPhaseBtn);
        lfoEnabledAtt   = std::make_unique<ButtonAtt> (apvts, pre + "Enabled",   lfoEnabledBtn);
    }

    env4DestAtt = std::make_unique<ComboAtt> (apvts, "env4Dest", env4DestBox);
    env4DestBox.setVisible (currentEnvTab == 3);

    // Refresh filter type toggle from APVTS
    if (auto* typeParam = audioProcessor.apvts.getRawParameterValue ("filterType"))
    {
        int ft = (int) typeParam->load();
        currentFilterType = ft;
        filterDisplay.setFilterType (ft);
        filterLPButton.setToggleState (ft == 0, juce::dontSendNotification);
        filterHPButton.setToggleState (ft == 1, juce::dontSendNotification);
        filterBPButton.setToggleState (ft == 2, juce::dontSendNotification);
        filterNTButton.setToggleState (ft == 3, juce::dontSendNotification);
    }

    // Refresh visual displays
    grainDisplay.setPosition ((float) positionKnob.getValue());
    grainDisplay.setSpray ((float) sprayKnob.getValue());
    grainDisplay.setGrainSize ((float) grainSizeKnob.getValue() / 500.0f);
    wavePreview.setWaveformIndex (juce::jlimit (0, 7, waveABox.getSelectedItemIndex()));
    wavePreview.setWaveBIndex (juce::jlimit (0, 7, waveBBox.getSelectedItemIndex()));
    wavePreview.setMorphAmount ((float) morphKnob.getValue());
    envDisplay.setAttack ((float) envAttackKnob.getValue());
    envDisplay.setDecay ((float) envDecayKnob.getValue());
    envDisplay.setSustain ((float) envSustainKnob.getValue());
    envDisplay.setRelease ((float) envReleaseKnob.getValue());
    lfoDisplay.setRate ((float) lfoRateKnob.getValue());

    if (filterCutoffKnob.onValueChange != nullptr)
        filterCutoffKnob.onValueChange();
    if (filterResKnob.onValueChange != nullptr)
        filterResKnob.onValueChange();

    repaint();
}

void OysterAudioProcessorEditor::timerCallback()
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    // Drain samples from processor's lock-free FIFO into our local FFT fifo
    {
        const int ready = audioProcessor.vizFifo.getNumReady();
        if (ready > 0)
        {
            const auto scope = audioProcessor.vizFifo.read (ready);
            for (int i = 0; i < scope.blockSize1; ++i)
                pushSampleToFifo (audioProcessor.vizBuffer[(size_t)(scope.startIndex1 + i)]);
            for (int i = 0; i < scope.blockSize2; ++i)
                pushSampleToFifo (audioProcessor.vizBuffer[(size_t)(scope.startIndex2 + i)]);
        }
    }

    if (nextFFTBlockReady)
    {
        updateFFT();
        nextFFTBlockReady = false;
    }
    else
    {
        for (auto& bar : smoothedBars)
            bar *= 0.92f;
    }

    // ── Compute overall audio energy from smoothed bars ──────────────────
    float rawEnergy = 0.0f;
    for (auto& bar : smoothedBars)
        rawEnergy += bar;
    rawEnergy = juce::jlimit (0.0f, 1.0f, rawEnergy / (float)numBars * 3.0f);

    // Gradual swell: slow attack so the ocean builds up over time,
    // gentle decay so it settles back after sound stops
    if (rawEnergy > waveEnergy)
        waveEnergy = waveEnergy * 0.94f + rawEnergy * 0.06f;   // slow rise
    else
        waveEnergy = waveEnergy * 0.985f + rawEnergy * 0.015f;  // gentle settle

    // ── Advance wave phases (louder = faster) ─────────────────────────────
    const float speedScale = 0.018f + waveEnergy * 0.072f;
    wavePhase1 += speedScale;
    wavePhase2 += speedScale * 1.37f;
    wavePhase3 += speedScale * 0.71f;

    // Wrap to avoid float drift
    const float twoPi = juce::MathConstants<float>::twoPi;
    if (wavePhase1 > twoPi * 100.0f) wavePhase1 -= twoPi * 100.0f;
    if (wavePhase2 > twoPi * 100.0f) wavePhase2 -= twoPi * 100.0f;
    if (wavePhase3 > twoPi * 100.0f) wavePhase3 -= twoPi * 100.0f;

    // ── Spawn and update foam particles ───────────────────────────────────
    const int dispX = 290;
    const int dispW = juce::jmax (1, getWidth() - 290 - 30);
    const float horizon = mainDispY + (float) mainDispH * (0.88f - waveEnergy * 0.40f);

    // Rain/foam always on — more when louder
    const int maxParticles = 100;
    if ((int)foamParticles.size() < maxParticles)
    {
        const int spawnCount = 1 + (int)(waveEnergy * 5.0f);
        for (int i = 0; i < spawnCount && (int)foamParticles.size() < maxParticles; ++i)
        {
            FoamParticle p;
            p.x    = (float)dispX + (float)(std::rand() % dispW);
            p.y    = horizon - 10.0f + (float)(std::rand() % 20) - 10.0f;
            p.life = 0.6f + ((float)(std::rand() % 40)) / 100.0f;
            p.size = 1.5f + ((float)(std::rand() % 30)) / 10.0f;
            foamParticles.push_back (p);
        }
    }

    // Age particles and remove dead ones
    for (auto& p : foamParticles)
    {
        p.life -= 0.018f;
        p.y    -= 0.4f + waveEnergy * 1.2f;  // drift upward with energy
        p.x    += (float)(std::rand() % 3) - 1.0f;
    }
    foamParticles.erase (
        std::remove_if (foamParticles.begin(), foamParticles.end(),
                        [] (const FoamParticle& p) { return p.life <= 0.0f; }),
        foamParticles.end());

    repaint();
}

void OysterAudioProcessorEditor::pushSampleToFifo (float sample) noexcept
{
    if (fifoIndex == fftSize)
    {
        if (! nextFFTBlockReady)
        {
            std::copy (audioFifo.begin(), audioFifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    audioFifo[(size_t) fifoIndex++] = sample;
}

void OysterAudioProcessorEditor::updateFFT()
{
    // Apply Hann window
    juce::dsp::WindowingFunction<float> window (fftSize,
        juce::dsp::WindowingFunction<float>::hann);
    window.multiplyWithWindowingTable (fftData.data(), fftSize);

    forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());

    // Map FFT bins to numBars bars using logarithmic frequency scaling
    const int nyquist = fftSize / 2;
    for (int bar = 0; bar < numBars; ++bar)
    {
        // Log scale: low bars cover more bins, high bars cover fewer
        const float startRatio = std::pow ((float) bar       / numBars, 2.0f);
        const float endRatio   = std::pow ((float)(bar + 1)  / numBars, 2.0f);

        const int startBin = juce::jlimit (0, nyquist - 1,
                             (int)(startRatio * nyquist));
        const int endBin   = juce::jlimit (0, nyquist - 1,
                             (int)(endRatio   * nyquist));

        float peak = 0.0f;
        for (int bin = startBin; bin <= endBin; ++bin)
            peak = juce::jmax (peak, fftData[(size_t) bin]);

        // Convert to dB, normalize to 0..1
        const float dB        = juce::Decibels::gainToDecibels (peak, -100.0f);
        const float normalized = juce::jlimit (0.0f, 1.0f,
                                 (dB + 100.0f) / 100.0f);

        // Smooth: fast attack, slow decay
        if (normalized > smoothedBars[bar])
            smoothedBars[bar] = normalized;
        else
            smoothedBars[bar] = smoothedBars[bar] * 0.85f + normalized * 0.15f;
    }
}

void OysterAudioProcessorEditor::drawOceanDisplay (juce::Graphics& g)
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const int dispX = 290;
    const int dispY = mainDispY;
    const int dispW = juce::jmax (1, getWidth() - 290 - 30);
    const int dispH = juce::jmax (1, mainDispH);

    const juce::Rectangle<float> db (
        (float)dispX, (float)dispY, (float)dispW, (float)dispH);

    // ── Deep ocean background gradient ────────────────────────────────────
    juce::ColourGradient bgGrad (
        juce::Colour (0xff010b14),  db.getCentreX(), db.getY(),
        juce::Colour (0xff021825),  db.getCentreX(), db.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (db, 16.0f);

    // border glow
    g.setColour (juce::Colour (0xff005f80).withAlpha (0.45f));
    g.drawRoundedRectangle (db.reduced (0.5f), 16.0f, 1.5f);

    if (! settShowOceanBtn.getToggleState()) return;

    // clip everything inside the rounded rect
    g.saveState();
    juce::Path clipPath;
    clipPath.addRoundedRectangle (db, 16.0f);
    g.reduceClipRegion (clipPath);

    const float W  = db.getWidth();
    const float H  = db.getHeight();
    const float ox = db.getX();
    const float oy = db.getY();

    // ── Underwater depth shimmer lines ────────────────────────────────────
    {
        const int numRays = 12;
        for (int i = 0; i < numRays; ++i)
        {
            const float t    = (float)i / numRays;
            const float xPos = ox + (t + std::fmod (wavePhase1 * 0.04f, 1.0f / numRays)) * W;
            const float alpha = 0.025f + waveEnergy * 0.035f;
            g.setColour (juce::Colour (0xff00c8e8).withAlpha (alpha));
            g.drawLine (xPos, oy, xPos + H * 0.3f, oy + H, 1.2f);
        }
    }

    // ── Wave layers ───────────────────────────────────────────────────────
    // Energy drives amplitude: calm = gentle ripples, loud = stormy crests
    const float baseAmp  = H * 0.03f;
    const float extraAmp = H * 0.22f * waveEnergy;
    const float totalAmp = baseAmp + extraAmp;

    // Horizon RISES with volume: silent = near bottom (88%), loud = mid (48%)
    const float horizon = oy + H * (0.88f - waveEnergy * 0.40f);

    // Wave layer descriptors: { phase ref, speed mult, amplitude mult, y-offset, colour }
    struct WaveLayer
    {
        float phase;
        float freqMult;
        float ampMult;
        float yOffset;
        juce::Colour colour;
    };

    const WaveLayer layers[] = {
        { wavePhase1, 1.0f,  1.0f,   0.0f,  juce::Colour (0xff006994) },   // deep mid wave
        { wavePhase2, 1.55f, 0.65f, -H*0.05f, juce::Colour (0xff0099bb) },  // surface wave
        { wavePhase3, 2.3f,  0.38f, -H*0.10f, juce::Colour (0xff00c8d8) },  // foam/crest layer
    };

    for (int li = (int)(std::size (layers)) - 1; li >= 0; --li)
    {
        const auto& lyr = layers[li];
        const float amp = totalAmp * lyr.ampMult;
        const int   steps = (int)W + 2;

        juce::Path wave;
        for (int i = 0; i <= steps; ++i)
        {
            const float nx   = (float)i / (float)steps;
            const float ph   = lyr.phase + nx * juce::MathConstants<float>::twoPi * lyr.freqMult;

            // Base swell — gentle, always present
            float y = std::sin (ph)                    * amp
                    + std::sin (ph * 0.43f - 0.3f)    * amp * 0.45f;

            // Storm harmonics — only kick in at higher sustained energy (cubed ramp)
            const float storm = waveEnergy * waveEnergy * waveEnergy;
            y += std::sin (ph * 2.7f + 1.1f) * amp * 0.5f  * storm;
            y += std::sin (ph * 4.3f - 0.7f) * amp * 0.28f * storm;
            y += std::sin (ph * 7.1f + 2.4f) * amp * 0.18f * storm;

            // Per-bar audio reactivity: warp the wave shape by the FFT data
            const int barIdx = juce::jlimit (0, numBars - 1, (int)(nx * numBars));
            y += smoothedBars[barIdx] * H * 0.14f * lyr.ampMult;

            const float px = ox + nx * W;
            const float py = horizon + lyr.yOffset + y;

            if (i == 0)
                wave.startNewSubPath (px, py);
            else
                wave.lineTo (px, py);
        }

        // Fill below each wave (ocean body)
        juce::Path filled = wave;
        filled.lineTo (ox + W, oy + H);
        filled.lineTo (ox,     oy + H);
        filled.closeSubPath();

        const float fillAlpha = (li == 0) ? 0.55f : (li == 1 ? 0.35f : 0.20f);
        juce::ColourGradient fillGrad (
            lyr.colour.withAlpha (fillAlpha),      ox, horizon,
            lyr.colour.darker(0.6f).withAlpha (0.0f), ox, oy + H, false);
        g.setGradientFill (fillGrad);
        g.fillPath (filled);

        // Wave crest line
        const float lineAlpha = 0.55f + waveEnergy * 0.35f;
        g.setColour (lyr.colour.brighter (0.3f).withAlpha (lineAlpha));
        g.strokePath (wave, juce::PathStrokeType (
            1.2f + (float)li * 0.5f + waveEnergy * 1.5f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));
    }

    // ── Foam/spray particles ──────────────────────────────────────────────
    for (auto& p : foamParticles)
    {
        const float alpha = p.life * (0.4f + waveEnergy * 0.4f);
        g.setColour (juce::Colour (0xffcceeff).withAlpha (alpha));
        g.fillEllipse (p.x - p.size * 0.5f, p.y - p.size * 0.5f, p.size, p.size);
    }

    // ── Bioluminescent surface glow ───────────────────────────────────────
    if (waveEnergy > 0.05f)
    {
        const float glowAlpha = waveEnergy * 0.18f;
        const float glowReach = std::min (totalAmp, H * 0.10f);  // cap upward reach
        juce::ColourGradient glowGrad (
            juce::Colour (0xff00e8ff).withAlpha (glowAlpha),
            ox, horizon - glowReach,
            juce::Colours::transparentBlack,
            ox, horizon + glowReach * 0.5f, false);
        g.setGradientFill (glowGrad);
        g.fillRect (ox, horizon - glowReach - 4.0f, W, glowReach * 2.0f + 8.0f);
    }

    g.restoreState();

    // ── Label ─────────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff00c8d8).withAlpha (0.45f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("OCEAN", dispX + 14, dispY + 8, 60, 14,
                juce::Justification::centredLeft);
}

void OysterAudioProcessorEditor::drawPresetSlots (juce::Graphics& g)
{
    const int mainPadL = 290;
    const int mainPadR = 30;
    const int slotH  = 44;
    const int slotAreaW = juce::jmax (1, getWidth() - mainPadL - mainPadR);
    const int slotGap   = 16;
    const int slotW  = juce::jmax (1, (slotAreaW - (slotGap * (numPresetSlots - 1)))
                       / numPresetSlots);

    for (int i = 0; i < numPresetSlots; ++i)
    {
        const int x = mainPadL + i * (slotW + slotGap);

        // Preset name label ABOVE the button
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (audioProcessor.presetSlotNames[i],
                    x, slotAreaY - 18, slotW, 16,
                    juce::Justification::centred);

        // Glow behind slot
        g.setColour (juce::Colour (0xffb0b8c1).withAlpha (0.08f));
        g.fillRoundedRectangle ((float)(x - 4), (float)(slotAreaY - 4),
                                (float)(slotW + 8), (float)(slotH + 8),
                                6.0f);
    }
}

void OysterAudioProcessorEditor::drawSynthPage (juce::Graphics& g)
{
    const int padX    = 200;
    const int padY    = 52;
    const int spacing = 10;
    const int totalW  = getWidth() - padX - 20;
    const int totalH  = getHeight() - padY - 10;

    auto drawPanel = [&] (juce::Rectangle<int> bounds, const juce::String& title)
    {
        const auto bf = bounds.toFloat();

        g.setColour (juce::Colours::black.withAlpha (0.38f));
        g.fillRoundedRectangle (bf, 10.0f);

        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.25f));
        g.drawRoundedRectangle (bf, 10.0f, 1.0f);

        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.7f));
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (title,
                    bounds.getX() + 12,
                    bounds.getY() + 8,
                    200, 16,
                    juce::Justification::centredLeft);

        g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.2f));
        g.drawLine ((float)(bounds.getX() + 12),
                    (float)(bounds.getY() + 26),
                    (float)(bounds.getRight() - 12),
                    (float)(bounds.getY() + 26),
                    0.8f);
    };

    // ── Osc column (far left) ────────────────────────────────
    const int leftColX   = 8;
    const int leftColW   = 182;
    const int subPanelH  = 306;   // synced with resized() — was 200
    const int osc2PanelY = padY + subPanelH + spacing;
    const int osc2PanelH = totalH - subPanelH - spacing;

    juce::Rectangle<int> subOscPanel  (leftColX, padY,       leftColW, subPanelH);
    juce::Rectangle<int> osc2Panel    (leftColX, osc2PanelY, leftColW, osc2PanelH);

    drawPanel (subOscPanel, "SUB OSC");
    drawPanel (osc2Panel,   "OSC 2");

    // ── Left column ─────────────────────────────────────────────────────────

    const int grainW      = (int)(totalW * 0.63f) - spacing / 2;
    const int waveW       = totalW - grainW - spacing;
    const int grainPanelH = 32 + 60 + 12 + 58 + 14 + 16;

    juce::Rectangle<int> grainPanel (padX, padY, grainW, grainPanelH);
    drawPanel (grainPanel, "GRAIN ENGINE");

    const int grainActualBottom = padY + grainPanelH;
    const int wtStartY          = grainActualBottom + spacing;
    juce::Rectangle<int> wavetablePanel (padX, wtStartY, grainW, wtPanelH);
    drawPanel (wavetablePanel, "WAVETABLE SOURCE");

    const int bottomRowY = wtStartY + wtPanelH + spacing;
    const int paintFullH = keyboard.isVisible() ? getHeight() : getHeight() + 90;
    const int bottomRowH = paintFullH - bottomRowY - 10;
    const int halfW      = (grainW - spacing) / 2;
    juce::Rectangle<int> envPanel (padX, bottomRowY, halfW, bottomRowH);
    juce::Rectangle<int> lfoPanel (padX + halfW + spacing, bottomRowY, halfW, bottomRowH);
    drawPanel (envPanel, "ENVELOPE");
    drawPanel (lfoPanel, "LFO");

    // ── Right column ─────────────────────────────────────────────────────────

    const int rightColX = padX + grainW + spacing;

    // Filter — tight height that wraps the knobs
    const int fltH = (filterPanelH > 0) ? filterPanelH : (int)(totalH * 0.55f);
    juce::Rectangle<int> filterPanel (rightColX, padY, waveW, fltH);
    drawPanel (filterPanel, "FILTER");

    // MODULATION + CONTROL stacked vertically, with modulation panel tightened
    // to the actual knob content so control can move up.
    const int stackTopY    = padY + fltH + spacing;
    const int stackTotalH  = totalH - fltH - spacing;
    const int defaultModPanelH = (stackTotalH - spacing) / 2;
    int modPanelH = defaultModPanelH;

    if (envToAmpLabel.getBottom() > stackTopY)
    {
        const int contentBottom = envToAmpLabel.getBottom() + 16; // keep breathing room below bottom row
        modPanelH = juce::jlimit (120, stackTotalH - spacing - 90, contentBottom - stackTopY);
    }

    const int controlY = stackTopY + modPanelH + spacing;
    const int controlH = stackTopY + stackTotalH - controlY;

    juce::Rectangle<int> modPanel (rightColX, stackTopY, waveW, modPanelH);
    juce::Rectangle<int> controlPanel (rightColX, controlY, waveW, controlH);
    drawPanel (modPanel,     "MODULATION");
    drawPanel (controlPanel, "CONTROL");

    // ── Wavetable section separator line ─────────────────────────────────────
    const float lineY  = (float)(wtStartY + 207);
    const float lineX1 = (float)(padX + 14);
    const float lineX2 = (float)(padX + grainW - 14);
    g.setColour (juce::Colour (0xff00b4cc).withAlpha (0.15f));
    g.drawLine (lineX1, lineY, lineX2, lineY, 0.8f);
}

void OysterAudioProcessorEditor::setupMacroKnob (juce::Slider& knob,
                                                   juce::Label& label,
                                                   const juce::String& name)
{
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRange (0.0, 1.0, 0.001);
    knob.setValue (0.5);
    knob.setLookAndFeel (&bioluminescentLAF);

    contentComponent.addAndMakeVisible (knob);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.0f));
    label.setColour (juce::Label::textColourId,
                     juce::Colour (0xff00e5ff).withAlpha (0.8f));

    contentComponent.addAndMakeVisible (label);
}

void OysterAudioProcessorEditor::setupSynthKnob (juce::Slider& knob,
                                                 juce::Label& label,
                                                 const juce::String& name)
{
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    knob.setLookAndFeel (&bioluminescentLAF);
    knob.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff00e5ff).withAlpha (0.7f));
    knob.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    knob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    contentComponent.addAndMakeVisible (knob);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (9.5f));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff00e5ff).withAlpha (0.65f));
    contentComponent.addAndMakeVisible (label);
}

void OysterAudioProcessorEditor::layoutKnobRow (
    juce::Rectangle<int> area,
    std::vector<std::pair<juce::Slider*, juce::Label*>> knobs,
    int knobSize)
{
    const int n = (int) knobs.size();
    const int labelH = 14;
    const int totalW = area.getWidth();
    const int spacing = (n > 1) ? (totalW - n * knobSize) / (n - 1) : 0;
    const int knobY = area.getCentreY() - (knobSize + labelH) / 2;

    for (int i = 0; i < n; ++i)
    {
        const int x = area.getX() + i * (knobSize + spacing);
        knobs[(size_t) i].first->setBounds (x, knobY, knobSize, knobSize);
        knobs[(size_t) i].second->setBounds (x, knobY + knobSize + 2, knobSize, labelH);
    }
}

void OysterAudioProcessorEditor::paintNavBar (juce::Graphics&)
{
}

void OysterAudioProcessorEditor::paintMainPage (juce::Graphics&)
{
}

void OysterAudioProcessorEditor::paintSynthPage (juce::Graphics&)
{
}

void OysterAudioProcessorEditor::paintMatrixPage (juce::Graphics& g)
{
    const int navH = 36;
    const int pageY = navH + 10;
    const int pageW = getWidth();
    const int pageH = getHeight() - navH - 10;
    const auto cyan      = juce::Colour (0xff00e5ff);
    const auto panelBg   = juce::Colour (0xff0a1e2e);
    const auto cellBg    = juce::Colour (0xff071825);
    const auto cellBorder = cyan.withAlpha (0.15f);

    auto& apvts = audioProcessor.apvts;

    // ── Title ──
    g.setColour (cyan.withAlpha (0.7f));
    g.setFont (juce::FontOptions (18.0f));
    g.drawText ("MODULATION MATRIX", 30, pageY, 400, 28, juce::Justification::centredLeft);

    g.setColour (cyan.withAlpha (0.3f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Adjust modulation amounts on the Synth page", 30, pageY + 26, 400, 18,
                juce::Justification::centredLeft);

    // ── Matrix Grid ──
    const int gridX = 30;
    const int gridY = pageY + 58;
    const int srcLabelW = 120;
    const int cellW = (pageW - 60 - srcLabelW) / matNumDests;
    const int cellH = 120;
    const int headerH = 40;

    const juce::StringArray destNames { "CUTOFF", "POSITION", "PITCH", "DENSITY", "AMP" };
    const juce::StringArray srcNames  { "LFO", "ENVELOPE" };

    // Parameter IDs for each cell: [source][dest]
    const juce::String paramIds[2][5] = {
        { "lfoToCutoff", "lfoToPosition", "lfTooPitch", "lfoToDensity", "" },
        { "envToCutoff", "envToPosition", "envToPitch", "",             "envToAmp" }
    };

    // Grid background panel
    int gridTotalW = srcLabelW + cellW * matNumDests;
    int gridTotalH = headerH + cellH * matNumSources;
    g.setColour (panelBg);
    g.fillRoundedRectangle ((float) gridX - 4, (float) gridY - 4,
                            (float) gridTotalW + 8, (float) gridTotalH + 8, 6.0f);
    g.setColour (cyan.withAlpha (0.2f));
    g.drawRoundedRectangle ((float) gridX - 4, (float) gridY - 4,
                            (float) gridTotalW + 8, (float) gridTotalH + 8, 6.0f, 1.0f);

    // Header row — destination names
    g.setFont (juce::FontOptions (13.0f));
    for (int d = 0; d < matNumDests; ++d)
    {
        int hx = gridX + srcLabelW + d * cellW;
        g.setColour (cyan.withAlpha (0.5f));
        g.drawText (destNames[d], hx, gridY, cellW, headerH, juce::Justification::centred);
        // Column divider
        g.setColour (cellBorder);
        g.drawLine ((float) hx, (float) gridY, (float) hx, (float)(gridY + gridTotalH), 0.5f);
    }

    // Header bottom line
    g.setColour (cyan.withAlpha (0.25f));
    g.drawLine ((float) gridX, (float)(gridY + headerH),
                (float)(gridX + gridTotalW), (float)(gridY + headerH), 1.0f);

    // Rows — source labels + cells
    for (int s = 0; s < matNumSources; ++s)
    {
        int ry = gridY + headerH + s * cellH;

        // Source label
        g.setColour (cyan.withAlpha (0.6f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (srcNames[s], gridX, ry, srcLabelW - 12, cellH, juce::Justification::centredRight);

        // Row divider
        if (s > 0)
        {
            g.setColour (cellBorder);
            g.drawLine ((float)(gridX + srcLabelW), (float) ry,
                        (float)(gridX + gridTotalW), (float) ry, 0.5f);
        }

        // Cells
        for (int d = 0; d < matNumDests; ++d)
        {
            int cx = gridX + srcLabelW + d * cellW;
            auto cellRect = juce::Rectangle<float> ((float) cx + 4, (float) ry + 4,
                                                     (float) cellW - 8, (float) cellH - 8);

            if (matCellValid[s][d])
            {
                // Read current value from APVTS
                float val = 0.0f;
                if (auto* param = apvts.getRawParameterValue (paramIds[s][d]))
                    val = param->load();

                // Cell background — brighter when active
                float intensity = val;
                g.setColour (cellBg.interpolatedWith (cyan, intensity * 0.15f));
                g.fillRoundedRectangle (cellRect, 4.0f);

                // Filled bar showing amount
                {
                    auto barRect = cellRect.reduced (12.0f, 0.0f);
                    barRect.setTop (cellRect.getBottom() - 28.0f);
                    barRect.setBottom (cellRect.getBottom() - 12.0f);

                    // Track background
                    g.setColour (juce::Colours::black.withAlpha (0.4f));
                    g.fillRoundedRectangle (barRect, 4.0f);

                    if (val > 0.001f)
                    {
                        // Filled portion
                        auto fillRect = barRect;
                        fillRect.setWidth (barRect.getWidth() * val);
                        g.setColour (cyan.withAlpha (0.5f + val * 0.4f));
                        g.fillRoundedRectangle (fillRect, 4.0f);
                    }
                }

                // Percentage text
                int pct = juce::roundToInt (val * 100.0f);
                g.setColour (cyan.withAlpha (val > 0.001f ? 0.9f : 0.25f));
                g.setFont (juce::FontOptions (22.0f));
                g.drawText (juce::String (pct) + "%", cellRect.withTrimmedBottom (30),
                            juce::Justification::centred);

                // Active dot indicator
                if (val > 0.001f)
                {
                    g.setColour (cyan.withAlpha (0.8f));
                    g.fillEllipse (cellRect.getRight() - 14, cellRect.getY() + 6, 7, 7);
                }

                // Cell border
                g.setColour (cyan.withAlpha (val > 0.001f ? 0.3f : 0.1f));
                g.drawRoundedRectangle (cellRect, 4.0f, 1.0f);
            }
            else
            {
                // Inactive cell — no routing available
                g.setColour (juce::Colours::black.withAlpha (0.2f));
                g.fillRoundedRectangle (cellRect, 4.0f);
                g.setColour (juce::Colours::white.withAlpha (0.08f));
                g.drawText ("--", cellRect, juce::Justification::centred);
                g.setColour (cellBorder.withAlpha (0.06f));
                g.drawRoundedRectangle (cellRect, 4.0f, 0.5f);
            }
        }
    }

    // ── LFO Summary Panel ──
    const int summaryY = gridY + gridTotalH + 24;
    const int panelW   = (pageW - 80) / 2;
    const int panelH   = 150;

    // LFO panel
    {
        auto lfoRect = juce::Rectangle<float> (30.0f, (float) summaryY, (float) panelW, (float) panelH);
        g.setColour (panelBg);
        g.fillRoundedRectangle (lfoRect, 6.0f);
        g.setColour (cyan.withAlpha (0.2f));
        g.drawRoundedRectangle (lfoRect, 6.0f, 1.0f);

        g.setColour (cyan.withAlpha (0.6f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("LFO STATUS", (int) lfoRect.getX() + 16, (int) lfoRect.getY() + 12, 160, 20,
                    juce::Justification::centredLeft);

        // Read LFO params
        float lfoRate  = apvts.getRawParameterValue ("lfo1Rate")  ? apvts.getRawParameterValue ("lfo1Rate")->load()  : 0.0f;
        float lfoDepth = apvts.getRawParameterValue ("lfo1Depth") ? apvts.getRawParameterValue ("lfo1Depth")->load() : 0.0f;

        g.setColour (cyan.withAlpha (0.4f));
        g.setFont (juce::FontOptions (13.0f));
        int infoY = (int) lfoRect.getY() + 44;
        int infoX = (int) lfoRect.getX() + 20;

        g.drawText ("Rate:",  infoX, infoY,      70, 20, juce::Justification::centredLeft);
        g.drawText ("Depth:", infoX, infoY + 26,  70, 20, juce::Justification::centredLeft);

        g.setColour (cyan.withAlpha (0.8f));
        g.drawText (juce::String (lfoRate, 2) + " Hz",  infoX + 70, infoY,      120, 20, juce::Justification::centredLeft);
        g.drawText (juce::String (juce::roundToInt (lfoDepth * 100)) + "%", infoX + 70, infoY + 26, 120, 20, juce::Justification::centredLeft);

        // Active routings summary
        int activeCount = 0;
        for (int d = 0; d < matNumDests; ++d)
            if (matCellValid[0][d] && apvts.getRawParameterValue (paramIds[0][d])
                && apvts.getRawParameterValue (paramIds[0][d])->load() > 0.001f)
                ++activeCount;

        g.setColour (cyan.withAlpha (0.4f));
        g.drawText ("Active routes:", infoX, infoY + 58, 120, 20, juce::Justification::centredLeft);
        g.setColour (activeCount > 0 ? cyan.withAlpha (0.9f) : cyan.withAlpha (0.3f));
        g.drawText (juce::String (activeCount) + " / 4", infoX + 120, infoY + 58, 60, 20,
                    juce::Justification::centredLeft);
    }

    // ENV panel
    {
        auto envRect = juce::Rectangle<float> (30.0f + panelW + 20, (float) summaryY, (float) panelW, (float) panelH);
        g.setColour (panelBg);
        g.fillRoundedRectangle (envRect, 6.0f);
        g.setColour (cyan.withAlpha (0.2f));
        g.drawRoundedRectangle (envRect, 6.0f, 1.0f);

        g.setColour (cyan.withAlpha (0.6f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("ENVELOPE STATUS", (int) envRect.getX() + 16, (int) envRect.getY() + 12, 180, 20,
                    juce::Justification::centredLeft);

        // Read ENV params
        float envA = apvts.getRawParameterValue ("envAttack")  ? apvts.getRawParameterValue ("envAttack")->load()  : 0.0f;
        float envD = apvts.getRawParameterValue ("envDecay")   ? apvts.getRawParameterValue ("envDecay")->load()   : 0.0f;
        float envS = apvts.getRawParameterValue ("envSustain") ? apvts.getRawParameterValue ("envSustain")->load() : 0.0f;
        float envR = apvts.getRawParameterValue ("envRelease") ? apvts.getRawParameterValue ("envRelease")->load() : 0.0f;

        g.setColour (cyan.withAlpha (0.4f));
        g.setFont (juce::FontOptions (13.0f));
        int infoY = (int) envRect.getY() + 44;
        int infoX = (int) envRect.getX() + 20;

        int colSpacing = juce::jmax (100, panelW / 4 - 20);
        g.drawText ("A:", infoX,                    infoY, 24, 20, juce::Justification::centredLeft);
        g.drawText ("D:", infoX + colSpacing,       infoY, 24, 20, juce::Justification::centredLeft);
        g.drawText ("S:", infoX + colSpacing * 2,   infoY, 24, 20, juce::Justification::centredLeft);
        g.drawText ("R:", infoX + colSpacing * 3,   infoY, 24, 20, juce::Justification::centredLeft);

        g.setColour (cyan.withAlpha (0.8f));
        g.drawText (juce::String (envA, 2) + "s", infoX + 20,                infoY, 80, 20, juce::Justification::centredLeft);
        g.drawText (juce::String (envD, 2) + "s", infoX + colSpacing + 20,   infoY, 80, 20, juce::Justification::centredLeft);
        g.drawText (juce::String (juce::roundToInt (envS * 100)) + "%", infoX + colSpacing * 2 + 20, infoY, 80, 20, juce::Justification::centredLeft);
        g.drawText (juce::String (envR, 2) + "s", infoX + colSpacing * 3 + 20, infoY, 80, 20, juce::Justification::centredLeft);

        // Active routings summary
        int activeCount = 0;
        for (int d = 0; d < matNumDests; ++d)
            if (matCellValid[1][d] && apvts.getRawParameterValue (paramIds[1][d])
                && apvts.getRawParameterValue (paramIds[1][d])->load() > 0.001f)
                ++activeCount;

        g.setColour (cyan.withAlpha (0.4f));
        g.drawText ("Active routes:", infoX, infoY + 30, 120, 20, juce::Justification::centredLeft);
        g.setColour (activeCount > 0 ? cyan.withAlpha (0.9f) : cyan.withAlpha (0.3f));
        g.drawText (juce::String (activeCount) + " / 4", infoX + 120, infoY + 30, 60, 20,
                    juce::Justification::centredLeft);
    }

    // ── Signal Flow Legend ──
    const int legendY = summaryY + panelH + 20;
    g.setFont (juce::FontOptions (11.0f));

    // Active indicator
    g.setColour (cyan.withAlpha (0.8f));
    g.fillEllipse (34, (float) legendY + 3, 7, 7);
    g.setColour (cyan.withAlpha (0.4f));
    g.drawText ("Active routing", 48, legendY, 120, 16, juce::Justification::centredLeft);

    // Inactive
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.fillEllipse (184, (float) legendY + 3, 7, 7);
    g.setColour (cyan.withAlpha (0.4f));
    g.drawText ("Inactive", 198, legendY, 80, 16, juce::Justification::centredLeft);

    // No route
    g.setColour (cyan.withAlpha (0.4f));
    g.drawText ("--  No route available", 298, legendY, 180, 16, juce::Justification::centredLeft);
}

void OysterAudioProcessorEditor::paintSettingsPage (juce::Graphics&)
{
}

void OysterAudioProcessorEditor::paint (juce::Graphics& g)
{
    // AU validation can trigger paint before layout is established.
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    // Ocean depth gradient — same as before, full window
    juce::ColourGradient gradient (
        juce::Colour (0xff8ecae6), 0.0f, 0.0f,
        juce::Colour (0xff000008), 0.0f, (float) getHeight(),
        false
    );
    gradient.addColour (0.12, juce::Colour (0xff219ebc));
    gradient.addColour (0.28, juce::Colour (0xff126782));
    gradient.addColour (0.45, juce::Colour (0xff023e5c));
    gradient.addColour (0.62, juce::Colour (0xff011a2e));
    gradient.addColour (0.80, juce::Colour (0xff000d1a));
    g.setGradientFill (gradient);
    g.fillRect (getLocalBounds());

    // Nav bar separator line
    const int navH = 36;
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (0, navH, getWidth(), navH, 1.0f);

    // Unified preset strip — one black bar behind SLOT label + PRESET/name/SAVE
    {
        const int stripX = presetStripX;
        const int stripY = 6;
        const int stripH = 24;
        const int stripW = 60 + 280;  // slot label width + preset box width
        g.setColour (juce::Colour (0xff0a1520));
        g.fillRoundedRectangle ((float)stripX, (float)stripY,
                                (float)stripW, (float)stripH, 4.0f);
        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.3f));
        g.drawRoundedRectangle ((float)stripX + 0.5f, (float)stripY + 0.5f,
                                (float)stripW - 1.0f, (float)stripH - 1.0f, 4.0f, 1.0f);
        // White divider between slot section and preset section
        const float divX = (float)(stripX + 60);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawLine (divX, (float)(stripY + 3), divX, (float)(stripY + stripH - 3), 1.0f);
    }

    // Page title
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.setFont (juce::FontOptions (11.0f));
    const juce::StringArray pageTitles { "MAIN", "SYNTH ENGINE", "MATRIX", "SETTINGS" };
    g.drawText (pageTitles[currentPage],
                getWidth() - 120, 10, 110, 20,
                juce::Justification::centredRight);

    // Page content placeholder
    const int contentY = navH + 10;
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::FontOptions (15.0f));

    if (currentPage == 0)
    {
        // --- Left sidebar panel (preset browser) ---
        const int sidebarW = 260;
        g.setColour (juce::Colour (0xff0a1520).withAlpha (0.75f));
        g.fillRect (0, navH, sidebarW, getHeight() - navH);

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawLine ((float) sidebarW, (float) navH,
                    (float) sidebarW, (float) getHeight(), 1.0f);

        g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.7f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("PRESETS", 12, navH + 8, sidebarW - 24, 18,
                    juce::Justification::centredLeft);

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawLine (10.0f, (float)(navH + 30), (float)(sidebarW - 10), (float)(navH + 30), 1.0f);

        // Preset list — stock then user
        {
            const int itemH     = 20;
            const int itemIndent = 16;
            int listY = navH + 36;
            const juce::String currentName = audioProcessor.presetManager.getCurrentPresetName();

            auto drawSectionHeader = [&] (const juce::String& title)
            {
                g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.35f));
                g.setFont (juce::FontOptions (9.0f));
                g.drawText (title, itemIndent, listY, sidebarW - itemIndent * 2, 14,
                            juce::Justification::centredLeft);
                listY += 15;
            };

            auto drawItem = [&] (const juce::String& name)
            {
                const bool isActive = (name == currentName);
                if (isActive)
                {
                    g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.12f));
                    g.fillRect (4, listY, sidebarW - 8, itemH);
                }
                g.setColour (juce::Colour (0xff00e5ff).withAlpha (isActive ? 1.0f : 0.75f));
                g.setFont (juce::FontOptions (11.0f));
                g.drawText (name, itemIndent + 4, listY, sidebarW - itemIndent * 2, itemH,
                            juce::Justification::centredLeft);
                listY += itemH;
            };

            // Initial preset — always shown at the top
            drawItem ("Initial");

            // Stock presets
            if (! sidebarStockPresets.isEmpty())
            {
                drawSectionHeader ("STOCK");
                for (auto& f : sidebarStockPresets)
                    drawItem (f.getFileNameWithoutExtension());
            }

            // User presets
            if (! sidebarUserPresets.isEmpty())
            {
                drawSectionHeader ("USER");
                for (auto& f : sidebarUserPresets)
                    drawItem (f.getFileNameWithoutExtension());
            }
        }

        // "MASTER" label to the left of the volume knob
        {
            const int mKnobSize = 96;
            const int mLabelH   = 18;
            const int mRowY     = navH + 10;  // fixed position matching resized()
            g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.7f));
            g.setFont (juce::FontOptions (12.0f));
            g.drawText ("MASTER", 290, navH + 2, getWidth() - 290 - 30, 18,
                        juce::Justification::centredLeft);
        }

        drawOceanDisplay (g);
        drawPresetSlots (g);
    }
    else if (currentPage == 1)
        drawSynthPage (g);
    else if (currentPage == 2)
        paintMatrixPage (g);
    else if (currentPage == 3)
        paintSettingsPanels (g);
}

void OysterAudioProcessorEditor::paintSettingsPanels (juce::Graphics& g)
{
    const auto panelBg    = juce::Colour (0xff0a1e2e);
    const auto panelBorder = juce::Colour (0xff00e5ff).withAlpha (0.25f);
    const float corner    = 6.0f;

    auto drawPanel = [&] (const juce::Rectangle<int>& bounds)
    {
        if (bounds.isEmpty()) return;
        g.setColour (panelBg);
        g.fillRoundedRectangle (bounds.toFloat(), corner);
        g.setColour (panelBorder);
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), corner, 1.0f);
    };

    // Left column panels
    drawPanel (settAudioPerfBounds);
    drawPanel (settMidiBounds);
    drawPanel (settPolyphonyBounds);
    drawPanel (settDisplayBounds);

    // Right column panels
    drawPanel (settTuningBounds);
    drawPanel (settMasterBounds);
    drawPanel (settEffectsBounds);
}

void OysterAudioProcessorEditor::resized()
{
    contentComponent.setBounds (getLocalBounds());

    if (getWidth() < 640 || getHeight() < 420)
        return;

    const int mainPadL = 290;   // left padding (was 160)
    const int mainPadR = 30;    // right padding (was 160)

    // Always compute layout as if keyboard is visible so content doesn't move
    const int fullHeight = keyboard.isVisible() ? getHeight() : getHeight() + 90;

    // Master knobs — fixed position just below nav bar
    {
        const int mKnobSize = 96;
        const int mLabelH   = 18;
        const int navH      = 36;
        const int mRowY     = navH + 10;  // fixed position below nav
        const int mAreaW    = juce::jmax (1, getWidth() - mainPadL - mainPadR);
        const int pairGap   = 12;   // gap between knobs within a pair

        // Left pair: VOLUME near sidebar edge, PAN next to it
        const int leftX1 = mainPadL + 4;  // VOLUME — just right of sidebar
        const int leftX2 = leftX1 + mKnobSize + pairGap + 40;  // PAN — same spot as before
        masterVolumeKnob.setBounds    (leftX1, mRowY, mKnobSize, mKnobSize);
        masterVolumeLabel.setBounds   (leftX1, mRowY + mKnobSize + 2, mKnobSize, mLabelH);
        masterPanKnob.setBounds       (leftX2, mRowY, mKnobSize, mKnobSize);
        masterPanLabel.setBounds      (leftX2, mRowY + mKnobSize + 2, mKnobSize, mLabelH);

        // Right pair: TRANSP stays put, TUNE near right edge
        const int rightX2 = mainPadL + mAreaW - 4 - mKnobSize;  // TUNE — near right edge
        const int rightX1 = rightX2 - mKnobSize - pairGap - 40;  // TRANSP — same spot as before
        masterTransposeKnob.setBounds  (rightX1, mRowY, mKnobSize, mKnobSize);
        masterTransposeLabel.setBounds (rightX1, mRowY + mKnobSize + 2, mKnobSize, mLabelH);
        masterTuneKnob2.setBounds      (rightX2, mRowY, mKnobSize, mKnobSize);
        masterTuneLabel2.setBounds     (rightX2, mRowY + mKnobSize + 2, mKnobSize, mLabelH);

        // Oyster logo centred between PAN and TRANSPOSE knobs
        const int panRight    = leftX2 + mKnobSize;
        const int transpLeft  = rightX1;
        {
            const int logoGap = transpLeft - panRight;
            const int logoW   = logoGap;         // use full width between knobs
            const int logoH   = mKnobSize + mLabelH + 24;  // taller box for bigger image
            oysterLogo.setBounds (panRight, mRowY - 14, logoW, logoH);
        }

        // Centre the preset strip between PAN's right edge and TRANSP's left edge
        // but never overlap the nav buttons (SETTINGS ends at 395 + 120 = 515)
        const int stripW      = 60 + 280;  // slot label + preset box
        const int navEndX     = 395 + 120 + 10;  // right edge of SETTINGS button + gap
        presetStripX = panRight + (transpLeft - panRight) / 2 - stripW / 2;
        if (presetStripX < navEndX)
            presetStripX = navEndX;

        const int presetStripY = 6;
        const int presetH      = 24;
        const int presetBoxW   = 280;
        const int presetLabelW = 52;
        const int saveBtnW     = 44;
        presetSlotLabel.setBounds (presetStripX,                          presetStripY, 60,     presetH);
        presetTypeLabel.setBounds (presetStripX + 60,                     presetStripY, presetBoxW, presetH);
        presetNameLabel.setBounds (presetStripX + 60 + presetLabelW + 4,  presetStripY, presetBoxW - presetLabelW - saveBtnW - 12, presetH);
        presetSaveBtn.setBounds   (presetStripX + 60 + presetBoxW - saveBtnW - 4, presetStripY + 2, saveBtnW, presetH - 4);
    }

    // Ocean display — top sits just below master knob labels
    {
        const int navH     = 36;
        const int mKnobBot = navH + 10 + 96 + 2 + 18;  // bottom of master knob labels
        mainDispY = mKnobBot + 4;                        // small gap below labels
    }

    // Ocean display height — fill down to the slot buttons area
    // Below ocean: gap(30) + slots(44) + gap(20) + macroKnobs(96+18) + gap(16)
    const int belowOceanNeeded = 30 + 44 + 20 + 96 + 18 + 16;
    mainDispH = juce::jmax (120, (fullHeight - 90) - mainDispY - belowOceanNeeded);
    const int dispBot = mainDispY + mainDispH;
    const int slotLabelY = dispBot + 30;
    slotAreaY = slotLabelY - 10;
    const int slotH      = 44;
    const int slotAreaW  = juce::jmax (1, getWidth() - mainPadL - mainPadR);
    const int slotGap    = 16;
    const int slotW      = juce::jmax (1, (slotAreaW - (slotGap * (numPresetSlots - 1)))
                           / numPresetSlots);


    for (int i = 0; i < numPresetSlots; ++i)
    {
        const int x = mainPadL + i * (slotW + slotGap);
        presetSlotButtons[i].setBounds (x, slotAreaY, slotW, slotH);
    }

    // Knob row below slots
    const int knobSize    = 96;
    const int labelH      = 18;
    const int knobRowY    = slotAreaY + slotH + 20;
    const int macroKnobAreaW = juce::jmax (1, getWidth() - mainPadL - mainPadR);
    const int knobSpacing = juce::jmax (1, macroKnobAreaW / 6);

    std::array<juce::Slider*, 6> knobs {
        &reverbKnob, &shimmerKnob, &warmthKnob,
        &driftKnob,  &bloomKnob,   &mixKnob
    };
    std::array<juce::Label*, 6> labels {
        &reverbLabel, &shimmerLabel, &warmthLabel,
        &driftLabel,  &bloomLabel,   &mixLabel
    };

    for (int i = 0; i < 6; ++i)
    {
        const int cx = mainPadL + i * knobSpacing + (knobSpacing / 2) - (knobSize / 2);
        knobs[i]->setBounds  (cx, knobRowY, knobSize, knobSize);
        labels[i]->setBounds (cx, knobRowY + knobSize + 2, knobSize, labelH);
    }

    const int keyboardH = keyboard.isVisible() ? 90 : 0;
    if (keyboard.isVisible())
        keyboard.setBounds (0, getHeight() - keyboardH, getWidth(), 90);
    // C2 (36) to C7 (96) = 5 octaves = 36 white keys
    // Divide available width by white key count to stretch perfectly
    const float whiteKeyCount = 36.0f;
    keyboard.setKeyWidth ((float) getWidth() / whiteKeyCount);
    const int synthPadX  = 200;  // left edge — where panels start
    const int synthPadXR = 20;   // right edge padding
    const int synthPadY = 52;
    const int spacing = 10;
    const int totalW = getWidth() - synthPadX - synthPadXR;
    const int totalH = getHeight() - synthPadY - 10 - keyboardH;
    // Left column panels
    const int leftColX     = 8;
    const int leftColW     = synthPadX - 18;
    const int subPanelH    = 306;
    const int subPanelY    = synthPadY;
    const int osc2PanelY   = subPanelY + subPanelH + spacing;
    const int osc2PanelH   = totalH - subPanelH - spacing;

    // Shared left column measurements
    const int displayW  = (leftColW - 16) / 3;
    const int knobAreaX = leftColX + 8 + displayW + 4;
    const int knobAreaW = leftColW - 16 - displayW - 4;
    const int colW      = (knobAreaW - 4) / 2;
    const int col1X     = knobAreaX;
    const int col2X     = knobAreaX + colW + 4;
    const int kSize     = 44;
    const int kLabelH   = 11;
    const int kStride   = kSize + kLabelH + 6;

    // Power button size — sits in the panel title bar, flush to the right
    const int pwrBtnW = 20;
    const int pwrBtnH = 18;
    const int pwrBtnX = leftColX + leftColW - pwrBtnW - 6;

    // ── SUB OSC layout ──────────────────────────────────────
    const int subInnerY  = subPanelY + 28;
    const int subDispH   = subPanelH - 36;

    subOscPowerBtn.setBounds (pwrBtnX, subPanelY + 6, pwrBtnW, pwrBtnH);

    subOscPreview.setBounds (leftColX + 8, subInnerY, displayW, subDispH);
    subOscPreview.setVertical (true);

    // Dropdown at top of knob area
    subWaveBox.setBounds (col1X, subInnerY, knobAreaW, 22);

    // 6 knobs in 3 rows × 2 cols:
    //   col1 = OCTAVE, MIX, PAN
    //   col2 = SEMI,   TUNE, PHASE
    const int subKnobStartY = subInnerY + 26;
    subOctaveKnob.setBounds (col1X, subKnobStartY,              kSize, kSize);
    subOctaveLabel.setBounds(col1X, subKnobStartY + kSize + 2,  kSize, kLabelH);
    subMixKnob.setBounds    (col1X, subKnobStartY + kStride,    kSize, kSize);
    subMixLabel.setBounds   (col1X, subKnobStartY + kStride + kSize + 2, kSize, kLabelH);
    subPanKnob.setBounds    (col1X, subKnobStartY + kStride * 2,  kSize, kSize);
    subPanLabel.setBounds   (col1X, subKnobStartY + kStride * 2 + kSize + 2, kSize, kLabelH);

    subSemiKnob.setBounds   (col2X, subKnobStartY,              kSize, kSize);
    subSemiLabel.setBounds  (col2X, subKnobStartY + kSize + 2,  kSize, kLabelH);
    subTuneKnob.setBounds   (col2X, subKnobStartY + kStride,    kSize, kSize);
    subTuneLabel.setBounds  (col2X, subKnobStartY + kStride + kSize + 2, kSize, kLabelH);
    subPhaseKnob.setBounds  (col2X, subKnobStartY + kStride * 2,  kSize, kSize);
    subPhaseLabel.setBounds (col2X, subKnobStartY + kStride * 2 + kSize + 2, kSize, kLabelH);

    // ── OSC 2 layout ────────────────────────────────────────
    const int osc2InnerY = osc2PanelY + 28;
    const int osc2PanelBottom = osc2PanelY + osc2PanelH - 8;
    const int osc2DispH  = juce::jmax (0, osc2PanelBottom - osc2InnerY);

    osc2PowerBtn.setBounds (pwrBtnX, osc2PanelY + 6, pwrBtnW, pwrBtnH);

    osc2Preview.setBounds (leftColX + 8, osc2InnerY, displayW, osc2DispH);
    osc2Preview.setVertical (true);

    // Dropdown at top
    osc2WaveBox.setBounds (col1X, osc2InnerY, knobAreaW, 22);

    // 7 knobs across 2 columns: col1=OCTAVE,FINE,MIX,PAN  col2=SEMI,PHASE,DETUNE
    const int osc2KnobStartY = osc2InnerY + 26;
    osc2OctaveKnob.setBounds (col1X, osc2KnobStartY,              kSize, kSize);
    osc2OctaveLabel.setBounds(col1X, osc2KnobStartY + kSize + 2,  kSize, kLabelH);
    osc2FineKnob.setBounds   (col1X, osc2KnobStartY + kStride,    kSize, kSize);
    osc2FineLabel.setBounds  (col1X, osc2KnobStartY + kStride + kSize + 2, kSize, kLabelH);
    osc2MixKnob.setBounds    (col1X, osc2KnobStartY + kStride * 2,  kSize, kSize);
    osc2MixLabel.setBounds   (col1X, osc2KnobStartY + kStride * 2 + kSize + 2, kSize, kLabelH);
    osc2PanKnob.setBounds    (col1X, osc2KnobStartY + kStride * 3,  kSize, kSize);
    osc2PanLabel.setBounds   (col1X, osc2KnobStartY + kStride * 3 + kSize + 2, kSize, kLabelH);

    osc2SemiKnob.setBounds   (col2X, osc2KnobStartY,              kSize, kSize);
    osc2SemiLabel.setBounds  (col2X, osc2KnobStartY + kSize + 2,  kSize, kLabelH);
    osc2PhaseKnob.setBounds  (col2X, osc2KnobStartY + kStride,    kSize, kSize);
    osc2PhaseLabel.setBounds (col2X, osc2KnobStartY + kStride + kSize + 2, kSize, kLabelH);
    osc2DetuneKnob.setBounds (col2X, osc2KnobStartY + kStride * 2,  kSize, kSize);
    osc2DetuneLabel.setBounds(col2X, osc2KnobStartY + kStride * 2 + kSize + 2, kSize, kLabelH);
    const int topRowH = (int) (totalH * 0.55f);
    const int grainW = (int) (totalW * 0.63f) - spacing / 2;
    const int waveW = totalW - grainW - spacing;
    const int botRowY = synthPadY + topRowH + spacing;
    const int botRowH = totalH - topRowH - spacing;
    const int grainPanelH = 32 + 60 + 12 + 58 + 14 + 16;
    const int grainActualBottom = synthPadY + grainPanelH;
    const int wtStartY = grainActualBottom + spacing;
    wtPanelH = 0; // calculated dynamically below

    const int grainInnerX = synthPadX + 10;
    const int grainInnerY = synthPadY + 32;
    const int grainInnerW = grainW - 20;
    grainPowerBtn.setBounds (synthPadX + grainW - 26, synthPadY + 6, 20, 18);
    grainDisplay.setBounds (grainInnerX, grainInnerY, grainInnerW, 60);

    const int knobAreaY = grainDisplay.getBottom() + 12;
    const int knobAreaH = 58 + 14 + 4;
    juce::Rectangle<int> grainKnobArea (grainInnerX,
                                        knobAreaY,
                                        grainInnerW,
                                        knobAreaH);
    layoutKnobRow (grainKnobArea,
                   { { &positionKnob, &positionLabel }, { &sprayKnob, &sprayLabel },
                     { &grainSizeKnob, &grainSizeLabel }, { &densityKnob, &densityLabel },
                     { &pitchScatterKnob, &pitchScatterLabel }, { &panSpreadKnob, &panSpreadLabel } });

    // -----------------------------------------------------------------------
    // WAVETABLE SOURCE — inner area
    // -----------------------------------------------------------------------
    const int wtPanelX = synthPadX + 10;
    const int wtPanelY = wtStartY + 32;
    const int wtPanelW = grainW - 20;

    const int previewH = 52;
    wavePreview.setBounds (wtPanelX, wtPanelY, wtPanelW, previewH);

    const int comboLabelH = 12;
    const int comboH = 24;
    const int comboGap = 10;
    const int comboW = (wtPanelW - comboGap) / 2;
    const int comboLabelY = wtPanelY + previewH + 8;
    const int comboY = comboLabelY + comboLabelH + 2;

    waveALabel.setBounds (wtPanelX, comboLabelY, comboW, comboLabelH);
    waveBLabel.setBounds (wtPanelX + comboW + comboGap, comboLabelY, comboW, comboLabelH);
    waveABox.setBounds (wtPanelX, comboY, comboW, comboH);
    waveBBox.setBounds (wtPanelX + comboW + comboGap, comboY, comboW, comboH);

    const int wtKnobSize = 44;
    const int wtLabelH = 12;
    const int wtRowGap = 10;
    const int wtRow1Y = comboY + comboH + 14;

    const int row1Count = 6;
    const int row1Spacing = (wtPanelW - row1Count * wtKnobSize) / (row1Count + 1);
    auto row1X = [&] (int i)
    {
        return wtPanelX + row1Spacing * (i + 1) + wtKnobSize * i;
    };

    morphKnob.setBounds (row1X (0), wtRow1Y, wtKnobSize, wtKnobSize);
    morphLabel.setBounds (row1X (0), wtRow1Y + wtKnobSize + 2, wtKnobSize, wtLabelH);

    wtOctaveKnob.setBounds (row1X (1), wtRow1Y, wtKnobSize, wtKnobSize);
    wtOctaveLabel.setBounds (row1X (1), wtRow1Y + wtKnobSize + 2, wtKnobSize, wtLabelH);

    wtSemitoneKnob.setBounds (row1X (2), wtRow1Y, wtKnobSize, wtKnobSize);
    wtSemitoneLabel.setBounds (row1X (2), wtRow1Y + wtKnobSize + 2, wtKnobSize, wtLabelH);

    wtFineKnob.setBounds (row1X (3), wtRow1Y, wtKnobSize, wtKnobSize);
    wtFineLabel.setBounds (row1X (3), wtRow1Y + wtKnobSize + 2, wtKnobSize, wtLabelH);

    wtPhaseKnob.setBounds (row1X (4), wtRow1Y, wtKnobSize, wtKnobSize);
    wtPhaseLabel.setBounds (row1X (4), wtRow1Y + wtKnobSize + 2, wtKnobSize, wtLabelH);

    wtTiltKnob.setBounds (row1X (5), wtRow1Y, wtKnobSize, wtKnobSize);
    wtTiltLabel.setBounds (row1X (5), wtRow1Y + wtKnobSize + 2, wtKnobSize, wtLabelH);

    const int wtRow2Y = wtRow1Y + wtKnobSize + wtLabelH + wtRowGap + 4;
    const int row2Count = 4;
    const int row2Spacing4 = (wtPanelW - row2Count * wtKnobSize) / (row2Count + 1);
    auto row2X4 = [&] (int i) { return wtPanelX + row2Spacing4 * (i + 1) + wtKnobSize * i; };

    unisonVoicesKnob.setBounds  (row2X4 (0), wtRow2Y, wtKnobSize, wtKnobSize);
    unisonVoicesLabel.setBounds (row2X4 (0), wtRow2Y + wtKnobSize + 2, wtKnobSize, wtLabelH);
    unisonDetuneKnob.setBounds  (row2X4 (1), wtRow2Y, wtKnobSize, wtKnobSize);
    unisonDetuneLabel.setBounds (row2X4 (1), wtRow2Y + wtKnobSize + 2, wtKnobSize, wtLabelH);
    unisonSpreadKnob.setBounds  (row2X4 (2), wtRow2Y, wtKnobSize, wtKnobSize);
    unisonSpreadLabel.setBounds (row2X4 (2), wtRow2Y + wtKnobSize + 2, wtKnobSize, wtLabelH);
    chorusMixKnob.setBounds     (row2X4 (3), wtRow2Y, wtKnobSize, wtKnobSize);
    chorusMixLabel.setBounds    (row2X4 (3), wtRow2Y + wtKnobSize + 2, wtKnobSize, wtLabelH);
    wtPanelH = (chorusMixLabel.getBottom() - wtStartY) + 16;
    const int bottomRowY = wtStartY + wtPanelH + spacing;

    pitchSectionLabel.setVisible (false);
    unisonSectionLabel.setVisible (false);

    // --- FILTER now in top-right panel ---
    const int fltPanelX = synthPadX + grainW + spacing + 10;
    const int fltPanelY = synthPadY + 32;
    const int fltPanelW = waveW - 20;
    const int fltPanelHBase = topRowH - 42;
    const int dispH = (int) (fltPanelHBase * 0.45f);
    filterDisplay.setBounds (fltPanelX, fltPanelY, fltPanelW, dispH);

    const int btnY = fltPanelY + dispH + 8;
    const int btnH = 22;
    const int btnGap = 4;
    const int btnW = (fltPanelW - btnGap * 3) / 4;
    filterLPButton.setBounds (fltPanelX, btnY, btnW, btnH);
    filterHPButton.setBounds (fltPanelX + (btnW + btnGap), btnY, btnW, btnH);
    filterBPButton.setBounds (fltPanelX + (btnW + btnGap) * 2, btnY, btnW, btnH);
    filterNTButton.setBounds (fltPanelX + (btnW + btnGap) * 3, btnY, btnW, btnH);

    const int fltKnobSize = 52;
    const int fltKnobLabelH = 14;
    const int fltKnobRowY = btnY + btnH + 12;
    const int fltKnobSpacing = (fltPanelW - 5 * fltKnobSize) / 6;
    filterCutoffKnob.setBounds (fltPanelX + fltKnobSpacing, fltKnobRowY, fltKnobSize, fltKnobSize);
    filterCutoffLabel.setBounds (fltPanelX + fltKnobSpacing, fltKnobRowY + fltKnobSize + 2, fltKnobSize, fltKnobLabelH);
    filterResKnob.setBounds (fltPanelX + fltKnobSpacing * 2 + fltKnobSize, fltKnobRowY, fltKnobSize, fltKnobSize);
    filterResLabel.setBounds (fltPanelX + fltKnobSpacing * 2 + fltKnobSize, fltKnobRowY + fltKnobSize + 2, fltKnobSize, fltKnobLabelH);
    filterDriveKnob.setBounds (fltPanelX + fltKnobSpacing * 3 + fltKnobSize * 2, fltKnobRowY, fltKnobSize, fltKnobSize);
    filterDriveLabel.setBounds (fltPanelX + fltKnobSpacing * 3 + fltKnobSize * 2, fltKnobRowY + fltKnobSize + 2, fltKnobSize, fltKnobLabelH);
    filterEnvAmtKnob.setBounds  (fltPanelX + fltKnobSpacing * 4 + fltKnobSize * 3, fltKnobRowY, fltKnobSize, fltKnobSize);
    filterEnvAmtLabel.setBounds (fltPanelX + fltKnobSpacing * 4 + fltKnobSize * 3, fltKnobRowY + fltKnobSize + 2, fltKnobSize, fltKnobLabelH);
    filterLfoAmtKnob.setBounds  (fltPanelX + fltKnobSpacing * 5 + fltKnobSize * 4, fltKnobRowY, fltKnobSize, fltKnobSize);
    filterLfoAmtLabel.setBounds (fltPanelX + fltKnobSpacing * 5 + fltKnobSize * 4, fltKnobRowY + fltKnobSize + 2, fltKnobSize, fltKnobLabelH);

    // Match filter panel bottom to 16px below the filter knob row.
    const int filterRowBottom = fltKnobRowY + fltKnobSize + 2 + fltKnobLabelH;
    filterPanelH = (filterRowBottom - synthPadY) + 16;

    const int bottomRowH = fullHeight - bottomRowY - 10;
    const int halfW = (grainW - spacing) / 2;

    const int envPanelX = synthPadX + 10;
    const int envPanelY = bottomRowY + 32;
    const int envPanelW = halfW - 20;
    const int envKnobSize = 44;
    const int envLabelH = 12;
    envDisplay.setBounds (envPanelX, envPanelY, envPanelW, 50);
    env4DestBox.setBounds (envPanelX, envPanelY + 56, envPanelW, 20);
    const int envKnobY = envPanelY + 56 + 20 + 6;
    const int envSpacing = (envPanelW - 4 * envKnobSize) / 5;
    envAttackKnob.setBounds (envPanelX + envSpacing, envKnobY, envKnobSize, envKnobSize);
    envAttackLabel.setBounds (envPanelX + envSpacing, envKnobY + envKnobSize + 2, envKnobSize, envLabelH);
    envDecayKnob.setBounds (envPanelX + envSpacing * 2 + envKnobSize, envKnobY, envKnobSize, envKnobSize);
    envDecayLabel.setBounds (envPanelX + envSpacing * 2 + envKnobSize, envKnobY + envKnobSize + 2, envKnobSize, envLabelH);
    envSustainKnob.setBounds (envPanelX + envSpacing * 3 + envKnobSize * 2, envKnobY, envKnobSize, envKnobSize);
    envSustainLabel.setBounds (envPanelX + envSpacing * 3 + envKnobSize * 2, envKnobY + envKnobSize + 2, envKnobSize, envLabelH);
    envReleaseKnob.setBounds (envPanelX + envSpacing * 4 + envKnobSize * 3, envKnobY, envKnobSize, envKnobSize);
    envReleaseLabel.setBounds (envPanelX + envSpacing * 4 + envKnobSize * 3, envKnobY + envKnobSize + 2, envKnobSize, envLabelH);

    const int lfoPanelX = synthPadX + halfW + spacing + 10;
    const int lfoPanelY = bottomRowY + 32;
    const int lfoPanelW = halfW - 20;
    const int lfoKnobSize = 44;
    const int lfoLabelH = 12;
    lfoDisplay.setBounds (lfoPanelX, lfoPanelY, lfoPanelW, 50);

    // Shape buttons — horizontal row under the display (same as before)
    const int shapeBtnY = lfoPanelY + 50 + 8;
    const int shapeBtnH = 20;
    const int shapeBtnGap = 3;
    const int shapeBtnW = (lfoPanelW - shapeBtnGap * 4) / 5;
    lfoSineBtn.setBounds (lfoPanelX,                                shapeBtnY, shapeBtnW, shapeBtnH);
    lfoTriBtn.setBounds  (lfoPanelX + (shapeBtnW + shapeBtnGap),     shapeBtnY, shapeBtnW, shapeBtnH);
    lfoSawBtn.setBounds  (lfoPanelX + (shapeBtnW + shapeBtnGap) * 2, shapeBtnY, shapeBtnW, shapeBtnH);
    lfoSqBtn.setBounds   (lfoPanelX + (shapeBtnW + shapeBtnGap) * 3, shapeBtnY, shapeBtnW, shapeBtnH);
    lfoSHBtn.setBounds   (lfoPanelX + (shapeBtnW + shapeBtnGap) * 4, shapeBtnY, shapeBtnW, shapeBtnH);

    // Toggle buttons (SYNC/RETRIG/PHASE) — stacked vertically on the left
    const int togBtnY = shapeBtnY + shapeBtnH + 4;
    const int togBtnW = 46;
    const int togBtnH = 18;
    const int togBtnGap = 2;
    lfoSyncBtn.setBounds      (lfoPanelX, togBtnY,                                togBtnW, togBtnH);
    lfoRetriggerBtn.setBounds (lfoPanelX, togBtnY + (togBtnH + togBtnGap),        togBtnW, togBtnH);
    lfoPhaseBtn.setBounds     (lfoPanelX, togBtnY + (togBtnH + togBtnGap) * 2,    togBtnW, togBtnH);
    lfoEnabledBtn.setBounds   (lfoPanelX, togBtnY + (togBtnH + togBtnGap) * 3,    togBtnW, togBtnH);

    // Knobs — 4 in a horizontal row to the right of toggle buttons
    const int lfoKnobAreaX = lfoPanelX + togBtnW + 8;
    const int lfoKnobAreaW = lfoPanelW - togBtnW - 8;
    const int lfoKnobY = togBtnY;
    const int lfoKnobSp = (lfoKnobAreaW - 4 * lfoKnobSize) / 5;
    lfoRateKnob.setBounds   (lfoKnobAreaX + lfoKnobSp,                          lfoKnobY, lfoKnobSize, lfoKnobSize);
    lfoRateLabel.setBounds  (lfoKnobAreaX + lfoKnobSp,                          lfoKnobY + lfoKnobSize + 2, lfoKnobSize, lfoLabelH);
    lfoDepthKnob.setBounds  (lfoKnobAreaX + lfoKnobSp * 2 + lfoKnobSize,        lfoKnobY, lfoKnobSize, lfoKnobSize);
    lfoDepthLabel.setBounds (lfoKnobAreaX + lfoKnobSp * 2 + lfoKnobSize,        lfoKnobY + lfoKnobSize + 2, lfoKnobSize, lfoLabelH);
    lfoAttackKnob.setBounds (lfoKnobAreaX + lfoKnobSp * 3 + lfoKnobSize * 2,    lfoKnobY, lfoKnobSize, lfoKnobSize);
    lfoAttackLabel.setBounds(lfoKnobAreaX + lfoKnobSp * 3 + lfoKnobSize * 2,    lfoKnobY + lfoKnobSize + 2, lfoKnobSize, lfoLabelH);
    lfoDecayKnob.setBounds  (lfoKnobAreaX + lfoKnobSp * 4 + lfoKnobSize * 3,    lfoKnobY, lfoKnobSize, lfoKnobSize);
    lfoDecayLabel.setBounds (lfoKnobAreaX + lfoKnobSp * 4 + lfoKnobSize * 3,    lfoKnobY + lfoKnobSize + 2, lfoKnobSize, lfoLabelH);

    // Tab selector buttons (in panel header area, right-aligned)
    {
        const int tabW = 18, tabH = 14, tabGap = 2;
        const int tabRowY = bottomRowY + 6;
        const int totalTabW = 4 * tabW + 3 * tabGap;
        const int envPanelRight = synthPadX + halfW;
        const int lfoPanelRight = synthPadX + halfW + spacing + halfW;
        const int envTabStartX = envPanelRight - totalTabW - 10;
        const int lfoTabStartX = lfoPanelRight - totalTabW - 10;
        for (int i = 0; i < 4; ++i)
        {
            envTabBtns[i].setBounds (envTabStartX + i * (tabW + tabGap), tabRowY, tabW, tabH);
            lfoTabBtns[i].setBounds (lfoTabStartX + i * (tabW + tabGap), tabRowY, tabW, tabH);
        }
    }

    // Modulation panel layout
    // The modulation panel sits at: x=rightColX, y=belowFilterY, w=waveW, h=stackPanelH
    // We derive inner coords here directly
    const int belowFilterY = synthPadY + filterPanelH + spacing;
    const int modInnerX   = synthPadX + grainW + spacing + 10;
    const int modInnerW   = waveW - 20;

    // Scale knob size based on available vertical space
    const int keyboardTop2 = fullHeight - keyboardH;
    const int availableH   = keyboardTop2 - belowFilterY - 20; // total space for mod + ctrl
    // Each section needs: header(32) + 2*(knob+label+gap)
    // That's 2 sections * (32 + 2*(knob + labelH + rowGap)) = total
    // Solve for knob size that fits
    const int idealKnobSize = 44;
    const int idealRowContent = 2 * (32 + 2 * (idealKnobSize + 12 + 12)) + spacing;
    const int modKnobSize = (availableH < idealRowContent)
        ? juce::jmax (28, idealKnobSize * availableH / idealRowContent)
        : idealKnobSize;
    const int modLabelH   = (modKnobSize < 36) ? 10 : 12;
    const int modRowGap   = (modKnobSize < 36) ? 6 : 12;
    const int modRow1Y    = belowFilterY + 32;
    const int modRow2Y    = modRow1Y + modKnobSize + modLabelH + modRowGap;
    const int modSpacing1 = (modInnerW - 4 * modKnobSize) / 5;
    auto modX = [&] (int i) { return modInnerX + modSpacing1 * (i + 1) + modKnobSize * i; };

    lfoToCutoffKnob.setBounds   (modX(0), modRow1Y, modKnobSize, modKnobSize);
    lfoToCutoffLabel.setBounds  (modX(0), modRow1Y + modKnobSize + 2, modKnobSize, modLabelH);
    lfoToPositionKnob.setBounds (modX(1), modRow1Y, modKnobSize, modKnobSize);
    lfoToPositionLabel.setBounds(modX(1), modRow1Y + modKnobSize + 2, modKnobSize, modLabelH);
    lfoToPitchKnob.setBounds    (modX(2), modRow1Y, modKnobSize, modKnobSize);
    lfoToPitchLabel.setBounds   (modX(2), modRow1Y + modKnobSize + 2, modKnobSize, modLabelH);
    lfoToDensityKnob.setBounds  (modX(3), modRow1Y, modKnobSize, modKnobSize);
    lfoToDensityLabel.setBounds (modX(3), modRow1Y + modKnobSize + 2, modKnobSize, modLabelH);

    envToCutoffKnob.setBounds   (modX(0), modRow2Y, modKnobSize, modKnobSize);
    envToCutoffLabel.setBounds  (modX(0), modRow2Y + modKnobSize + 2, modKnobSize, modLabelH);
    envToPositionKnob.setBounds (modX(1), modRow2Y, modKnobSize, modKnobSize);
    envToPositionLabel.setBounds(modX(1), modRow2Y + modKnobSize + 2, modKnobSize, modLabelH);
    envToPitchKnob.setBounds    (modX(2), modRow2Y, modKnobSize, modKnobSize);
    envToPitchLabel.setBounds   (modX(2), modRow2Y + modKnobSize + 2, modKnobSize, modLabelH);
    envToAmpKnob.setBounds      (modX(3), modRow2Y, modKnobSize, modKnobSize);
    envToAmpLabel.setBounds     (modX(3), modRow2Y + modKnobSize + 2, modKnobSize, modLabelH);

    const int rightColBottom = synthPadY + totalH;
    const int remainingRightH = rightColBottom - belowFilterY;

    // CONTROL panel layout — sits at stackTopY + stackPanelH + spacing
    const int ctrlPanelTopY  = belowFilterY + ((remainingRightH - spacing) / 2) + spacing;
    const int ctrlInnerX     = synthPadX + grainW + spacing + 10;
    const int ctrlInnerW     = waveW - 20;
    const int ctrlKnobSize   = modKnobSize;  // same scaled size as modulation knobs
    const int ctrlLabelH     = modLabelH;
    int ctrlRow1Y            = ctrlPanelTopY + 32;
    int ctrlRow2Y            = ctrlRow1Y + ctrlKnobSize + ctrlLabelH + modRowGap;
    const int ctrlSpacing    = (ctrlInnerW - 4 * ctrlKnobSize) / 5;
    auto ctrlX = [&] (int i) { return ctrlInnerX + ctrlSpacing * (i + 1) + ctrlKnobSize * i; };

    // Keep both control rows fully above the keyboard strip.
    const int keyboardTop = fullHeight - keyboardH;
    const int ctrlContentBottom = ctrlRow2Y + ctrlKnobSize + 2 + ctrlLabelH;
    const int maxBottom = keyboardTop - 10;
    if (ctrlContentBottom > maxBottom)
    {
        const int shiftUp = ctrlContentBottom - maxBottom;
        ctrlRow1Y -= shiftUp;
        ctrlRow2Y -= shiftUp;
    }

    glideKnob.setBounds    (ctrlX(0), ctrlRow1Y, ctrlKnobSize, ctrlKnobSize);
    glideLabel.setBounds   (ctrlX(0), ctrlRow1Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);
    bendUpKnob.setBounds   (ctrlX(1), ctrlRow1Y, ctrlKnobSize, ctrlKnobSize);
    bendUpLabel.setBounds  (ctrlX(1), ctrlRow1Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);
    bendDownKnob.setBounds (ctrlX(2), ctrlRow1Y, ctrlKnobSize, ctrlKnobSize);
    bendDownLabel.setBounds(ctrlX(2), ctrlRow1Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);
    velSensKnob.setBounds  (ctrlX(3), ctrlRow1Y, ctrlKnobSize, ctrlKnobSize);
    velSensLabel.setBounds (ctrlX(3), ctrlRow1Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);

    filterKeyTrackKnob.setBounds  (ctrlX(0), ctrlRow2Y, ctrlKnobSize, ctrlKnobSize);
    filterKeyTrackLabel.setBounds (ctrlX(0), ctrlRow2Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);
    octaveShiftKnob.setBounds (ctrlX(1), ctrlRow2Y, ctrlKnobSize, ctrlKnobSize);
    octaveShiftLabel.setBounds(ctrlX(1), ctrlRow2Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);
    stereoWidthKnob.setBounds (ctrlX(2), ctrlRow2Y, ctrlKnobSize, ctrlKnobSize);
    stereoWidthLabel.setBounds(ctrlX(2), ctrlRow2Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);
    masterTuneKnob.setBounds  (ctrlX(3), ctrlRow2Y, ctrlKnobSize, ctrlKnobSize);
    masterTuneLabel.setBounds (ctrlX(3), ctrlRow2Y + ctrlKnobSize + 2, ctrlKnobSize, ctrlLabelH);

    const int rightColX = synthPadX + grainW + spacing;
    const int rightColW = waveW;
    const int fxPanelH = (remainingRightH - spacing) / 2;
    const int modPanelH = fxPanelH;
    juce::ignoreUnused (rightColX, rightColW, belowFilterY, remainingRightH, fxPanelH, modPanelH);

    // Reverb knobs hidden — no FX panel
    reverbSizeKnob.setBounds  (0, 0, 0, 0);
    reverbSizeLabel.setBounds (0, 0, 0, 0);
    reverbMixKnob.setBounds   (0, 0, 0, 0);
    reverbMixLabel.setBounds  (0, 0, 0, 0);

    // ── Settings page two-column layout ────────────────────────────────
    {
        const int navH      = 36;
        const int kbH       = 90;
        const int settTop   = navH + 10;
        // Always lay out as if keyboard is visible so panels don't resize
        const int fullH     = keyboard.isVisible() ? getHeight() : getHeight() + kbH;
        const int settBot   = fullH - kbH - 10;
        const int settH     = settBot - settTop;

        const int colGap    = 16;
        const int padX      = 20;
        const int totalW    = getWidth() - padX * 2;
        const int colW      = (totalW - colGap) / 2;

        const int leftX     = padX;
        const int rightX    = padX + colW + colGap;

        const int sectionGap = 10;
        const int headerH   = 24;
        const int panelPad  = 6;

        // ── Left column: Audio/Perf + Display + MIDI (sized to content), rest split evenly ──
        const int rowH = 24;
        const int rowGap = 4;
        const int audioPerfH = panelPad + headerH + 4 + (rowH + rowGap) * 5 + panelPad;
        const int displayH   = panelPad + headerH + 4 + (rowH + rowGap) * 5 + panelPad;
        const int midiH       = panelPad + headerH + 4 + (rowH + rowGap) * 5 + panelPad;  // channel row + 4 toggles
        const int polyH       = panelPad + headerH + 4 + (rowH + rowGap) * 4 + panelPad;  // 4 dropdowns
        // No remaining left-column panels — Preset Management removed (full Preset page coming later)

        int ly = settTop;
        settAudioPerfBounds = { leftX, ly, colW, audioPerfH };
        settAudioPerfLabel.setBounds (leftX + panelPad, ly + panelPad, colW - panelPad * 2, headerH);

        // Audio/Performance controls layout
        {
            const int cx    = leftX + panelPad;
            const int cw    = colW - panelPad * 2;
            const int rowH  = 24;
            const int gap   = 4;
            const int labelW = cw / 2;
            const int ctrlW  = cw - labelW - 8;
            int cy = ly + panelPad + headerH + 4;

            settOversamplingLabel.setBounds (cx, cy, labelW, rowH);
            settOversamplingBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + gap;

            settQualityLabel.setBounds (cx, cy, labelW, rowH);
            settQualityBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + gap;

            settBufferSizeLabel.setBounds (cx, cy, labelW, rowH);
            settBufferSizeVal.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + gap;

            settSampleRateLabel.setBounds (cx, cy, labelW, rowH);
            settSampleRateVal.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + gap;

            settCpuOptBtn.setBounds (cx, cy, cw, rowH);
        }
        ly += audioPerfH + sectionGap;

        settDisplayBounds = { leftX, ly, colW, displayH };
        settDisplayLabel.setBounds (leftX + panelPad, ly + panelPad, colW - panelPad * 2, headerH);

        // Display controls layout
        {
            const int cx     = leftX + panelPad;
            const int cw     = colW - panelPad * 2;
            const int labelW = cw / 2;
            const int ctrlW  = cw - labelW - 8;
            int cy = ly + panelPad + headerH + 4;

            settWindowSizeLabel.setBounds (cx, cy, labelW, rowH);
            settWindowSizeBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settShowKeyboardBtn.setBounds  (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settShowWaveformsBtn.setBounds (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settShowOceanBtn.setBounds    (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settShowLabelsBtn.setBounds   (cx, cy, cw, rowH);
        }
        ly += displayH + sectionGap;

        settMidiBounds = { leftX, ly, colW, midiH };
        settMidiLabel.setBounds (leftX + panelPad, ly + panelPad, colW - panelPad * 2, headerH);

        // MIDI controls layout — 4 channel dropdowns in a row, then 4 toggles
        {
            const int cx     = leftX + panelPad;
            const int cw     = colW - panelPad * 2;
            int cy = ly + panelPad + headerH + 4;

            // 4 per-slot MIDI channel dropdowns in a single row
            {
                const int boxGap  = 6;
                const int lblW    = 18;  // "Ch" or slot number label
                const int boxW    = (cw - 4 * lblW - 3 * boxGap - 4 * 2) / 4;
                for (int i = 0; i < 4; ++i)
                {
                    const int cellW = lblW + 2 + boxW;
                    const int xOff  = cx + i * (cellW + boxGap);
                    slotMidiChannelLabel[i].setBounds (xOff, cy, lblW, rowH);
                    slotMidiChannelBox[i].setBounds   (xOff + lblW + 2, cy, boxW, rowH);
                }
            }
            cy += rowH + rowGap;

            settSustainPedalBtn.setBounds  (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settLegatoModeBtn.setBounds    (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settProgramChangeBtn.setBounds (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settMidiLearnBtn.setBounds     (cx, cy, cw, rowH);
        }
        ly += midiH + sectionGap;

        settPolyphonyBounds = { leftX, ly, colW, polyH };
        settPolyphonyLabel.setBounds (leftX + panelPad, ly + panelPad, colW - panelPad * 2, headerH);

        // Polyphony / Voicing controls layout
        {
            const int cx     = leftX + panelPad;
            const int cw     = colW - panelPad * 2;
            const int labelW = cw / 2;
            const int ctrlW  = cw - labelW - 8;
            int cy = ly + panelPad + headerH + 4;

            settVoiceCountLabel.setBounds (cx, cy, labelW, rowH);
            settVoiceCountBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settVoiceStealLabel.setBounds (cx, cy, labelW, rowH);
            settVoiceStealBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settUnisonVoicesLabel.setBounds (cx, cy, labelW, rowH);
            settUnisonVoicesKnob.setBounds  (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settUnisonDetuneLabel.setBounds (cx, cy, labelW, rowH);
            settUnisonDetuneKnob.setBounds  (cx + labelW + 8, cy, ctrlW, rowH);
        }
        ly += polyH + sectionGap;

        // ── Right column: Tuning, Master, Effects (content-sized), Mod Matrix gets rest ──
        const int tuningH  = panelPad + headerH + 4 + (rowH + rowGap) * 6 + panelPad;  // 6 rows
        const int masterH  = panelPad + headerH + 4 + (rowH + rowGap) * 4 + panelPad;  // 4 rows
        const int effectsH = panelPad + headerH + 4 + (rowH + rowGap) * 4 + panelPad;  // 4 rows

        int ry = settTop;
        settTuningBounds = { rightX, ry, colW, tuningH };
        settTuningLabel.setBounds (rightX + panelPad, ry + panelPad, colW - panelPad * 2, headerH);

        // Tuning controls layout
        {
            const int cx     = rightX + panelPad;
            const int cw     = colW - panelPad * 2;
            const int labelW = cw / 2;
            const int ctrlW  = cw - labelW - 8;
            int cy = ry + panelPad + headerH + 4;

            settMasterTuneLabel2.setBounds (cx, cy, labelW, rowH);
            settMasterTuneKnob2.setBounds  (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settTuningSystemLabel.setBounds (cx, cy, labelW, rowH);
            settTuningSystemBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settRefPitchLabel.setBounds (cx, cy, labelW, rowH);
            settRefPitchBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settTransposeLabel.setBounds (cx, cy, labelW, rowH);
            settTransposeBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settOctaveShiftLabel.setBounds (cx, cy, labelW, rowH);
            settOctaveShiftBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settScaleLabel.setBounds (cx, cy, labelW, rowH);
            settScaleBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
        }
        ry += tuningH + sectionGap;

        settMasterBounds = { rightX, ry, colW, masterH };
        settMasterLabel.setBounds (rightX + panelPad, ry + panelPad, colW - panelPad * 2, headerH);

        // Master controls layout
        {
            const int cx     = rightX + panelPad;
            const int cw     = colW - panelPad * 2;
            const int labelW = cw / 2;
            const int ctrlW  = cw - labelW - 8;
            int cy = ry + panelPad + headerH + 4;

            settMasterVolLabel.setBounds (cx, cy, labelW, rowH);
            settMasterVolBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settLimiterLabel.setBounds (cx, cy, labelW, rowH);
            settLimiterBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settMeteringBtn.setBounds (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settOutputPanLabel.setBounds (cx, cy, labelW, rowH);
            settOutputPanBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
        }
        ry += masterH + sectionGap;

        settEffectsBounds = { rightX, ry, colW, effectsH };
        settEffectsLabel.setBounds (rightX + panelPad, ry + panelPad, colW - panelPad * 2, headerH);

        // Effects Defaults controls layout
        {
            const int cx     = rightX + panelPad;
            const int cw     = colW - panelPad * 2;
            const int labelW = cw / 2;
            const int ctrlW  = cw - labelW - 8;
            int cy = ry + panelPad + headerH + 4;

            settDryWetLabel.setBounds (cx, cy, labelW, rowH);
            settDryWetBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settDitherLabel.setBounds (cx, cy, labelW, rowH);
            settDitherBox.setBounds   (cx + labelW + 8, cy, ctrlW, rowH);
            cy += rowH + rowGap;

            settSoftClipBtn.setBounds   (cx, cy, cw, rowH);
            cy += rowH + rowGap;

            settPhaseInvertBtn.setBounds (cx, cy, cw, rowH);
        }
        ry += effectsH + sectionGap;
    }
}

//==============================================================================
void OysterAudioProcessorEditor::refreshPresetLists()
{
    sidebarStockPresets = audioProcessor.presetManager.getStockPresets();
    sidebarUserPresets  = audioProcessor.presetManager.getUserPresets();
}

void OysterAudioProcessorEditor::loadPresetFromSidebar (const juce::File& file)
{
    if (audioProcessor.presetManager.loadPreset (file))
    {
        auto name = audioProcessor.presetManager.getCurrentPresetName();

        // Update the header bar preset name
        presetNameLabel.setText (name, juce::dontSendNotification);

        // Update the active slot's label on the MAIN page
        audioProcessor.presetSlotNames[(size_t) activeSlot] = name;

        rebuildAllAttachments();
        repaint();
    }
}

void OysterAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (currentPage != 0)
        return;

    const int navH     = 36;
    const int sidebarW = 260;

    if (e.x >= sidebarW || e.y < navH)
        return;

    // Work out which row was clicked — mirrors the drawing logic
    const int itemH      = 20;
    const int headerH    = 15;
    int listY = navH + 36;

    // "Initial" is always the first row
    if (e.y >= listY && e.y < listY + itemH)
    {
        audioProcessor.presetManager.loadInitialPresetIntoSlot (activeSlot);
        audioProcessor.presetSlotNames[(size_t) activeSlot] = "Initial";
        presetNameLabel.setText ("Initial", juce::dontSendNotification);
        rebuildAllAttachments();
        repaint();
        return;
    }
    listY += itemH;

    auto hitTest = [&] (const juce::Array<juce::File>& files,
                        bool hasHeader) -> const juce::File*
    {
        if (files.isEmpty()) return nullptr;
        if (hasHeader) listY += headerH;
        for (auto& f : files)
        {
            if (e.y >= listY && e.y < listY + itemH)
                return &f;
            listY += itemH;
        }
        return nullptr;
    };

    if (auto* hit = hitTest (sidebarStockPresets, ! sidebarStockPresets.isEmpty()))
    {
        loadPresetFromSidebar (*hit);
        return;
    }
    if (auto* hit = hitTest (sidebarUserPresets, ! sidebarUserPresets.isEmpty()))
        loadPresetFromSidebar (*hit);
}
