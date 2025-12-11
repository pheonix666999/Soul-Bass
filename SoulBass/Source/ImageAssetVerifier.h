#pragma once

#include <JuceHeader.h>
#include "BinaryData.h"

namespace soulbass
{
    /**
     * Utility class to verify and debug image assets at runtime
     * Helps identify issues with filmstrip images and component rendering
     */
    class ImageAssetVerifier
    {
    public:
        struct AssetInfo
        {
            juce::String name;
            int width = 0;
            int height = 0;
            bool isValid = false;
            int estimatedFrames = 0;
            juce::String message;
        };

        /**
         * Verify a dial/knob filmstrip asset (vertical strip)
         * @param fileName Name of the asset file (e.g., "Dial On.png")
         * @return AssetInfo structure with validation details
         */
        static AssetInfo verifyDialAsset (const juce::String& fileName)
        {
            AssetInfo info;
            info.name = fileName;

            // Load the image using the same method as the actual components
            auto resourceName = toResourceName (fileName);
            int dataSize = 0;
            if (auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize))
            {
                juce::MemoryInputStream stream (data, (size_t) dataSize, false);
                auto image = juce::ImageFileFormat::loadFrom (stream);

                if (image.isValid())
                {
                    info.isValid = true;
                    info.width = image.getWidth();
                    info.height = image.getHeight();

                    // For vertical filmstrips, frames are stacked vertically
                    // Each frame should be square (width x width)
                    if (info.width > 0)
                    {
                        info.estimatedFrames = info.height / info.width;
                        float remainder = (float) info.height / (float) info.width - info.estimatedFrames;

                        if (remainder > 0.01f)
                        {
                            info.message = "WARNING: Height not evenly divisible by width. May cause rendering issues.";
                        }
                        else
                        {
                            info.message = "OK: Dial filmstrip appears valid (" + juce::String (info.estimatedFrames) + " frames)";
                        }
                    }
                }
                else
                {
                    info.message = "ERROR: Image loaded but is invalid (possibly corrupted)";
                }
            }
            else
            {
                info.message = "ERROR: Failed to load resource: " + fileName + " -> " + resourceName;
            }

            return info;
        }

        /**
         * Verify a slider filmstrip asset (horizontal strip)
         * @param fileName Name of the asset file (e.g., "Attack Slider.png")
         * @param expectedFrames Expected number of frames (default 63)
         * @return AssetInfo structure with validation details
         */
        static AssetInfo verifySliderAsset (const juce::String& fileName, int expectedFrames = 63)
        {
            AssetInfo info;
            info.name = fileName;

            auto resourceName = toResourceName (fileName);
            int dataSize = 0;
            if (auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize))
            {
                juce::MemoryInputStream stream (data, (size_t) dataSize, false);
                auto image = juce::ImageFileFormat::loadFrom (stream);

                if (image.isValid())
                {
                    info.isValid = true;
                    info.width = image.getWidth();
                    info.height = image.getHeight();

                    // For horizontal filmstrips, frames are side by side
                    float frameWidth = (float) info.width / (float) expectedFrames;
                    info.estimatedFrames = expectedFrames;

                    if (std::abs (frameWidth - std::round (frameWidth)) > 0.5f)
                    {
                        info.message = "WARNING: Width not evenly divisible by " + juce::String (expectedFrames) +
                                       " frames. Frame width: " + juce::String (frameWidth, 2);
                    }
                    else
                    {
                        info.message = "OK: Slider filmstrip appears valid (frame size: " +
                                       juce::String ((int) frameWidth) + "x" + juce::String (info.height) + ")";
                    }
                }
                else
                {
                    info.message = "ERROR: Image loaded but is invalid (possibly corrupted)";
                }
            }
            else
            {
                info.message = "ERROR: Failed to load resource: " + fileName + " -> " + resourceName;
            }

            return info;
        }

        /**
         * Verify all dial and slider assets used in the plugin
         * Prints results to console using DBG()
         */
        static void verifyAllAssets()
        {
            DBG ("===== SoulBass Image Asset Verification =====");

            // Verify dial assets
            DBG ("\n--- Dial Assets ---");
            auto dialOn = verifyDialAsset ("Dial On.png");
            printInfo (dialOn);

            auto dialOff = verifyDialAsset ("Dial Off.png");
            printInfo (dialOff);

            // Verify slider assets
            DBG ("\n--- Slider Assets ---");
            juce::StringArray sliderFiles = {
                "Attack Slider.png",
                "Decay Slider.png",
                "Sustain Slider.png",
                "Release Slider.png",
                "Smoothing Slider.png",
                "Phase Slider.png",
                "Intensity Slider.png",
                "Time Slider.png"
            };

            for (auto& fileName : sliderFiles)
            {
                auto info = verifySliderAsset (fileName);
                printInfo (info);
            }

            DBG ("\n==============================================");
        }

    private:
        static void printInfo (const AssetInfo& info)
        {
            DBG (info.name << ": " << info.width << "x" << info.height <<
                 " (" << info.estimatedFrames << " frames) - " << info.message);
        }

        // Same resource name conversion as in SoulLookAndFeel.h
        static juce::String toResourceName (const juce::String& fileName)
        {
            juce::String result;
            for (int i = 0; i < fileName.length(); ++i)
            {
                auto c = fileName[i];
                if (juce::CharacterFunctions::isLetter (c) || juce::CharacterFunctions::isDigit (c))
                    result << c;
                else if (c == ' ' || c == '.')
                    result << '_';
            }

            if (result.isNotEmpty() && juce::CharacterFunctions::isDigit (result[0]))
                result = "_" + result;

            return result;
        }
    };

} // namespace soulbass
