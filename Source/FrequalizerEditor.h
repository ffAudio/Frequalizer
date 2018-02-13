/*
  ==============================================================================

 This is the Frequalizer UI editor

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "FrequalizerProcessor.h"


//==============================================================================
/**
*/
class FrequalizerAudioProcessorEditor  : public AudioProcessorEditor,
                                         public ChangeListener
{
public:
    FrequalizerAudioProcessorEditor (FrequalizerAudioProcessor&);
    ~FrequalizerAudioProcessorEditor();

    //==============================================================================

    void paint (Graphics&) override;
    void resized() override;
    void changeListenerCallback (ChangeBroadcaster* sender) override;

    //==============================================================================

    class BandEditor : public Component
    {
    public:
        BandEditor (const int i, FrequalizerAudioProcessor& processor);

        void resized () override;

        void updateControls (FrequalizerAudioProcessor::FilterType type);

    private:
        int index;

        GroupComponent frame;
        ComboBox       filterType;
        Slider         frequency;
        Slider         quality;
        Slider         gain;
        OwnedArray<AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachments;
        OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachments;
    };


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FrequalizerAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessorEditor)

    OwnedArray<BandEditor>  bandEditors;

    Rectangle<int>          plotFrame;
    Rectangle<int>          branding;

    GroupComponent          frame;
    Slider                  output;

    OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachments;

};
