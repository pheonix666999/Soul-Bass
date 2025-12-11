cmake_minimum_required(VERSION 3.15)

project(SoulBass VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE is vendored in the repo.
add_subdirectory(JUCE)

# Collect resources (GUI + samples) into BinaryData for easy cross‑platform bundling.
set(SOULBASS_RESOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SoulBass/Resources")

file(GLOB_RECURSE SOULBASS_GUI_ASSETS
     "${SOULBASS_RESOURCE_ROOT}/GUI/*.png")
file(GLOB SOULBASS_SAMPLE_FILES
     "${SOULBASS_RESOURCE_ROOT}/Samples/*.wav")

juce_add_binary_data(SoulBassBinaryData
    SOURCES
        ${SOULBASS_GUI_ASSETS}
        ${SOULBASS_SAMPLE_FILES}
)

juce_add_plugin(SoulBass
    COMPANY_NAME "12 Bit Soul"
    BUNDLE_ID com.twelvebitsoul.soulbass
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    PRODUCT_NAME "Soul Bass"
)

juce_generate_juce_header(SoulBass)

target_sources(SoulBass PRIVATE
    SoulBass/Source/PluginProcessor.cpp
    SoulBass/Source/PluginProcessor.h
    SoulBass/Source/PluginEditor.cpp
    SoulBass/Source/PluginEditor.h
    SoulBass/Source/SoulLookAndFeel.h
    SoulBass/Source/SoulSampler.h
)

target_compile_definitions(SoulBass
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SoulBass
    PRIVATE
        SoulBassBinaryData
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

if (WIN32)
    target_compile_definitions(SoulBass PRIVATE NOMINMAX)
endif()
