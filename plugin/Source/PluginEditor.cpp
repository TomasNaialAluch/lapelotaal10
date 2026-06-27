#include "PluginEditor.h"

namespace
{
void setupRotary(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
}
} // namespace

LaPelotaAl10AudioProcessorEditor::LaPelotaAl10AudioProcessorEditor(LaPelotaAl10AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    titleLabel.setText("La Pelota al 10 -- 4 bandas (Warm / Tube / Diode / Tape)",
                        juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    const char* crossoverIds[3] = {"crossover1", "crossover2", "crossover3"};
    const char* crossoverNames[3] = {"250 Hz", "400 Hz", "2000 Hz"};
    for (size_t i = 0; i < crossovers.size(); ++i)
    {
        auto& c = crossovers[i];
        setupRotary(c.slider);
        addAndMakeVisible(c.slider);
        c.label.setText(crossoverNames[i], juce::NotificationType::dontSendNotification);
        c.label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(c.label);
        c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.apvts, crossoverIds[i], c.slider);
    }

    const char* bandIds[4] = {"band1", "band2", "band3", "band4"};
    const char* bandNames[4] = {"BAND 1\n< 250 Hz", "BAND 2\n250-400 Hz", "BAND 3\n400-2000 Hz", "BAND 4\n> 2000 Hz"};
    for (size_t i = 0; i < bands.size(); ++i)
    {
        auto& b = bands[i];

        b.sectionLabel.setText(bandNames[i], juce::NotificationType::dontSendNotification);
        b.sectionLabel.setJustificationType(juce::Justification::centred);
        b.sectionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(b.sectionLabel);

        setupRotary(b.driveSlider);
        addAndMakeVisible(b.driveSlider);

        b.driveLabel.setText("Drive", juce::NotificationType::dontSendNotification);
        b.driveLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(b.driveLabel);

        b.typeBox.addItemList({"Warm", "Tube", "Diode", "Tape"}, 1);
        addAndMakeVisible(b.typeBox);

        b.driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.apvts, juce::String(bandIds[i]) + "Drive", b.driveSlider);
        b.typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            p.apvts, juce::String(bandIds[i]) + "Type", b.typeBox);
    }

    setSize(780, 380);
}

LaPelotaAl10AudioProcessorEditor::~LaPelotaAl10AudioProcessorEditor() = default;

void LaPelotaAl10AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void LaPelotaAl10AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    titleLabel.setBounds(area.removeFromTop(26));
    area.removeFromTop(6);

    auto crossoverRow = area.removeFromTop(110);
    auto crossoverWidth = crossoverRow.getWidth() / (int) crossovers.size();
    for (auto& c : crossovers)
    {
        auto slot = crossoverRow.removeFromLeft(crossoverWidth);
        c.slider.setBounds(slot.removeFromTop(90).withSizeKeepingCentre(80, 90));
        c.label.setBounds(slot);
    }

    area.removeFromTop(10);

    auto bandWidth = area.getWidth() / (int) bands.size();
    for (auto& b : bands)
    {
        auto column = area.removeFromLeft(bandWidth).reduced(8, 0);
        b.sectionLabel.setBounds(column.removeFromTop(36));
        b.driveSlider.setBounds(column.removeFromTop(110).withSizeKeepingCentre(90, 110));
        b.driveLabel.setBounds(column.removeFromTop(18));
        column.removeFromTop(6);
        b.typeBox.setBounds(column.removeFromTop(26));
    }
}
