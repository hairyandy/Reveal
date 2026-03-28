#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

RevealAudioProcessor::RevealAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache the bypass parameter — returned by getBypassParameter() so that
    // hosts (Logic Pro, Gig Performer 5, etc.) can route audio around the
    // plugin entirely when bypass is engaged — true hardware-style bypass.
    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter ("bypass"));
}

RevealAudioProcessor::~RevealAudioProcessor() {}

//==============================================================================
// Parameter layout
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
RevealAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Volume: 0 dB (unity / bypass) to kMaxGainDb (full boost).
    // Step size of 0.1 dB gives 280 discrete steps – fine enough for smooth
    // automation while keeping the parameter tree compact.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "volume",                                    // parameter ID
        "Volume",                                    // display name
        juce::NormalisableRange<float> (0.0f, kMaxGainDb, 0.1f),
        6.0f,                                        // default: gentle boost
        "dB"));                                      // suffix label

    // Bypass: false = effect active (default), true = bypassed.
    // Saved automatically as part of the APVTS state.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "bypass",   // parameter ID
        "Bypass",   // display name
        false));    // default: not bypassed

    return layout;
}

//==============================================================================
// Preparation
//==============================================================================

void RevealAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    lastLpGainDb      = -999.0f; // force LP recalculation on first block

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    processorChain.prepare (spec);

    // High-pass: 1st-order RC model of the 0.1 µF input coupling cap.
    // Fixed at 15.9 Hz – only blocks DC, inaudible on programme material.
    *processorChain.get<kHighPass>().state =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (
            sampleRate, kInputHPFreqHz);

    // Seed gain + LP from the current (or restored) APVTS state.
    updateDSP();
}

void RevealAudioProcessor::releaseResources()
{
    // Nothing to release – the DSP objects clean up after themselves.
}

//==============================================================================
// Bus layout validation
//==============================================================================

bool RevealAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();

    // Output must be mono or stereo.
    if (out != juce::AudioChannelSet::mono()
     && out != juce::AudioChannelSet::stereo())
        return false;

    // Input must be mono or stereo, and no wider than the output.
    if (in != juce::AudioChannelSet::mono()
     && in != juce::AudioChannelSet::stereo())
        return false;

    // Reject down-mix (stereo-in → mono-out).
    if (in == juce::AudioChannelSet::stereo()
     && out == juce::AudioChannelSet::mono())
        return false;

    return true; // mono→mono, stereo→stereo, and mono→stereo all accepted
}

//==============================================================================
// DSP helpers
//==============================================================================

void RevealAudioProcessor::updateDSP()
{
    const float gainDb     = apvts.getRawParameterValue ("volume")->load();
    const float gainLinear = juce::Decibels::decibelsToGain (gainDb);

    // Gain stage: SmoothedValue inside dsp::Gain prevents clicks.
    processorChain.get<kGain>().setGainDecibels (gainDb);

    // LP cutoff: only recompute coefficients when the gain has changed.
    // makeFirstOrderLowPass allocates on the heap, so we skip it every block.
    if (std::abs (gainDb - lastLpGainDb) < 0.01f)
        return;

    lastLpGainDb = gainDb;

    // Circuit model: Gain = 1 + Rf/R1
    //   → Rf = R1 × (gainLinear − 1)
    //   → fc = 1 / (2π × Rf × Cf)
    //
    // R1 is derived from the known Rf and gain at maximum boost.
    const float gainMaxLinear = juce::Decibels::decibelsToGain (kMaxGainDb);
    const float R1            = kFeedbackRfMaxOhm / (gainMaxLinear - 1.0f);
    const float Rf            = R1 * (gainLinear - 1.0f);

    if (Rf < 1.0f) // unity gain → Rf → 0 → LP pole at infinity; bypass
    {
        processorChain.setBypassed<kLowPass> (true);
    }
    else
    {
        const float fc = 1.0f / (juce::MathConstants<float>::twoPi * Rf * kFeedbackCapF);
        // Clamp below Nyquist to keep the biquad numerically stable.
        const float fcSafe = juce::jmin (fc, static_cast<float> (currentSampleRate * 0.499));

        *processorChain.get<kLowPass>().state =
            *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (
                currentSampleRate, fcSafe);

        processorChain.setBypassed<kLowPass> (false);
    }
}

//==============================================================================
// Process block
//==============================================================================

void RevealAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
{
    // Suppress denormal numbers – they can cause CPU spikes with IIR filters.
    juce::ScopedNoDenormals noDenormals;

    // Safety bypass: hosts that fully support setBypassParameter() will never
    // call processBlock when bypassed, but we guard here for hosts that don't.
    if (bypassParam != nullptr && bypassParam->get())
    {
        // Pass audio through unmodified; clear any extra output channels.
        for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
            buffer.clear (ch, 0, buffer.getNumSamples());
        return;
    }

    // Clear any output channels that have no corresponding input
    // (e.g. a mono-in / stereo-out layout that a DAW might request).
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Pull the latest Volume value and update gain + LP cutoff.
    updateDSP();

    // Wrap the AudioBuffer in a non-owning AudioBlock and run the chain:
    //   HP Biquad  →  Gain  →  LP Biquad
    juce::dsp::AudioBlock<float>             block   (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    processorChain.process (context);
}

//==============================================================================
// State persistence
//==============================================================================

void RevealAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Serialise the entire APVTS to XML, then store as binary.
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RevealAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// Boilerplate
//==============================================================================

juce::AudioProcessorEditor* RevealAudioProcessor::createEditor()
{
    return new RevealAudioProcessorEditor (*this);
}

const juce::String RevealAudioProcessor::getName() const { return JucePlugin_Name; }

// Entry point called by the host / plugin wrapper.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RevealAudioProcessor();
}
