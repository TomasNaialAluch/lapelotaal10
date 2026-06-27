#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

// V1 del saturador: sigue full-band (sin bandas todavia, eso es V2),
// pero ahora el "caracter" es intercambiable entre 4 curvas estaticas.
// Son aproximaciones simples, no modelado fisico (ver
// docs/saturacion-multibanda.md) -- alcanza para empezar a comparar de
// oido antes de invertir en modelos mas costosos.
enum class SaturationType
{
    Warm,
    Tube,
    Diode,
    Tape
};

class Saturator
{
public:
    void prepare(double /*sampleRate*/)
    {
        // Sin estado dependiente de sample rate todavia.
    }

    void setDriveDb(float driveDb)
    {
        driveGain = juce::Decibels::decibelsToGain(driveDb);
    }

    void setType(SaturationType newType)
    {
        type = newType;
    }

    float processSample(float x) const noexcept
    {
        const float g = x * driveGain;

        switch (type)
        {
            case SaturationType::Warm:  return processWarm(g);
            case SaturationType::Tube:  return processTube(g);
            case SaturationType::Diode: return processDiode(g);
            case SaturationType::Tape:  return processTape(g);
        }

        return std::tanh(g);
    }

private:
    // Warm: tanh con menos drive efectivo -- saturacion suave, pocos
    // armonicos, pensada para no agregar "brillo" en graves.
    static float processWarm(float g) noexcept
    {
        return std::tanh(g * 0.6f);
    }

    // Tube: tanh asimetrica (distinta ganancia en el semiciclo positivo
    // vs negativo) -- la asimetria es lo que genera armonicos pares,
    // el rasgo caracteristico de la saturacion a valvula.
    static float processTube(float g) noexcept
    {
        return g >= 0.0f ? std::tanh(g) : 0.85f * std::tanh(g * 1.25f);
    }

    // Diode: clipper exponencial -- rodilla mas dura que la tanh a
    // partir de cierto nivel, mas armonicos altos.
    static float processDiode(float g) noexcept
    {
        const float sign = g >= 0.0f ? 1.0f : -1.0f;
        return sign * (1.0f - std::exp(-std::abs(g)));
    }

    // Tape: soft-clip cubico clasico -- curva distinta a la tanh,
    // aproxima la compresion suave de cinta sin modelar histeresis real.
    static float processTape(float g) noexcept
    {
        const float clipped = juce::jlimit(-1.0f, 1.0f, g);
        return clipped - (clipped * clipped * clipped) / 3.0f;
    }

    float driveGain = 1.0f;
    SaturationType type = SaturationType::Warm;
};
