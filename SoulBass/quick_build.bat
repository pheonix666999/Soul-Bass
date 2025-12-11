@echo off
echo ========================================
echo Building SoulBass VST...
echo ========================================
echo.

cd /d "%~dp0"

echo Configuring CMake...
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    pause
    exit /b 1
)

echo.
echo Building Debug version...
cmake --build build --config Debug
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed!
    echo Check the error messages above.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executable location:
echo build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe
echo.
echo Press any key to launch the plugin...
pause

start "" "build\SoulBass_artefacts\Debug\Standalone\SoulBass.exe"
