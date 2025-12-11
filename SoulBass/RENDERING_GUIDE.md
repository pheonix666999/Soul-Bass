# SoulBass VST - Slider & Knob Rendering Guide

## Overview
This guide explains how to ensure proper rendering of filmstrip-based sliders and knobs in the SoulBass VST plugin.

## Filmstrip Animation Technique

### What is Filmstrip Animation?
Filmstrip animation is a technique where multiple frames of an animation are stored in a single image file, either:
- **Vertical Filmstrip**: Frames stacked vertically (used for knobs/dials)
- **Horizontal Filmstrip**: Frames arranged horizontally (used for sliders)

The component extracts and displays the appropriate frame based on the control's current value.

## Asset Requirements

### Dial/Knob Filmstrips (Vertical)
- **File**: `Dial On.png`
- **Layout**: Vertical strip where each frame is square
- **Expected Dimensions**: Width × (Width × NumFrames)
- **Example**: 83×5501 px = 83px wide × 66 frames (83px each)
- **Frame Calculation**: `numFrames = imageHeight / imageWidth`

### Slider Filmstrips (Horizontal)
- **Files**: `Attack Slider.png`, `Decay Slider.png`, etc.
- **Layout**: Horizontal strip with 63 frames
- **Expected Dimensions**: (FrameWidth × 63) × FrameHeight
- **Frame Count**: Fixed at 63 frames
- **Frame Calculation**: `frameWidth = imageWidth / 63`

## How the Rendering Works

### FilmstripKnob ([SoulLookAndFeel.h:62-158](SoulLookAndFeel.h#L62-L158))
```cpp
// 1. Load the filmstrip image
filmstrip = loadImageFromBinary("Dial On.png");

// 2. Calculate frame dimensions
frameWidth = filmstrip.getWidth();
numFrames = filmstrip.getHeight() / frameWidth;

// 3. On paint, normalize the knob value (0.0 to 1.0)
normValue = (getValue() - getMinimum()) / (getMaximum() - getMinimum());

// 4. Determine which frame to display
frameIndex = round(normValue * (numFrames - 1));

// 5. Extract the frame from the filmstrip
y0 = frameHeightF * frameIndex;
y1 = frameHeightF * (frameIndex + 1);
frame = filmstrip.getClippedImage(Rectangle{0, y0, frameWidth, height});

// 6. Draw with high quality resampling
g.drawImageWithin(frame, 0, 0, getWidth(), getHeight(), centred);
```

### FilmstripSlider ([SoulLookAndFeel.h:161-253](SoulLookAndFeel.h#L161-L253))
Same principle, but extracts frames horizontally:
```cpp
frameWidthF = filmstrip.getWidth() / 63.0f;
frameIndex = round(normValue * 62);  // 0-62 for 63 frames
x0 = frameWidthF * frameIndex;
frame = filmstrip.getClippedImage(Rectangle{x0, 0, width, frameHeight});
```

## Component Bounds vs Asset Dimensions

### Current Component Sizes ([PluginEditor.cpp:329-432](PluginEditor.cpp#L329-L432))

**EQ Knobs** (Main knobs):
- Size: 55×55 px
- Asset frame size: 83×83 px
- Scaling: Downscaled with high-quality resampling

**Side Panel Knobs** (Shaper, Chorus, Delay, Reverb):
- Size: 48×48 px
- Asset frame size: 83×83 px
- Scaling: Downscaled with high-quality resampling

**Sliders** (LFO section):
- Size: 120×20 px
- Asset frame size: ~127×14 px (varies by slider)
- Scaling: Slightly downscaled

### Best Practices
1. **Asset dimensions should be larger than component bounds** for crisp rendering
2. **Use high-quality resampling** (already implemented via `Graphics::highResamplingQuality`)
3. **Maintain aspect ratio** to avoid distortion
4. **Test at different UI scales** (100%, 150%, 200%)

## Debugging Tools

### Asset Verification
Run the plugin in DEBUG mode to see automatic asset verification:

```cpp
// In PluginEditor.cpp constructor:
#if JUCE_DEBUG
soulbass::ImageAssetVerifier::verifyAllAssets();
#endif
```

This will output to console:
```
===== SoulBass Image Asset Verification =====

--- Dial Assets ---
Dial On.png: 83x5501 (66 frames) - OK: Dial filmstrip appears valid (66 frames)

--- Slider Assets ---
Attack Slider.png: 7938x14 (63 frames) - OK: Slider filmstrip appears valid (frame size: 126x14)
...
```

### Visual Debugging
In DEBUG builds, missing assets will show:
- **Knobs**: Grey circle with red X and "MISSING" text
- **Sliders**: Grey rounded rectangle with red border and "MISSING" text

### Frame Index Display (Optional)
Enable by defining `DEBUG_SHOW_FRAME_INDEX`:
```cpp
#define DEBUG_SHOW_FRAME_INDEX 1  // Add to preprocessor definitions
```
This will show the current frame number overlaid on each control.

