// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <random>
#include <utility>

namespace bellows
{
struct ReedSettings
{
    bool low = true;
    bool middle = true;
    bool musette = true;
    bool high = false;
    float musetteWidthCents = 12.0f;
    float brightness = 0.7f;
    float reedVariation = 0.25f;
};

class ReedOscillator
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = std::max(8000.0, newSampleRate);
        reset();
    }

    void reset()
    {
        phases.fill(0.0);
        pressure = 0.0f;
        noiseState = 0x9e3779b9u;
        noiseFilter = 0.0f;
    }

    void startNote(int midiNote, float velocity, std::uint32_t seed)
    {
        const auto semitones = static_cast<double>(midiNote - 69) / 12.0;
        baseFrequency = 440.0 * std::pow(2.0, semitones);
        noteVelocity = std::clamp(velocity, 0.0f, 1.0f);
        noiseState ^= seed + 0x85ebca6bu + (noiseState << 6u) + (noiseState >> 2u);

        for (std::size_t i = 0; i < phases.size(); ++i)
            phases[i] = randomUnit() * twoPi;

        const auto variation = (randomUnit() * 2.0 - 1.0) * 2.2;
        noteDetuneCents = variation;
    }

    [[nodiscard]] std::pair<float, float> process(float pressureTarget,
                                                   const ReedSettings& settings)
    {
        pressureTarget = std::clamp(pressureTarget, 0.0f, 1.0f);
        const auto pressureSmoothing = static_cast<float>(1.0 - std::exp(-1.0 / (0.018 * sampleRate)));
        pressure += (pressureTarget - pressure) * pressureSmoothing;

        const auto dynamicBrightness = std::clamp(settings.brightness * (0.35f + 0.65f * pressure), 0.0f, 1.0f);
        const auto instability = (1.0f - pressure) * 0.0035f * settings.reedVariation;
        const auto flutter = 1.0 + static_cast<double>(instability * filteredNoise());
        const auto velocityGain = 0.35f + 0.65f * std::sqrt(noteVelocity);

        float left = 0.0f;
        float right = 0.0f;
        float totalWeight = 0.0f;

        const auto addReed = [&](std::size_t index,
                                 double octaveRatio,
                                 float cents,
                                 float gain,
                                 float pan,
                                 float& l,
                                 float& r,
                                 float& weight)
        {
            const auto detune = cents + static_cast<float>(noteDetuneCents * settings.reedVariation);
            const auto ratio = octaveRatio * std::pow(2.0, static_cast<double>(detune) / 1200.0) * flutter;
            const auto frequency = baseFrequency * ratio;
            phases[index] = wrapPhase(phases[index] + twoPi * frequency / sampleRate);
            const auto sample = waveform(phases[index], frequency, dynamicBrightness) * gain;
            const auto leftGain = std::sqrt(0.5f * (1.0f - pan));
            const auto rightGain = std::sqrt(0.5f * (1.0f + pan));
            l += sample * leftGain;
            r += sample * rightGain;
            weight += gain;
        };

        if (settings.low)
            addReed(0, 0.5, -1.5f, 0.72f, -0.12f, left, right, totalWeight);

        if (settings.middle)
            addReed(1, 1.0, 0.0f, 1.00f, 0.0f, left, right, totalWeight);

        if (settings.musette)
        {
            const auto width = std::max(0.0f, settings.musetteWidthCents);
            addReed(2, 1.0, -width, 0.62f, -0.42f, left, right, totalWeight);
            addReed(3, 1.0, width * 0.86f, 0.62f, 0.42f, left, right, totalWeight);
        }

        if (settings.high)
            addReed(4, 2.0, 1.0f, 0.44f, 0.14f, left, right, totalWeight);

        if (totalWeight > 0.0f)
        {
            const auto normalization = 1.0f / std::sqrt(totalWeight);
            left *= normalization;
            right *= normalization;
        }

        const auto pressureGain = pressure * pressure * (3.0f - 2.0f * pressure);
        const auto outputGain = velocityGain * pressureGain * 0.34f;
        return { left * outputGain, right * outputGain };
    }

    [[nodiscard]] float getPressure() const noexcept { return pressure; }

private:
    static constexpr double twoPi = 2.0 * std::numbers::pi;

    [[nodiscard]] static double wrapPhase(double phase)
    {
        while (phase >= twoPi)
            phase -= twoPi;
        while (phase < 0.0)
            phase += twoPi;
        return phase;
    }

    [[nodiscard]] float waveform(double phase, double fundamental, float brightness) const
    {
        const auto maxHarmonic = std::clamp(static_cast<int>((sampleRate * 0.47) / std::max(20.0, fundamental)), 1, 18);
        double sum = 0.0;
        double norm = 0.0;

        for (int harmonic = 1; harmonic <= maxHarmonic; ++harmonic)
        {
            const auto h = static_cast<double>(harmonic);
            const auto oddBoost = (harmonic % 2 == 1) ? 1.0 : (0.50 + 0.30 * brightness);
            const auto rolloff = std::pow(h, -(1.58 - 0.62 * brightness));
            const auto formant = 1.0 + 0.22 * std::exp(-0.5 * std::pow((h - 4.0) / 1.8, 2.0));
            const auto amplitude = oddBoost * rolloff * formant;
            sum += std::sin(phase * h + 0.035 * h * h) * amplitude;
            norm += amplitude;
        }

        return static_cast<float>(norm > 0.0 ? sum / norm : 0.0);
    }

    [[nodiscard]] float randomUnit()
    {
        noiseState ^= noiseState << 13u;
        noiseState ^= noiseState >> 17u;
        noiseState ^= noiseState << 5u;
        return static_cast<float>(noiseState & 0x00ffffffu) / static_cast<float>(0x01000000u);
    }

    [[nodiscard]] float filteredNoise()
    {
        const auto white = randomUnit() * 2.0f - 1.0f;
        noiseFilter += 0.08f * (white - noiseFilter);
        return noiseFilter;
    }

    double sampleRate = 48000.0;
    double baseFrequency = 440.0;
    double noteDetuneCents = 0.0;
    std::array<double, 5> phases {};
    float noteVelocity = 1.0f;
    float pressure = 0.0f;
    float noiseFilter = 0.0f;
    std::uint32_t noiseState = 0x9e3779b9u;
};
} // namespace bellows
