#include "PluginEditor.h"

//==============================================================================
// Colour constants
//==============================================================================
namespace RevealColours
{
    // Deep brushed-red aluminum — matches hardware pedal body
    static const juce::Colour background  { 0xff8C1420 };
    // LED states
    static const juce::Colour ledActive   { 0xffFF4040 };
    static const juce::Colour ledInactive { 0xff3A0808 };
}

//==============================================================================
// RevealLookAndFeel
//==============================================================================

RevealLookAndFeel::RevealLookAndFeel()
{
    setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff888888));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffAAAAAA));
    setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff333333));
    setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff222222));
}

void RevealLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& /*slider*/)
{
    const float cx      = (float)x + (float)width  * 0.5f;
    const float cy      = (float)y + (float)height * 0.5f;
    const float radius  = (float)juce::jmin (width, height) * 0.5f - 2.0f;
    const float toAngle = rotaryStartAngle
                        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Knob geometry: a raised outer lip ring + a recessed inner dish.
    const float lipW  = radius * 0.12f;       // width of the raised lip
    const float dishR = radius - lipW;        // radius of the recessed dish

    const float dx = std::sin (toAngle);
    const float dy = -std::cos (toAngle);

    // ── 1. Recessed inner dish ────────────────────────────────────────────────
    {
        juce::ColourGradient grad (juce::Colour (0xffA4A4A4),
                                   cx - dishR * 0.28f, cy - dishR * 0.38f,
                                   juce::Colour (0xff606060),
                                   cx + dishR * 0.22f, cy + dishR * 0.32f,
                                   /*isRadial=*/true);
        grad.addColour (0.45, juce::Colour (0xff888888));
        g.setGradientFill (grad);
        g.fillEllipse (cx - dishR, cy - dishR, dishR * 2.0f, dishR * 2.0f);
    }

    // ── 2. Brush-finish lines clipped to dish ─────────────────────────────────
    {
        g.saveState();
        juce::Path clip;
        clip.addEllipse (cx - dishR, cy - dishR, dishR * 2.0f, dishR * 2.0f);
        g.reduceClipRegion (clip);

        juce::Random rng (99);
        for (float ly = cy - dishR; ly <= cy + dishR; ly += 1.0f)
        {
            const float t = rng.nextFloat();
            if (t > 0.77f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.08f));
                g.drawHorizontalLine ((int)ly, cx - dishR, cx + dishR);
            }
            else if (t < 0.09f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.07f));
                g.drawHorizontalLine ((int)ly, cx - dishR, cx + dishR);
            }
        }
        g.restoreState();
    }

    // ── 3. Shadow at dish edge — depth between dish and lip ───────────────────
    g.setColour (juce::Colour (0xff222222).withAlpha (0.80f));
    g.drawEllipse (cx - dishR, cy - dishR, dishR * 2.0f, dishR * 2.0f, 2.5f);

    // ── 4. Raised outer lip (annular ring) ────────────────────────────────────
    {
        // Even-odd fill: inner ellipse punches a hole in the outer ellipse.
        juce::Path lip;
        lip.addEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        lip.addEllipse (cx - dishR,  cy - dishR,  dishR  * 2.0f, dishR  * 2.0f);
        lip.setUsingNonZeroWinding (false);

        juce::ColourGradient grad (juce::Colour (0xffDEDEDE),
                                   cx - radius * 0.32f, cy - radius * 0.42f,
                                   juce::Colour (0xff8A8A8A),
                                   cx + radius * 0.28f, cy + radius * 0.38f,
                                   /*isRadial=*/false);
        grad.addColour (0.5, juce::Colour (0xffBCBCBC));
        g.setGradientFill (grad);
        g.fillPath (lip);
    }

    // ── 5. Outer rim edge ─────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xffAAAAAA));
    g.drawEllipse (cx - radius + 0.5f, cy - radius + 0.5f,
                   (radius - 0.5f) * 2.0f, (radius - 0.5f) * 2.0f, 1.0f);

    // ── 6. Indicator line — from near-centre to outer lip edge ───────────────
    g.setColour (juce::Colours::black.withAlpha (0.92f));
    g.drawLine (cx + dx * (radius * 0.12f), cy + dy * (radius * 0.12f),
                cx + dx * (radius - 1.5f),  cy + dy * (radius - 1.5f),
                3.75f);
}

void RevealLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                               juce::Button& button,
                                               const juce::Colour& /*backgroundColour*/,
                                               bool isMouseOver,
                                               bool isButtonDown)
{
    const float cx = (float)button.getWidth()  * 0.5f;
    const float cy = (float)button.getHeight() * 0.5f;
    const float r  = (float)juce::jmin (button.getWidth(), button.getHeight()) * 0.5f - 2.0f;

    // Chrome dome — brighter highlight when hovered, pressed shifts darker
    const float brightness = isButtonDown ? 0.70f : (isMouseOver ? 1.06f : 1.0f);

    juce::ColourGradient grad (
        juce::Colour (0xffE8E8E8).withMultipliedBrightness (brightness),
        cx - r * 0.32f, cy - r * 0.42f,
        juce::Colour (0xff888888).withMultipliedBrightness (brightness),
        cx + r * 0.28f, cy + r * 0.38f,
        /*isRadial=*/true);
    grad.addColour (0.55, juce::Colour (0xffB2B2B2).withMultipliedBrightness (brightness));

    g.setGradientFill (grad);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    // Outer rim
    g.setColour (juce::Colour (0xff3C3C3C));
    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);

    // Inner highlight ring — specular glint
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawEllipse (cx - r + 2.0f, cy - r + 2.0f,
                   (r - 2.0f) * 2.0f, (r - 2.0f) * 2.0f, 0.7f);
}

//==============================================================================
// RevealAudioProcessorEditor – construction
//==============================================================================

RevealAudioProcessorEditor::RevealAudioProcessorEditor (RevealAudioProcessor& p)
    : AudioProcessorEditor (p),
      audioProcessor        (p),
      volumeAttachment      (p.getAPVTS(), "volume", volumeKnob)
{
    setLookAndFeel (&lookAndFeel);
    loadSVGs();

    // ── Volume knob ──────────────────────────────────────────────────────────
    volumeKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    volumeKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volumeKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f,
                                    true);
    addAndMakeVisible (volumeKnob);

    // ── Bypass footswitch ────────────────────────────────────────────────────
    // toggleState==true  → effect ACTIVE  (LED on)
    // toggleState==false → effect BYPASSED (LED dim)
    bypassButton.setClickingTogglesState (true);
    bypassButton.setToggleState (!audioProcessor.isSuspended(),
                                 juce::dontSendNotification);
    bypassButton.onClick = [this]
    {
        const bool active = bypassButton.getToggleState();
        audioProcessor.suspendProcessing (!active);
        repaint();   // refresh LED
    };
    addAndMakeVisible (bypassButton);

    setSize (200, 380);
    setResizable (false, false);
}

RevealAudioProcessorEditor::~RevealAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
// SVG loading
//==============================================================================

void RevealAudioProcessorEditor::loadSVGs()
{
    // The spoke surround and REVEAL logotype are now drawn procedurally in
    // drawSurround(), so no SVG loading is needed for those elements.

    // ── Gear / C-washer graphic ───────────────────────────────────────────────
    // Clean vector path (fill:#000000, transparent background).
    // replaceColour() remaps the path fill to brushed silver.
    {
        juce::File f ("/Users/andy/Downloads/GWasher.jpg.svg");
        if (f.existsAsFile())
        {
            auto xml = juce::XmlDocument::parse (f);
            if (xml)
            {
                gearDrawable = juce::Drawable::createFromSVG (*xml);
                if (gearDrawable)
                    gearDrawable->replaceColour (juce::Colours::black,
                                                 juce::Colour (0xffBABABA));
            }
        }
    }
}

//==============================================================================
// Background drawing helper
//==============================================================================

void RevealAudioProcessorEditor::drawBrushedAluminum (juce::Graphics& g,
                                                       juce::Rectangle<float> bounds)
{
    // Base red fill
    g.setColour (RevealColours::background);
    g.fillRect (bounds);

    // Fine horizontal brush lines — deterministic variation via fixed seed
    {
        juce::Random rng (17);
        for (float ly = bounds.getY(); ly < bounds.getBottom(); ly += 1.0f)
        {
            const float t = rng.nextFloat();
            if (t > 0.76f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.032f));
                g.drawHorizontalLine ((int)ly, bounds.getX(), bounds.getRight());
            }
            else if (t < 0.10f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.038f));
                g.drawHorizontalLine ((int)ly, bounds.getX(), bounds.getRight());
            }
        }
    }

    // Subtle radial centre-bright highlight (simulates convex metallic surface)
    {
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        juce::ColourGradient vignette (
            juce::Colours::white.withAlpha (0.07f), cx, cy,
            juce::Colours::transparentBlack,        bounds.getX(), cy,
            /*isRadial=*/true);
        g.setGradientFill (vignette);
        g.fillRect (bounds);
    }
}

//==============================================================================
// Procedural spoke surround + logotype
//==============================================================================

