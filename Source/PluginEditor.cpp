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
    // ── Gear / G-washer graphic ───────────────────────────────────────────────
    // 1. Load SVG and recolour black → silver.
    // 2. Render to an offscreen ARGB image.
    // 3. BFS flood-fill from image border to identify "exterior" transparent
    //    pixels (outside the gear silhouette).
    // 4. Fill every remaining interior transparent pixel with brushed-aluminium
    //    silver — this fills the gear interior while preserving any G-shaped gap
    //    that connects through the gear ring to the outside (red shows through).
    // 5. Cache the result so paint() only calls g.drawImage().

    juce::File f ("/Users/andy/Downloads/GWasher.jpg.svg");
    if (! f.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse (f);
    if (! xml) return;

    gearDrawable = juce::Drawable::createFromSVG (*xml);
    if (! gearDrawable) return;

    gearDrawable->replaceColour (juce::Colours::black, juce::Colour (0xffC0C0C0));

    // ── Build the cached image ────────────────────────────────────────────────
    const float gearH = 150.0f;
    const float sc    = gearH / 893.0f;   // ≈ 0.1679
    const int   imgW  = juce::roundToInt (1844.0f * sc);  // ≈ 309
    const int   imgH  = juce::roundToInt (gearH);          // 150

    cachedGearImage = juce::Image (juce::Image::ARGB, imgW, imgH, true);
    {
        juce::Graphics gImg (cachedGearImage);
        gearDrawable->draw (gImg, 1.0f, juce::AffineTransform::scale (sc));
    }

    // Pre-cache alpha values so we avoid repeated getPixelAt() calls in the BFS.
    const int totalPx = imgW * imgH;
    std::vector<bool> transp (totalPx, false);
    for (int py = 0; py < imgH; ++py)
        for (int px = 0; px < imgW; ++px)
            transp[py * imgW + px] = (cachedGearImage.getPixelAt (px, py).getAlpha() < 128);

    // BFS: seed from all border pixels that are transparent, expand inward.
    std::vector<bool> isExterior (totalPx, false);
    std::vector<int>  queue;
    queue.reserve (imgW * 2 + imgH * 2);

    auto enqueue = [&](int px, int py)
    {
        const int i = py * imgW + px;
        if (! isExterior[i] && transp[i])
        {
            isExterior[i] = true;
            queue.push_back (i);
        }
    };

    for (int px = 0; px < imgW; ++px) { enqueue (px, 0);       enqueue (px, imgH - 1); }
    for (int py = 1; py < imgH - 1; ++py) { enqueue (0, py);   enqueue (imgW - 1, py); }

    for (int qi = 0; qi < (int) queue.size(); ++qi)
    {
        const int i  = queue[qi];
        const int px = i % imgW;
        const int py = i / imgW;
        if (px > 0)       enqueue (px - 1, py);
        if (px < imgW-1)  enqueue (px + 1, py);
        if (py > 0)       enqueue (px,     py - 1);
        if (py < imgH-1)  enqueue (px,     py + 1);
    }

    // ── REVEAL logotype PNG ───────────────────────────────────────────────────
    // The logotype is embedded as a PNG in RevealNameAndSurround.svg
    // (element id="image3013", direct child of the SVG root).
    // Extract the base64 data, decode it, load as juce::Image, then remove the
    // white background by using inverted luminance as the alpha channel.
    {
        juce::File logoSvg ("/Users/andy/Downloads/RevealNameAndSurround.svg");
        if (logoSvg.existsAsFile())
        {
            auto lxml = juce::XmlDocument::parse (logoSvg);
            if (lxml != nullptr)
            {
                forEachXmlChildElement (*lxml, child)
                {
                    if (child->hasTagName ("image")
                        && child->getStringAttribute ("id") == "image3013")
                    {
                        juce::String href = child->getStringAttribute ("xlink:href");
                        const juce::String prefix ("data:image/png;base64,");
                        if (href.startsWith (prefix))
                        {
                            // Strip prefix and remove all whitespace
                            juce::String b64 = href.substring (prefix.length())
                                                   .removeCharacters (" \n\r\t");

                            juce::MemoryOutputStream decoded;
                            if (juce::Base64::convertFromBase64 (decoded, b64))
                            {
                                logoImage = juce::ImageFileFormat::loadFrom (
                                    decoded.getData(), decoded.getDataSize());
                            }
                        }
                        break;
                    }
                }

                // Remove white background: invert luminance → alpha so the red
                // body shows through.  Boost contrast with a smoothstep curve.
                if (logoImage.isValid())
                {
                    logoImage = logoImage.convertedToFormat (juce::Image::ARGB);
                    for (int iy = 0; iy < logoImage.getHeight(); ++iy)
                    {
                        for (int ix = 0; ix < logoImage.getWidth(); ++ix)
                        {
                            auto c   = logoImage.getPixelAt (ix, iy);
                            float lm = (c.getRed()   * 0.299f
                                      + c.getGreen() * 0.587f
                                      + c.getBlue()  * 0.114f) / 255.0f;
                            // smoothstep: white (lm=1) → alpha 0; dark (lm=0) → alpha 255
                            float t  = juce::jlimit (0.0f, 1.0f, 1.0f - lm);
                            float a  = t * t * (3.0f - 2.0f * t);
                            logoImage.setPixelAt (ix, iy,
                                c.withAlpha ((uint8_t) juce::roundToInt (a * 255.0f)));
                        }
                    }
                }
            }
        }
    }

    // Fill every interior transparent pixel with brushed-aluminum silver.
    juce::Random rng (17);
    for (int py = 0; py < imgH; ++py)
    {
        const float t  = rng.nextFloat();
        const float la = (t > 0.76f) ? 0.055f : (t < 0.10f ? -0.055f : 0.0f);

        for (int px = 0; px < imgW; ++px)
        {
            const int i = py * imgW + px;
            if (isExterior[i] || ! transp[i])
                continue;

            const float gx     = (float) px / (float) imgW - 0.5f;
            const float gy     = (float) py / (float) imgH - 0.5f;
            const float bright = 0.92f - gx * 0.14f - gy * 0.18f + la;
            const auto  v      = (uint8_t) juce::jlimit (0, 255, (int) (192.0f * bright));
            cachedGearImage.setPixelAt (px, py, juce::Colour (255, v, v, v));
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
    // PNG extracted from RevealNameAndSurround.svg (image3013), white
    // background removed.  Scaled to match the spoke surround width and
    // positioned just below the lowest spoke tips.
    if (logoImage.isValid())
    {
        // Target width: same as spoke surround diameter
        const float targetW = outerR * 2.0f;
        const float scale   = targetW / (float) logoImage.getWidth();
        const float targetH = (float) logoImage.getHeight() * scale;

        const float bottomSpokeY = cy + (-std::cos (startAngle)) * outerR;
        const float logoX = cx - targetW * 0.5f;
        const float logoY = bottomSpokeY + 10.0f;

        g.drawImage (logoImage,
                     juce::roundToInt (logoX), juce::roundToInt (logoY),
                     juce::roundToInt (targetW), juce::roundToInt (targetH),
                     0, 0, logoImage.getWidth(), logoImage.getHeight());
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
    if (cachedGearImage.isValid())
    {
        const float btnCx = w * 0.5f;
        const float btnCy = 287.0f;
        const int   imgW  = cachedGearImage.getWidth();
        const int   imgH  = cachedGearImage.getHeight();

        g.drawImage (cachedGearImage,
                     juce::roundToInt (btnCx - imgW * 0.5f),
                     juce::roundToInt (btnCy - imgH * 0.5f),
                     imgW, imgH,
                     0, 0, imgW, imgH);
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

    // Bypass button — sits inside the gear's transparent centre cutout.
    const int btnSize = 48;
    bypassButton.setBounds ((getWidth() - btnSize) / 2,  // x: centred
                            287 - btnSize / 2,            // y: centred at gear centre
                            btnSize,
                            btnSize);
}
