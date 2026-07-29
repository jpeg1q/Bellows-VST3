// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <JuceHeader.h>
#include "DSP/AccordionVoice.h"

class BellowsAudioProcessor final : public juce::AudioProcessor
{
public:
    BellowsAudioProcessor();
    ~BellowsAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;

private:
    juce::Synthesiser synth;
    juce::dsp::Reverb reverb;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> highPass;
    juce::dsp::IIR::Filter<float> bodyFilterLeft;
    juce::dsp::IIR::Filter<float> bodyFilterRight;
    juce::Random noise;
    float airNoiseState = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BellowsAudioProcessor)
};
