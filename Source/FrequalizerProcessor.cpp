/*
  ==============================================================================

    This is the Frequalizer implementation

  ==============================================================================
*/

#include "FrequalizerProcessor.h"
#include "FrequalizerEditor.h"

int FrequalizerAudioProcessor::numBands = 6;

std::vector<Colour> FrequalizerAudioProcessor::bandColours = {Colour (0xff0000ff), Colour (0xffa52a2a), Colour (0xff008000), Colour (0xffff00ff), Colour (0xffffa500), Colour (0xffff0000)};

std::vector<String> FrequalizerAudioProcessor::bandNames   = {"Lowest", "Low", "Low mids", "High mids", "High", "Highest"};

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
    frequencies.resize (300);
    for (int i=0; i < frequencies.size(); ++i) {
        frequencies [i] = 20.0 * std::pow (2.0, i / 30.0);
    }

    bands.resize (numBands);


    std::vector<double> freqDefaults       = {20.0,     250.0,     500.0,    1000.0,   5000.0,   10000.0};
    std::vector<FilterType> filterDefaults = {HighPass, LowShelf, BandPass, BandPass, HighShelf, LowPass};
    for (int i = 0; i < numBands; ++i) {
        state.createAndAddParameter (getTypeParamName (i), bandNames [i] + " Type", TRANS ("Filter Type"),
                                     NormalisableRange<float> (0, LastFilterID, 1),
                                     filterDefaults [i],
                                     [](float value) { return FrequalizerAudioProcessor::getFilterTypeName (static_cast<FilterType>(value)); },
                                     [](String text) {
                                         for (int i=0; i < LastFilterID; ++i)
                                             if (text == FrequalizerAudioProcessor::getFilterTypeName (static_cast<FilterType>(i)))
                                                 return static_cast<FilterType>(i);
                                         return NoFilter; },
                                     false, true, true);

        state.createAndAddParameter (getFrequencyParamName (i), bandNames [i] + " freq", "Frequency",
                                     NormalisableRange<float> (20.0, 20000.0, 1.0),
                                     freqDefaults [i],
                                     [](float value) { return (value < 1000) ?
                                         String (value, 0) + " Hz" :
                                         String (value / 1000.0, 2) + " kHz"; },
                                     [](String text) { return text.endsWith(" kHz") ?
                                         text.dropLastCharacters (4).getFloatValue() * 1000.0 :
                                         text.dropLastCharacters (3).getFloatValue(); },
                                     false, true, false);
        state.createAndAddParameter (getQualityParamName (i), bandNames [i] + " Q", TRANS ("Quality"),
                                     NormalisableRange<float> (0.1, 10.0, 0.1),
                                     1.0, nullptr, nullptr,
                                     false, true, false);
        state.createAndAddParameter (getGainParamName (i), bandNames [i] + " gain", TRANS ("Gain"),
                                     NormalisableRange<float> (0.25f, 4.0f, 0.001f), 1.0f,
                                     [](float value) {return String (Decibels::gainToDecibels(value)) + " dB";},
                                     [](String text) {return Decibels::decibelsToGain (text.dropLastCharacters (3).getFloatValue());},
                                     false, true, false);
        bands [i].type      = filterDefaults [i];
        bands [i].frequency = freqDefaults   [i];
        bands [i].magnitudes.resize (frequencies.size(), 1.0);

        state.addParameterListener (getTypeParamName (i), this);
        state.addParameterListener (getFrequencyParamName (i), this);
        state.addParameterListener (getQualityParamName (i), this);
        state.addParameterListener (getGainParamName (i), this);
    }

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

    for (int i=0; i < numBands; ++i) {
        updateBand (i);
    }
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

String FrequalizerAudioProcessor::getTypeParamName (const int index)
{
    if (isPositiveAndBelow (index, FrequalizerAudioProcessor::bandNames.size()))
        return FrequalizerAudioProcessor::bandNames [index] + "-type";

    return "type";
}

String FrequalizerAudioProcessor::getFrequencyParamName (const int index)
{
    if (isPositiveAndBelow (index, FrequalizerAudioProcessor::bandNames.size()))
        return FrequalizerAudioProcessor::bandNames [index] + "-frequency";

    return "frequency";
}

String FrequalizerAudioProcessor::getQualityParamName (const int index)
{
    if (isPositiveAndBelow (index, FrequalizerAudioProcessor::bandNames.size()))
        return FrequalizerAudioProcessor::bandNames [index] + "-quality";

    return "quality";
}

String FrequalizerAudioProcessor::getGainParamName (const int index)
{
    if (isPositiveAndBelow (index, FrequalizerAudioProcessor::bandNames.size()))
        return FrequalizerAudioProcessor::bandNames [index] + "-gain";

    return "gain";
}

void FrequalizerAudioProcessor::parameterChanged (const String& parameter, float newValue)
{
    for (int i=0; i < bandNames.size(); ++i) {
        if (parameter.startsWith (bandNames [i] + "-")) {
            if (parameter.endsWith ("type")) {
                bands [i].type = static_cast<FilterType> (newValue);
            }
            else if (parameter.endsWith ("frequency")) {
                bands [i].frequency = newValue;
            }
            else if (parameter.endsWith ("quality")) {
                bands [i].quality = newValue;
            }
            else if (parameter.endsWith ("gain")) {
                bands [i].gain = newValue;
            }
            
            updateBand (i);
            return;
        }
    }
}

FrequalizerAudioProcessor::FilterType FrequalizerAudioProcessor::getFilterType (const int index)
{
    if (isPositiveAndBelow (index, bands.size())) {
        return bands [index].type;
    }
    return AllPass;
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
            magnitudes = bands [0].magnitudes;
            for (int i=1; i < bands.size(); ++i) {
                FloatVectorOperations::multiply (magnitudes.data(), bands [i].magnitudes.data(), static_cast<int> (magnitudes.size()));
            }
        }
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
