/*
  ==============================================================================

    This is the Frequalizer implementation

  ==============================================================================
*/

#include "Analyser.h"
#include "FrequalizerProcessor.h"
#include "SocialButtons.h"
#include "FrequalizerEditor.h"


juce::String FrequalizerAudioProcessor::paramOutput   ("output");
juce::String FrequalizerAudioProcessor::paramType     ("type");
juce::String FrequalizerAudioProcessor::paramFrequency("frequency");
juce::String FrequalizerAudioProcessor::paramQuality  ("quality");
juce::String FrequalizerAudioProcessor::paramGain     ("gain");
juce::String FrequalizerAudioProcessor::paramActive   ("active");

namespace IDs
{
    juce::String editor {"editor"};
    juce::String sizeX  {"size-x"};
    juce::String sizeY  {"size-y"};
}

juce::String FrequalizerAudioProcessor::getBandID (size_t index)
{
    switch (index)
    {
        case 0: return "Lowest";
        case 1: return "Low";
        case 2: return "Low Mids";
        case 3: return "High Mids";
        case 4: return "High";
        case 5: return "Highest";
        default: break;
    }
    return "unknown";
}

int FrequalizerAudioProcessor::getBandIndexFromID (juce::String paramID)
{
    for (size_t i=0; i < 6; ++i)
        if (paramID.startsWith (getBandID (i) + "-"))
            return int (i);

    return -1;
}

