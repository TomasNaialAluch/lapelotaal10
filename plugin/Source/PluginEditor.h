#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

// UI mínima de debug (paso 1 del roadmap). Todavía no es la UI circular
// final -- solo confirma que el editor carga dentro de un DAW real.
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPelotaAl10AudioProcessorEditor)
};
