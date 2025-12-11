#pragma once

#include <JuceHeader.h>

namespace soulbass
{
    enum class FilterType
    {
        lowPass = 0,
        highPass
    };

    struct SampleSound : public juce::SynthesiserSound
    {
        SampleSound (juce::String nameIn,
                     std::unique_ptr<juce::AudioBuffer<float>> dataIn,
                     double sourceSampleRateIn,
                     int midiNoteStartIn,
                     int midiNoteEndIn,
                     int midiRootNoteIn)
            : name (std::move (nameIn)),
              data (std::move (dataIn)),
              sourceSampleRate (sourceSampleRateIn),
              midiNoteStart (midiNoteStartIn),
              midiNoteEnd (midiNoteEndIn),
              midiRootNote (midiRootNoteIn)
        {
        }

        bool appliesToNote (int midiNoteNumber) override
        {
            return midiNoteNumber >= midiNoteStart && midiNoteNumber <= midiNoteEnd;
        }

        bool appliesToChannel (int /*midiChannel*/) override { return true; }

        juce::String name;
        std::unique_ptr<juce::AudioBuffer<float>> data;
        double sourceSampleRate = 44100.0;
        int midiNoteStart = 0;
        int midiNoteEnd = 127;
        int midiRootNote = 60;
    };

    class SampleVoice : public juce::SynthesiserVoice
    {
    public:
        SampleVoice() = default;

        bool canPlaySound (juce::SynthesiserSound* s) override
        {
            return dynamic_cast<SampleSound*> (s) != nullptr;
        }

        void prepare (const juce::dsp::ProcessSpec& spec)
        {
            currentSampleRate = spec.sampleRate;
            adsr.setSampleRate (currentSampleRate);
            filter.prepare (spec);
            filter.reset();
            resetLfo();
        }

        void setEnvelope (const juce::ADSR::Parameters& params)    { envParams = params; adsr.setParameters (envParams); }
        void setFilter (FilterType typeIn, float cutoffHz, float resonanceIn)
        {
            filterType = typeIn;
            cutoff = cutoffHz;
            resonance = resonanceIn;
            updateFilter();
        }

        void setLfo (float rateHz, float depthIn, float phaseIn, float smoothingIn)
        {
            lfoRate = rateHz;
            lfoDepth = depthIn;
            lfoPhaseOffset = phaseIn * juce::MathConstants<float>::twoPi;
            lfoSmoothing = juce::jlimit (0.0f, 0.999f, smoothingIn);
        }

        void setModWheel (float wheelValue) { modWheel = juce::jlimit (0.0f, 1.0f, wheelValue); }
        void setPitchBendRange (int semitones) { pitchBendRange = semitones; }
        void setGlide (bool enabled, float timeSeconds, int directionMode)
        {
            glideEnabled = enabled;
            glideTimeSeconds = juce::jmax (0.0f, timeSeconds);
            glideDirection = juce::jlimit (0, 1, directionMode);
        }

        void setLegato (bool enabled, bool retriggerIn)
        {
            legatoEnabled = enabled;
            retriggerEnabled = retriggerIn;
        }

        void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* s,
                        int /*currentPitchWheelPosition*/) override
        {
            if (auto* sampleSound = dynamic_cast<SampleSound*> (s))
            {
                currentSound = sampleSound;
                sourceSamplePosition = 0.0;
                leftGain = velocity;
                rightGain = velocity;
                const bool shouldRetrigger = !legatoEnabled || retriggerEnabled || !adsr.isActive();

                if (shouldRetrigger)
                {
                    adsr.reset();
                    adsr.setSampleRate (getSampleRate());
                    adsr.setParameters (envParams);
                    adsr.noteOn();
                }

                updatePitchRatio (midiNoteNumber, pitchWheelPosition);
                resetLfo();
                resetFilterState();
            }
        }

        void stopNote (float /*velocity*/, bool allowTailOff) override
        {
            if (allowTailOff)
            {
                adsr.noteOff();
            }
            else
            {
                clearCurrentNote();
                adsr.reset();
            }
        }

        void pitchWheelMoved (int newValue) override
        {
            pitchWheelPosition = newValue;
            updatePitchRatio (getCurrentlyPlayingNote(), pitchWheelPosition);
        }