std::vector<FrequalizerAudioProcessor::Band> createDefaultBands()
{
    std::vector<FrequalizerAudioProcessor::Band> defaults;
    defaults.push_back (FrequalizerAudioProcessor::Band (TRANS ("Lowest"),    juce::Colours::blue,   FrequalizerAudioProcessor::HighPass,    20.0f, 0.707f));
    defaults.push_back (FrequalizerAudioProcessor::Band (TRANS ("Low"),       juce::Colours::brown,  FrequalizerAudioProcessor::LowShelf,   250.0f, 0.707f));
    defaults.push_back (FrequalizerAudioProcessor::Band (TRANS ("Low Mids"),  juce::Colours::green,  FrequalizerAudioProcessor::Peak,       500.0f, 0.707f));
    defaults.push_back (FrequalizerAudioProcessor::Band (TRANS ("High Mids"), juce::Colours::coral,  FrequalizerAudioProcessor::Peak,      1000.0f, 0.707f));
    defaults.push_back (FrequalizerAudioProcessor::Band (TRANS ("High"),      juce::Colours::orange, FrequalizerAudioProcessor::HighShelf, 5000.0f, 0.707f));
    defaults.push_back (FrequalizerAudioProcessor::Band (TRANS ("Highest"),   juce::Colours::red,    FrequalizerAudioProcessor::LowPass,  12000.0f, 0.707f));
    return defaults;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::AudioProcessorParameterGroup>> params;

    // setting defaults
    const float maxGain = juce::Decibels::decibelsToGain (24.0f);
    auto defaults = createDefaultBands();

    {
        auto param = std::make_unique<juce::AudioParameterFloat> (FrequalizerAudioProcessor::paramOutput, TRANS ("Output"),
                                                            juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f,
                                                            TRANS ("Output level"),
                                                            juce::AudioProcessorParameter::genericParameter,
                                                            [](float value, int) {return juce::String (juce::Decibels::gainToDecibels(value), 1) + " dB";},
                                                            [](juce::String text) {return juce::Decibels::decibelsToGain (text.dropLastCharacters (3).getFloatValue());});

        auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("global", TRANS ("Globals"), "|", std::move (param));
        params.push_back (std::move (group));
    }

    for (size_t i = 0; i < defaults.size(); ++i)
    {
        auto prefix = "Q" + juce::String (i + 1) + ": ";

        auto typeParameter = std::make_unique<juce::AudioParameterChoice> (FrequalizerAudioProcessor::getTypeParamName (i),
                                                                     prefix + TRANS ("Filter Type"),
                                                                     FrequalizerAudioProcessor::getFilterTypeNames(),
                                                                     defaults [i].type);

        auto freqParameter = std::make_unique<juce::AudioParameterFloat> (FrequalizerAudioProcessor::getFrequencyParamName (i),
                                                                    prefix + TRANS ("Frequency"),
                                                                    juce::NormalisableRange<float> {20.0f, 20000.0f, 1.0f, std::log (0.5f) / std::log (980.0f / 19980.0f)},
                                                                    defaults [i].frequency,
                                                                    juce::String(),
                                                                    juce::AudioProcessorParameter::genericParameter,
                                                                    [](float value, int) { return (value < 1000.0f) ?
                                                                        juce::String (value, 0) + " Hz" :
                                                                        juce::String (value / 1000.0f, 2) + " kHz"; },
                                                                    [](juce::String text) { return text.endsWith(" kHz") ?
                                                                        text.dropLastCharacters (4).getFloatValue() * 1000.0f :
                                                                        text.dropLastCharacters (3).getFloatValue(); });

        auto qltyParameter = std::make_unique<juce::AudioParameterFloat> (FrequalizerAudioProcessor::getQualityParamName (i),
                                                                    prefix + TRANS ("Quality"),
                                                                    juce::NormalisableRange<float> {0.1f, 10.0f, 1.0f, std::log (0.5f) / std::log (0.9f / 9.9f)},
                                                                    defaults [i].quality,
                                                                    juce::String(),
                                                                    juce::AudioProcessorParameter::genericParameter,
                                                                    [](float value, int) { return juce::String (value, 1); },
                                                                    [](const juce::String& text) { return text.getFloatValue(); });

        auto gainParameter = std::make_unique<juce::AudioParameterFloat> (FrequalizerAudioProcessor::getGainParamName (i),
                                                                    prefix + TRANS ("Gain"),
                                                                    juce::NormalisableRange<float> {1.0f / maxGain, maxGain, 0.001f,
                                                                        std::log (0.5f) / std::log ((1.0f - (1.0f / maxGain)) / (maxGain - (1.0f / maxGain)))},
                                                                    defaults [i].gain,
                                                                    juce::String(),
                                                                    juce::AudioProcessorParameter::genericParameter,
                                                                    [](float value, int) {return juce::String (juce::Decibels::gainToDecibels(value), 1) + " dB";},
                                                                    [](juce::String text) {return juce::Decibels::decibelsToGain (text.dropLastCharacters (3).getFloatValue());});

        auto actvParameter = std::make_unique<juce::AudioParameterBool> (FrequalizerAudioProcessor::getActiveParamName (i),
                                                                   prefix + TRANS ("Active"),
                                                                   defaults [i].active,
                                                                   juce::String(),
                                                                   [](float value, int) {return value > 0.5f ? TRANS ("active") : TRANS ("bypassed");},
                                                                   [](juce::String text) {return text == TRANS ("active");});

        auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("band" + juce::String (i), defaults [i].name, "|",
                                                                     std::move (typeParameter),
                                                                     std::move (freqParameter),
                                                                     std::move (qltyParameter),
                                                                     std::move (gainParameter),
                                                                     std::move (actvParameter));

        params.push_back (std::move (group));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
FrequalizerAudioProcessor::FrequalizerAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor (BusesProperties()
                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                    ),
#endif
state (*this, &undo, "PARAMS", createParameterLayout())
{
    frequencies.resize (300);
    for (size_t i=0; i < frequencies.size(); ++i) {
        frequencies [i] = 20.0 * std::pow (2.0, i / 30.0);
    }
    magnitudes.resize (frequencies.size());

    // needs to be in sync with the ProcessorChain filter
    bands = createDefaultBands();

    for (size_t i = 0; i < bands.size(); ++i)
    {
        bands [i].magnitudes.resize (frequencies.size(), 1.0);

        state.addParameterListener (getTypeParamName (i), this);
        state.addParameterListener (getFrequencyParamName (i), this);
        state.addParameterListener (getQualityParamName (i), this);
        state.addParameterListener (getGainParamName (i), this);
        state.addParameterListener (getActiveParamName (i), this);
    }

    state.addParameterListener (paramOutput, this);

    state.state = juce::ValueTree (JucePlugin_Name);
}

FrequalizerAudioProcessor::~FrequalizerAudioProcessor()
{
    inputAnalyser.stopThread (1000);
    outputAnalyser.stopThread (1000);
}

//==============================================================================
const juce::String FrequalizerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FrequalizerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FrequalizerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FrequalizerAudioProcessor::isMidiEffect() const
{
    return false;
}

double FrequalizerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FrequalizerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FrequalizerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FrequalizerAudioProcessor::setCurrentProgram (int)
{
}

const juce::String FrequalizerAudioProcessor::getProgramName (int)
{
    return {};
}

void FrequalizerAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void FrequalizerAudioProcessor::prepareToPlay (double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = juce::uint32 (newSamplesPerBlock);
    spec.numChannels = juce::uint32 (getTotalNumOutputChannels ());

    for (size_t i=0; i < bands.size(); ++i) {
        updateBand (i);
    }
    filter.get<6>().setGainLinear (*state.getRawParameterValue (paramOutput));

    updatePlots();

    filter.prepare (spec);

    inputAnalyser.setupAnalyser  (int (sampleRate), float (sampleRate));
    outputAnalyser.setupAnalyser (int (sampleRate), float (sampleRate));
}

void FrequalizerAudioProcessor::releaseResources()
{
    inputAnalyser.stopThread (1000);
    outputAnalyser.stopThread (1000);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FrequalizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void FrequalizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    if (getActiveEditor() != nullptr)
        inputAnalyser.addAudioData (buffer, 0, getTotalNumInputChannels());

    if (wasBypassed) {
        filter.reset();
        wasBypassed = false;
    }
    juce::dsp::AudioBlock<float>              ioBuffer (buffer);
    juce::dsp::ProcessContextReplacing<float> context  (ioBuffer);
    filter.process (context);

    if (getActiveEditor() != nullptr)
        outputAnalyser.addAudioData (buffer, 0, getTotalNumOutputChannels());
}

juce::AudioProcessorValueTreeState& FrequalizerAudioProcessor::getPluginState()
{
    return state;
}

juce::String FrequalizerAudioProcessor::getTypeParamName (size_t index)
{
    return getBandID (index) + "-" + paramType;
}

juce::String FrequalizerAudioProcessor::getFrequencyParamName (size_t index)
{
    return getBandID (index) + "-" + paramFrequency;
}

juce::String FrequalizerAudioProcessor::getQualityParamName (size_t index)
{
    return getBandID (index) + "-" + paramQuality;
}

juce::String FrequalizerAudioProcessor::getGainParamName (size_t index)
{
    return getBandID (index) + "-" + paramGain;
}

juce::String FrequalizerAudioProcessor::getActiveParamName (size_t index)
{
    return getBandID (index) + "-" + paramActive;
}

void FrequalizerAudioProcessor::parameterChanged (const juce::String& parameter, float newValue)
{
    if (parameter == paramOutput) {
        filter.get<6>().setGainLinear (newValue);
        updatePlots();
        return;
    }

    int index = getBandIndexFromID (parameter);
    if (juce::isPositiveAndBelow (index, bands.size()))
    {
        auto* band = getBand (size_t (index));
        if (parameter.endsWith (paramType)) {
            band->type = static_cast<FilterType> (static_cast<int> (newValue));
        }
        else if (parameter.endsWith (paramFrequency)) {
            band->frequency = newValue;
        }
        else if (parameter.endsWith (paramQuality)) {
            band->quality = newValue;
        }
        else if (parameter.endsWith (paramGain)) {
            band->gain = newValue;
        }
        else if (parameter.endsWith (paramActive)) {
            band->active = newValue >= 0.5f;
        }

        updateBand (size_t (index));
    }
}

size_t FrequalizerAudioProcessor::getNumBands () const
{
    return bands.size();
}

juce::String FrequalizerAudioProcessor::getBandName   (size_t index) const
{
    if (juce::isPositiveAndBelow (index, bands.size()))
        return bands [size_t (index)].name;
    return TRANS ("unknown");
}
juce::Colour FrequalizerAudioProcessor::getBandColour (size_t index) const
{
    if (juce::isPositiveAndBelow (index, bands.size()))
        return bands [size_t (index)].colour;
    return juce::Colours::silver;
}

bool FrequalizerAudioProcessor::getBandSolo (int index) const
{
    return index == soloed;
}

void FrequalizerAudioProcessor::setBandSolo (int index)
{
    soloed = index;
    updateBypassedStates();
}

void FrequalizerAudioProcessor::updateBypassedStates ()
{
    if (juce::isPositiveAndBelow (soloed, bands.size())) {
        filter.setBypassed<0>(soloed != 0);
        filter.setBypassed<1>(soloed != 1);
        filter.setBypassed<2>(soloed != 2);
        filter.setBypassed<3>(soloed != 3);
        filter.setBypassed<4>(soloed != 4);
        filter.setBypassed<5>(soloed != 5);
    }
    else {
        filter.setBypassed<0>(!bands[0].active);
        filter.setBypassed<1>(!bands[1].active);
        filter.setBypassed<2>(!bands[2].active);
        filter.setBypassed<3>(!bands[3].active);
        filter.setBypassed<4>(!bands[4].active);
        filter.setBypassed<5>(!bands[5].active);
    }
    updatePlots();
}

FrequalizerAudioProcessor::Band* FrequalizerAudioProcessor::getBand (size_t index)
{
    if (juce::isPositiveAndBelow (index, bands.size()))
        return &bands [index];
    return nullptr;
}

juce::StringArray FrequalizerAudioProcessor::getFilterTypeNames()
{
    return {
        TRANS ("No Filter"),
        TRANS ("High Pass"),
        TRANS ("1st High Pass"),
        TRANS ("Low Shelf"),
        TRANS ("Band Pass"),
        TRANS ("All Pass"),
        TRANS ("1st All Pass"),
        TRANS ("Notch"),
        TRANS ("Peak"),
        TRANS ("High Shelf"),
        TRANS ("1st Low Pass"),
        TRANS ("Low Pass")
    };
}

void FrequalizerAudioProcessor::updateBand (const size_t index)
{
    if (sampleRate > 0) {
        juce::dsp::IIR::Coefficients<float>::Ptr newCoefficients;
        switch (bands [index].type) {
            case NoFilter:
                newCoefficients = new juce::dsp::IIR::Coefficients<float> (1, 0, 1, 0);
                break;
            case LowPass:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case LowPass1st:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sampleRate, bands [index].frequency);
                break;
            case LowShelf:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, bands [index].frequency, bands [index].quality, bands [index].gain);
                break;
            case BandPass:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case AllPass:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case AllPass1st:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (sampleRate, bands [index].frequency);
                break;
            case Notch:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeNotch (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case Peak:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, bands [index].frequency, bands [index].quality, bands [index].gain);
                break;
            case HighShelf:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, bands [index].frequency, bands [index].quality, bands [index].gain);
                break;
            case HighPass1st:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sampleRate, bands [index].frequency);
                break;
            case HighPass:
                newCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case LastFilterID:
            default:
                break;
        }

        if (newCoefficients)
        {
            {
                // minimise lock scope, get<0>() needs to be a  compile time constant
                juce::ScopedLock processLock (getCallbackLock());
                if (index == 0)
                    *filter.get<0>().state = *newCoefficients;
                else if (index == 1)
                    *filter.get<1>().state = *newCoefficients;
                else if (index == 2)
                    *filter.get<2>().state = *newCoefficients;
                else if (index == 3)
                    *filter.get<3>().state = *newCoefficients;
                else if (index == 4)
                    *filter.get<4>().state = *newCoefficients;
                else if (index == 5)
                    *filter.get<5>().state = *newCoefficients;
            }
            newCoefficients->getMagnitudeForFrequencyArray (frequencies.data(),
                                                            bands [index].magnitudes.data(),
                                                            frequencies.size(), sampleRate);

        }
        updateBypassedStates();
        updatePlots();
    }
}

