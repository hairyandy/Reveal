#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// RevealLookAndFeel
//
// Custom look-and-feel for the Reveal pedal UI:
//   - Brushed silver rotary knob with a black indicator line (no arc track)
//   - Chrome dome bypass footswitch button
//==============================================================================
class RevealLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RevealLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown) override;
};

//==============================================================================
// RevealAudioProcessorEditor
//==============================================================================
class RevealAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RevealAudioProcessorEditor (RevealAudioProcessor& processor);
    ~RevealAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void loadSVGs ();
    void drawBrushedAluminum (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawSurround        (juce::Graphics& g);

    RevealAudioProcessor& audioProcessor;
    RevealLookAndFeel     lookAndFeel;

    juce::Slider   volumeKnob;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment;

    // Footswitch bypass toggle — getToggleState()==true means effect is ACTIVE
    juce::TextButton bypassButton;

    // Gear/washer SVG (clean vector path — loaded and colour-swapped to silver)
    std::unique_ptr<juce::Drawable> gearDrawable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevealAudioProcessorEditor)
};
