#include "PluginEditor.h"

LaPelotaAl10AudioProcessorEditor::LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p),
      driveAttachment(p.apvts, "drive", driveSlider),
      typeAttachment(p.apvts, "type", typeBox)
{
    titleLabel.setText("La Pelota al 10 -- V1 (saturador full-band, 4 tipos)", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(driveSlider);

    driveLabel.setText("Drive", juce::NotificationType::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);

    typeBox.addItemList({"Warm", "Tube", "Diode", "Tape"}, 1);
    addAndMakeVisible(typeBox);

    typeLabel.setText("Type", juce::NotificationType::dontSendNotification);
    typeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(typeLabel);

    setSize(400, 340);
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
    area.removeFromTop(10);
    typeBox.setBounds(area.removeFromTop(30).reduced(100, 0));
    typeLabel.setBounds(area.removeFromTop(20));
}