void FrequalizerAudioProcessor::updatePlots ()
{
    auto gain = filter.get<6>().getGainLinear();
    std::fill (magnitudes.begin(), magnitudes.end(), gain);

    if (juce::isPositiveAndBelow (soloed, bands.size())) {
        juce::FloatVectorOperations::multiply (magnitudes.data(), bands [size_t (soloed)].magnitudes.data(), static_cast<int> (magnitudes.size()));
    }
    else
    {
        for (size_t i=0; i < bands.size(); ++i)
            if (bands[i].active)
                juce::FloatVectorOperations::multiply (magnitudes.data(), bands [i].magnitudes.data(), static_cast<int> (magnitudes.size()));
    }

    sendChangeMessage();
}

//==============================================================================
bool FrequalizerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FrequalizerAudioProcessor::createEditor()
{
    return new FrequalizerAudioProcessorEditor (*this);
}

const std::vector<double>& FrequalizerAudioProcessor::getMagnitudes ()
{
    return magnitudes;
}

void FrequalizerAudioProcessor::createFrequencyPlot (juce::Path& p, const std::vector<double>& mags, const juce::Rectangle<int> bounds, float pixelsPerDouble)
{
    p.startNewSubPath (float (bounds.getX()), mags [0] > 0 ? float (bounds.getCentreY() - pixelsPerDouble * std::log (mags [0]) / std::log (2.0)) : bounds.getBottom());
    const auto xFactor = static_cast<double> (bounds.getWidth()) / frequencies.size();
    for (size_t i=1; i < frequencies.size(); ++i)
    {
        p.lineTo (float (bounds.getX() + i * xFactor),
                  float (mags [i] > 0 ? bounds.getCentreY() - pixelsPerDouble * std::log (mags [i]) / std::log (2.0) : bounds.getBottom()));
    }
}

