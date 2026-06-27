#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPelotaAl10AudioProcessor::LaPelotaAl10AudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

LaPelotaAl10AudioProcessor::~LaPelotaAl10AudioProcessor() = default;

void LaPelotaAl10AudioProcessor::prepareToPlay(double, int)
{
    // V0 no tiene estado que preparar -- se va a usar a partir de V1/V2
    // (saturador, oversampling, etc.).
}

void LaPelotaAl10AudioProcessor::releaseResources()
{
}

bool LaPelotaAl10AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet()
           && !layouts.getMainOutputChannelSet().isDisabled();
}

void LaPelotaAl10AudioProcessor::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    // Passthrough deliberado: el buffer de entrada ya es el de salida,
    // no se toca nada todavía.
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

void LaPelotaAl10AudioProcessor::getStateInformation(juce::MemoryBlock&)
{
    // Sin parámetros todavía en V0.
}

void LaPelotaAl10AudioProcessor::setStateInformation(const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPelotaAl10AudioProcessor();
}
