/*
  ==============================================================================
    PresetManager.cpp
  ==============================================================================
*/

#include "PresetManager.h"

//==============================================================================
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvtsRef,
                               SlotState* slotStatesPtr,
                               int numSlotsIn,
                               int& activeSlotRef)
    : apvts       (apvtsRef),
      slotStates  (slotStatesPtr),
      numSlots    (numSlotsIn),
      activeSlot  (activeSlotRef)
{
    ensureFoldersExist();
}

//==============================================================================
juce::File PresetManager::getStockPresetsFolder()
{
    return juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory)
               .getChildFile ("Oyster/Presets");
}

juce::File PresetManager::getUserPresetsFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Oyster/Presets");
}

void PresetManager::ensureFoldersExist()
{
    getUserPresetsFolder().createDirectory();

    // Create stock folder with basic sub-categories
    auto stock = getStockPresetsFolder();
    stock.createDirectory();
    stock.getChildFile ("Pads").createDirectory();
    stock.getChildFile ("Lead").createDirectory();
    stock.getChildFile ("Bass").createDirectory();
    stock.getChildFile ("Texture").createDirectory();
    stock.getChildFile ("Sequence").createDirectory();
    stock.getChildFile ("FX").createDirectory();
}

//==============================================================================
bool PresetManager::savePreset (const juce::String& name)
{
    // 1. Flush active slot parameters into its SlotState
    slotStates[activeSlot].loadFromAPVTS (apvts);

    // 2. Build the preset tree — just the active slot's state
    juce::ValueTree root ("OysterPreset");
    root.setProperty ("version", kFormatVersion, nullptr);
    root.setProperty ("name",    name,            nullptr);
    root.addChild (slotStates[activeSlot].toValueTree(), -1, nullptr);

    // 3. Write to file
    auto file = getUserPresetsFolder().getChildFile (name + kPresetExtension);
    if (auto xml = root.createXml())
    {
        if (xml->writeTo (file))
        {
            currentPresetName = name;
            return true;
        }
    }

    return false;
}

//==============================================================================
bool PresetManager::loadPreset (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return false;

    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.isValid() || root.getType().toString() != "OysterPreset")
        return false;

    // The preset file contains a single SlotState as the first child
    auto slotTree = root.getChild (0);
    if (! slotTree.isValid())
        return false;

    // Load into the active slot
    slotStates[activeSlot].fromValueTree (slotTree);

    // Push into APVTS so the UI updates immediately
    slotStates[activeSlot].writeToAPVTS (apvts);

    currentPresetName = file.getFileNameWithoutExtension();
    return true;
}

//==============================================================================
juce::Array<juce::File> PresetManager::getStockPresets() const
{
    juce::Array<juce::File> results;
    getStockPresetsFolder().findChildFiles (results,
                                            juce::File::findFiles,
                                            true,          // recursive
                                            "*" + juce::String (kPresetExtension));
    results.sort();
    return results;
}

juce::Array<juce::File> PresetManager::getUserPresets() const
{
    juce::Array<juce::File> results;
    getUserPresetsFolder().findChildFiles (results,
                                           juce::File::findFiles,
                                           true,
                                           "*" + juce::String (kPresetExtension));
    results.sort();
    return results;
}

//==============================================================================
void PresetManager::loadInitialPresetIntoAllSlots()
{
    static const char* initialXml = R"(
<OysterPreset version="1" name="Initial">
  <SlotState position="0.5" spray="0.1" grainSize="200.0"
             density="40.0" pitchScatter="0.0" panSpread="0.5" waveA="0" waveB="1"
             morphAmount="0.0" wtOctave="0" wtSemitone="0" wtFine="0.0"
             wtPhase="0.0" wtTilt="0.0" unisonVoices="1"
             unisonDetune="0.0" unisonSpread="0.5" filterCutoff="8000.0" filterRes="0.7"
             filterType="2" filterDrive="0.0" filterEnvAmt="0.0"
             filterLfoAmt="0.0" filterKeyTrack="0.0" lfo1Rate="1.0"
             lfo1Depth="0.5" lfo1Shape="0" lfo1Attack="0.0" lfo1Decay="0.0"
             lfo1Sync="0" lfo1Retrigger="0" lfo1Phase="0" lfo1Enabled="1"
             lfo2Rate="1.0" lfo2Depth="0.5" lfo2Shape="0" lfo2Attack="0.0"
             lfo2Decay="0.0" lfo2Sync="0" lfo2Retrigger="0" lfo2Phase="0"
             lfo2Enabled="0" lfo3Rate="1.0" lfo3Depth="0.5" lfo3Shape="0"
             lfo3Attack="0.0" lfo3Decay="0.0" lfo3Sync="0" lfo3Retrigger="0"
             lfo3Phase="0" lfo3Enabled="0" lfo4Rate="1.0" lfo4Depth="0.5"
             lfo4Shape="0" lfo4Attack="0.0" lfo4Decay="0.0" lfo4Sync="0" lfo4Retrigger="0"
             lfo4Phase="0" lfo4Enabled="0" envAttack="0.011"
             envDecay="0.101" envSustain="0.8"
             envRelease="0.301" env2Attack="0.011"
             env2Decay="0.101" env2Sustain="0.8"
             env2Release="0.301" env3Attack="0.011"
             env3Decay="0.101" env3Sustain="0.8"
             env3Release="0.301" env4Attack="0.011"
             env4Decay="0.101" env4Sustain="0.8"
             env4Release="0.301" env4Dest="0" osc2Wave="0" osc2Octave="0"
             osc2Semi="0" osc2Fine="0.0" osc2Phase="0.0"
             osc2Mix="0.5" osc2Detune="0.0" osc2Pan="0.5" subWave="0" subOctave="-1"
             subSemi="0" subTune="0.0" subMix="0.5" subPan="0.5"
             subPhase="0.0" subOscEnabled="1" osc2Enabled="1" grainEnabled="1"
             glide="0.0" bendUp="2" bendDown="2" velSens="1.0" transpose="0"
             octaveShift="0" stereoWidth="1.0" masterTune="0.0"
             lfoToCutoff="0.0" lfoToPosition="0.0" lfoToPitch="0.0" lfoToDensity="0.0"
             envToCutoff="0.0" envToPosition="0.0" envToPitch="0.0" envToAmp="0.0"
             masterVolume="0.5" masterPan="0.0" lfoPhaseRuntime="0.0"
             currentLfoValue="0.0" lastMidiNote="60" currentEnvelopeValue="0.0"/>
</OysterPreset>
)";

    auto xml = juce::XmlDocument::parse (initialXml);
    if (xml == nullptr)
        return;

    auto root = juce::ValueTree::fromXml (*xml);
    auto slotTree = root.getChild (0);
    if (! slotTree.isValid())
        return;

    // Apply the initial preset to every slot
    for (int s = 0; s < numSlots; ++s)
        slotStates[s].fromValueTree (slotTree);

    // Slots 2-4 (indices 1-3): turn off all oscillators so only slot 1 is active
    for (int s = 1; s < numSlots; ++s)
    {
        slotStates[s].subOscEnabled = false;
        slotStates[s].osc2Enabled   = false;
        slotStates[s].grainEnabled  = false;
    }

    // Push the active slot into APVTS
    slotStates[activeSlot].writeToAPVTS (apvts);
}

