#include "PluginEditor.h"

SoulBassAudioProcessorEditor::SoulBassAudioProcessorEditor (SoulBassAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&darkLaf);

    // Verify all image assets on startup (debug builds only)
    #if JUCE_DEBUG
    soulbass::ImageAssetVerifier::verifyAllAssets();
    #endif

    // Load images
    background = soulbass::loadImageFromBinary ("Main Background.png");
    pianoOff = soulbass::loadImageFromBinary ("Piano Roll Off.png");
    logo12bit = soulbass::loadImageFromBinary ("12bitsoul-logo.png");
    logoSoulBass = soulbass::loadImageFromBinary ("soulbass-logo.png");

    // Create LFO section sliders
    attackSlider = std::make_unique<soulbass::FilmstripSlider> ("Attack Slider.png");
    decaySlider = std::make_unique<soulbass::FilmstripSlider> ("Decay Slider.png");
    sustainSlider = std::make_unique<soulbass::FilmstripSlider> ("Sustain Slider.png");
    releaseSlider = std::make_unique<soulbass::FilmstripSlider> ("Release Slider.png");
    smoothingSlider = std::make_unique<soulbass::FilmstripSlider> ("Smoothing Slider.png");
    phaseSlider = std::make_unique<soulbass::FilmstripSlider> ("Phase Slider.png");
    intensitySlider = std::make_unique<soulbass::FilmstripSlider> ("Intensity Slider.png");

    // Create all knobs (using Dial On.png filmstrip)
    auto createKnob = []() { return std::make_unique<soulbass::FilmstripKnob> ("Dial On.png"); };

    eqLowKnob = createKnob(); eqLowGainKnob = createKnob(); eqLowQKnob = createKnob();
    eqMidKnob = createKnob(); eqMidGainKnob = createKnob(); eqMidQKnob = createKnob();
    eqHighKnob = createKnob(); eqHighGainKnob = createKnob(); eqHighQKnob = createKnob();

    thresholdKnob = createKnob(); dynAttackKnob = createKnob();
    ratioKnob = createKnob(); dynReleaseKnob = createKnob();

    driveKnob = createKnob(); biasKnob = createKnob();
    chorusRateKnob = createKnob(); chorusBlendKnob = createKnob();
    delayTimeKnob = createKnob(); delayFeedbackKnob = createKnob();
    reverbBlendKnob = createKnob(); reverbDecayKnob = createKnob();

    filterTimeSlider = std::make_unique<soulbass::FilmstripSlider> ("Time Slider.png");

    // Add all components
    addAndMakeVisible (*attackSlider);
    addAndMakeVisible (*decaySlider);
    addAndMakeVisible (*sustainSlider);
    addAndMakeVisible (*releaseSlider);
    addAndMakeVisible (*smoothingSlider);
    addAndMakeVisible (*phaseSlider);
    addAndMakeVisible (*intensitySlider);
    addAndMakeVisible (tempoSyncToggle);
    addAndMakeVisible (lfoPowerBtn);

    addAndMakeVisible (*eqLowKnob); addAndMakeVisible (*eqLowGainKnob); addAndMakeVisible (*eqLowQKnob);
    addAndMakeVisible (*eqMidKnob); addAndMakeVisible (*eqMidGainKnob); addAndMakeVisible (*eqMidQKnob);
    addAndMakeVisible (*eqHighKnob); addAndMakeVisible (*eqHighGainKnob); addAndMakeVisible (*eqHighQKnob);
    addAndMakeVisible (eqPowerBtn);

    addAndMakeVisible (*thresholdKnob); addAndMakeVisible (*dynAttackKnob);
    addAndMakeVisible (*ratioKnob); addAndMakeVisible (*dynReleaseKnob);
    addAndMakeVisible (compLimitToggle);
    addAndMakeVisible (dynPowerBtn);

    addAndMakeVisible (*driveKnob); addAndMakeVisible (*biasKnob);
    addAndMakeVisible (shaperTypeBox);
    addAndMakeVisible (shaperPowerBtn);

    addAndMakeVisible (*chorusRateKnob); addAndMakeVisible (*chorusBlendKnob);
    addAndMakeVisible (chorusPowerBtn);

    addAndMakeVisible (*delayTimeKnob); addAndMakeVisible (*delayFeedbackKnob);
    addAndMakeVisible (delayPowerBtn);

    addAndMakeVisible (*reverbBlendKnob); addAndMakeVisible (*reverbDecayKnob);
    addAndMakeVisible (reverbTypeBox);
    addAndMakeVisible (reverbPowerBtn);

    addAndMakeVisible (legatoToggle);
    addAndMakeVisible (retriggerToggle);
    addAndMakeVisible (polyBox);

    addAndMakeVisible (filterTypeBox);
    addAndMakeVisible (glideToggle);
    addAndMakeVisible (glideDirectionBox);
    addAndMakeVisible (pitchRangeBox);
    addAndMakeVisible (*filterTimeSlider);

    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (outputGainSlider);
    addAndMakeVisible (presetBox);

    // Setup combo boxes
    filterTypeBox.addItem ("CLASSIC LPF", 1);
    filterTypeBox.addItem ("CLASSIC HPF", 2);
    filterTypeBox.setSelectedId (2);

    glideDirectionBox.addItem ("UP", 1);
    glideDirectionBox.addItem ("DOWN", 2);
    glideDirectionBox.setSelectedId (1);

    pitchRangeBox.addItem ("2", 1);
    pitchRangeBox.addItem ("7", 2);
    pitchRangeBox.addItem ("12", 3);
    pitchRangeBox.addItem ("24", 4);
    pitchRangeBox.setSelectedId (3);

    polyBox.addItem ("1", 1);
    polyBox.addItem ("2", 2);
    polyBox.addItem ("3", 3);
    polyBox.addItem ("4", 4);
    polyBox.addItem ("8", 5);
    polyBox.addItem ("16", 6);
    polyBox.setSelectedId (3);

    reverbTypeBox.addItem ("SPRING", 1);
    reverbTypeBox.addItem ("HALL", 2);
    reverbTypeBox.addItem ("PLATE", 3);
    reverbTypeBox.setSelectedId (1);

    shaperTypeBox.addItem ("TYPE", 1);
    shaperTypeBox.addItem ("TUBE", 2);
    shaperTypeBox.addItem ("TAPE", 3);
    shaperTypeBox.setSelectedId (1);

    presetBox.addItem ("BASS 101", 1);
    presetBox.addItem ("Sub Bass", 2);
    presetBox.addItem ("Warm Fuzz", 3);
    presetBox.setSelectedId (1);

    // Setup gain sliders
    inputGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    inputGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    outputGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);

    // Attach parameters
    auto& params = processor.apvts;

    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "attack", *attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "decay", *decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "sustain", *sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "release", *releaseSlider);

    lfoDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "lfoDepth", *intensitySlider);
    lfoPhaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "lfoPhase", *phaseSlider);
    lfoSmoothAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "lfoSmoothing", *smoothingSlider);

    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "filterCutoff", *filterTimeSlider);
    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (params, "filterType", filterTypeBox);

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "inputGain", inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "outputGain", outputGainSlider);

    lfoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "lfoSync", tempoSyncToggle);
    lfoPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "lfoEnabled", lfoPowerBtn);

    eqPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "eqEnabled", eqPowerBtn);
    eqLowFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqLowFreq", *eqLowKnob);
    eqLowGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqLowGain", *eqLowGainKnob);
    eqLowQAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqLowQ", *eqLowQKnob);

    eqMidFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqMidFreq", *eqMidKnob);
    eqMidGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqMidGain", *eqMidGainKnob);
    eqMidQAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqMidQ", *eqMidQKnob);

    eqHighFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqHighFreq", *eqHighKnob);
    eqHighGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqHighGain", *eqHighGainKnob);
    eqHighQAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "eqHighQ", *eqHighQKnob);

    dynPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "dynEnabled", dynPowerBtn);
    dynLimitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "dynLimit", compLimitToggle);
    dynThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "dynThreshold", *thresholdKnob);
    dynAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "dynAttack", *dynAttackKnob);
    dynRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "dynRatio", *ratioKnob);
    dynReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "dynRelease", *dynReleaseKnob);

    shaperPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "shaperEnabled", shaperPowerBtn);
    shaperDriveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "shaperDrive", *driveKnob);
    shaperBiasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "shaperBias", *biasKnob);
    shaperTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (params, "shaperType", shaperTypeBox);

    chorusPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "chorusEnabled", chorusPowerBtn);
    chorusRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "chorusRate", *chorusRateKnob);
    chorusBlendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "chorusBlend", *chorusBlendKnob);

    delayPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "delayEnabled", delayPowerBtn);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "delayTimeMs", *delayTimeKnob);
    delayFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "delayFeedback", *delayFeedbackKnob);

    reverbPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "reverbEnabled", reverbPowerBtn);
    reverbBlendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "reverbBlend", *reverbBlendKnob);
    reverbDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (params, "reverbDecay", *reverbDecayKnob);
    reverbTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (params, "reverbType", reverbTypeBox);

    legatoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "legato", legatoToggle);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "retrigger", retriggerToggle);
    polyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (params, "polyphony", polyBox);

    glideAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (params, "glideEnabled", glideToggle);
    glideDirectionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (params, "glideDirection", glideDirectionBox);
    pitchRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (params, "pitchRange", pitchRangeBox);

    // Set power states
    lfoPowerBtn.setToggleState (true, juce::dontSendNotification);
    eqPowerBtn.setToggleState (true, juce::dontSendNotification);
    dynPowerBtn.setToggleState (true, juce::dontSendNotification);
    shaperPowerBtn.setToggleState (true, juce::dontSendNotification);
    chorusPowerBtn.setToggleState (true, juce::dontSendNotification);
    delayPowerBtn.setToggleState (true, juce::dontSendNotification);
    reverbPowerBtn.setToggleState (true, juce::dontSendNotification);

    setSize (850, 600);
}

