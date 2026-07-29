// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DSP/ReedOscillator.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr int samples = 48000;

    bellows::ReedOscillator oscillator;
    oscillator.prepare(sampleRate);
    oscillator.startNote(60, 0.82f, 123456u);

    bellows::ReedSettings settings;
    settings.low = true;
    settings.middle = true;
    settings.musette = true;
    settings.high = false;
    settings.musetteWidthCents = 13.5f;
    settings.brightness = 0.72f;
    settings.reedVariation = 0.24f;

    double energy = 0.0;
    float peak = 0.0f;

    for (int i = 0; i < samples; ++i)
    {
        const auto pressure = std::clamp(static_cast<float>(i) / 7000.0f, 0.0f, 0.85f);
        const auto [left, right] = oscillator.process(pressure, settings);

        if (! std::isfinite(left) || ! std::isfinite(right))
        {
            std::cerr << "Non-finite DSP output\n";
            return 1;
        }

        peak = std::max({ peak, std::abs(left), std::abs(right) });
        energy += static_cast<double>(left * left + right * right);
    }

    const auto rms = std::sqrt(energy / (samples * 2.0));
    std::cout << "peak=" << peak << " rms=" << rms << '\n';

    if (peak < 0.01f || peak > 1.2f)
    {
        std::cerr << "Unexpected peak level\n";
        return 2;
    }

    if (rms < 0.002 || rms > 0.5)
    {
        std::cerr << "Unexpected RMS level\n";
        return 3;
    }

    return 0;
}
