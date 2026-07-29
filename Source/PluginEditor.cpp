// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginEditor.h"
#include "DSP/ParameterIDs.h"

namespace
{
constexpr std::array<const char*, 4> registerIds {
    bellows::param::low, bellows::param::middle, bellows::param::musette, bellows::param::high
};
constexpr std::array<const char*, 4> registerNames { "LOW 16'", "MIDDLE 8'", "MUSETTE", "HIGH 4'" };
constexpr std::array<const char*, 11> knobIds {
    bellows::param::bellows, bellows::param::brightness, bellows::param::musetteWidth,
    bellows::param::variation, bellows::param::attack, bellows::param::release,
    bellows::param::mechanical, bellows::param::air, bellows::param::body,
    bellows::param::room, bellows::param::output
};
constexpr std::array<const char*, 11> knobNames {
    "BELLOWS", "BRIGHTNESS", "MUSETTE", "VARIATION", "ATTACK", "RELEASE",
    "KEY NOISE", "AIR", "BODY", "ROOM", "OUTPUT"
};
}

BellowsAudioProcessorEditor::BellowsAudioProcessorEditor(BellowsAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      keyboard(processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize(920, 610);
    setResizable(true, true);
    setResizeLimits(760, 520, 1280, 850);

    title.setText("BELLOWS", juce::dontSendNotification);
    title.setFont(juce::FontOptions(34.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centredLeft);
    title.setColour(juce::Label::textColourId, juce::Colour(0xfff3dfb4));
    addAndMakeVisible(title);

    subtitle.setText("expressive free-reed instrument  â€¢  CC11 controls bellows pressure",
                     juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(14.0f));
    subtitle.setJustificationType(juce::Justification::centredLeft);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(0xffc4b8a4));
    addAndMakeVisible(subtitle);

    for (std::size_t i = 0; i < registerButtons.size(); ++i)
    {
        auto& button = registerButtons[i];
        button.setButtonText(registerNames[i]);
        addAndMakeVisible(button);
        registerAttachments[i] = std::make_unique<ButtonAttachment>(processor.parameters, registerIds[i], button);
    }

    for (std::size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 20);
        knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffd5a448));
        knob.slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff4dfb1));
        knob.label.setText(knobNames[i], juce::dontSendNotification);
        knob.label.setJustificationType(juce::Justification::centred);
        knob.label.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        knob.label.setColour(juce::Label::textColourId, juce::Colour(0xffe6d7bd));
        addAndMakeVisible(knob.slider);
        addAndMakeVisible(knob.label);
        knob.attachment = std::make_unique<SliderAttachment>(processor.parameters, knobIds[i], knob.slider);
    }

    addAndMakeVisible(keyboard);
}

void BellowsAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(0xff181512));

    juce::ColourGradient gradient(juce::Colour(0xff3b2719), 0.0f, 0.0f,
                                  juce::Colour(0xff161313), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.reduced(10.0f), 16.0f);

    g.setColour(juce::Colour(0xffd5a448).withAlpha(0.45f));
    g.drawRoundedRectangle(bounds.reduced(10.0f), 16.0f, 1.5f);

    g.setColour(juce::Colour(0xff9d2f24).withAlpha(0.35f));
    const auto bellowsArea = bounds.withTrimmedTop(90.0f).withTrimmedBottom(145.0f).reduced(22.0f);
    for (int x = static_cast<int>(bellowsArea.getX()); x < static_cast<int>(bellowsArea.getRight()); x += 18)
        g.drawLine(static_cast<float>(x), bellowsArea.getY(), static_cast<float>(x + 28), bellowsArea.getBottom(), 1.0f);
}

void BellowsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);
    auto header = area.removeFromTop(64);
    title.setBounds(header.removeFromLeft(245));
    subtitle.setBounds(header);

    area.removeFromTop(10);
    auto registers = area.removeFromTop(48);
    const auto registerWidth = registers.getWidth() / static_cast<int>(registerButtons.size());
    for (auto& button : registerButtons)
        button.setBounds(registers.removeFromLeft(registerWidth).reduced(5));

    area.removeFromTop(14);
    auto knobArea = area.removeFromTop(300);
    constexpr int columns = 6;
    const int cellWidth = knobArea.getWidth() / columns;
    const int cellHeight = knobArea.getHeight() / 2;

    for (std::size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = static_cast<int>(i) / columns;
        const int column = static_cast<int>(i) % columns;
        auto cell = juce::Rectangle<int>(knobArea.getX() + column * cellWidth,
                                         knobArea.getY() + row * cellHeight,
                                         cellWidth, cellHeight).reduced(6);
        knobs[i].label.setBounds(cell.removeFromTop(22));
        knobs[i].slider.setBounds(cell);
    }

    area.removeFromTop(10);
    keyboard.setBounds(area.removeFromBottom(105));
}
