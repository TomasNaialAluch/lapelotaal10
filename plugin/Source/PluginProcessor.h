#pragma once

#include "DSP/BandSplitter.h"
#include "DSP/Saturator.h"
#include <juce_audio_processors/juce_audio_processors.h>

// 4 bandas finales -- el mismo Saturator de V1, instanciado una vez por
// banda (x canal), con un FourBandSplitter Linkwitz-Riley separando la
// senal en los cortes definidos en el README (250/400/2000 Hz).
// Ver docs/arquitectura-codigo.md.
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

    static constexpr int numBands = 4;
    static constexpr int numChannelsHandled = 2; // estereo

    std::array<std::atomic<float>*, 3> crossoverParams{};       // 250/400/2000 Hz
    std::array<std::atomic<float>*, numBands> driveParams{};    // por banda
    std::array<juce::AudioParameterChoice*, numBands> typeParams{};

    std::array<FourBandSplitter, numChannelsHandled> splitters;
    std::array<std::array<Saturator, numBands>, numChannelsHandled> saturators;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPelotaAl10AudioProcessor)
};
