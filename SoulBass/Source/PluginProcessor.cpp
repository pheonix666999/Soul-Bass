#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const std::vector<const char*> kSampleNames
    {
        "01-Jazz Bass 1.wav",
        "02-Jazz Bass 2.wav",
        "03-PJ Bass Thump.wav",
        "04-PJ Lofi.wav",
        "05-PJ Tape.wav",
        "06-Mg Sub1.wav",
        "07-Mg Sub2.wav",
        "08-Mg Sub3.wav",
        "09-Mg 2Tri.wav",
        "10-Mg 3TriSaw.wav",
        "11-Mg Sync1.wav",
        "12-Mg Sync2.wav",
        "13-Mg PWM.wav",
        "13-Mg Saw Pluck.wav",
        "14-Mg Square Pluck.wav",
        "15-Art Fat Analog.wav",
        "16-Art Fauxlectric.wav",
        "17-Art Square Up.wav",
        "18-Art Dirty Bit.wav",
        "19-Art Slappy.wav",
        "20-Art Vowel.wav",
        "21-Prof Funkshun.wav",
        "22-Prof Shimmy.wav",
        "23-Prof Rollin.wav",
        "24-Prof Brass Attack.wav",
        "25-Prof Buzz off.wav",
        "26-Prof Buzz Vibes.wav",
        "27-Prof Substitute.wav",
        "28-Prof Eightieswav.wav",
        "29-Prof Cruise.wav",
        "30-Prof SH Bass.wav",
        "31-Prof REZ.wav",
        "32-Prof Big Saw.wav",
        "33-Prof Soundtrack.wav",
        "34-Prof Ripper.wav",
        "35-808 Rattle.wav",
        "36-808 Roll.wav",
        "37-808 Shake.wav",
        "38-808 Rick.wav",
        "39-Reeses.wav",
        "40-808 Smooth.wav"
    };

    constexpr int kStartNote = 36; // map samples from C2 upwards
    constexpr int kPitchBendRange = 12;
} // namespace

SoulBassAudioProcessor::SoulBassAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    formatManager.registerBasicFormats();
    synth.setNoteStealingEnabled (true);
    updateVoices();
}

void SoulBassAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    processSpec = { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
    inputGain.prepare (processSpec);
    outputGain.prepare (processSpec);
    inputGain.setRampDurationSeconds (0.02);
    outputGain.setRampDurationSeconds (0.02);

    eqLow.prepare (processSpec);
    eqMid.prepare (processSpec);
    eqHigh.prepare (processSpec);
    compressor.prepare (processSpec);
    chorus.prepare (processSpec);
    for (auto& d : delayLines)
        d.prepare (processSpec);
    reverb.prepare (processSpec);

    synth.setCurrentPlaybackSampleRate (sampleRate);
    updateVoices();
    updateFxParameters();
    loadSamples();
}

void SoulBassAudioProcessor::releaseResources()
{
    for (auto& d : delayLines)
        d.reset();
    chorus.reset();
    reverb.reset();
    compressor.reset();
    inputGain.reset();
    outputGain.reset();
}

bool SoulBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void SoulBassAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Track mod wheel for LFO depth.
    for (const auto metadata : midiMessages)
    {
        const auto& m = metadata.getMessage();
        if (m.isController() && m.getControllerNumber() == 1)
            currentModWheel = (float) m.getControllerValue() / 127.0f;
    }

    updateVoiceParameters();
    updateFxParameters();

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    auto* inGain = apvts.getRawParameterValue ("inputGain");
    auto* outGain = apvts.getRawParameterValue ("outputGain");

    inputGain.setGainDecibels (*inGain);
    outputGain.setGainDecibels (*outGain);

    juce::dsp::AudioBlock<float> block (buffer);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);

    inputGain.process (context);

    const bool eqOn = apvts.getRawParameterValue ("eqEnabled")->load();
    const bool dynOn = apvts.getRawParameterValue ("dynEnabled")->load();
    const bool shaperOn = apvts.getRawParameterValue ("shaperEnabled")->load();
    const bool chorusOn = apvts.getRawParameterValue ("chorusEnabled")->load();
    const bool delayOn = apvts.getRawParameterValue ("delayEnabled")->load();
    const bool reverbOn = apvts.getRawParameterValue ("reverbEnabled")->load();

    if (eqOn)
    {
        eqLow.process (context);
        eqMid.process (context);
        eqHigh.process (context);
    }

    if (dynOn)
        compressor.process (context);

    if (shaperOn)
    {
        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = buffer.getNumChannels();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = shaperFn ? shaperFn (data[i]) : data[i];
        }
    }

    if (chorusOn)
        chorus.process (context);

    if (delayOn)
    {
        const float feedback = apvts.getRawParameterValue ("delayFeedback")->load();
        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = buffer.getNumChannels();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            auto& delay = delayLines[(size_t) juce::jmin (ch, (int) delayLines.size() - 1)];

            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = data[i];
                const float delayed = delay.popSample (ch, (float) delaySamples);
                data[i] = dry + delayed * delayMix;
                delay.pushSample (ch, dry + delayed * feedback);
            }
        }
    }

    if (reverbOn)
        reverb.process (context);

    outputGain.process (context);
}

