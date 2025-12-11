@echo off
REM SoulBass VST - Quick Debug Build and Test Script
REM This script builds the plugin in DEBUG mode with asset verification enabled

echo ========================================
echo SoulBass VST - Debug Build & Test
echo ========================================
echo.

echo [1/4] Cleaning previous build...
if exist build (
    echo Removing old build directory...
    rd /s /q build
)

echo.
echo [2/4] Configuring CMake...
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Building in Debug mode...
cmake --build build --config Debug --clean-first
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Build successful!
echo.
echo ========================================
echo RENDERING FIXES APPLIED - TESTING GUIDE
echo ========================================
echo.
echo WHAT WAS FIXED:
echo - Sliders now show SINGLE handle (not multiple circles)
echo - Knobs now show SINGLE frame (not overlapping images)
echo - Proper frame extraction from filmstrips
echo - Transparent backgrounds to prevent artifacts
echo.
echo HOW TO TEST:
echo.
echo 1. Launch the standalone executable:
echo    build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe
echo.
echo 2. Or load the VST3 in your DAW:
echo    build\SoulBass_artefacts\Debug\VST3\SoulBass.vst3
echo.
echo 3. Visual checks - Each control should show:
echo    [x] KNOBS: Single clear image that rotates smoothly (270 degrees)
echo    [x] SLIDERS: Single image that animates left-to-right
echo    [x] NO multiple overlapping frames
echo    [x] NO flickering or artifacts
echo.
echo 4. Functional checks:
echo    [x] LFO sliders: Drag from extreme left to extreme right
echo    [x] EQ knobs: Rotate each knob through full range
echo    [x] Dynamics knobs: Smooth rotation with single image
echo    [x] Filter cutoff: Drag slider to both extremes
echo.
echo 5. Check console output (if visible):
echo    - Should show "OK" for all assets
echo    - No "ERROR" or "MISSING" messages
echo.
echo 6. Read RENDERING_FIXES.md for detailed explanation
echo.
echo ========================================

pause
