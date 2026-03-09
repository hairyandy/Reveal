# Reveal — Clean Boost Plugin

VST3 / AU audio plug-in for macOS modelling an op-amp based clean boost pedal.
Built with JUCE 7/8 and CMake.

---

## Project structure

```
Reveal/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h/.cpp   DSP + parameter state
    └── PluginEditor.h/.cpp      GUI (single rotary knob)
```

---

## Signal path

```
Audio In  →  [HP Biquad]  →  [Gain]  →  [LP Biquad]  →  Audio Out
```

| Stage | Filter | Analogue model | Corner frequency |
|-------|--------|----------------|-----------------|
| HP | 1st-order high-pass | 0.1 µF input coupling cap + 100 kΩ input R | ~15.9 Hz |
| Gain | Linear boost | TL072 non-inverting gain stage | — |
| LP | 1st-order low-pass | 100 pF feedback cap ‖ 100 kΩ gain resistor | ~15.9 kHz |

The HP removes DC before the gain stage.
The LP adds the natural high-frequency rolloff of the real op-amp circuit.
Both are implemented with `juce::dsp::IIR::Filter` (first-order biquads) wrapped in
`juce::dsp::ProcessorDuplicator` for correct stereo/mono handling.

---

## Prerequisites

* macOS 12 or later
* Xcode 14 or later (with Command Line Tools)
* CMake 3.22 or later  (`brew install cmake`)
* JUCE 7.0.x or 8.x — either a local clone or fetched automatically

---

## Building

### Option A — supply your own JUCE clone (faster, recommended)

```bash
git clone https://github.com/juce-framework/JUCE.git ~/JUCE

cd Reveal
cmake -B build -G Xcode -DJUCE_PATH=~/JUCE
open build/Reveal.xcodeproj
```

Select the **Reveal_VST3** or **Reveal_AU** scheme and press ▶.

### Option B — let CMake fetch JUCE automatically

```bash
cd Reveal
cmake -B build -G Xcode
open build/Reveal.xcodeproj
```

The first configure will clone JUCE (~800 MB) into the build tree.

### Command-line build (no Xcode GUI)

```bash
cmake --build build --config Release --target Reveal_VST3 -- -jobs 8
```

---

## Installing the plug-in

After a successful build the plug-ins appear at:

| Format | Path |
|--------|------|
| VST3 | `build/Reveal_artefacts/Release/VST3/Reveal.vst3` |
| AU   | `build/Reveal_artefacts/Release/AU/Reveal.component` |

Copy them to the appropriate system folder, then re-scan in your DAW:

```bash
# VST3
cp -r build/Reveal_artefacts/Release/VST3/Reveal.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/Reveal_artefacts/Release/AU/Reveal.component \
      ~/Library/Audio/Plug-Ins/Components/

# Re-register the AU with the system
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aufx Rvl1 YrSt   # validate the AU
```

---

## JUCE 8 font API note

JUCE 8 deprecates `juce::Font(name, size, style)` in favour of `juce::FontOptions`.
The editor uses the simpler size-only constructor which compiles cleanly on both
JUCE 7 and 8.  If you want named fonts on JUCE 8 replace:

```cpp
// JUCE 7
juce::Font (14.0f, juce::Font::bold)

// JUCE 8
juce::Font (juce::FontOptions{}.withHeight (14.0f).withStyle ("Bold"))
```

---

## Customising the circuit model

| Parameter | Constant | File | Notes |
|-----------|----------|------|-------|
| HP corner | `kInputHPFreqHz` | `PluginProcessor.h` | Change if input R differs |
| LP corner | `kFeedbackLPFreqHz` | `PluginProcessor.h` | Scale with Rf at target gain |
| Max gain  | NormalisableRange max | `PluginProcessor.cpp` | Currently 28 dB |