void RevealAudioProcessorEditor::drawSurround (juce::Graphics& g)
{
    // Centre matches the knob centre: knobY(39) + knobSize(120)/2 = 99
    const float cx = (float)getWidth() * 0.5f;
    const float cy = 99.0f;

    const juce::Colour silver { 0xffC8C8C8 };

    // ── 11 spokes ─────────────────────────────────────────────────────────────
    // Short, thick radial rays spanning the knob's 270° sweep, no outer ring.
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;  // 7 o'clock
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;  // 5 o'clock

    const float innerR = 61.0f;   // knob r≈57 + 4 px margin
    const float outerR = 78.0f;   // free spoke tip — no outer ring

    // Draw spokes first so the inner ring overlaps their bases cleanly.
    g.setColour (silver);
    for (int i = 0; i < 11; ++i)
    {
        const float frac  = (float)i / 10.0f;
        const float angle = startAngle + frac * (endAngle - startAngle);
        const float dx    = std::sin (angle);
        const float dy    = -std::cos (angle);
        g.drawLine (cx + dx * innerR, cy + dy * innerR,
                    cx + dx * outerR, cy + dy * outerR,
                    6.0f);
    }

    // ── Inner arc (drawn on top of spoke bases, open at the bottom) ───────────
    // Built point-by-point in the same angle convention as the spokes so there
    // is no ambiguity about JUCE's arc direction or 2π wrapping.
    {
        juce::Path arc;
        constexpr int kSegs = 120;
        for (int i = 0; i <= kSegs; ++i)
        {
            const float t     = (float)i / (float)kSegs;
            const float angle = startAngle + t * (endAngle - startAngle);
            const float px    = cx + std::sin (angle) * innerR;
            const float py    = cy - std::cos (angle) * innerR;
            if (i == 0) arc.startNewSubPath (px, py);
            else        arc.lineTo (px, py);
        }
        g.strokePath (arc, juce::PathStrokeType (2.5f));
    }

    // ── "REVEAL" logotype ─────────────────────────────────────────────────────
    // Large bold sans-serif, positioned just below the spoke tips.
    {
        const float textY = cy + outerR + 8.0f;   // 8 px gap below tip

        // Top-bright metallic gradient matching the hardware engraving
        juce::ColourGradient grad (juce::Colour (0xffDDDDDD), cx, textY,
                                   juce::Colour (0xff888888), cx, textY + 24.0f,
                                   /*isRadial=*/false);
        g.setGradientFill (grad);
        g.setFont (juce::Font (24.0f, juce::Font::bold));
        g.drawText ("REVEAL",
                    juce::Rectangle<float> (cx - 80.0f, textY, 160.0f, 28.0f),
                    juce::Justification::centred, false);
    }
}

//==============================================================================
// Paint
//==============================================================================

void RevealAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float)getWidth();

    // ── 1. Brushed red aluminium background ───────────────────────────────────
    drawBrushedAluminum (g, getLocalBounds().toFloat());

    // ── 2. LED indicator — top centre ─────────────────────────────────────────
    {
        const bool  active = bypassButton.getToggleState();
        const float ledCx  = w * 0.5f;
        const float ledCy  = 16.0f;
        const float ledR   = 5.0f;

        // Soft glow halo when active
        if (active)
        {
            g.setColour (RevealColours::ledActive.withAlpha (0.22f));
            g.fillEllipse (ledCx - ledR * 2.4f, ledCy - ledR * 2.4f,
                           ledR * 4.8f, ledR * 4.8f);
        }

        // LED dome
        juce::ColourGradient dome (
            active ? RevealColours::ledActive.brighter (0.55f)
                   : juce::Colour (0xff882020),
            ledCx - ledR * 0.3f, ledCy - ledR * 0.4f,
            active ? RevealColours::ledActive.darker (0.4f)
                   : RevealColours::ledInactive,
            ledCx + ledR * 0.2f, ledCy + ledR * 0.3f,
            /*isRadial=*/true);
        g.setGradientFill (dome);
        g.fillEllipse (ledCx - ledR, ledCy - ledR, ledR * 2.0f, ledR * 2.0f);

        // Rim
        g.setColour (juce::Colour (0xff2A2A2A));
        g.drawEllipse (ledCx - ledR, ledCy - ledR, ledR * 2.0f, ledR * 2.0f, 0.8f);
    }

    // ── 3. Spoke surround + REVEAL logotype (procedural) ─────────────────────
    drawSurround (g);

    // ── 4. Gear / washer graphic ──────────────────────────────────────────────
    if (gearDrawable)
    {
        // Maintain SVG aspect ratio (1844 × 893 → ≈ 2.063 : 1)
        const float gearW = 180.0f;
        const float gearH = gearW * (893.0f / 1844.0f);   // ≈ 87 px
        const float gearX = (w - gearW) * 0.5f;           // = 10 px
        const float gearY = 244.0f;   // below REVEAL text (textY≈209 + 28 height + gap)

        gearDrawable->drawWithin (g,
            juce::Rectangle<float> (gearX, gearY, gearW, gearH),
            juce::RectanglePlacement::centred,
            1.0f);
    }
}

//==============================================================================
// Layout
//==============================================================================

void RevealAudioProcessorEditor::resized()
{
    // Volume knob — 120 px diameter, centre at y=99 (matches surround centre).
    const int knobSize = 120;
    volumeKnob.setBounds ((getWidth() - knobSize) / 2,   // x: centred → x=40
                          39,                             // y: 99 - 60 = 39
                          knobSize,
                          knobSize);

    // Bypass button — centred inside the gear graphic.
    // Gear at y=244, height≈87 → vertical centre ≈ y=287.
    const int btnSize = 52;
    bypassButton.setBounds ((getWidth() - btnSize) / 2,  // x: centred
                            261,                          // y: 287 - 26 = 261
                            btnSize,
                            btnSize);
}
