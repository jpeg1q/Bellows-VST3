// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <JuceHeader.h>
#include "ReedOscillator.h"

namespace bellows
{
struct VoiceParameters
{
    std::atomic<float>* low {};
    std::atomic<float>* middle {};
    std::atomic<float>* musette {};
    std::atomic<float>* high {};
    std::atomic<float>* bellows {};
    std::atomic<float>* attack {};
    std::atomic<float>* release {};
    std::atomic<float>* brightness {};
    std::atomic<float>* musetteWidth {};
    std::atomic<float>* variation {};
    std::atomic<float>* mechanical {};
};

class AccordionSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class AccordionVoice final : public juce::SynthesiserVoice
{
public:
    explicit AccordionVoice(VoiceParameters parameters);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;

private:
    void updateEnvelope();

    VoiceParameters params;
    ReedOscillator oscillator;
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParameters;
    juce::Random random;
    float expression = 1.0f;
    float pitchWheelSemitones = 0.0f;
    float clickEnvelope = 0.0f;
    float clickDecay = 0.99f;
    int activeMidiNote = 60;
};
} // namespace bellows
