// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class BellowsAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BellowsAudioProcessorEditor(BellowsAudioProcessor&);
    ~BellowsAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    BellowsAudioProcessor& processor;
    juce::Label title;
    juce::Label subtitle;
    juce::MidiKeyboardComponent keyboard;

    std::array<juce::ToggleButton, 4> registerButtons;
    std::array<std::unique_ptr<ButtonAttachment>, 4> registerAttachments;
    std::array<Knob, 11> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BellowsAudioProcessorEditor)
};