void SoulBassAudioProcessor::loadSamples()
{
    if (samplesLoaded)
        return;

    auto toResourceName = [] (const juce::String& fileName)
    {
        // Mirror JUCE's BinaryData identifier generation.
        juce::String cleaned;
        for (int i = 0; i < fileName.length(); ++i)
        {
            auto c = fileName[i];
            const bool first = (i == 0);
            const bool isLetter = juce::CharacterFunctions::isLetter (c);
            const bool isDigit = juce::CharacterFunctions::isDigit (c);
            const bool allowed = first ? (isLetter || c == '_') : (isLetter || isDigit || c == '_');
            cleaned << (allowed ? juce::String::charToString (c) : juce::String ("_"));
        }

        if (cleaned.isEmpty() || !(juce::CharacterFunctions::isLetter (cleaned[0]) || cleaned[0] == '_'))
            cleaned = "_" + cleaned;

        return cleaned;
    };

    const int numVoices = 16;
    while (synth.getNumVoices() < numVoices)
        synth.addVoice (new soulbass::SampleVoice());

    int midiNote = kStartNote;

    for (auto* name : kSampleNames)
    {
        int dataSize = 0;
        auto resourceName = toResourceName (juce::File (name).getFileName());
        if (auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize))
        {
            auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) dataSize, false);
            auto reader = std::unique_ptr<juce::AudioFormatReader> (formatManager.createReaderFor (std::move (stream)));

            if (reader != nullptr)
            {
                auto length = (int) reader->lengthInSamples;
                auto buffer = std::make_unique<juce::AudioBuffer<float>> ((int) reader->numChannels, length);
                reader->read (buffer.get(), 0, length, 0, true, true);

                auto sound = std::make_unique<soulbass::SampleSound> (name,
                                                                      std::move (buffer),
                                                                      reader->sampleRate,
                                                                      midiNote,
                                                                      midiNote,
                                                                      midiNote);
                synth.addSound (sound.release());
            }
        }

        ++midiNote;
    }

    updateVoices();
    samplesLoaded = true;
}

void SoulBassAudioProcessor::updateVoices()
{
    for (int i = synth.getNumVoices(); --i >= 0;)
        if (auto* v = dynamic_cast<soulbass::SampleVoice*> (synth.getVoice (i)))
            v->prepare (processSpec);
}

