/*
  ==============================================================================

    This is the Frequalizer implementation

  ==============================================================================
*/

#include "LabeledSlider.h"
#include "FrequalizerProcessor.h"
#include "FrequalizerEditor.h"


String FrequalizerAudioProcessor::paramOutput   ("output");
String FrequalizerAudioProcessor::paramType     ("type");
String FrequalizerAudioProcessor::paramFrequency("frequency");
String FrequalizerAudioProcessor::paramQuality  ("quality");
String FrequalizerAudioProcessor::paramGain     ("gain");



//==============================================================================
FrequalizerAudioProcessor::FrequalizerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                       ),
#else
:
#endif
state (*this, &undo)
{
    state.createAndAddParameter (paramOutput, TRANS ("Output"), TRANS ("Output level"),
                                 NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f,
                                 [](float value) {return String (Decibels::gainToDecibels(value)) + " dB";},
                                 [](String text) {return Decibels::decibelsToGain (text.dropLastCharacters (3).getFloatValue());},
                                 false, true, false);

    frequencies.resize (300);
    for (int i=0; i < frequencies.size(); ++i) {
        frequencies [i] = 20.0 * std::pow (2.0, i / 30.0);
    }
    magnitudes.resize (frequencies.size());

    // needs to be in sync with the ProcessorChain filter
    bands.resize (6);

    // setting defaults
    {
        auto& band = bands [0];
        band.name       = "Lowest";
        band.colour     = Colours::blue;
        band.frequency  = 20.0;
        band.type       = HighPass;
    }
    {
        auto& band = bands [1];
        band.name       = "Low";
        band.colour     = Colours::brown;
        band.frequency  = 250.0;
        band.type       = LowShelf;
    }
    {
        auto& band = bands [2];
        band.name       = "Low Mids";
        band.colour     = Colours::green;
        band.frequency  = 500.0;
        band.type       = Peak;
    }
    {
        auto& band = bands [3];
        band.name       = "High Mids";
        band.colour     = Colours::coral;
        band.frequency  = 1000.0;
        band.type       = Peak;
    }
    {
        auto& band = bands [4];
        band.name       = "High";
        band.colour     = Colours::orange;
        band.frequency  = 5000.0;
        band.type       = HighShelf;
    }
    {
        auto& band = bands [5];
        band.name       = "Highest";
        band.colour     = Colours::red;
        band.frequency  = 12000.0;
        band.type       = LowPass;
    }

    for (int i = 0; i < bands.size(); ++i) {
        auto& band = bands [i];

        band.magnitudes.resize (frequencies.size(), 1.0);

        state.createAndAddParameter (getTypeParamName (i), band.name + " Type", TRANS ("Filter Type"),
                                     NormalisableRange<float> (0, LastFilterID, 1),
                                     band.type,
                                     [](float value) { return FrequalizerAudioProcessor::getFilterTypeName (static_cast<FilterType>(value)); },
                                     [](String text) {
                                         for (int i=0; i < LastFilterID; ++i)
                                             if (text == FrequalizerAudioProcessor::getFilterTypeName (static_cast<FilterType>(i)))
                                                 return static_cast<FilterType>(i);
                                         return NoFilter; },
                                     false, true, true);

        state.createAndAddParameter (getFrequencyParamName (i), band.name + " freq", "Frequency",
                                     NormalisableRange<float> (20.0, 20000.0, 1.0),
                                     band.frequency,
                                     [](float value) { return (value < 1000) ?
                                         String (value, 0) + " Hz" :
                                         String (value / 1000.0, 2) + " kHz"; },
                                     [](String text) { return text.endsWith(" kHz") ?
                                         text.dropLastCharacters (4).getFloatValue() * 1000.0 :
                                         text.dropLastCharacters (3).getFloatValue(); },
                                     false, true, false);
        state.createAndAddParameter (getQualityParamName (i), band.name + " Q", TRANS ("Quality"),
                                     NormalisableRange<float> (0.1, 10.0, 0.1),
                                     band.quality,
                                     nullptr, nullptr,
                                     false, true, false);
        state.createAndAddParameter (getGainParamName (i), band.name + " gain", TRANS ("Gain"),
                                     NormalisableRange<float> (0.25f, 4.0f, 0.001f),
                                     band.gain,
                                     [](float value) {return String (Decibels::gainToDecibels(value)) + " dB";},
                                     [](String text) {return Decibels::decibelsToGain (text.dropLastCharacters (3).getFloatValue());},
                                     false, true, false);

        state.addParameterListener (getTypeParamName (i), this);
        state.addParameterListener (getFrequencyParamName (i), this);
        state.addParameterListener (getQualityParamName (i), this);
        state.addParameterListener (getGainParamName (i), this);
    }

    state.addParameterListener (paramOutput, this);

    state.state = ValueTree (JucePlugin_Name);
}

