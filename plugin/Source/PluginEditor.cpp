#include "PluginEditor.h"

namespace
{
void setupRotary(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
}
} // namespace

LaPelotaAl10AudioProcessorEditor::LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p),
      crossoverAttachment(p.apvts, "crossover", crossoverSlider),
      lowDriveAttachment(p.apvts, "lowDrive", lowDriveSlider),
      lowTypeAttachment(p.apvts, "lowType", lowTypeBox),
      highDriveAttachment(p.apvts, "highDrive", highDriveSlider),
      highTypeAttachment(p.apvts, "highType", highTypeBox)
{
    titleLabel.setText("La Pelota al 10 -- V2 (2 bandas, Linkwitz-Riley)",
                        juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    setupRotary(crossoverSlider);
    addAndMakeVisible(crossoverSlider);
    crossoverLabel.setText("Crossover", juce::NotificationType::dontSendNotification);
    crossoverLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(crossoverLabel);

    lowSectionLabel.setText("LOW BAND", juce::NotificationType::dontSendNotification);
    lowSectionLabel.setJustificationType(juce::Justification::centred);
    lowSectionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(lowSectionLabel);

    setupRotary(lowDriveSlider);
    addAndMakeVisible(lowDriveSlider);
    lowDriveLabel.setText("Drive", juce::NotificationType::dontSendNotification);
    lowDriveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lowDriveLabel);
    lowTypeBox.addItemList({"Warm", "Tube", "Diode", "Tape"}, 1);
    addAndMakeVisible(lowTypeBox);

    highSectionLabel.setText("HIGH BAND", juce::NotificationType::dontSendNotification);
    highSectionLabel.setJustificationType(juce::Justification::centred);
    highSectionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(highSectionLabel);

    setupRotary(highDriveSlider);
    addAndMakeVisible(highDriveSlider);
    highDriveLabel.setText("Drive", juce::NotificationType::dontSendNotification);
    highDriveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(highDriveLabel);
    highTypeBox.addItemList({"Warm", "Tube", "Diode", "Tape"}, 1);
    addAndMakeVisible(highTypeBox);

    setSize(520, 400);
}

LaPelotaAl10AudioProcessorEditor::~LaPelotaAl10AudioProcessorEditor() = default;

void LaPelotaAl10AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void LaPelotaAl10AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    titleLabel.setBounds(area.removeFromTop(30));

    crossoverSlider.setBounds(area.removeFromTop(110).withSizeKeepingCentre(100, 110));
    crossoverLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);

    auto columns = area;
    auto lowColumn = columns.removeFromLeft(columns.getWidth() / 2).reduced(10, 0);
    auto highColumn = columns.reduced(10, 0);

    lowSectionLabel.setBounds(lowColumn.removeFromTop(24));
    lowDriveSlider.setBounds(lowColumn.removeFromTop(120).withSizeKeepingCentre(100, 120));
    lowDriveLabel.setBounds(lowColumn.removeFromTop(20));
    lowColumn.removeFromTop(8);
    lowTypeBox.setBounds(lowColumn.removeFromTop(28));

    highSectionLabel.setBounds(highColumn.removeFromTop(24));
    highDriveSlider.setBounds(highColumn.removeFromTop(120).withSizeKeepingCentre(100, 120));
    highDriveLabel.setBounds(highColumn.removeFromTop(20));
    highColumn.removeFromTop(8);
    highTypeBox.setBounds(highColumn.removeFromTop(28));
}