## Common Issues & Solutions

### Issue 1: Blurry or Pixelated Rendering
**Cause**: Asset resolution too low for component size
**Solution**:
- Use assets at least 2x the component size for retina displays
- Current setup already uses 83px assets for 48-55px components ✓

### Issue 2: Animation Appears Jerky
**Cause**: Not enough frames in filmstrip
**Solution**:
- Knobs use 66 frames (270° / 66 = ~4° per frame) ✓
- Sliders use 63 frames ✓
- This provides smooth animation

### Issue 3: Wrong Frame Displayed
**Cause**:
- Filmstrip dimensions not evenly divisible
- Frame calculation drift

**Solution**: Already implemented via float-based slicing:
```cpp
frameHeightF = (float)height / (float)numFrames;  // Use float precision
y0 = floor(frameHeightF * frameIndex);           // Avoid accumulation errors
```

### Issue 4: Asset Failed to Load
**Check**:
1. File exists in `Resources/GUI/Dials/` or `Resources/GUI/Sliders/`
2. File added to CMake via `juce_add_binary_data`
3. Resource name conversion matches (spaces→underscores, etc.)

## Optimizing Performance

### Memory Usage
- Each knob loads the same `Dial On.png` (803 KB)
- **Current**: 18 knobs × 803 KB = ~14.5 MB
- **Optimization**: Share single image instance across all knobs

### Rendering Performance
✓ Already optimized:
- `highResamplingQuality` only when drawing (not real-time)
- Single frame extraction per paint (not entire strip)
- Efficient `getClippedImage()` creates lightweight sub-image

## Asset Creation Guidelines

### For Designers Creating Filmstrips

**Dial/Knob Filmstrips**:
1. Create 66 frames of knob rotation (0° to 270°)
2. Each frame: 83×83 px (or higher resolution, keep square)
3. Stack vertically in order: frame 0 (minimum) at top, frame 65 (maximum) at bottom
4. Total height: 83 × 66 = 5478 px
5. Export as PNG with transparency

**Slider Filmstrips**:
1. Create 63 frames of slider animation
2. Each frame: 126×14 px (or proportionally larger)
3. Arrange horizontally: frame 0 (minimum) at left, frame 62 (maximum) at right
4. Total width: 126 × 63 = 7938 px
5. Export as PNG with transparency

## Testing Checklist

- [ ] Build in DEBUG mode and check console for asset verification
- [ ] All assets show "OK" status
- [ ] No "MISSING" indicators visible on any controls
- [ ] Knobs rotate smoothly through full 270° range
- [ ] Sliders animate smoothly from 0% to 100%
- [ ] No visual glitches at minimum/maximum values
- [ ] Controls look crisp at 100%, 150%, and 200% UI scale
- [ ] Memory usage acceptable (check in profiler)

## Current Implementation Status

✅ **Implemented**:
- Filmstrip loading and frame extraction
- High-quality resampling for crisp rendering
- Float-based frame calculation (no drift)
- 270-degree rotary arc for natural knob feel
- Automatic asset verification in DEBUG builds
- Fallback rendering for missing assets
- Debug visualization for troubleshooting

✅ **Working Components**:
- 9 EQ knobs (Low/Mid/High × Freq/Gain/Q)
- 4 Dynamics knobs (Threshold, Attack, Ratio, Release)
- 6 Effects knobs (Drive, Bias, Chorus×2, Delay×2)
- 2 Reverb knobs (Blend, Decay)
- 7 LFO sliders (Attack, Decay, Sustain, Release, Smoothing, Phase, Intensity)
- 1 Filter slider (Time/Cutoff)

## Additional Resources

- **JUCE Documentation**: https://docs.juce.com/master/tutorial_slider_values.html
- **Asset Files**: `h:\PROJECTS\Fiverr\VST\SoulBass\Resources\GUI\`
- **Implementation**: [SoulLookAndFeel.h](SoulLookAndFeel.h)
- **Usage**: [PluginEditor.cpp](PluginEditor.cpp)
- **Verification Utility**: [ImageAssetVerifier.h](ImageAssetVerifier.h)

---

## Quick Start

To verify your assets are rendering correctly:

1. **Build in DEBUG mode**:
   ```bash
   cmake --build build --config Debug
   ```

2. **Launch the plugin** in your DAW or standalone

3. **Check the console output** for asset verification results

4. **Test all controls**:
   - Rotate each knob from minimum to maximum
   - Drag each slider from left to right
   - Verify smooth animation and crisp rendering

5. **If issues occur**, check:
   - Console for ERROR messages
   - Visual MISSING indicators
   - Asset file existence and dimensions

Need help? Check the [ImageAssetVerifier.h](ImageAssetVerifier.h) utility for diagnostic tools.
