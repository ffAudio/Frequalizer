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
                                         public ChangeListener,
                                         public Timer
{
public:
    FrequalizerAudioProcessorEditor (FrequalizerAudioProcessor&);
    ~FrequalizerAudioProcessorEditor();

    //==============================================================================

    void paint (Graphics&) override;
    void resized() override;
    void changeListenerCallback (ChangeBroadcaster* sender) override;
    void timerCallback() override;

    void mouseDown (const MouseEvent& e) override;

    void mouseMove (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    void mouseDoubleClick (const MouseEvent& e) override;

    //==============================================================================

    class BandEditor : public Component,
                       public Button::Listener
    {
    public:
        BandEditor (size_t i, FrequalizerAudioProcessor& processor);

        void resized () override;

        void updateControls (FrequalizerAudioProcessor::FilterType type);

        void updateSoloState (bool isSolo);

        void setFrequency (float frequency);

        void setGain (float gain);

        void setType (int type);

        void buttonClicked (Button* b) override;

        Path frequencyResponse;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandEditor)

        size_t index;
        FrequalizerAudioProcessor& processor;

        GroupComponent      frame;
        ComboBox            filterType;
        Slider              frequency;
        Slider              quality;
        Slider              gain;
        TextButton          solo;
        TextButton          activate;
        OwnedArray<AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachments;
        OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachments;
        OwnedArray<AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    };

private:

    void updateFrequencyResponses ();

    static float getPositionForFrequency (float freq);

    static float getFrequencyForPosition (float pos);

    static float getPositionForGain (float gain, float top, float bottom);

    static float getGainForPosition (float pos, float top, float bottom);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FrequalizerAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessorEditor)

#ifdef JUCE_OPENGL
    OpenGLContext           openGLContext;
#endif

    OwnedArray<BandEditor>  bandEditors;

    Rectangle<int>          plotFrame;
    Rectangle<int>          brandingFrame;

    Path                    frequencyResponse;
    Path                    analyserPath;

    GroupComponent          frame;
    Slider                  output;

    SocialButtons           socialButtons;

    int                     draggingBand = -1;
    bool                    draggingGain = false;

    OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachments;
    SharedResourcePointer<TooltipWindow> tooltipWindow;

    PopupMenu               contextMenu;
};
