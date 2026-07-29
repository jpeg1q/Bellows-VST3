// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DSP/ReedOscillator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
void writeU16(std::ofstream& out, std::uint16_t value)
{
    const std::array<char, 2> bytes {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu)
    };
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeU32(std::ofstream& out, std::uint32_t value)
{
    const std::array<char, 4> bytes {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu),
        static_cast<char>((value >> 16u) & 0xffu),
        static_cast<char>((value >> 24u) & 0xffu)
    };
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void writeWav(const std::string& path, const std::vector<float>& interleaved, int sampleRate)
{
    std::ofstream out(path, std::ios::binary);
    const auto dataSize = static_cast<std::uint32_t>(interleaved.size() * sizeof(std::int16_t));

    out.write("RIFF", 4);
    writeU32(out, 36u + dataSize);
    out.write("WAVEfmt ", 8);
    writeU32(out, 16u);
    writeU16(out, 1u);
    writeU16(out, 2u);
    writeU32(out, static_cast<std::uint32_t>(sampleRate));
    writeU32(out, static_cast<std::uint32_t>(sampleRate * 2 * sizeof(std::int16_t)));
    writeU16(out, static_cast<std::uint16_t>(2 * sizeof(std::int16_t)));
    writeU16(out, 16u);
    out.write("data", 4);
    writeU32(out, dataSize);

    for (const auto sample : interleaved)
    {
        const auto clipped = std::clamp(sample, -1.0f, 1.0f);
        const auto pcm = static_cast<std::int16_t>(std::lrint(clipped * 32767.0f));
        writeU16(out, static_cast<std::uint16_t>(pcm));
    }
}
}

int main(int argc, char** argv)
{
    constexpr int sampleRate = 48000;
    constexpr float durationSeconds = 5.0f;
    const auto output = argc > 1 ? argv[1] : "Bellows-demo.wav";

    bellows::ReedOscillator oscillator;
    oscillator.prepare(sampleRate);
    oscillator.startNote(57, 0.88f, 0xB3110u);

    bellows::ReedSettings settings;
    settings.low = true;
    settings.middle = true;
    settings.musette = true;
    settings.high = false;
    settings.musetteWidthCents = 14.0f;
    settings.brightness = 0.72f;
    settings.reedVariation = 0.27f;

    const auto frameCount = static_cast<int>(durationSeconds * sampleRate);
    std::vector<float> audio;
    audio.reserve(static_cast<std::size_t>(frameCount * 2));

    for (int frame = 0; frame < frameCount; ++frame)
    {
        const auto time = static_cast<float>(frame) / sampleRate;
        float pressure = 0.0f;
        if (time < 0.7f)
            pressure = time / 0.7f * 0.78f;
        else if (time < 3.8f)
            pressure = 0.78f + 0.08f * std::sin(time * 2.4f);
        else
            pressure = std::max(0.0f, 0.78f * (1.0f - (time - 3.8f) / 1.2f));

        const auto [left, right] = oscillator.process(pressure, settings);
        audio.push_back(left * 1.9f);
        audio.push_back(right * 1.9f);
    }

    writeWav(output, audio, sampleRate);
    std::cout << "Rendered " << output << '\n';
    return 0;
}