void PresetManager::loadInitialPresetIntoSlot (int slot)
{
    static const char* initialXml = R"(
<OysterPreset version="1" name="Initial">
  <SlotState position="0.5" spray="0.1" grainSize="200.0"
             density="40.0" pitchScatter="0.0" panSpread="0.5" waveA="0" waveB="1"
             morphAmount="0.0" wtOctave="0" wtSemitone="0" wtFine="0.0"
             wtPhase="0.0" wtTilt="0.0" unisonVoices="1"
             unisonDetune="0.0" unisonSpread="0.5" filterCutoff="8000.0" filterRes="0.7"
             filterType="2" filterDrive="0.0" filterEnvAmt="0.0"
             filterLfoAmt="0.0" filterKeyTrack="0.0" lfo1Rate="1.0"
             lfo1Depth="0.5" lfo1Shape="0" lfo1Attack="0.0" lfo1Decay="0.0"
             lfo1Sync="0" lfo1Retrigger="0" lfo1Phase="0" lfo1Enabled="1"
             lfo2Rate="1.0" lfo2Depth="0.5" lfo2Shape="0" lfo2Attack="0.0"
             lfo2Decay="0.0" lfo2Sync="0" lfo2Retrigger="0" lfo2Phase="0"
             lfo2Enabled="0" lfo3Rate="1.0" lfo3Depth="0.5" lfo3Shape="0"
             lfo3Attack="0.0" lfo3Decay="0.0" lfo3Sync="0" lfo3Retrigger="0"
             lfo3Phase="0" lfo3Enabled="0" lfo4Rate="1.0" lfo4Depth="0.5"
             lfo4Shape="0" lfo4Attack="0.0" lfo4Decay="0.0" lfo4Sync="0" lfo4Retrigger="0"
             lfo4Phase="0" lfo4Enabled="0" envAttack="0.011"
             envDecay="0.101" envSustain="0.8"
             envRelease="0.301" env2Attack="0.011"
             env2Decay="0.101" env2Sustain="0.8"
             env2Release="0.301" env3Attack="0.011"
             env3Decay="0.101" env3Sustain="0.8"
             env3Release="0.301" env4Attack="0.011"
             env4Decay="0.101" env4Sustain="0.8"
             env4Release="0.301" env4Dest="0" osc2Wave="0" osc2Octave="0"
             osc2Semi="0" osc2Fine="0.0" osc2Phase="0.0"
             osc2Mix="0.5" osc2Detune="0.0" osc2Pan="0.5" subWave="0" subOctave="-1"
             subSemi="0" subTune="0.0" subMix="0.5" subPan="0.5"
             subPhase="0.0" subOscEnabled="1" osc2Enabled="1" grainEnabled="1"
             glide="0.0" bendUp="2" bendDown="2" velSens="1.0" transpose="0"
             octaveShift="0" stereoWidth="1.0" masterTune="0.0"
             lfoToCutoff="0.0" lfoToPosition="0.0" lfoToPitch="0.0" lfoToDensity="0.0"
             envToCutoff="0.0" envToPosition="0.0" envToPitch="0.0" envToAmp="0.0"
             masterVolume="0.5" masterPan="0.0" lfoPhaseRuntime="0.0"
             currentLfoValue="0.0" lastMidiNote="60" currentEnvelopeValue="0.0"/>
</OysterPreset>
)";

    auto xml = juce::XmlDocument::parse (initialXml);
    if (xml == nullptr) return;

    auto root = juce::ValueTree::fromXml (*xml);
    auto slotTree = root.getChild (0);
    if (! slotTree.isValid()) return;

    slotStates[slot].fromValueTree (slotTree);
    currentPresetName = "Initial";

    if (slot == activeSlot)
        slotStates[activeSlot].writeToAPVTS (apvts);
}
