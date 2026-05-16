/*
  ==============================================================================
    PresetManager.h
    Handles save/load/scan for Oyster presets.

    Stock presets  → /Library/Application Support/Oyster/Presets/   (macOS)
                     C:\ProgramData\Oyster\Presets\                 (Windows)

    User presets   → ~/Documents/Oyster/Presets/                    (macOS)
                     C:\Users\<name>\Documents\Oyster\Presets\      (Windows)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SlotState.h"

class PresetManager
{
public:
    static constexpr const char* kPresetExtension = ".oyster";
    static constexpr int         kFormatVersion   = 1;

    PresetManager (juce::AudioProcessorValueTreeState& apvtsRef,
                   SlotState* slotStatesPtr,
                   int numSlotsIn,
                   int& activeSlotRef);

    // ── Folder helpers ──────────────────────────────────────────────────────
    static juce::File getStockPresetsFolder();
    static juce::File getUserPresetsFolder();

    /** Creates both folders on disk if they don't already exist. */
    static void ensureFoldersExist();

    // ── Save / Load ─────────────────────────────────────────────────────────
    /** Serialises the full plugin state and writes it to the user presets folder. */
    bool savePreset (const juce::String& name);

    /** Loads a preset from any .oyster file. */
    bool loadPreset (const juce::File& file);

    // ── Scanning ────────────────────────────────────────────────────────────
    juce::Array<juce::File> getStockPresets() const;
    juce::Array<juce::File> getUserPresets()  const;

    // ── Initial preset ────────────────────────────────────────────────────
    /** Applies the built-in "Initial" preset to all slots. Called on first launch. */
    void loadInitialPresetIntoAllSlots();

    /** Applies the built-in "Initial" preset to a single slot. */
    void loadInitialPresetIntoSlot (int slot);

    // ── Current preset name ─────────────────────────────────────────────────
    const juce::String& getCurrentPresetName() const { return currentPresetName; }
    void                setCurrentPresetName  (const juce::String& n) { currentPresetName = n; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    SlotState* slotStates;
    int  numSlots;
    int& activeSlot;

    juce::String currentPresetName { "Initial" };
};
