#include "PluginEditor.h"

LaPelotaAl10AudioProcessorEditor::LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p),
      driveAttachment(p.apvts, "drive", driveSlider)
{
    titleLabel.setText("La Pelota al 10 -- V0 (saturador full-band)", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(driveSlider);

    driveLabel.setText("Drive", juce::NotificationType::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);

    setSize(400, 280);
}

LaPelotaAl10AudioProcessorEditor::~LaPelotaAl10AudioProcessorEditor() = default;

void LaPelotaAl10AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void LaPelotaAl10AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    titleLabel.setBounds(area.removeFromTop(40));
    driveSlider.setBounds(area.removeFromTop(150).reduced(60, 0));
    driveLabel.setBounds(area.removeFromTop(20));
}
