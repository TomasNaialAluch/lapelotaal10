#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout LaPelotaAl10AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1},
        "Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return {params.begin(), params.end()};
}

LaPelotaAl10AudioProcessor::LaPelotaAl10AudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    driveParam = apvts.getRawParameterValue("drive");
}

LaPelotaAl10AudioProcessor::~LaPelotaAl10AudioProcessor() = default;

void LaPelotaAl10AudioProcessor::prepareToPlay(double sampleRate, int)
{
    for (auto& sat : saturators)
        sat.prepare(sampleRate);
}

void LaPelotaAl10AudioProcessor::releaseResources()
{
}

bool LaPelotaAl10AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet()
           && !layouts.getMainOutputChannelSet().isDisabled();
}

void LaPelotaAl10AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const float driveDb = driveParam != nullptr ? driveParam->load() : 0.0f;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels && channel < (int) saturators.size(); ++channel)
    {
        auto& sat = saturators[(size_t) channel];
        sat.setDriveDb(driveDb);

        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
            data[i] = sat.processSample(data[i]);
    }
}

juce::AudioProcessorEditor* LaPelotaAl10AudioProcessor::createEditor()
{
    return new LaPelotaAl10AudioProcessorEditor(*this);
}

bool LaPelotaAl10AudioProcessor::hasEditor() const
{
    return true;
}

const juce::String LaPelotaAl10AudioProcessor::getName() const
{
    return "La Pelota al 10";
}

bool LaPelotaAl10AudioProcessor::acceptsMidi() const
{
    return false;
}

bool LaPelotaAl10AudioProcessor::producesMidi() const
{
    return false;
}

double LaPelotaAl10AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LaPelotaAl10AudioProcessor::getNumPrograms()
{
    return 1;
}

int LaPelotaAl10AudioProcessor::getCurrentProgram()
{
    return 0;
}

void LaPelotaAl10AudioProcessor::setCurrentProgram(int)
{
}

const juce::String LaPelotaAl10AudioProcessor::getProgramName(int)
{
    return {};
}

void LaPelotaAl10AudioProcessor::changeProgramName(int, const juce::String&)
{
}

void LaPelotaAl10AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void LaPelotaAl10AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPelotaAl10AudioProcessor();
}
