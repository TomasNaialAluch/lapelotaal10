#include "PluginEditor.h"

LaPelotaAl10AudioProcessorEditor::LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    titleLabel.setText("La Pelota al 10 -- V0 (passthrough)", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    setSize(400, 200);
}

LaPelotaAl10AudioProcessorEditor::~LaPelotaAl10AudioProcessorEditor() = default;

void LaPelotaAl10AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void LaPelotaAl10AudioProcessorEditor::resized()
{
    titleLabel.setBounds(getLocalBounds());
}
