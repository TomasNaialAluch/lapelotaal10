#pragma once

#include <juce_dsp/juce_dsp.h>

// V2: crossover Linkwitz-Riley de 2 bandas (24 dB/oct, dos Butterworth
// de 2do orden en cascada por banda). Suma plana en magnitud SIN
// saturacion -- una vez que cada banda satura distinto, validar de
// oido/espectro que la suma sigue siendo coherente (ver
// docs/saturacion-multibanda.md, seccion 1).
//
// Arranca en 2 bandas a proposito, no las 4 finales -- ver
// docs/arquitectura-codigo.md, paso V2.
class LinkwitzRileyBandSplitter
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        updateCoefficients();

        for (auto* f : {&lowStage1, &lowStage2, &highStage1, &highStage2})
            f->reset();
    }

    void setCrossoverFrequency(float newFrequencyHz)
    {
        if (frequencyHz == newFrequencyHz)
            return;

        frequencyHz = newFrequencyHz;
        updateCoefficients();
    }

    void processSample(float input, float& low, float& high) noexcept
    {
        low = lowStage2.processSample(lowStage1.processSample(input));
        high = highStage2.processSample(highStage1.processSample(input));
    }

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;

        *lowStage1.coefficients = *Coeffs::makeLowPass(sampleRate, frequencyHz);
        *lowStage2.coefficients = *Coeffs::makeLowPass(sampleRate, frequencyHz);
        *highStage1.coefficients = *Coeffs::makeHighPass(sampleRate, frequencyHz);
        *highStage2.coefficients = *Coeffs::makeHighPass(sampleRate, frequencyHz);
    }

    Filter lowStage1, lowStage2;
    Filter highStage1, highStage2;

    double sampleRate = 44100.0;
    float frequencyHz = 250.0f;
};
