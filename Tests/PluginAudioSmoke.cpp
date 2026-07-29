// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;
    constexpr int blocks = 32;

    BellowsAudioProcessor processor;
    processor.setRateAndBufferSizeDetails(sampleRate, blockSize);
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> audio(2, blockSize);
    double energy = 0.0;
    float peak = 0.0f;

    for (int block = 0; block < blocks; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(110)), 0);

        audio.clear();
        processor.processBlock(audio, midi);

        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        {
            const auto* samples = audio.getReadPointer(channel);
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            {
                peak = std::max(peak, std::abs(samples[sample]));
                energy += static_cast<double>(samples[sample]) * samples[sample];
            }
        }
    }

    const auto rms = std::sqrt(energy / static_cast<double>(blocks * blockSize * 2));
    std::cout << "plugin peak=" << peak << " rms=" << rms << '\n';

    if (! std::isfinite(peak) || ! std::isfinite(rms) || peak < 0.001f || rms < 0.0001)
    {
        std::cerr << "Bellows produced no measurable audio\n";
        return 1;
    }

    return 0;
}