FrequalizerAudioProcessor::~FrequalizerAudioProcessor()
{
}

//==============================================================================
const String FrequalizerAudioProcessor::getName() const
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

void FrequalizerAudioProcessor::setCurrentProgram (int index)
{
}

const String FrequalizerAudioProcessor::getProgramName (int index)
{
    return {};
}

void FrequalizerAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void FrequalizerAudioProcessor::prepareToPlay (double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;

    dsp::ProcessSpec spec;
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = newSamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels ();

    for (int i=0; i < bands.size(); ++i) {
        updateBand (i);
    }
    filter.get<6>().setGainLinear (*state.getRawParameterValue (paramOutput));

    updatePlots();

    filter.prepare (spec);

}

void FrequalizerAudioProcessor::releaseResources()
{
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

void FrequalizerAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    ignoreUnused (midiMessages);

    if (wasBypassed) {
        filter.reset();
        wasBypassed = false;
    }
    dsp::AudioBlock<float>              ioBuffer (buffer);
    dsp::ProcessContextReplacing<float> context  (ioBuffer);
    filter.process (context);

}

AudioProcessorValueTreeState& FrequalizerAudioProcessor::getPluginState()
{
    return state;
}

String FrequalizerAudioProcessor::getTypeParamName (const int index) const
{
    return getBandName (index) + "-" + paramType;
}

String FrequalizerAudioProcessor::getFrequencyParamName (const int index) const
{
    return getBandName (index) + "-" + paramFrequency;
}

String FrequalizerAudioProcessor::getQualityParamName (const int index) const
{
    return getBandName (index) + "-" + paramQuality;
}

String FrequalizerAudioProcessor::getGainParamName (const int index) const
{
    return getBandName (index) + "-" + paramGain;
}

void FrequalizerAudioProcessor::parameterChanged (const String& parameter, float newValue)
{
    if (parameter == paramOutput) {
        filter.get<6>().setGainLinear (newValue);
        updatePlots();
        return;
    }

    for (int i=0; i < bands.size(); ++i) {
        if (parameter.startsWith (getBandName (i) + "-")) {
            if (parameter.endsWith (paramType)) {
                bands [i].type = static_cast<FilterType> (newValue);
            }
            else if (parameter.endsWith (paramFrequency)) {
                bands [i].frequency = newValue;
            }
            else if (parameter.endsWith (paramQuality)) {
                bands [i].quality = newValue;
            }
            else if (parameter.endsWith (paramGain)) {
                bands [i].gain = newValue;
            }
            
            updateBand (i);
            return;
        }
    }
}

int FrequalizerAudioProcessor::getNumBands () const
{
    return static_cast<int> (bands.size());
}

FrequalizerAudioProcessor::FilterType FrequalizerAudioProcessor::getFilterType (const int index) const
{
    if (isPositiveAndBelow (index, bands.size()))
        return bands [index].type;
    return AllPass;
}
String FrequalizerAudioProcessor::getBandName   (const int index) const
{
    if (isPositiveAndBelow (index, bands.size()))
        return bands [index].name;
    return TRANS ("unknown");
}
Colour FrequalizerAudioProcessor::getBandColour (const int index) const
{
    if (isPositiveAndBelow (index, bands.size()))
        return bands [index].colour;
    return Colours::silver;
}

