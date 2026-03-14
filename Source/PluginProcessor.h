#pragma once

#include <JuceHeader.h>

/**
 * RevealAudioProcessor
 *
 * Signal path (left to right):
 *
 *   [Audio In] → [HP Biquad] → [Gain] → [LP Biquad] → [Audio Out]
 *
 *   HP Biquad  – 1st-order high-pass at ~15.9 Hz
 *                Models the 0.1 µF input coupling capacitor in series with a
 *                100 kΩ input resistance ( fc = 1 / (2π × R × C) ≈ 15.9 Hz ).
 *                Removes DC offset; audible content is completely unaffected.
 *
 *   Gain       – Clean linear amplification, 0 dB to +28 dB.
 *                dsp::Gain uses an internal SmoothedValue so there are no
 *                clicks when the knob moves.
 *
 *   LP Biquad  – 1st-order low-pass at ~15.9 kHz
 *                Models the 100 pF feedback capacitor in parallel with the
 *                100 kΩ gain resistor of the TL072 stage
 *                ( fc = 1 / (2π × Rf × Cf) ≈ 15.9 kHz ).
 *                Gives a gentle, natural high-frequency rolloff.
 *
 * Both biquads are wrapped in dsp::ProcessorDuplicator so that each audio
 * channel gets its own independent filter state while sharing a single
 * Coefficients object (thread-safe coefficient update).
 */
class RevealAudioProcessor : public juce::AudioProcessor
{
public:
    RevealAudioProcessor();
    ~RevealAudioProcessor() override;

    //==========================================================================
    // AudioProcessor overrides
    //==========================================================================
    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Exposes the parameter tree to the editor for knob attachment. */
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    //==========================================================================
    // DSP chain
    //==========================================================================

    // ProcessorDuplicator instantiates one IIR::Filter per channel and routes
    // the shared Coefficients pointer to all of them.
    using StereoHP = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    using StereoLP = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    // Named indices into the ProcessorChain for readable get<>() calls.
    enum ChainIndex { kHighPass = 0, kGain, kLowPass };

    juce::dsp::ProcessorChain<StereoHP,
                               juce::dsp::Gain<float>,
                               StereoLP> processorChain;

    //==========================================================================
    // Filter design constants
    //==========================================================================

    // 0.1 µF coupling cap + 100 kΩ input resistance → fc ≈ 15.9 Hz
    static constexpr float kInputHPFreqHz    = 15.9f;

    // Gain stage component values (TL072 non-inverting topology)
    //   Gain = 1 + Rf/R1
    //   LP pole: fc = 1 / (2π × Rf × Cf)
    //   Rf = 100 kΩ at maximum boost (28 dB); Cf = 100 pF feedback cap.
    //   R1 is derived from Rf_max and the max gain value.
    static constexpr float kMaxGainDb        = 28.0f;
    static constexpr float kFeedbackRfMaxOhm = 100e3f;  // 100 kΩ
    static constexpr float kFeedbackCapF     = 100e-12f; // 100 pF

    //==========================================================================
    // Parameters
    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Updates the Gain stage and LP cutoff from the current Volume parameter. */
    void updateDSP();

    double currentSampleRate = 44100.0;
    float  lastLpGainDb      = -999.0f; // sentinel: force first-block LP update

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevealAudioProcessor)
};
