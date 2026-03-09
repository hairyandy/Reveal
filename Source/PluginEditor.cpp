#include "PluginEditor.h"

//==============================================================================
// Colour palette
//==============================================================================

namespace Colours
{
    // Background – near-black with a slight warm tint
    static const juce::Colour background    { 0xff1C1C1E };
    // Separator / border lines
    static const juce::Colour separator     { 0xff333333 };
    // Main amber/gold accent used for the value arc and indicator dot
    static const juce::Colour accent        { 0xffE8A020 };
    // Muted track (unlit portion of the arc)
    static const juce::Colour trackMuted    { 0xff2E2E2E };
    // Primary text (plugin name)
    static const juce::Colour textPrimary   { 0xffEEEEEE };
    // Muted text (subtitle, labels)
    static const juce::Colour textMuted     { 0xff777777 };
    // Knob highlight edge
    static const juce::Colour knobHighlight { 0xff606060 };
}

//==============================================================================
// RevealLookAndFeel
//==============================================================================

RevealLookAndFeel::RevealLookAndFeel()
{
    // Make the optional text-box (hidden, but set up in case it is ever shown)
    // match the dark theme.
    setColour (juce::Slider::textBoxTextColourId,      juce::Colour (0xffCCCCCC));
    setColour (juce::Slider::textBoxBackgroundColourId, Colours::background);
    setColour (juce::Slider::textBoxOutlineColourId,   Colours::separator);
}

void RevealLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& /*slider*/)
{
    // Geometry
    const float radius  = static_cast<float> (juce::jmin (width, height)) * 0.5f - 6.0f;
    const float cx      = static_cast<float> (x) + static_cast<float> (width)  * 0.5f;
    const float cy      = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float toAngle = rotaryStartAngle
                        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Radii used for the arc track and the knob body
    const float trackR  = radius * 0.82f;
    const float knobR   = radius * 0.68f;
    const float trackW  = 5.0f;

    // ------------------------------------------------------------------
    // 1. Background (unlit) arc
    // ------------------------------------------------------------------
    {
        juce::Path track;
        track.addCentredArc (cx, cy, trackR, trackR,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Colours::trackMuted);
        g.strokePath (track,
                      juce::PathStrokeType (trackW,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    }

    // ------------------------------------------------------------------
    // 2. Value (lit) arc – only drawn when there is actual gain applied
    // ------------------------------------------------------------------
    if (sliderPosProportional > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (cx, cy, trackR, trackR,
                                0.0f, rotaryStartAngle, toAngle, true);
        g.setColour (Colours::accent);
        g.strokePath (valueArc,
                      juce::PathStrokeType (trackW,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    }

    // ------------------------------------------------------------------
    // 3. Knob body – dark metallic gradient
    // ------------------------------------------------------------------
    {
        // Highlight offset – light appears to come from the upper-left
        juce::ColourGradient grad (juce::Colour (0xff4E4E4E),
                                   cx - knobR * 0.4f, cy - knobR * 0.5f,
                                   juce::Colour (0xff1A1A1A),
                                   cx + knobR * 0.3f, cy + knobR * 0.5f,
                                   /*isRadial=*/false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);
    }

    // Rim highlight
    g.setColour (Colours::knobHighlight);
    g.drawEllipse (cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);

    // ------------------------------------------------------------------
    // 4. Indicator dot at the knob face
    // ------------------------------------------------------------------
    {
        const float dotR    = 3.5f;
        const float dotDist = knobR * 0.60f;  // how far from centre
        const float dotX    = cx + std::sin (toAngle) * dotDist;
        const float dotY    = cy - std::cos (toAngle) * dotDist;

        g.setColour (Colours::accent);
        g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    }
}

//==============================================================================
// RevealAudioProcessorEditor
//==============================================================================

RevealAudioProcessorEditor::RevealAudioProcessorEditor (RevealAudioProcessor& p)
    : AudioProcessorEditor (p),
      audioProcessor       (p),
      volumeAttachment     (p.getAPVTS(), "volume", volumeKnob)
      // SliderAttachment must be constructed AFTER the Slider exists and
      // BEFORE we customise it, so its range / value are set from the APVTS.
{
    setLookAndFeel (&lookAndFeel);

    // ------------------------------------------------------------------
    // Volume knob
    // ------------------------------------------------------------------
    volumeKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    // Hide the text box – we show a custom Label instead.
    volumeKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    // Standard 270° sweep: min at bottom-left, max at bottom-right.
    volumeKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f,
                                    true);

    // Refresh the value label whenever the knob moves (user drag, automation,
    // or preset load all fire onValueChange via the SliderAttachment).
    volumeKnob.onValueChange = [this] { updateValueLabel(); };

    addAndMakeVisible (volumeKnob);

    // ------------------------------------------------------------------
    // "VOLUME" caption
    // ------------------------------------------------------------------
    volumeLabel.setText ("VOLUME", juce::dontSendNotification);
    volumeLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    volumeLabel.setColour (juce::Label::textColourId, Colours::textMuted);
    volumeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (volumeLabel);

    // ------------------------------------------------------------------
    // Live dB readout
    // ------------------------------------------------------------------
    valueLabel.setFont (juce::Font (13.0f, juce::Font::plain));
    valueLabel.setColour (juce::Label::textColourId, Colours::accent);
    valueLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (valueLabel);

    // Populate the label with the current (restored) value right away.
    updateValueLabel();

    setSize (300, 340);
    setResizable (false, false);
}

RevealAudioProcessorEditor::~RevealAudioProcessorEditor()
{
    // Always reset LookAndFeel before the object is destroyed to prevent
    // dangling references in child components.
    setLookAndFeel (nullptr);
}

//==============================================================================
// Paint
//==============================================================================

void RevealAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (Colours::background);

    // ------------------------------------------------------------------
    // Plugin title – "REVEAL"
    // ------------------------------------------------------------------
    g.setFont (juce::Font (30.0f, juce::Font::bold));
    g.setColour (Colours::textPrimary);
    g.drawText ("REVEAL",
                getLocalBounds().removeFromTop (62).translated (0, 14),
                juce::Justification::centred, false);

    // ------------------------------------------------------------------
    // Subtitle
    // ------------------------------------------------------------------
    g.setFont (juce::Font (9.5f, juce::Font::plain));
    g.setColour (Colours::textMuted);
    // Draw inside the strip just above the separator
    g.drawText ("CLEAN BOOST",
                juce::Rectangle<int> (0, 62, getWidth(), 18),
                juce::Justification::centred, false);

    // ------------------------------------------------------------------
    // Horizontal separator
    // ------------------------------------------------------------------
    g.setColour (Colours::separator);
    g.drawHorizontalLine (84, 24.0f, static_cast<float> (getWidth()) - 24.0f);
}

//==============================================================================
// Layout
//==============================================================================

void RevealAudioProcessorEditor::resized()
{
    const int w        = getWidth();
    const int knobSize = 158;                        // px – diameter of hit target
    const int knobX    = (w - knobSize) / 2;
    const int knobY    = 95;

    volumeKnob.setBounds  (knobX, knobY, knobSize, knobSize);
    // "VOLUME" label, 8 px below the knob
    volumeLabel.setBounds (0, knobY + knobSize + 8,  w, 16);
    // dB value readout, 6 px below the caption
    valueLabel.setBounds  (0, knobY + knobSize + 28, w, 20);
}

//==============================================================================
// Helpers
//==============================================================================

void RevealAudioProcessorEditor::updateValueLabel()
{
    const float db = static_cast<float> (volumeKnob.getValue());

    // Format: "+14.0 dB" with an explicit '+' for non-zero values
    juce::String text;
    if (db > 0.05f)
        text = "+" + juce::String (db, 1) + " dB";
    else
        text = "0.0 dB";

    valueLabel.setText (text, juce::dontSendNotification);
}
