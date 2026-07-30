// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/ParameterIDs.h"

BellowsAudioProcessor::BellowsAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    bellows::VoiceParameters voiceParams {
        parameters.getRawParameterValue(bellows::param::low),
        parameters.getRawParameterValue(bellows::param::middle),
        parameters.getRawParameterValue(bellows::param::musette),
        parameters.getRawParameterValue(bellows::param::high),
        parameters.getRawParameterValue(bellows::param::bellows),
        parameters.getRawParameterValue(bellows::param::attack),
        parameters.getRawParameterValue(bellows::param::release),
        parameters.getRawParameterValue(bellows::param::brightness),
        parameters.getRawParameterValue(bellows::param::musetteWidth),
        parameters.getRawParameterValue(bellows::param::variation),
        parameters.getRawParameterValue(bellows::param::mechanical)
    };

    for (int i = 0; i < 16; ++i)
        synth.addVoice(new bellows::AccordionVoice(voiceParams));

    synth.addSound(new bellows::AccordionSound());
}

void BellowsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dspPrepared.store(false, std::memory_order_release);

    if (sampleRate <= 0.0 || samplesPerBlock <= 0)
        return;

    synth.setCurrentPlaybackSampleRate(sampleRate);

    const juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32>(samplesPerBlock),
        2
    };

    reverb.reset();
    reverb.prepare(spec);
    highPass.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 34.0);
    highPass.prepare(spec);
    highPass.reset();
    bodyFilterLeft.reset();
    bodyFilterRight.reset();
    bodyFilterLeft.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 310.0, 0.75f, 1.0f);
    bodyFilterRight.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 310.0, 0.75f, 1.0f);
    dspPrepared.store(true, std::memory_order_release);
}

void BellowsAudioProcessor::releaseResources()
{
    dspPrepared.store(false, std::memory_order_release);
}

bool BellowsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::stereo();
}

void BellowsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (buffer.getNumSamples() <= 0 || buffer.getNumChannels() <= 0)
        return;

    if (! dspPrepared.load(std::memory_order_acquire))
        return;

    keyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    const auto airAmount = parameters.getRawParameterValue(bellows::param::air)->load();
    const auto pressure = parameters.getRawParameterValue(bellows::param::bellows)->load();
    const auto bodyAmount = parameters.getRawParameterValue(bellows::param::body)->load();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto white = noise.nextFloat() * 2.0f - 1.0f;
        airNoiseState += 0.035f * (white - airNoiseState);
        const auto air = airNoiseState * airAmount * pressure * 0.018f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addSample(channel, sample, air);
    }

    if (buffer.getNumChannels() > 0)
    {
        auto* left = buffer.getWritePointer(0);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto dry = left[sample];
            const auto resonant = bodyFilterLeft.processSample(dry);
            left[sample] = juce::jmap(bodyAmount, dry, resonant);
        }
    }

    if (buffer.getNumChannels() > 1)
    {
        auto* right = buffer.getWritePointer(1);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto dry = right[sample];
            const auto resonant = bodyFilterRight.processSample(dry);
            right[sample] = juce::jmap(bodyAmount, dry, resonant);
        }
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    highPass.process(context);

    juce::dsp::Reverb::Parameters room;
    const auto roomAmount = parameters.getRawParameterValue(bellows::param::room)->load();
    room.roomSize = 0.18f + roomAmount * 0.48f;
    room.damping = 0.58f;
    room.wetLevel = roomAmount * 0.34f;
    room.dryLevel = 1.0f;
    room.width = 0.82f;
    room.freezeMode = 0.0f;
    reverb.setParameters(room);
    reverb.process(context);

    const auto outputDb = parameters.getRawParameterValue(bellows::param::output)->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(outputDb));

    // Gentle safety saturation for dense chords; this is not intended as an audible distortion stage.
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            samples[sample] = std::tanh(samples[sample]);
    }
}

juce::AudioProcessorEditor* BellowsAudioProcessor::createEditor()
{
    return new BellowsAudioProcessorEditor(*this);
}

void BellowsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void BellowsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout BellowsAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using Range = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<APB>(juce::ParameterID { bellows::param::low, 1 }, "Low 16'", true));
    layout.add(std::make_unique<APB>(juce::ParameterID { bellows::param::middle, 1 }, "Middle 8'", true));
    layout.add(std::make_unique<APB>(juce::ParameterID { bellows::param::musette, 1 }, "Musette", true));
    layout.add(std::make_unique<APB>(juce::ParameterID { bellows::param::high, 1 }, "High 4'", false));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::bellows, 1 }, "Bellows", Range { 0.0f, 1.0f, 0.001f }, 0.78f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::attack, 1 }, "Attack", Range { 0.002f, 0.55f, 0.001f, 0.35f }, 0.025f, "s"));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::release, 1 }, "Release", Range { 0.02f, 2.5f, 0.001f, 0.38f }, 0.22f, "s"));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::brightness, 1 }, "Brightness", Range { 0.0f, 1.0f, 0.001f }, 0.68f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::musetteWidth, 1 }, "Musette Width", Range { 0.0f, 35.0f, 0.1f }, 12.0f, " cents"));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::variation, 1 }, "Reed Variation", Range { 0.0f, 1.0f, 0.001f }, 0.28f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::mechanical, 1 }, "Mechanical", Range { 0.0f, 1.0f, 0.001f }, 0.22f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::air, 1 }, "Air", Range { 0.0f, 1.0f, 0.001f }, 0.18f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::body, 1 }, "Body", Range { 0.0f, 1.0f, 0.001f }, 0.35f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::room, 1 }, "Room", Range { 0.0f, 1.0f, 0.001f }, 0.14f));
    layout.add(std::make_unique<APF>(juce::ParameterID { bellows::param::output, 1 }, "Output", Range { -24.0f, 6.0f, 0.1f }, -3.0f, " dB"));
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BellowsAudioProcessor();
}
