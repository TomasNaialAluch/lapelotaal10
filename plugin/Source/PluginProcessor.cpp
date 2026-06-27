#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout LaPelotaAl10AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Cortes de banda, coincidiendo con la tabla del README.
    struct CrossoverDef { const char* id; const char* name; float defaultHz; };
    constexpr CrossoverDef crossoverDefs[] = {
        {"crossover1", "Crossover 1", 250.0f},
        {"crossover2", "Crossover 2", 400.0f},
        {"crossover3", "Crossover 3", 2000.0f},
    };
    for (auto& def : crossoverDefs)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{def.id, 1},
            def.name,
            juce::NormalisableRange<float>(40.0f, 18000.0f, 1.0f, 0.3f),
            def.defaultHz,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    // Bandas: < 250 (Warm), 250-400 (Tube), 400-2000 (Diode), > 2000 (Tape).
    struct BandDef { const char* idPrefix; const char* namePrefix; int defaultType; };
    constexpr BandDef bandDefs[] = {
        {"band1", "Band 1 (Low)", 0},     // Warm
        {"band2", "Band 2 (Low-Mid)", 1}, // Tube
        {"band3", "Band 3 (High-Mid)", 2},// Diode
        {"band4", "Band 4 (High)", 3},    // Tape
    };
    for (auto& def : bandDefs)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{juce::String(def.idPrefix) + "Drive", 1},
            juce::String(def.namePrefix) + " Drive",
            juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{juce::String(def.idPrefix) + "Type", 1},
            juce::String(def.namePrefix) + " Type",
            juce::StringArray{"Warm", "Tube", "Diode", "Tape"},
            def.defaultType));
    }

    return {params.begin(), params.end()};
}

LaPelotaAl10AudioProcessor::LaPelotaAl10AudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    crossoverParams[0] = apvts.getRawParameterValue("crossover1");
    crossoverParams[1] = apvts.getRawParameterValue("crossover2");
    crossoverParams[2] = apvts.getRawParameterValue("crossover3");

    const char* bandIds[numBands] = {"band1", "band2", "band3", "band4"};
    for (int b = 0; b < numBands; ++b)
    {
        driveParams[(size_t) b] = apvts.getRawParameterValue(juce::String(bandIds[b]) + "Drive");
        typeParams[(size_t) b] = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(juce::String(bandIds[b]) + "Type"));
    }
}

LaPelotaAl10AudioProcessor::~LaPelotaAl10AudioProcessor() = default;

void LaPelotaAl10AudioProcessor::prepareToPlay(double sampleRate, int)
{
    for (auto& splitter : splitters)
        splitter.prepare(sampleRate);

    for (auto& channelSaturators : saturators)
        for (auto& sat : channelSaturators)
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
    // Los 3 cortes deben quedar ordenados (crossover1 < crossover2 < crossover3)
    // para que las 4 bandas tengan sentido -- se fuerza con jlimit por las dudas
    // de que el usuario cruce los valores entre si desde la UI de debug.
    float c1 = crossoverParams[0] != nullptr ? crossoverParams[0]->load() : 250.0f;
    float c2 = crossoverParams[1] != nullptr ? crossoverParams[1]->load() : 400.0f;
    float c3 = crossoverParams[2] != nullptr ? crossoverParams[2]->load() : 2000.0f;
    c2 = juce::jmax(c2, c1 + 1.0f);
    c3 = juce::jmax(c3, c2 + 1.0f);

    std::array<float, numBands> driveDb{};
    std::array<SaturationType, numBands> types{};
    for (int b = 0; b < numBands; ++b)
    {
        driveDb[(size_t) b] = driveParams[(size_t) b] != nullptr ? driveParams[(size_t) b]->load() : 0.0f;
        types[(size_t) b] = typeParams[(size_t) b] != nullptr
                                 ? static_cast<SaturationType>(typeParams[(size_t) b]->getIndex())
                                 : SaturationType::Warm;
    }

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels && channel < numChannelsHandled; ++channel)
    {
        auto& splitter = splitters[(size_t) channel];
        auto& channelSaturators = saturators[(size_t) channel];

        splitter.setCrossovers(c1, c2, c3);
        for (int b = 0; b < numBands; ++b)
        {
            channelSaturators[(size_t) b].setDriveDb(driveDb[(size_t) b]);
            channelSaturators[(size_t) b].setType(types[(size_t) b]);
        }

        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            float band1 = 0.0f, band2 = 0.0f, band3 = 0.0f, band4 = 0.0f;
            splitter.processSample(data[i], band1, band2, band3, band4);

            data[i] = channelSaturators[0].processSample(band1)
                    + channelSaturators[1].processSample(band2)
                    + channelSaturators[2].processSample(band3)
                    + channelSaturators[3].processSample(band4);
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
