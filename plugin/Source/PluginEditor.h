#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

// UI minima de debug. Todavia no es la UI circular final -- solo permite
// validar de oido el splitter de 2 bandas (V2) antes de construir la UI
// real (ultimo paso del roadmap, ver docs/arquitectura-codigo.md).
class LaPelotaAl10AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor&);
    ~LaPelotaAl10AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    LaPelotaAl10AudioProcessor& processorRef;

    juce::Label titleLabel;

    juce::Slider crossoverSlider;
    juce::Label crossoverLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment crossoverAttachment;

    juce::Label lowSectionLabel;
    juce::Slider lowDriveSlider;
    juce::Label lowDriveLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment lowDriveAttachment;
    juce::ComboBox lowTypeBox;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment lowTypeAttachment;

    juce::Label highSectionLabel;
    juce::Slider highDriveSlider;
    juce::Label highDriveLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment highDriveAttachment;
    juce::ComboBox highTypeBox;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment highTypeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPelotaAl10AudioProcessorEditor)
};
