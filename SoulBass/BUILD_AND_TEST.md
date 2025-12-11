# Build and Test Instructions

## The plugin hasn't been built yet with the new changes!

Follow these steps carefully:

## Step 1: Build the Plugin

### Option A: Use the Quick Build Script (RECOMMENDED)
```batch
quick_build.bat
```

This will:
1. Configure CMake
2. Build the Debug version
3. Automatically launch the plugin when complete

### Option B: Manual Build
```batch
# Configure CMake
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# Build Debug version
cmake --build build --config Debug
```

### Option C: Build in Visual Studio
1. Open `build\SoulBass.sln` in Visual Studio
2. Set configuration to **Debug**
3. Build > Build Solution (F7)

## Step 2: Verify Build Success

Check that this file exists:
```
build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe
```

If it doesn't exist, the build failed. Check the error messages.

## Step 3: Run the Plugin

### Launch Standalone:
```
build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe
```

Or double-click:
```
build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe
```

### Load VST3 in DAW:
```
build\SoulBass_artefacts\Debug\VST3\SoulBass.vst3
```

## Step 4: What to Look For

### ✅ New Code Indicator (DEBUG builds only)
All knobs and sliders will have a **GREEN BORDER** around them. This confirms the new rendering code is running.

If you DON'T see green borders:
- The build didn't work
- You're running an old cached version
- You built Release instead of Debug

### ✅ Expected Behavior
1. **Knobs**: Single clear image that rotates smoothly (no overlapping circles)
2. **Sliders**: Single slider handle that moves left-to-right (no multiple circles)
3. **Green borders**: Visible around all knobs and sliders (DEBUG only)

### ❌ Old Behavior (What you saw before)
1. Multiple overlapping circular frames on knobs
2. Multiple white circles on sliders
3. No green borders

## Step 5: Check Console Output

If you run from command prompt or Visual Studio, you should see:

```
===== SoulBass Image Asset Verification =====

--- Dial Assets ---
Dial On.png: 83x5501 (66 frames) - OK

--- Slider Assets ---
Attack Slider.png: 7938x14 (63 frames) - OK
...

FilmstripKnob::paint() called for: Dial On.png
FilmstripSlider::paint() called for: Attack Slider.png
...
```

## Troubleshooting

### "Build does not exist"
Run `quick_build.bat` or manually build with CMake

### "Still seeing multiple frames"
1. Verify you have GREEN BORDERS (confirms new code)
2. If no green borders, rebuild completely:
   ```batch
   rd /s /q build
   quick_build.bat
   ```

### "Build fails"
1. Check you have Visual Studio 2022 installed
2. Check CMake is in your PATH
3. Read error messages carefully
4. Post the error message for help

### "Can't find the executable"
Look here:
```
build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe
```

Not here:
```
build\Debug\SoulBass.exe  ❌ (wrong location)
```

## Quick Test Checklist

After launching the plugin:

- [ ] Green borders visible around knobs? (If DEBUG build)
- [ ] Green borders visible around sliders? (If DEBUG build)
- [ ] Each knob shows SINGLE image (not multiple)?
- [ ] Knob rotates smoothly when you drag it?
- [ ] Each slider shows SINGLE handle (not multiple)?
- [ ] Slider moves from far left to far right?
- [ ] No flickering or visual glitches?

If ALL checkboxes are ✅ then the fix is working!

## Removing Debug Borders (After Testing)

Once you confirm it's working, build Release version for clean look:

```batch
cmake --build build --config Release
```

Release builds won't have green borders.

---

**Current Status**: Build does not exist yet. Please run `quick_build.bat`
