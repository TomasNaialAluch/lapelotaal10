#pragma once

#include "DSP/BandSplitter.h"
#include "DSP/Saturator.h"
#include <juce_audio_processors/juce_audio_processors.h>

// V2: multibanda real (2 bandas) -- el mismo Saturator de V1, instanciado
// una vez por banda, con un BandSplitter Linkwitz-Riley separando la
// senal. Ver docs/arquitectura-codigo.md, paso V2.
class LaPelotaAl10AudioProcessor : public juce::AudioProcessor
{
public:
    LaPelotaAl10AudioProcessor();
    ~LaPelotaAl10AudioProcessor() override;

    juce::AudioProcessorValueTreeState apvts;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float>* crossoverParam = nullptr;
    std::atomic<float>* lowDriveParam = nullptr;
    std::atomic<float>* highDriveParam = nullptr;
    juce::AudioParameterChoice* lowTypeParam = nullptr;
    juce::AudioParameterChoice* highTypeParam = nullptr;

    static constexpr int numChannelsHandled = 2; // estereo

    std::array<LinkwitzRileyBandSplitter, numChannelsHandled> splitters;
    std::array<Saturator, numChannelsHandled> lowSaturators;
    std::array<Saturator, numChannelsHandled> highSaturators;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPelotaAl10AudioProcessor)
};