void SoulBassAudioProcessor::updateVoiceParameters()
{
    const auto* attack = apvts.getRawParameterValue ("attack");
    const auto* decay = apvts.getRawParameterValue ("decay");
    const auto* sustain = apvts.getRawParameterValue ("sustain");
    const auto* release = apvts.getRawParameterValue ("release");

    const auto* filterCutoff = apvts.getRawParameterValue ("filterCutoff");
    const auto* filterRes = apvts.getRawParameterValue ("filterResonance");
    const auto* filterType = apvts.getRawParameterValue ("filterType");

    const auto* lfoRate = apvts.getRawParameterValue ("lfoRate");
    const auto* lfoDepth = apvts.getRawParameterValue ("lfoDepth");
    const auto* lfoPhase = apvts.getRawParameterValue ("lfoPhase");
    const auto* lfoSmooth = apvts.getRawParameterValue ("lfoSmoothing");
    const auto* lfoEnabled = apvts.getRawParameterValue ("lfoEnabled");
    const auto* pitchRange = apvts.getRawParameterValue ("pitchRange");
    const auto* glideOn = apvts.getRawParameterValue ("glideEnabled");
    const auto* glideDirection = apvts.getRawParameterValue ("glideDirection");
    const auto* glideTime = apvts.getRawParameterValue ("glideTime");
    const auto* polyphony = apvts.getRawParameterValue ("polyphony");
    const auto* legato = apvts.getRawParameterValue ("legato");
    const auto* retrigger = apvts.getRawParameterValue ("retrigger");

    juce::ADSR::Parameters env { attack->load(), decay->load(), sustain->load(), release->load() };

    auto type = filterType->load() < 0.5f ? soulbass::FilterType::lowPass : soulbass::FilterType::highPass;

    const int pitchRanges[] { 2, 7, 12, 24 };
    const int rangeIdx = juce::jlimit (0, 3, (int) std::round (pitchRange->load()));
    const int pitchRangeSemis = pitchRanges[rangeIdx];

    const int polyChoices[] { 1, 2, 3, 4, 8, 16 };
    const int polyIdx = juce::jlimit (0, 5, (int) std::round (polyphony->load()));
    const int targetVoices = polyChoices[polyIdx];

    // Grow/shrink voice pool to requested polyphony.
    bool voiceCountChanged = false;
    while (synth.getNumVoices() < targetVoices)
    {
        synth.addVoice (new soulbass::SampleVoice());
        voiceCountChanged = true;
    }
    while (synth.getNumVoices() > targetVoices && synth.getNumVoices() > 0)
    {
        synth.removeVoice (synth.getNumVoices() - 1);
        voiceCountChanged = true;
    }
    if (voiceCountChanged)
        updateVoices();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* v = dynamic_cast<soulbass::SampleVoice*> (synth.getVoice (i)))
        {
            v->setEnvelope (env);
            v->setFilter (type, filterCutoff->load(), filterRes->load());
            const float lfoDepthValue = lfoEnabled->load() > 0.5f ? lfoDepth->load() : 0.0f;
            v->setLfo (lfoRate->load(), lfoDepthValue, lfoPhase->load(), lfoSmooth->load());
            v->setModWheel (currentModWheel);
            v->setPitchBendRange (pitchRangeSemis);
            v->setGlide (glideOn->load() > 0.5f, glideTime->load(), (int) std::round (glideDirection->load()));
            v->setLegato (legato->load() > 0.5f, retrigger->load() > 0.5f);
        }
    }
}