void FrequalizerAudioProcessor::createAnalyserPlot (juce::Path& p, const juce::Rectangle<int> bounds, float minFreq, bool input)
{
    if (input)
        inputAnalyser.createPath (p, bounds.toFloat(), minFreq);
    else
        outputAnalyser.createPath (p, bounds.toFloat(), minFreq);
}

bool FrequalizerAudioProcessor::checkForNewAnalyserData()
{
    return inputAnalyser.checkForNewData() || outputAnalyser.checkForNewData();
}

//==============================================================================
void FrequalizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto editor = state.state.getOrCreateChildWithName (IDs::editor, nullptr);
    editor.setProperty (IDs::sizeX, editorSize.x, nullptr);
    editor.setProperty (IDs::sizeY, editorSize.y, nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.state.writeToStream (stream);
}

void FrequalizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, size_t (sizeInBytes));
    if (tree.isValid()) {
        state.state = tree;

        auto editor = state.state.getChildWithName (IDs::editor);
        if (editor.isValid())
        {
            editorSize.setX (editor.getProperty (IDs::sizeX, 900));
            editorSize.setY (editor.getProperty (IDs::sizeY, 500));
            if (auto* thisEditor = getActiveEditor())
                thisEditor->setSize (editorSize.x, editorSize.y);
        }
    }
}

juce::Point<int> FrequalizerAudioProcessor::getSavedSize() const
{
    return editorSize;
}

void FrequalizerAudioProcessor::setSavedSize (const juce::Point<int>& size)
{
    editorSize = size;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FrequalizerAudioProcessor();
}