String FrequalizerAudioProcessor::getFilterTypeName (const FilterType type)
{
    switch (type) {
        case NoFilter:      return TRANS ("No Filter");
        case HighPass:      return TRANS ("High Pass");
        case HighPass1st:   return TRANS ("1st High Pass");
        case LowShelf:      return TRANS ("Low Shelf");
        case BandPass:      return TRANS ("Band Pass");
        case AllPass:       return TRANS ("All Pass");
        case AllPass1st:    return TRANS ("1st All Pass");
        case Notch:         return TRANS ("Notch");
        case Peak:          return TRANS ("Peak");
        case HighShelf:     return TRANS ("High Shelf");
        case LowPass1st:    return TRANS ("1st Low Pass");
        case LowPass:       return TRANS ("Low Pass");
        default:            return TRANS ("unknown");
    }
}

void FrequalizerAudioProcessor::updateBand (const int index)
{
    if (sampleRate > 0) {
        dsp::IIR::Coefficients<float>::Ptr newCoefficients;
        switch (bands [index].type) {
            case NoFilter:
                newCoefficients = new dsp::IIR::Coefficients<float> (1, 0, 1, 0);
                break;
            case LowPass:
                newCoefficients = dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, bands [index].frequency);
                break;
            case LowPass1st:
                newCoefficients = dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sampleRate, bands [index].frequency);
                break;
            case LowShelf:
                newCoefficients = dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, bands [index].frequency, bands [index].quality, bands [index].gain);
                break;
            case BandPass:
                newCoefficients = dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case AllPass:
                newCoefficients = dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case AllPass1st:
                newCoefficients = dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (sampleRate, bands [index].frequency);
                break;
            case Notch:
                newCoefficients = dsp::IIR::Coefficients<float>::makeNotch (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            case Peak:
                newCoefficients = dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, bands [index].frequency, bands [index].quality, bands [index].gain);
                break;
            case HighShelf:
                newCoefficients = dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, bands [index].frequency, bands [index].quality, bands [index].gain);
                break;
            case HighPass1st:
                newCoefficients = dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sampleRate, bands [index].frequency);
                break;
            case HighPass:
                newCoefficients = dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, bands [index].frequency, bands [index].quality);
                break;
            default:
                break;
        }

        if (newCoefficients)
        {
            {
                // minimise lock scope, get<0>() needs to be a  compile time constant
                ScopedLock processLock (getCallbackLock());
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
        updatePlots();
    }
}

void FrequalizerAudioProcessor::updatePlots ()
{
    auto gain = filter.get<6>().getGainLinear();
    std::fill (magnitudes.begin(), magnitudes.end(), gain);

    for (int i=0; i < bands.size(); ++i) {
        FloatVectorOperations::multiply (magnitudes.data(), bands [i].magnitudes.data(), static_cast<int> (magnitudes.size()));
    }

    sendChangeMessage();
}

//==============================================================================
bool FrequalizerAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* FrequalizerAudioProcessor::createEditor()
{
    return new FrequalizerAudioProcessorEditor (*this);
}

void FrequalizerAudioProcessor::createFrequencyPlot (Path& p, const Rectangle<int> bounds)
{
    auto yFactor = 0.5 * bounds.getHeight();
    p.startNewSubPath (bounds.getX(), bounds.getBottom() - yFactor * magnitudes [0]);
    const double xFactor = static_cast<double> (bounds.getWidth()) / frequencies.size();
    for (int i=1; i < frequencies.size(); ++i) {
        p.lineTo (bounds.getX() + i * xFactor, bounds.getBottom() - yFactor * magnitudes [i]);
    }
}

void FrequalizerAudioProcessor::createFrequencyPlot (Path& p, const int index, const Rectangle<int> bounds)
{
    auto& m = bands [index].magnitudes;

    auto yFactor = 0.5 * bounds.getHeight();
    p.startNewSubPath (bounds.getX(), bounds.getBottom() - yFactor * m [0]);
    const double xFactor = static_cast<double> (bounds.getWidth()) / frequencies.size();
    for (int i=1; i < frequencies.size(); ++i) {
        p.lineTo (bounds.getX() + i * xFactor, bounds.getBottom() - yFactor * m [i]);
    }
}

//==============================================================================
void FrequalizerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    MemoryOutputStream stream(destData, false);
    state.state.writeToStream (stream);
}

void FrequalizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        state.state = tree;
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FrequalizerAudioProcessor();
}
