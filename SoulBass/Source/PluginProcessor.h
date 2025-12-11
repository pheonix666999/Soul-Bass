#pragma once

#include <JuceHeader.h>
#include "SoulSampler.h"

class SoulBassAudioProcessor : public juce::AudioProcessor
{
public:
    SoulBassAudioProcessor();
    ~SoulBassAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::Synthesiser& getSynth() { return synth; }

private:
    void loadSamples();
    void updateVoices();
    void updateVoiceParameters();
    void updateFxParameters();

    juce::Synthesiser synth;
    juce::AudioFormatManager formatManager;
    juce::dsp::ProcessSpec processSpec { 44100.0, 512, 2 };

    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;

    using IIRFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                     juce::dsp::IIR::Coefficients<float>>;

    IIRFilter eqLow, eqMid, eqHigh;
    juce::dsp::Compressor<float> compressor;
    std::function<float (float)> shaperFn = [] (float x) { return x; };
    juce::dsp::Chorus<float> chorus;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, 2> delayLines {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> (192000),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> (192000)
    };
    juce::dsp::Reverb reverb;

    float delayMix = 0.35f;
    size_t delaySamples = 0;

    float currentModWheel = 0.0f;
    bool samplesLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoulBassAudioProcessor)
};
