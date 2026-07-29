# Bellows VST3

Bellows is an expressive, physically inspired accordion instrument built with C++20 and JUCE. It is a **modeled prototype**, not a repackaged soundfont and not a claim that VST3 itself improves sound quality.

## Current sound engine

- Five independently phased reed oscillators: low 16', middle 8', two musette reeds, and high 4'
- Continuous bellows pressure via the Bellows parameter and MIDI CC11 expression
- Pressure-dependent brightness and reed instability
- Adjustable musette detuning rather than a generic chorus effect
- Per-note tuning variation and randomized reed phase
- Mechanical key transient and filtered air noise
- Body resonance, high-pass filtering, stereo spread, and room reverb
- 16-voice polyphony
- VST3 and standalone targets

This v0.1 engine is intended to be playable and expressive without copyrighted samples. A future sampled/model hybrid can replace or layer the oscillator bank with legally recorded multi-velocity accordion samples.

## Build on Windows

Requirements:

- Windows 10 or newer
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+
- Git

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

The output is normally located at:

```text
build\Bellows_artefacts\Release\VST3\Bellows.vst3
```

Copy the complete `Bellows.vst3` bundle into:

```text
C:\Program Files\Common Files\VST3\
```

Then rescan plug-ins in your DAW.

JUCE 8.0.15 is downloaded automatically by CMake during configuration.

## Publish as a new GitHub repository

From PowerShell in the extracted project folder:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish-github.ps1
```

The script installs GitHub CLI through `winget` when needed, opens GitHub browser authentication, creates `Bellows-VST3` as a public repository, pushes `main`, and starts the included GitHub Actions build. To make it private instead:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish-github.ps1 -Visibility private
```

## Performance controls

- **MIDI velocity:** initial reed response
- **CC11 Expression:** continuous bellows pressure multiplier
- **Bellows knob:** baseline pressure and automation source
- **Musette Width:** detuning distance in cents
- **Reed Variation:** note-to-note tuning and low-pressure instability
- **Mechanical / Air:** key and airflow texture

For realistic phrasing, automate CC11 throughout sustained notes instead of leaving it fixed at 127.

## Validate only the DSP core

The core oscillator has no JUCE dependency and can be tested on any C++20 compiler:

```bash
cmake -S . -B build-core -G Ninja \
  -DBELLOWS_BUILD_PLUGIN=OFF \
  -DBELLOWS_BUILD_TESTS=ON \
  -DBELLOWS_BUILD_DEMO=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
./build-core/BellowsRenderDemo Bellows-demo.wav
```

## Limitations of v0.1

- It is modeled synthesis, so it will not reproduce the exact acoustic identity of a particular accordion.
- Bellows direction, bass/chord-button layouts, true release samples, register-specific recordings, and MPE are not implemented yet.
- Pitch bend is accepted but not applied continuously in v0.1.
- The generated VST3 is unsigned; Windows or a DAW may display a publisher warning.

## Roadmap

1. Legally recorded multi-sample reed layers with round robins and releases
2. Bellows-open and bellows-close articulation states
3. Stradella bass and chord system
4. Sample streaming and library relinking
5. Presets for dry, French musette, Italian, folk, and concert registers
6. MPE/polyphonic expression and improved pitch-bend behavior

## License

AGPL-3.0-or-later. JUCE 8 offers AGPLv3 or commercial licensing; obtain the appropriate JUCE commercial license before distributing a closed-source commercial build.
