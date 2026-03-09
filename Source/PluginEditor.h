#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// RevealLookAndFeel
//
// Draws a custom rotary knob that suits the minimal "pedal" aesthetic:
//   - Dark metallic body with a subtle gradient
//   - Amber arc tracking the current value
//   - Amber indicator dot at the knob tip
//==============================================================================
class RevealLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RevealLookAndFeel();

    /** Called by JUCE's Slider when it needs to paint a rotary control. */
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
};

//==============================================================================
// RevealAudioProcessorEditor
//==============================================================================
class RevealAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RevealAudioProcessorEditor (RevealAudioProcessor& processor);
    ~RevealAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    /** Refreshes valueLabel text from the knob's current value. */
    void updateValueLabel();

    RevealAudioProcessor& audioProcessor;

    RevealLookAndFeel lookAndFeel;

    juce::Slider volumeKnob;   // Main rotary control
    juce::Label  volumeLabel;  // Static "VOLUME" caption below the knob
    juce::Label  valueLabel;   // Live dB readout (e.g. "14.0 dB")

    // Attaches the Slider to the "volume" APVTS parameter – handles
    // host automation, undo, and preset recall automatically.
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevealAudioProcessorEditor)
};
