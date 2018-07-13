/*
  ==============================================================================

    This is the Frequalizer processor

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


//==============================================================================
/**
*/
class FrequalizerAudioProcessor  : public AudioProcessor,
                                   public AudioProcessorValueTreeState::Listener,
                                   public ChangeBroadcaster
{
public:
    enum FilterType
    {
        NoFilter = 0,
        HighPass,
        HighPass1st,
        LowShelf,
        BandPass,
        AllPass,
        AllPass1st,
        Notch,
        Peak,
        HighShelf,
        LowPass1st,
        LowPass,
        LastFilterID
    };

    static String paramOutput;
    static String paramType;
    static String paramFrequency;
    static String paramQuality;
    static String paramGain;
    static String paramActive;

    String getTypeParamName (const int index) const;
    String getFrequencyParamName (const int index) const;
    String getQualityParamName (const int index) const;
    String getGainParamName (const int index) const;
    String getActiveParamName (const int index) const;

    //==============================================================================
    FrequalizerAudioProcessor();
    ~FrequalizerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double newSampleRate, int newSamplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    void parameterChanged (const String& parameter, float newValue) override;

    AudioProcessorValueTreeState& getPluginState();

    int getNumBands () const;

    String     getBandName   (const int index) const;
    Colour     getBandColour (const int index) const;

    void setBandSolo (const int index);
    bool getBandSolo (const int index) const;

    static String getFilterTypeName (const FilterType type);

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const std::vector<double>& getMagnitudes ();

    void createFrequencyPlot (Path& p, const std::vector<double>& mags, const Rectangle<int> bounds);

    void createAnalyserPlot (Path& p, const Rectangle<int> bounds, float minFreq, bool input);

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    struct Band {
        String      name;
        Colour      colour;
        FilterType  type      = BandPass;
        double      frequency = 1000.0;
        double      quality   = 1.0;
        double      gain      = 1.0;
        bool        active    = true;
        std::vector<double> magnitudes;
    };

    Band* getBand (const int index);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessor)

    void updateBand (const int index);

    void updateBypassedStates ();

    void updatePlots ();

    UndoManager                  undo;
    AudioProcessorValueTreeState state;

    std::vector<Band>    bands;

    std::vector<double> frequencies;
    std::vector<double> magnitudes;

    bool wasBypassed = true;

    using FilterBand = dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>>;
    using Gain       = dsp::Gain<float>;
    dsp::ProcessorChain<FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, Gain> filter;

    double sampleRate = 0;

    int soloed = -1;

    Analyser<float> inputAnalyser;
    Analyser<float> outputAnalyser;

};
