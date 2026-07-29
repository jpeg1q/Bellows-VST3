// SPDX-License-Identifier: AGPL-3.0-or-later

#include "AccordionVoice.h"

namespace bellows
{
AccordionVoice::AccordionVoice(VoiceParameters parameters)
    : params(parameters)
{
}

bool AccordionVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<AccordionSound*>(sound) != nullptr;
}

void AccordionVoice::startNote(int midiNoteNumber, float velocity,
                               juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    activeMidiNote = midiNoteNumber;
    oscillator.prepare(getSampleRate());
    oscillator.startNote(midiNoteNumber, velocity,
                         static_cast<std::uint32_t>(random.nextInt()));
    pitchWheelMoved(currentPitchWheelPosition);
    updateEnvelope();
    envelope.reset();
    envelope.noteOn();
    clickEnvelope = 1.0f;
    clickDecay = static_cast<float>(std::exp(-1.0 / (0.012 * getSampleRate())));
}

void AccordionVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        envelope.noteOff();
    }
    else
    {
        envelope.reset();
        clearCurrentNote();
    }
}

void AccordionVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchWheelSemitones = juce::jmap(static_cast<float>(newPitchWheelValue),
                                    0.0f, 16383.0f, -2.0f, 2.0f);
}

void AccordionVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    if (controllerNumber == 11)
        expression = juce::jlimit(0.0f, 1.0f, static_cast<float>(newControllerValue) / 127.0f);
}

void AccordionVoice::updateEnvelope()
{
    envelopeParameters.attack = juce::jlimit(0.002f, 1.0f, params.attack->load());
    envelopeParameters.decay = 0.05f;
    envelopeParameters.sustain = 1.0f;
    envelopeParameters.release = juce::jlimit(0.02f, 3.0f, params.release->load());
    envelope.setSampleRate(getSampleRate());
    envelope.setParameters(envelopeParameters);
}

void AccordionVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                     int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    updateEnvelope();

    ReedSettings settings;
    settings.low = params.low->load() >= 0.5f;
    settings.middle = params.middle->load() >= 0.5f;
    settings.musette = params.musette->load() >= 0.5f;
    settings.high = params.high->load() >= 0.5f;
    settings.musetteWidthCents = params.musetteWidth->load();
    settings.brightness = params.brightness->load();
    settings.reedVariation = params.variation->load();

    // Pitch-wheel support is intentionally quantized to a note restart in v0.1.
    // This avoids phase-discontinuous pitch mutation inside the dependency-free core.
    juce::ignoreUnused(pitchWheelSemitones, activeMidiNote);

    const auto bellowsParameter = params.bellows->load();
    const auto pressureTarget = juce::jlimit(0.0f, 1.0f, bellowsParameter * expression);
    const auto clickLevel = params.mechanical->load();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto env = envelope.getNextSample();
        auto [left, right] = oscillator.process(pressureTarget, settings);

        if (clickEnvelope > 0.0001f)
        {
            const auto click = (random.nextFloat() * 2.0f - 1.0f) * clickEnvelope * clickLevel * 0.10f;
            left += click;
            right += click * 0.82f;
            clickEnvelope *= clickDecay;
        }

        left *= env;
        right *= env;

        outputBuffer.addSample(0, startSample + sample, left);
        if (outputBuffer.getNumChannels() > 1)
            outputBuffer.addSample(1, startSample + sample, right);

        if (! envelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}
} // namespace bellows
