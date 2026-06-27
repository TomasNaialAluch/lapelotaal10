#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

// V0 del saturador: full-band, una sola curva (tanh), sin tipos de
// caracter intercambiables todavia (eso es V1) y sin bandas (eso es V2).
// A proposito minimo -- ver docs/arquitectura-codigo.md.
class Saturator
{
public:
    void prepare(double /*sampleRate*/)
    {
        // Sin estado dependiente de sample rate en V0.
    }

    void setDriveDb(float driveDb)
    {
        driveGain = juce::Decibels::decibelsToGain(driveDb);
    }

    float processSample(float x) const noexcept
    {
        return std::tanh(x * driveGain);
    }

private:
    float driveGain = 1.0f;
};
