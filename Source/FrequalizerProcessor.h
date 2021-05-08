/*
  ==============================================================================

    This is the Frequalizer processor

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>


//==============================================================================
/**
*/
class FrequalizerAudioProcessor  : public juce::AudioProcessor,
                                   public juce::AudioProcessorValueTreeState::Listener,
                                   public juce::ChangeBroadcaster
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

    static juce::String paramOutput;
    static juce::String paramType;
    static juce::String paramFrequency;
    static juce::String paramQuality;
    static juce::String paramGain;
    static juce::String paramActive;

    static juce::String getBandID (size_t index);
    static juce::String getTypeParamName (size_t index);
    static juce::String getFrequencyParamName (size_t index);
    static juce::String getQualityParamName (size_t index);
    static juce::String getGainParamName (size_t index);
    static juce::String getActiveParamName (size_t index);

    //==============================================================================
    FrequalizerAudioProcessor();
    ~FrequalizerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double newSampleRate, int newSamplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void parameterChanged (const juce::String& parameter, float newValue) override;

    juce::AudioProcessorValueTreeState& getPluginState();

    size_t getNumBands () const;

    juce::String getBandName   (size_t index) const;
    juce::Colour getBandColour (size_t index) const;

    void setBandSolo (int index);
    bool getBandSolo (int index) const;

    static juce::StringArray getFilterTypeNames();

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const std::vector<double>& getMagnitudes ();

    void createFrequencyPlot (juce::Path& p, const std::vector<double>& mags, const juce::Rectangle<int> bounds, float pixelsPerDouble);

    void createAnalyserPlot (juce::Path& p, const juce::Rectangle<int> bounds, float minFreq, bool input);

    bool checkForNewAnalyserData();

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    juce::Point<int> getSavedSize() const;
    void setSavedSize (const juce::Point<int>& size);

    //==============================================================================
    struct Band {
        Band (const juce::String& nameToUse, juce::Colour colourToUse, FilterType typeToUse,
            float frequencyToUse, float qualityToUse, float gainToUse=1.0f, bool shouldBeActive=true)
          : name (nameToUse),
            colour (colourToUse),
            type (typeToUse),
            frequency (frequencyToUse),
            quality (qualityToUse),
            gain (gainToUse),
            active (shouldBeActive)
        {}

        juce::String name;
        juce::Colour colour;
        FilterType   type      = BandPass;
        float        frequency = 1000.0f;
        float        quality   = 1.0f;
        float        gain      = 1.0f;
        bool         active    = true;
        std::vector<double> magnitudes;
    };

    Band* getBand (size_t index);
    int getBandIndexFromID (juce::String paramID);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessor)

    void updateBand (const size_t index);

    void updateBypassedStates ();

    void updatePlots ();

    juce::UndoManager                  undo;
    juce::AudioProcessorValueTreeState state;

    std::vector<Band>    bands;

    std::vector<double> frequencies;
    std::vector<double> magnitudes;

    bool wasBypassed = true;

    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain       = juce::dsp::Gain<float>;
    juce::dsp::ProcessorChain<FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, FilterBand, Gain> filter;

    double sampleRate = 0;

    int soloed = -1;

    Analyser<float> inputAnalyser;
    Analyser<float> outputAnalyser;

    juce::Point<int> editorSize = { 900, 500 };
};
