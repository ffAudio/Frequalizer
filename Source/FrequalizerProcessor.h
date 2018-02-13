/*
  ==============================================================================

    This is the Fruequalizer processor

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

    static int numBands;

    String getTypeParamName (const int index) const;
    String getFrequencyParamName (const int index) const;
    String getQualityParamName (const int index) const;
    String getGainParamName (const int index) const;

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

    FilterType getFilterType (const int index) const;
    String     getBandName   (const int index) const;
    Colour     getBandColour (const int index) const;

    static String getFilterTypeName (const FilterType type);

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    void createFrequencyPlot (Path& p, const Rectangle<int> bounds);
    void createFrequencyPlot (Path& p, const int index, const Rectangle<int> bounds);

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

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessor)

    void updateBand (const int index);

    UndoManager                  undo;
    AudioProcessorValueTreeState state;

    struct Band {
        String      name;
        Colour      colour;
        FilterType  type      = BandPass;
        double      frequency = 1000.0;
        double      quality   = 1.0;
        double      gain      = 1.0;
        bool        active    = false;
        std::vector<double> magnitudes;
    };

    std::vector<Band>    bands;

    std::vector<double> frequencies;
    std::vector<double> magnitudes;

    bool wasBypassed = true;

    using FilterBand = dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>>;
    dsp::ProcessorChain<FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand> filter;

    double sampleRate = 0;

};