SoulBassAudioProcessorEditor::~SoulBassAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SoulBassAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw main background
    if (background.isValid())
        g.drawImageAt (background, 0, 0);
    else
        g.fillAll (juce::Colour::fromRGB (28, 25, 38));

    // Draw piano keyboard (positioned after pitch/mod wheel area)
    if (pianoOff.isValid())
        g.drawImageAt (pianoOff, 140, 480);

    // Draw logos in header
    if (logo12bit.isValid())
        g.drawImageAt (logo12bit, 20, 15);

    if (logoSoulBass.isValid())
        g.drawImage (logoSoulBass, 80, 10, 160, 32, 0, 0, logoSoulBass.getWidth(), logoSoulBass.getHeight());

    // Set font for labels
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (juce::Colours::white.withAlpha (0.85f));

    // ==================== Section Headers ====================
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText ("LFO", 25, 60, 50, 15, juce::Justification::left);
    g.drawText ("EQ", 320, 60, 50, 15, juce::Justification::left);
    g.drawText ("DYNAMICS", 510, 60, 90, 15, juce::Justification::left);
    g.drawText ("SHAPER", 695, 60, 70, 15, juce::Justification::left);
    g.drawText ("CHORUS", 695, 172, 70, 15, juce::Justification::left);
    g.drawText ("DELAY", 695, 258, 70, 15, juce::Justification::left);
    g.drawText ("LEGATO", 25, 312, 70, 15, juce::Justification::left);
    g.drawText ("REVERB", 320, 312, 70, 15, juce::Justification::left);
    g.drawText ("FILTER", 25, 428, 50, 15, juce::Justification::left);

    // ==================== LFO Slider Labels ====================
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.setColour (juce::Colour::fromRGB (100, 200, 230));  // Cyan
    g.drawText ("SMOOTHING", 25, 120, 90, 12, juce::Justification::left);
    g.setColour (juce::Colour::fromRGB (180, 120, 200));  // Purple
    g.drawText ("PHASE", 25, 160, 90, 12, juce::Justification::left);
    g.setColour (juce::Colour::fromRGB (230, 180, 80));   // Yellow/Orange
    g.drawText ("INTENSITY", 25, 200, 90, 12, juce::Justification::left);

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawText ("ATTACK", 160, 95, 70, 12, juce::Justification::left);
    g.drawText ("DECAY", 160, 135, 70, 12, juce::Justification::left);
    g.drawText ("SUSTAIN", 160, 175, 70, 12, juce::Justification::left);
    g.drawText ("RELEASE", 160, 215, 70, 12, juce::Justification::left);

    g.drawText ("TEMPO SYNC", 25, 88, 80, 12, juce::Justification::left);

    // ==================== EQ Labels ====================
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    // Column headers above the knobs
    g.drawText ("FREQ", 330, 80, 45, 12, juce::Justification::centred);
    g.drawText ("GAIN", 388, 80, 45, 12, juce::Justification::centred);
    g.drawText ("Q", 446, 80, 35, 12, juce::Justification::centred);
    // Row labels to the left
    g.drawText ("LOW", 295, 115, 30, 12, juce::Justification::right);
    g.drawText ("MID", 295, 177, 30, 12, juce::Justification::right);
    g.drawText ("HIGH", 290, 239, 35, 12, juce::Justification::right);

    // ==================== Dynamics Labels ====================
    g.setFont (juce::Font (8.0f, juce::Font::bold));
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    // Labels below knobs
    g.drawText ("THRESHOLD", 510, 165, 55, 10, juce::Justification::centred);
    g.drawText ("ATTACK", 582, 165, 55, 10, juce::Justification::centred);
    g.drawText ("RATIO", 510, 233, 55, 10, juce::Justification::centred);
    g.drawText ("RELEASE", 582, 233, 55, 10, juce::Justification::centred);
    // Comp/Limit toggle labels
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText ("COMP/LIMIT", 540, 85, 70, 12, juce::Justification::centred);

    // ==================== Shaper Labels ====================
    g.setFont (juce::Font (8.0f, juce::Font::bold));
    g.drawText ("DRIVE", 695, 162, 48, 10, juce::Justification::centred);
    g.drawText ("BIAS", 752, 162, 48, 10, juce::Justification::centred);

    // ==================== Chorus Labels ====================
    g.drawText ("RATE", 695, 245, 48, 10, juce::Justification::centred);
    g.drawText ("BLEND", 752, 245, 48, 10, juce::Justification::centred);

    // ==================== Delay Labels ====================
    g.drawText ("TIME", 695, 330, 48, 10, juce::Justification::centred);
    g.drawText ("FEEDBACK", 752, 330, 55, 10, juce::Justification::centred);

    // ==================== Legato Labels ====================
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText ("LEGATO", 30, 334, 65, 12, juce::Justification::left);
    g.drawText ("RETRIGGER", 30, 360, 70, 12, juce::Justification::left);
    g.drawText ("POLY", 165, 348, 40, 12, juce::Justification::left);

    // ==================== Reverb Labels ====================
    g.setFont (juce::Font (8.0f, juce::Font::bold));
    g.drawText ("BLEND", 430, 385, 48, 10, juce::Justification::centred);
    g.drawText ("DECAY", 430, 438, 48, 10, juce::Justification::centred);

    // ==================== Filter Bar Labels ====================
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    g.drawText ("GLIDE", 225, 432, 40, 12, juce::Justification::left);
    g.drawText ("RANGE", 410, 432, 48, 12, juce::Justification::left);
    g.drawText ("CUTOFF", 530, 432, 50, 12, juce::Justification::left);

    // ==================== Input/Output Labels ====================
    g.setFont (juce::Font (8.0f, juce::Font::bold));
    g.drawText ("INPUT", 755, 518, 35, 10, juce::Justification::centred);
    g.drawText ("OUTPUT", 800, 518, 45, 10, juce::Justification::centred);
}

