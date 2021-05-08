/*
  ==============================================================================

 This is the Frequalizer UI editor

  ==============================================================================
*/

#pragma once

#include "FrequalizerProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>


//==============================================================================
/**
*/
class FrequalizerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         public juce::ChangeListener,
                                         public juce::Timer
{
public:
    FrequalizerAudioProcessorEditor (FrequalizerAudioProcessor&);
    ~FrequalizerAudioProcessorEditor() override;

    //==============================================================================

    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback (juce::ChangeBroadcaster* sender) override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseDoubleClick (const juce::MouseEvent& e) override;

    //==============================================================================

    class BandEditor : public juce::Component,
                       public juce::Button::Listener
    {
    public:
        BandEditor (size_t i, FrequalizerAudioProcessor& processor);

        void resized () override;

        void updateControls (FrequalizerAudioProcessor::FilterType type);

        void updateSoloState (bool isSolo);

        void setFrequency (float frequency);

        void setGain (float gain);

        void setType (int type);

        void buttonClicked (juce::Button* b) override;

        juce::Path frequencyResponse;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandEditor)

        size_t index;
        FrequalizerAudioProcessor& processor;

        juce::GroupComponent      frame;
        juce::ComboBox            filterType;
        juce::Slider              frequency { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Slider              quality   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Slider              gain      { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::TextButton          solo      { TRANS ("S") };
        juce::TextButton          activate  { TRANS ("A") };
        juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachments;
        juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
        juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    };

private:

    void updateFrequencyResponses ();

    static float getPositionForFrequency (float freq);

    static float getFrequencyForPosition (float pos);

    static float getPositionForGain (float gain, float top, float bottom);

    static float getGainForPosition (float pos, float top, float bottom);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FrequalizerAudioProcessor& freqProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequalizerAudioProcessorEditor)

#ifdef JUCE_OPENGL
    juce::OpenGLContext     openGLContext;
#endif

    juce::OwnedArray<BandEditor>  bandEditors;

    juce::Rectangle<int>          plotFrame;
    juce::Rectangle<int>          brandingFrame;

    juce::Path                    frequencyResponse;
    juce::Path                    analyserPath;

    juce::GroupComponent          frame;
    juce::Slider                  output { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };

    SocialButtons                 socialButtons;

    int                           draggingBand = -1;
    bool                          draggingGain = false;

    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
    juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;

    juce::PopupMenu               contextMenu;
};
