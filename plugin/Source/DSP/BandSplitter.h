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

// 4 bandas finales (ver tabla de saturacion del README): se construyen
// encadenando 3 splits de 2 bandas, reusando LinkwitzRileyBandSplitter
// sin modificarlo -- mismo patron de "agregar splitter alrededor, no
// reescribir la pieza" que describe docs/arquitectura-codigo.md.
class FourBandSplitter
{
public:
    void prepare(double sampleRate)
    {
        splitAt250.prepare(sampleRate);
        splitAt400.prepare(sampleRate);
        splitAt2000.prepare(sampleRate);
    }

    void setCrossovers(float freq1, float freq2, float freq3)
    {
        splitAt250.setCrossoverFrequency(freq1);
        splitAt400.setCrossoverFrequency(freq2);
        splitAt2000.setCrossoverFrequency(freq3);
    }

    // band1: < freq1 ("Warm")      band2: freq1-freq2 ("Tube")
    // band3: freq2-freq3 ("Diode") band4: > freq3 ("Tape")
    void processSample(float input, float& band1, float& band2, float& band3, float& band4) noexcept
    {
        float low1, rest1;
        splitAt250.processSample(input, low1, rest1);
        band1 = low1;

        float low2, rest2;
        splitAt400.processSample(rest1, low2, rest2);
        band2 = low2;

        float low3, high3;
        splitAt2000.processSample(rest2, low3, high3);
        band3 = low3;
        band4 = high3;
    }

private:
    LinkwitzRileyBandSplitter splitAt250;
    LinkwitzRileyBandSplitter splitAt400;
    LinkwitzRileyBandSplitter splitAt2000;
};
