#pragma once

#include <JuceHeader.h>
#include "BinaryData.h"

namespace soulbass
{
    //==============================================================================
    // Helper to convert filename to BinaryData resource name
    // Matches JUCE's BinaryData naming: spaces→underscore, hyphens→removed, dot→underscore
    inline juce::String toResourceName (const juce::String& fileName)
    {
        juce::String result;
        for (int i = 0; i < fileName.length(); ++i)
        {
            auto c = fileName[i];
            if (juce::CharacterFunctions::isLetter (c) || juce::CharacterFunctions::isDigit (c))
                result << c;
            else if (c == ' ' || c == '.')
                result << '_';
            // Hyphens and other chars are removed
        }

        // If starts with digit, prepend underscore
        if (result.isNotEmpty() && juce::CharacterFunctions::isDigit (result[0]))
            result = "_" + result;

        return result;
    }

    inline juce::Image loadImageFromBinary (const juce::String& fileName)
    {
        auto resourceName = toResourceName (fileName);
        int dataSize = 0;
        if (auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize))
        {
            juce::MemoryInputStream stream (data, (size_t) dataSize, false);
            return juce::ImageFileFormat::loadFrom (stream);
        }
        DBG ("Failed to load resource: " << fileName << " -> " << resourceName);
        return {};
    }

    //==============================================================================
    // LookAndFeel that prevents default slider/knob drawing for filmstrip controls
    class FilmstripLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
        void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override {}
    };

    // Shared instance to avoid creating many LookAndFeel objects
    inline FilmstripLookAndFeel& getFilmstripLookAndFeel()
    {
        static FilmstripLookAndFeel instance;
        return instance;
    }

    //==============================================================================
    // Filmstrip rotary knob
    class FilmstripKnob : public juce::Slider
    {
    public:
        FilmstripKnob (const juce::String& filmstripFileName, bool isVerticalStrip = true)
            : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
              isVertical (isVerticalStrip),
              assetFileName (filmstripFileName)
        {
            setLookAndFeel (&getFilmstripLookAndFeel());

            // CRITICAL: setOpaque tells JUCE to clear before paint
            // DO NOT use setBufferedToImage - it caches old frames!
            setOpaque (true);

            // Set mouse drag sensitivity for smooth control
            setMouseDragSensitivity (128);

            filmstrip = loadImageFromBinary (filmstripFileName);
            if (filmstrip.isValid())
            {
                frameWidth = filmstrip.getWidth();
                // Derive frames from strip geometry (Dial On.png is 83x5501 => ~66 frames).
                numFrames = juce::jmax (1, juce::roundToInt ((float) filmstrip.getHeight() / (float) frameWidth));
                frameHeightF = (float) filmstrip.getHeight() / (float) numFrames;
                frameHeight = juce::roundToInt (frameHeightF);

                #if JUCE_DEBUG
                DBG ("FilmstripKnob '" << filmstripFileName << "': " << filmstrip.getWidth() << "x" << filmstrip.getHeight()
                     << " => " << numFrames << " frames (frame size: " << frameWidth << "x" << frameHeight << ")");
                #endif
            }
            else
            {
                DBG ("ERROR: FilmstripKnob failed to load: " << filmstripFileName);
            }

            setRange (0.0, 1.0, 0.0);
            // Use a 270-degree rotary arc for natural knob interaction.
            setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                 juce::MathConstants<float>::pi * 2.75f,
                                 true);
        }

        ~FilmstripKnob() override { setLookAndFeel (nullptr); }

        void paint (juce::Graphics& g) override
        {
            #if JUCE_DEBUG
            static bool firstPaint = true;
            if (firstPaint)
            {
                DBG ("FilmstripKnob::paint() called for: " << assetFileName);
                firstPaint = false;
            }
            #endif

            if (!filmstrip.isValid() || numFrames <= 0)
            {
                // Fallback rendering when asset fails to load
                g.setColour (juce::Colours::darkgrey);
                g.fillEllipse (getLocalBounds().toFloat().reduced (2.0f));

                #if JUCE_DEBUG
                // Draw a red X to indicate missing asset
                g.setColour (juce::Colours::red);
                auto bounds = getLocalBounds().toFloat().reduced (4.0f);
                g.drawLine (bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 2.0f);
                g.drawLine (bounds.getRight(), bounds.getY(), bounds.getX(), bounds.getBottom(), 2.0f);

                // Draw text indicating error
                g.setFont (8.0f);
                g.drawText ("MISSING", getLocalBounds(), juce::Justification::centred);
                #endif
                return;
            }

            // Calculate normalized value (0.0 to 1.0)
            auto normValue = juce::jlimit (0.0f, 1.0f,
                static_cast<float> ((getValue() - getMinimum()) / (getMaximum() - getMinimum())));

            // Determine which frame to display (forum solution: add 0.5 for proper rounding)
            const int frameIndex = juce::jlimit (0, numFrames - 1,
                static_cast<int> (0.5f + normValue * static_cast<float> (numFrames - 1)));

            // Calculate source rectangle using floating widths to avoid drift when the strip isn't perfectly divisible
            const float startYf = frameHeightF * static_cast<float> (frameIndex);
            const float endYf   = frameHeightF * static_cast<float> (frameIndex + 1);
            const int sourceY   = juce::roundToInt (startYf);
            int sourceH         = juce::roundToInt (endYf - startYf);
            sourceH = juce::jlimit (1, filmstrip.getHeight() - sourceY, sourceH);

            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (filmstrip,
                        0, 0, getWidth(), getHeight(),              // destination rectangle
                        0, sourceY, frameWidth, sourceH,            // source rectangle from filmstrip
                        false);                                     // don't fill alpha channel

            #if JUCE_DEBUG
            // Draw a green border to confirm new code is running
            g.setColour (juce::Colours::green.withAlpha (0.5f));
            g.drawRect (getLocalBounds(), 2);
            #endif

            #if JUCE_DEBUG && DEBUG_SHOW_FRAME_INDEX
            // Optional: Show current frame index for debugging
            g.setColour (juce::Colours::yellow);
            g.setFont (10.0f);
            g.drawText (juce::String (frameIndex), getLocalBounds().removeFromBottom (15), juce::Justification::centred);
            #endif
        }

        int getFrameWidth() const { return frameWidth; }
        int getFrameHeight() const { return frameHeight; }
        int getNumFrames() const { return numFrames; }
        bool isAssetValid() const { return filmstrip.isValid(); }

    private:
        juce::Image filmstrip;
        juce::String assetFileName;
        bool isVertical = true;
        int frameWidth = 0;
        float frameHeightF = 0.0f;
        int frameHeight = 0;
        int numFrames = 0;
    };

    //==============================================================================
    // Filmstrip horizontal slider - displays frames from horizontal filmstrip
    class FilmstripSlider : public juce::Slider
    {
    public:
        FilmstripSlider (const juce::String& filmstripFileName)
            : juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox),
              assetFileName (filmstripFileName)
        {
            setLookAndFeel (&getFilmstripLookAndFeel());

            // CRITICAL: setOpaque tells JUCE to clear before paint
            // DO NOT use setBufferedToImage - it caches old frames!
            setOpaque (true);

            // Set mouse drag sensitivity for smooth control
            setMouseDragSensitivity (128);

            filmstrip = loadImageFromBinary (filmstripFileName);
            if (filmstrip.isValid())
            {
                frameHeight = filmstrip.getHeight();  // 14 pixels
                // Slider filmstrips are 14177x14 with 127 frames of ~112x14 each
                numFrames = 127;
                frameWidthF = (float) filmstrip.getWidth() / (float) numFrames;
                frameWidth = juce::roundToInt (frameWidthF);

                #if JUCE_DEBUG
                DBG ("FilmstripSlider '" << filmstripFileName << "': " << filmstrip.getWidth() << "x" << filmstrip.getHeight()
                     << " => " << numFrames << " frames (frame size: " << frameWidth << "x" << frameHeight << ")");
                #endif
            }
            else
            {
                DBG ("ERROR: FilmstripSlider failed to load: " << filmstripFileName);
            }

            setRange (0.0, 1.0, 0.0);
        }

        ~FilmstripSlider() override { setLookAndFeel (nullptr); }

        void paint (juce::Graphics& g) override
        {
            #if JUCE_DEBUG
            static bool firstPaint = true;
            if (firstPaint)
            {
                DBG ("FilmstripSlider::paint() called for: " << assetFileName);
                firstPaint = false;
            }
            #endif

            if (!filmstrip.isValid() || numFrames <= 0)
            {
                // Fallback rendering when asset fails to load
                g.setColour (juce::Colours::darkgrey);
                g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);

                #if JUCE_DEBUG
                // Draw a red border to indicate missing asset
                g.setColour (juce::Colours::red);
                g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);

                // Draw text indicating error
                g.setFont (8.0f);
                g.drawText ("MISSING", getLocalBounds(), juce::Justification::centred);
                #endif
                return;
            }

            // Calculate normalized value (0.0 to 1.0)
            auto normValue = juce::jlimit (0.0f, 1.0f,
                static_cast<float> ((getValue() - getMinimum()) / (getMaximum() - getMinimum())));

            // Determine which frame to display (forum solution: add 0.5 for proper rounding)
            const int frameIndex = juce::jlimit (0, numFrames - 1,
                static_cast<int> (0.5f + normValue * static_cast<float> (numFrames - 1)));

            // Calculate source rectangle using floating widths to avoid drift
            const float startXf = frameWidthF * static_cast<float> (frameIndex);
            const float endXf   = frameWidthF * static_cast<float> (frameIndex + 1);
            const int sourceX   = juce::roundToInt (startXf);
            int sourceW         = juce::roundToInt (endXf - startXf);
            sourceW = juce::jlimit (1, filmstrip.getWidth() - sourceX, sourceW);

            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (filmstrip,
                        0, 0, getWidth(), getHeight(),              // destination rectangle
                        sourceX, 0, sourceW, frameHeight,           // source rectangle from filmstrip
                        false);                                      // don't fill alpha channel

            #if JUCE_DEBUG
            // Draw a green border to confirm new code is running
            g.setColour (juce::Colours::green.withAlpha (0.5f));
            g.drawRect (getLocalBounds(), 2);
            #endif

            #if JUCE_DEBUG && DEBUG_SHOW_FRAME_INDEX
            // Optional: Show current frame index for debugging
            g.setColour (juce::Colours::yellow);
            g.setFont (8.0f);
            g.drawText (juce::String (frameIndex), getLocalBounds().removeFromTop (12), juce::Justification::centred);
            #endif
        }

        int getFrameWidth() const { return frameWidth; }
        int getFrameHeight() const { return frameHeight; }
        int getNumFrames() const { return numFrames; }
        bool isAssetValid() const { return filmstrip.isValid(); }

    private:
        juce::Image filmstrip;
        juce::String assetFileName;
        float frameWidthF = 111.63f;  // 14177 / 127 frames
        int frameWidth = 112;
        int frameHeight = 14;
        int numFrames = 127;
    };

    //==============================================================================
    // Power button (filmstrip toggle)
    class PowerButton : public juce::ToggleButton
    {
    public:
        PowerButton()
        {
            filmstrip = loadImageFromBinary ("Power Button.png");
            if (filmstrip.isValid())
            {
                frameWidth = filmstrip.getWidth();
                frameHeight = filmstrip.getHeight() / 2;
            }
            setClickingTogglesState (true);
        }

        void paintButton (juce::Graphics& g, bool, bool) override
        {
            if (!filmstrip.isValid())
            {
                g.setColour (getToggleState() ? juce::Colours::cyan : juce::Colours::darkgrey);
                g.fillEllipse (getLocalBounds().toFloat().reduced (2.0f));
                return;
            }

            int frameIndex = getToggleState() ? 1 : 0;
            juce::Rectangle<int> srcRect = { 0, frameIndex * frameHeight, frameWidth, frameHeight };
            auto frame = filmstrip.getClippedImage (srcRect);
            g.drawImageWithin (frame, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred);
        }

    private:
        juce::Image filmstrip;
        int frameWidth = 0;
        int frameHeight = 0;
    };

    //==============================================================================
    // Toggle switch (filmstrip) - uses first and last frames for off/on states
    class ToggleSwitch : public juce::ToggleButton
    {
    public:
        ToggleSwitch()
        {
            filmstrip = loadImageFromBinary ("Toggle.png");
            if (filmstrip.isValid())
            {
                frameWidth = filmstrip.getWidth();
                // Toggle.png has 8 frames vertically stacked (animation from off to on)
                numFrames = 8;
                frameHeight = filmstrip.getHeight() / numFrames;
            }
            setClickingTogglesState (true);
        }

        void paintButton (juce::Graphics& g, bool, bool) override
        {
            if (!filmstrip.isValid())
            {
                g.setColour (getToggleState() ? juce::Colours::cyan : juce::Colours::darkgrey);
                g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
                return;
            }

            // Use first frame for off, last frame for on
            int frameIndex = getToggleState() ? (numFrames - 1) : 0;
            juce::Rectangle<int> srcRect = { 0, frameIndex * frameHeight, frameWidth, frameHeight };
            auto frame = filmstrip.getClippedImage (srcRect);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImageWithin (frame, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred);
        }

        int getFrameWidth() const { return frameWidth; }
        int getFrameHeight() const { return frameHeight; }

    private:
        juce::Image filmstrip;
        int frameWidth = 37;
        int frameHeight = 25;
        int numFrames = 8;
    };

    //==============================================================================
    // Dark look and feel for standard components
    class DarkLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkLookAndFeel()
        {
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::thumbColourId, juce::Colour::fromRGB (255, 123, 131));
            setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (45, 48, 70));
            setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (40, 42, 60));
            setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (50, 45, 65));
            setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            setColour (juce::ComboBox::textColourId, juce::Colours::white);
            setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
            setColour (juce::ToggleButton::textColourId, juce::Colours::white);
            setColour (juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB (30, 32, 45));
            setColour (juce::PopupMenu::textColourId, juce::Colours::white);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB (60, 65, 90));
        }

        void drawComboBox (juce::Graphics& g, int width, int height, bool,
                           int, int, int, int, juce::ComboBox& box) override
        {
            auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();
            g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
            g.fillRoundedRectangle (bounds, 4.0f);

            auto arrowZone = bounds.removeFromRight (25.0f).reduced (8.0f);
            juce::Path arrow;
            arrow.addTriangle (arrowZone.getX(), arrowZone.getCentreY() - 2,
                               arrowZone.getRight(), arrowZone.getCentreY() - 2,
                               arrowZone.getCentreX(), arrowZone.getCentreY() + 4);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillPath (arrow);
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float, float,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            if (style == juce::Slider::LinearVertical)
            {
                auto trackWidth = 10.0f;
                auto trackX = (float) x + ((float) width - trackWidth) * 0.5f;

                // Track background
                g.setColour (slider.findColour (juce::Slider::trackColourId));
                g.fillRoundedRectangle (trackX, (float) y, trackWidth, (float) height, 5.0f);

                // Filled portion
                auto fillHeight = (float) height - (sliderPos - (float) y);
                g.setColour (slider.findColour (juce::Slider::thumbColourId));
                g.fillRoundedRectangle (trackX, sliderPos, trackWidth, fillHeight, 5.0f);

                // Thumb
                auto thumbSize = 16.0f;
                g.setColour (juce::Colours::white);
                g.fillEllipse (trackX + trackWidth * 0.5f - thumbSize * 0.5f,
                               sliderPos - thumbSize * 0.5f, thumbSize, thumbSize);
            }
            else
            {
                LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0, 0, style, slider);
            }
        }
    };

} // namespace soulbass