void SoulBassAudioProcessor::updateFxParameters()
{
    const double sr = processSpec.sampleRate;
    if (sr <= 0.0)
        return;

    const auto lowFreq = apvts.getRawParameterValue ("eqLowFreq")->load();
    const auto lowGain = apvts.getRawParameterValue ("eqLowGain")->load();
    const auto lowQ = apvts.getRawParameterValue ("eqLowQ")->load();

    if (auto coeff = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, lowFreq, lowQ,
                                                                        juce::Decibels::decibelsToGain (lowGain)))
        *eqLow.state = *coeff;

    const auto midFreq = apvts.getRawParameterValue ("eqMidFreq")->load();
    const auto midGain = apvts.getRawParameterValue ("eqMidGain")->load();
    const auto midQ = apvts.getRawParameterValue ("eqMidQ")->load();

    if (auto coeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, midFreq, midQ,
                                                                          juce::Decibels::decibelsToGain (midGain)))
        *eqMid.state = *coeff;

    const auto highFreq = apvts.getRawParameterValue ("eqHighFreq")->load();
    const auto highGain = apvts.getRawParameterValue ("eqHighGain")->load();
    const auto highQ = apvts.getRawParameterValue ("eqHighQ")->load();

    if (auto coeff = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, highFreq, highQ,
                                                                         juce::Decibels::decibelsToGain (highGain)))
        *eqHigh.state = *coeff;

    const auto dynThreshold = apvts.getRawParameterValue ("dynThreshold")->load();
    const auto dynAttack = apvts.getRawParameterValue ("dynAttack")->load();
    const auto dynRatio = apvts.getRawParameterValue ("dynRatio")->load();
    const auto dynRelease = apvts.getRawParameterValue ("dynRelease")->load();
    const bool dynLimit = apvts.getRawParameterValue ("dynLimit")->load() > 0.5f;

    compressor.setThreshold (dynThreshold);
    compressor.setRatio (dynLimit ? juce::jmax (10.0f, dynRatio * 2.0f) : dynRatio);
    compressor.setAttack (dynAttack);
    compressor.setRelease (dynRelease);

    const auto shaperDriveDb = apvts.getRawParameterValue ("shaperDrive")->load();
    const auto shaperBias = apvts.getRawParameterValue ("shaperBias")->load();
    const auto shaperType = (int) std::round (apvts.getRawParameterValue ("shaperType")->load());
    const float drive = juce::Decibels::decibelsToGain (shaperDriveDb);

    shaperFn = [drive, shaperBias, shaperType] (float x)
    {
        const float biased = (x + shaperBias) * drive;
        switch (shaperType)
        {
            case 1: // Tube-ish
                return juce::jlimit (-1.2f, 1.2f, std::tanh (biased) * 0.8f);
            case 2: // Tape-ish soft clip
            {
                const float s = juce::jlimit (-2.5f, 2.5f, biased);
                return s - (s * s * s) * 0.08f;
            }
            default: // Soft clip
                return juce::jlimit (-1.0f, 1.0f, biased / (1.0f + std::abs (biased)));
        }
    };

    const auto chorusRate = apvts.getRawParameterValue ("chorusRate")->load();
    const auto chorusBlend = apvts.getRawParameterValue ("chorusBlend")->load();
    chorus.setRate (chorusRate);
    chorus.setDepth (0.45f);
    chorus.setCentreDelay (7.5f);
    chorus.setFeedback (0.12f);
    chorus.setMix (chorusBlend);

    const auto delayMs = apvts.getRawParameterValue ("delayTimeMs")->load();
    delaySamples = (size_t) juce::jlimit (1, 192000, (int) std::round (delayMs * sr / 1000.0));
    for (auto& d : delayLines)
        d.setDelay ((int) delaySamples);
    delayMix = 0.35f;
    const bool delayOn = apvts.getRawParameterValue ("delayEnabled")->load() > 0.5f;
    if (! delayOn)
        for (auto& d : delayLines)
            d.reset();

    const auto reverbBlend = apvts.getRawParameterValue ("reverbBlend")->load();
    const auto reverbDecay = apvts.getRawParameterValue ("reverbDecay")->load();
    const auto reverbType = (int) std::round (apvts.getRawParameterValue ("reverbType")->load());

    juce::dsp::Reverb::Parameters params;
    params.wetLevel = reverbBlend;
    params.dryLevel = 1.0f - reverbBlend;
    params.width = 1.0f;
    params.freezeMode = 0.0f;
    params.roomSize = juce::jlimit (0.1f, 1.0f, reverbDecay * 0.25f);
    params.damping = 0.3f;

    if (reverbType == 1) // Hall
    {
        params.roomSize = juce::jlimit (0.4f, 1.0f, reverbDecay * 0.3f);
        params.damping = 0.35f;
    }
    else if (reverbType == 2) // Plate
    {
        params.roomSize = juce::jlimit (0.2f, 0.9f, reverbDecay * 0.28f);
        params.damping = 0.45f;
    }

    reverb.setParameters (params);
}
juce::AudioProcessorValueTreeState::ParameterLayout SoulBassAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inputGain", "Input Gain", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("outputGain", "Output Gain", -24.0f, 24.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("attack", "Attack", juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.4f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("decay", "Decay", juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.4f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("sustain", "Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("release", "Release", juce::NormalisableRange<float> (0.01f, 8.0f, 0.0f, 0.4f), 0.6f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterCutoff", "Filter Cutoff", juce::NormalisableRange<float> (40.0f, 20000.0f, 0.0f, 0.35f), 1200.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterResonance", "Filter Resonance", 0.1f, 2.0f, 0.7f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("filterType", "Filter Type", juce::StringArray { "LPF", "HPF" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoRate", "LFO Rate", juce::NormalisableRange<float> (0.1f, 12.0f, 0.0f, 0.3f), 2.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoDepth", "LFO Depth", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoPhase", "LFO Phase", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoSmoothing", "LFO Smoothing", 0.0f, 1.0f, 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("lfoSync", "LFO Tempo Sync", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("lfoEnabled", "LFO Enabled", true));

    // EQ
    params.push_back (std::make_unique<juce::AudioParameterBool> ("eqEnabled", "EQ Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqLowFreq", "EQ Low Freq",
                                                                   juce::NormalisableRange<float> (40.0f, 400.0f, 0.0f, 0.5f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqLowGain", "EQ Low Gain", -18.0f, 18.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqLowQ", "EQ Low Q", 0.3f, 2.0f, 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqMidFreq", "EQ Mid Freq",
                                                                   juce::NormalisableRange<float> (200.0f, 2000.0f, 0.0f, 0.5f), 600.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqMidGain", "EQ Mid Gain", -18.0f, 18.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqMidQ", "EQ Mid Q", 0.3f, 3.0f, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqHighFreq", "EQ High Freq",
                                                                   juce::NormalisableRange<float> (2000.0f, 12000.0f, 0.0f, 0.5f), 6000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqHighGain", "EQ High Gain", -18.0f, 18.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("eqHighQ", "EQ High Q", 0.3f, 2.0f, 0.8f));

    // Dynamics
    params.push_back (std::make_unique<juce::AudioParameterBool> ("dynEnabled", "Dynamics Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("dynLimit", "Dynamics Mode Limit", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dynThreshold", "Dynamics Threshold", -60.0f, 0.0f, -12.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dynAttack", "Dynamics Attack", juce::NormalisableRange<float> (1.0f, 50.0f, 0.0f, 0.4f), 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dynRatio", "Dynamics Ratio", 1.0f, 20.0f, 4.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("dynRelease", "Dynamics Release", juce::NormalisableRange<float> (20.0f, 400.0f, 0.0f, 0.4f), 80.0f));

    // Shaper
    params.push_back (std::make_unique<juce::AudioParameterBool> ("shaperEnabled", "Shaper Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("shaperDrive", "Shaper Drive", 0.0f, 24.0f, 6.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("shaperBias", "Shaper Bias", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("shaperType", "Shaper Type",
                                                                    juce::StringArray { "Soft", "Tube", "Tape" }, 0));

    // Chorus
    params.push_back (std::make_unique<juce::AudioParameterBool> ("chorusEnabled", "Chorus Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("chorusRate", "Chorus Rate", juce::NormalisableRange<float> (0.1f, 5.0f, 0.0f, 0.35f), 1.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("chorusBlend", "Chorus Blend", 0.0f, 1.0f, 0.35f));

    // Delay
    params.push_back (std::make_unique<juce::AudioParameterBool> ("delayEnabled", "Delay Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayTimeMs", "Delay Time", juce::NormalisableRange<float> (50.0f, 700.0f, 0.0f, 0.35f), 280.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayFeedback", "Delay Feedback", 0.0f, 0.9f, 0.35f));

    // Reverb
    params.push_back (std::make_unique<juce::AudioParameterBool> ("reverbEnabled", "Reverb Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbBlend", "Reverb Blend", 0.0f, 1.0f, 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("reverbDecay", "Reverb Decay", juce::NormalisableRange<float> (0.2f, 4.0f, 0.0f, 0.35f), 1.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("reverbType", "Reverb Type",
                                                                    juce::StringArray { "Spring", "Hall", "Plate" }, 0));

    // Pitch / glide / poly
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("pitchRange", "Pitch Bend Range",
                                                                    juce::StringArray { "2", "7", "12", "24" }, 2));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("glideEnabled", "Glide Enabled", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("glideDirection", "Glide Direction",
                                                                    juce::StringArray { "Up", "Down" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("glideTime", "Glide Time", juce::NormalisableRange<float> (0.0f, 0.4f, 0.0f, 0.4f), 0.08f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("polyphony", "Polyphony",
                                                                    juce::StringArray { "1", "2", "3", "4", "8", "16" }, 2));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("legato", "Legato", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("retrigger", "Retrigger", true));

    return { params.begin(), params.end() };
}

juce::AudioProcessorEditor* SoulBassAudioProcessor::createEditor()
{
    return new SoulBassAudioProcessorEditor (*this);
}

void SoulBassAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SoulBassAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoulBassAudioProcessor();
}
