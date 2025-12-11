#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SoulLookAndFeel.h"
#include "ImageAssetVerifier.h"

class SoulBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SoulBassAudioProcessorEditor (SoulBassAudioProcessor&);
    ~SoulBassAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SoulBassAudioProcessor& processor;

    soulbass::DarkLookAndFeel darkLaf;

    // Background and overlay images
    juce::Image background;
    juce::Image pianoOff;
    juce::Image pianoOn;
    juce::Image logo12bit;
    juce::Image logoSoulBass;

    // LFO Section
    std::unique_ptr<soulbass::FilmstripSlider> attackSlider;
    std::unique_ptr<soulbass::FilmstripSlider> decaySlider;
    std::unique_ptr<soulbass::FilmstripSlider> sustainSlider;
    std::unique_ptr<soulbass::FilmstripSlider> releaseSlider;
    std::unique_ptr<soulbass::FilmstripSlider> smoothingSlider;
    std::unique_ptr<soulbass::FilmstripSlider> phaseSlider;
    std::unique_ptr<soulbass::FilmstripSlider> intensitySlider;
    soulbass::ToggleSwitch tempoSyncToggle;
    soulbass::PowerButton lfoPowerBtn;

    // EQ Section
    std::unique_ptr<soulbass::FilmstripKnob> eqLowKnob, eqLowGainKnob, eqLowQKnob;
    std::unique_ptr<soulbass::FilmstripKnob> eqMidKnob, eqMidGainKnob, eqMidQKnob;
    std::unique_ptr<soulbass::FilmstripKnob> eqHighKnob, eqHighGainKnob, eqHighQKnob;
    soulbass::PowerButton eqPowerBtn;

    // Dynamics Section
    std::unique_ptr<soulbass::FilmstripKnob> thresholdKnob, dynAttackKnob, ratioKnob, dynReleaseKnob;
    soulbass::ToggleSwitch compLimitToggle;
    soulbass::PowerButton dynPowerBtn;

    // Shaper Section
    std::unique_ptr<soulbass::FilmstripKnob> driveKnob, biasKnob;
    juce::ComboBox shaperTypeBox;
    soulbass::PowerButton shaperPowerBtn;

    // Chorus Section
    std::unique_ptr<soulbass::FilmstripKnob> chorusRateKnob, chorusBlendKnob;
    soulbass::PowerButton chorusPowerBtn;

    // Delay Section
    std::unique_ptr<soulbass::FilmstripKnob> delayTimeKnob, delayFeedbackKnob;
    soulbass::PowerButton delayPowerBtn;

    // Reverb Section
    std::unique_ptr<soulbass::FilmstripKnob> reverbBlendKnob, reverbDecayKnob;
    juce::ComboBox reverbTypeBox;
    soulbass::PowerButton reverbPowerBtn;

    // Legato Section
    soulbass::ToggleSwitch legatoToggle;
    soulbass::ToggleSwitch retriggerToggle;
    juce::ComboBox polyBox;

    // Filter Bar
    juce::ComboBox filterTypeBox;
    soulbass::ToggleSwitch glideToggle;
    juce::ComboBox glideDirectionBox;
    juce::ComboBox pitchRangeBox;
    std::unique_ptr<soulbass::FilmstripSlider> filterTimeSlider;

    // Input/Output Gains
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;

    // Header
    juce::ComboBox presetBox;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoPhaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoSmoothAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoSyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoPowerAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqLowFreqAttachment, eqLowGainAttachment, eqLowQAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqMidFreqAttachment, eqMidGainAttachment, eqMidQAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqHighFreqAttachment, eqHighGainAttachment, eqHighQAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynLimitAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dynThresholdAttachment, dynAttackAttachment, dynRatioAttachment, dynReleaseAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shaperPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shaperDriveAttachment, shaperBiasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shaperTypeAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chorusRateAttachment, chorusBlendAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment, delayFeedbackAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbPowerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbBlendAttachment, reverbDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbTypeAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> legatoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> retriggerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> polyAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> glideAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> glideDirectionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> pitchRangeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoulBassAudioProcessorEditor)
};
