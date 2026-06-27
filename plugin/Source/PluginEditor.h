#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

// UI minima de debug. Todavia no es la UI circular final -- solo permite
// validar de oido las 4 bandas finales antes de construir la UI real
// (ultimo paso del roadmap, ver docs/arquitectura-codigo.md).
class LaPelotaAl10AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor&);
    ~LaPelotaAl10AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct BandControls
    {
        juce::Label sectionLabel;
        juce::Slider driveSlider;
        juce::Label driveLabel;
        juce::ComboBox typeBox;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    };

    struct CrossoverControls
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    LaPelotaAl10AudioProcessor& processorRef;
    juce::Label titleLabel;

    std::array<CrossoverControls, 3> crossovers;
    std::array<BandControls, 4> bands;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPelotaAl10AudioProcessorEditor)
};