void SoulBassAudioProcessorEditor::resized()
{
    // UI sizing - bigger knobs, proper slider heights for filmstrip display
    const int sliderW = 120;
    const int sliderH = 20;          // Taller to show filmstrip properly
    const int knobSize = 55;         // Main knobs (EQ, Dynamics) - BIGGER
    const int smallKnobSize = 48;    // Side panel knobs - BIGGER
    const int toggleW = 40;          // Toggle width to show filmstrip properly
    const int toggleH = 22;          // Toggle height to show filmstrip properly
    const int powerSize = 18;

    // ==================== LFO Section ====================
    lfoPowerBtn.setBounds (268, 58, powerSize, powerSize);
    tempoSyncToggle.setBounds (105, 85, toggleW, toggleH);

    // Left column sliders (Smoothing, Phase, Intensity)
    smoothingSlider->setBounds (25, 135, sliderW, sliderH);
    phaseSlider->setBounds (25, 175, sliderW, sliderH);
    intensitySlider->setBounds (25, 215, sliderW, sliderH);

    // Right column sliders (ADSR)
    attackSlider->setBounds (160, 110, sliderW, sliderH);
    decaySlider->setBounds (160, 150, sliderW, sliderH);
    sustainSlider->setBounds (160, 190, sliderW, sliderH);
    releaseSlider->setBounds (160, 230, sliderW, sliderH);

    // ==================== EQ Section ====================
    eqPowerBtn.setBounds (460, 58, powerSize, powerSize);

    int eqX = 320;
    int eqY = 95;
    int eqKnobGap = 58;    // Gap between knobs in row
    int eqRowGap = 62;     // Gap between rows

    // Row 1: LOW (Freq, Gain, Q)
    eqLowKnob->setBounds (eqX, eqY, knobSize, knobSize);
    eqLowGainKnob->setBounds (eqX + eqKnobGap, eqY, knobSize, knobSize);
    eqLowQKnob->setBounds (eqX + eqKnobGap * 2, eqY, knobSize, knobSize);

    // Row 2: MID
    eqMidKnob->setBounds (eqX, eqY + eqRowGap, knobSize, knobSize);
    eqMidGainKnob->setBounds (eqX + eqKnobGap, eqY + eqRowGap, knobSize, knobSize);
    eqMidQKnob->setBounds (eqX + eqKnobGap * 2, eqY + eqRowGap, knobSize, knobSize);

    // Row 3: HIGH
    eqHighKnob->setBounds (eqX, eqY + eqRowGap * 2, knobSize, knobSize);
    eqHighGainKnob->setBounds (eqX + eqKnobGap, eqY + eqRowGap * 2, knobSize, knobSize);
    eqHighQKnob->setBounds (eqX + eqKnobGap * 2, eqY + eqRowGap * 2, knobSize, knobSize);

    // ==================== DYNAMICS Section ====================
    dynPowerBtn.setBounds (655, 58, powerSize, powerSize);
    compLimitToggle.setBounds (548, 85, 55, toggleH);

    int dynX = 510;
    int dynY = 108;
    int dynGapH = 72;      // Horizontal gap
    int dynGapV = 68;      // Vertical gap

    thresholdKnob->setBounds (dynX, dynY, knobSize, knobSize);
    dynAttackKnob->setBounds (dynX + dynGapH, dynY, knobSize, knobSize);
    ratioKnob->setBounds (dynX, dynY + dynGapV, knobSize, knobSize);
    dynReleaseKnob->setBounds (dynX + dynGapH, dynY + dynGapV, knobSize, knobSize);

    // ==================== SHAPER Section ====================
    shaperPowerBtn.setBounds (820, 58, powerSize, powerSize);
    shaperTypeBox.setBounds (695, 82, 90, 24);
    driveKnob->setBounds (695, 112, smallKnobSize, smallKnobSize);
    biasKnob->setBounds (752, 112, smallKnobSize, smallKnobSize);

    // ==================== CHORUS Section ====================
    chorusPowerBtn.setBounds (820, 172, powerSize, powerSize);
    chorusRateKnob->setBounds (695, 195, smallKnobSize, smallKnobSize);
    chorusBlendKnob->setBounds (752, 195, smallKnobSize, smallKnobSize);

    // ==================== DELAY Section ====================
    delayPowerBtn.setBounds (820, 258, powerSize, powerSize);
    delayTimeKnob->setBounds (695, 280, smallKnobSize, smallKnobSize);
    delayFeedbackKnob->setBounds (752, 280, smallKnobSize, smallKnobSize);

    // ==================== LEGATO Section ====================
    legatoToggle.setBounds (100, 332, toggleW, toggleH);
    retriggerToggle.setBounds (100, 358, toggleW, toggleH);
    polyBox.setBounds (205, 345, 55, 24);

    // ==================== REVERB Section ====================
    reverbPowerBtn.setBounds (460, 312, powerSize, powerSize);
    reverbTypeBox.setBounds (320, 340, 100, 24);
    reverbBlendKnob->setBounds (430, 335, smallKnobSize, smallKnobSize);
    reverbDecayKnob->setBounds (430, 388, smallKnobSize, smallKnobSize);

    // ==================== FILTER Bar ====================
    filterTypeBox.setBounds (80, 428, 130, 26);
    glideToggle.setBounds (265, 430, toggleW, toggleH);
    glideDirectionBox.setBounds (320, 428, 60, 26);
    pitchRangeBox.setBounds (460, 428, 55, 26);
    filterTimeSlider->setBounds (580, 428, 160, sliderH);

    // ==================== Input/Output Gains ====================
    inputGainSlider.setBounds (755, 530, 30, 55);
    outputGainSlider.setBounds (805, 530, 30, 55);

    // ==================== Header Preset Box ====================
    presetBox.setBounds (340, 10, 160, 30);
}
