#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout LaPelotaAl10AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"crossover", 1},
        "Crossover",
        juce::NormalisableRange<float>(80.0f, 2000.0f, 1.0f, 0.4f), // skew: mas resolucion en graves
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lowDrive", 1},
        "Low Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lowType", 1},
        "Low Type",
        juce::StringArray{"Warm", "Tube", "Diode", "Tape"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"highDrive", 1},
        "High Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"highType", 1},
        "High Type",
        juce::StringArray{"Warm", "Tube", "Diode", "Tape"},
        3)); // default Tape, coherente con la tabla de bandas del README

    return {params.begin(), params.end()};
}

LaPelotaAl10AudioProcessor::LaPelotaAl10AudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    crossoverParam = apvts.getRawParameterValue("crossover");
    lowDriveParam = apvts.getRawParameterValue("lowDrive");
    highDriveParam = apvts.getRawParameterValue("highDrive");
    lowTypeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("lowType"));
    highTypeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("highType"));
}

LaPelotaAl10AudioProcessor::~LaPelotaAl10AudioProcessor() = default;

void LaPelotaAl10AudioProcessor::prepareToPlay(double sampleRate, int)
{
    for (auto& splitter : splitters)
        splitter.prepare(sampleRate);

    for (auto& sat : lowSaturators)
        sat.prepare(sampleRate);

    for (auto& sat : highSaturators)
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
    const float crossoverHz = crossoverParam != nullptr ? crossoverParam->load() : 250.0f;
    const float lowDriveDb = lowDriveParam != nullptr ? lowDriveParam->load() : 0.0f;
    const float highDriveDb = highDriveParam != nullptr ? highDriveParam->load() : 0.0f;
    const auto lowType = lowTypeParam != nullptr
                              ? static_cast<SaturationType>(lowTypeParam->getIndex())
                              : SaturationType::Warm;
    const auto highType = highTypeParam != nullptr
                               ? static_cast<SaturationType>(highTypeParam->getIndex())
                               : SaturationType::Tape;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels && channel < numChannelsHandled; ++channel)
    {
        auto& splitter = splitters[(size_t) channel];
        auto& lowSat = lowSaturators[(size_t) channel];
        auto& highSat = highSaturators[(size_t) channel];

        splitter.setCrossoverFrequency(crossoverHz);
        lowSat.setDriveDb(lowDriveDb);
        lowSat.setType(lowType);
        highSat.setDriveDb(highDriveDb);
        highSat.setType(highType);

        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            float low = 0.0f;
            float high = 0.0f;
            splitter.processSample(data[i], low, high);

            data[i] = lowSat.processSample(low) + highSat.processSample(high);
        }
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