        void controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/) override {}

        void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
        {
            if (currentSound == nullptr || currentSound->data == nullptr)
                return;

            auto& data = *currentSound->data;
            const auto* inL = data.getReadPointer (0);
            const auto* inR = data.getNumChannels() > 1 ? data.getReadPointer (1) : inL;
            const auto dataLength = data.getNumSamples();

            auto* outL = outputBuffer.getWritePointer (0);
            auto* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer (1) : outL;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                auto pos = (int) sourceSamplePosition;
                auto alpha = (float) (sourceSamplePosition - (double) pos);
                auto invAlpha = 1.0f - alpha;

                if (pos >= dataLength - 1)
                {
                    clearCurrentNote();
                    break;
                }

                float sampleL = inL[pos] * invAlpha + inL[pos + 1] * alpha;
                float sampleR = inR[pos] * invAlpha + inR[pos + 1] * alpha;

                auto env = adsr.getNextSample();
                auto lfoValue = getNextLfoValue();

                auto cutoffMod = juce::jlimit (40.0f, 20000.0f, cutoff * (1.0f + lfoValue * 0.5f));
                if (cutoffMod != lastCutoffModulated)
                {
                    lastCutoffModulated = cutoffMod;
                    filter.setCutoffFrequency (cutoffMod);
                }

                sampleL = filter.processSample (0, sampleL);
                sampleR = filter.processSample (1, sampleR);

                sampleL *= env * leftGain;
                sampleR *= env * rightGain;

                outL[startSample + sample] += sampleL;
                outR[startSample + sample] += sampleR;

                if (glideEnabled)
                {
                    const float alphaGlide = glideTimeSeconds > 0.0f
                                                 ? (1.0f - std::exp (-1.0f / (float) (glideTimeSeconds * currentSampleRate)))
                                                 : 1.0f;

                    const bool allowGlideUp = glideDirection == 0;
                    const bool allowGlideDown = glideDirection == 1;
                    const auto delta = targetPitchRatio - currentPitchRatio;

                    if ((delta > 0.0 && allowGlideUp) || (delta < 0.0 && allowGlideDown) || (allowGlideUp && allowGlideDown))
                        currentPitchRatio += delta * juce::jlimit (0.0f, 1.0f, alphaGlide);
                    else
                        currentPitchRatio = targetPitchRatio;
                }
                else
                {
                    currentPitchRatio = targetPitchRatio;
                }

                sourceSamplePosition += currentPitchRatio;

                if (! adsr.isActive())
                {
                    clearCurrentNote();
                    break;
                }
            }
        }

        void aftertouchChanged (int /*newAftertouchValue*/) override {}
        void channelPressureChanged (int /*newChannelPressureValue*/) override {}

        void reset()
        {
            adsr.reset();
            filter.reset();
            resetLfo();
        }

    private:
        void resetFilterState()
        {
            filter.reset();
            updateFilter();
        }

        void resetLfo()
        {
            lfoPhase = 0.0f;
            lfoState = 0.0f;
        }

        void updatePitchRatio (int midiNoteNumber, int wheelPosition)
        {
            if (currentSound == nullptr || currentSound->data == nullptr)
                return;

            auto pitchBend = (wheelPosition - 8192) / 8192.0; // -1..1
            auto bendSemitones = pitchBend * (double) pitchBendRange;
            auto noteDelta = (double) midiNoteNumber + bendSemitones - (double) currentSound->midiRootNote;
            auto ratio = std::pow (2.0, noteDelta / 12.0);
            targetPitchRatio = ratio * (currentSound->sourceSampleRate / getSampleRate());
            if (! glideEnabled)
                currentPitchRatio = targetPitchRatio;
        }

        void updateFilter()
        {
            if (filterType == FilterType::lowPass)
                filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            else
                filter.setType (juce::dsp::StateVariableTPTFilterType::highpass);

            filter.setResonance (resonance);
            filter.setCutoffFrequency (cutoff);
            lastCutoffModulated = cutoff;
        }

        float getNextLfoValue()
        {
            if (currentSampleRate <= 0.0)
                return 0.0f;

            auto increment = juce::MathConstants<float>::twoPi * lfoRate / (float) currentSampleRate;
            lfoPhase += increment;
            if (lfoPhase > juce::MathConstants<float>::twoPi)
                lfoPhase -= juce::MathConstants<float>::twoPi;

            auto raw = std::sin (lfoPhase + lfoPhaseOffset);
            lfoState += lfoSmoothing * (raw - lfoState);
            return lfoState * lfoDepth * modWheel;
        }

        double sourceSamplePosition = 0.0;
        double currentPitchRatio = 1.0;
        double targetPitchRatio = 1.0;
        double currentSampleRate = 44100.0;
        int pitchWheelPosition = 8192;
        int pitchBendRange = 12;
        float glideTimeSeconds = 0.0f;
        int glideDirection = 0; // 0 up, 1 down
        bool glideEnabled = false;
        bool legatoEnabled = false;
        bool retriggerEnabled = true;

        float leftGain = 1.0f;
        float rightGain = 1.0f;

        float cutoff = 1200.0f;
        float lastCutoffModulated = cutoff;
        float resonance = 0.7f;
        FilterType filterType = FilterType::lowPass;

        float lfoRate = 2.0f;
        float lfoDepth = 0.5f;
        float lfoPhaseOffset = 0.0f;
        float lfoSmoothing = 0.15f;
        float lfoPhase = 0.0f;
        float lfoState = 0.0f;
        float modWheel = 0.0f;

        juce::ADSR adsr;
        juce::ADSR::Parameters envParams;
        juce::dsp::StateVariableTPTFilter<float> filter;

        SampleSound* currentSound = nullptr;
    };
} // namespace soulbass
