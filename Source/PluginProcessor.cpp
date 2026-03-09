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
}

RevealAudioProcessor::~RevealAudioProcessor() {}

//==============================================================================
// Parameter layout
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
RevealAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Volume: 0 dB (unity / bypass) to +28 dB (full boost).
    // Step size of 0.1 dB gives 280 discrete steps – fine enough for smooth
    // automation while keeping the parameter tree compact.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "volume",                                    // parameter ID
        "Volume",                                    // display name
        juce::NormalisableRange<float> (0.0f, 28.0f, 0.1f),
        0.0f,                                        // default: unity gain
        "dB"));                                      // suffix label

    return layout;
}

//==============================================================================
// Preparation
//==============================================================================

void RevealAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    processorChain.prepare (spec);

    // -------------------------------------------------------------------------
    // High-pass: input coupling capacitor model
    //
    // 1st-order Butterworth HP – the simplest accurate model for a single RC
    // pole.  At 15.9 Hz this is inaudible, but it prevents DC from accumulating
    // in the downstream gain stage (which would clip at high boost settings).
    // -------------------------------------------------------------------------
    *processorChain.get<kHighPass>().state =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (
            sampleRate, kInputHPFreqHz);

    // -------------------------------------------------------------------------
    // Low-pass: feedback capacitor model
    //
    // 1st-order LP at ~15.9 kHz.  The real TL072 feedback network rolls off
    // the gain at high frequencies due to the pole formed by Rf || Cf.  At the
    // extremes of human hearing this contributes a smooth, analogue feel without
    // a harsh brick-wall cut.
    // -------------------------------------------------------------------------
    *processorChain.get<kLowPass>().state =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (
            sampleRate, kFeedbackLPFreqHz);

    // Seed the Gain stage with whatever value the APVTS already holds
    // (e.g. restored preset state).
    updateGain();
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

    // Input and output channel counts must match (no up/down-mixing).
    if (in != out)
        return false;

    // Accept mono or stereo; reject surround or empty layouts.
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

//==============================================================================
// DSP helpers
//==============================================================================

void RevealAudioProcessor::updateGain()
{
    // getRawParameterValue returns an std::atomic<float>* – load() is a
    // lock-free, realtime-safe read.
    const float gainDb = apvts.getRawParameterValue ("volume")->load();

    // dsp::Gain::setGainDecibels converts dB → linear internally and feeds the
    // value into a SmoothedValue, so parameter changes never cause clicks.
    processorChain.get<kGain>().setGainDecibels (gainDb);
}

//==============================================================================
// Process block
//==============================================================================

void RevealAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
{
    // Suppress denormal numbers – they can cause CPU spikes with IIR filters.
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that have no corresponding input
    // (e.g. a mono-in / stereo-out layout that a DAW might request).
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Pull the latest Volume value.  dsp::Gain smooths it internally, so it
    // is safe to call every block even at high automation rates.
    updateGain();

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

const juce::String RevealAudioProcessor::getName() const { return JucePlugin_Name; }

// Entry point called by the host / plugin wrapper.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RevealAudioProcessor();
}
