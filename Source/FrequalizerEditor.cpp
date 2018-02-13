/*
  ==============================================================================

    This is the Frequalizer UI editor implementation

  ==============================================================================
*/

#include "FrequalizerProcessor.h"
#include "FrequalizerEditor.h"


//==============================================================================
FrequalizerAudioProcessorEditor::FrequalizerAudioProcessorEditor (FrequalizerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{

    for (int i=0; i < FrequalizerAudioProcessor::numBands; ++i) {
        auto* bandEditor = bandEditors.add (new BandEditor (i, processor));
        addAndMakeVisible (bandEditor);
    }

    setResizable (true, false);
    setSize (770, 500);
    processor.addChangeListener (this);
}

FrequalizerAudioProcessorEditor::~FrequalizerAudioProcessorEditor()
{
    processor.removeChangeListener (this);
}

//==============================================================================
void FrequalizerAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (15.0f);

    auto bounds = getLocalBounds();
    for (int i=0; i < FrequalizerAudioProcessor::numBands; ++i) {
        Path p;
        processor.createFrequencyPlot (p, i, plot);
        g.setColour (FrequalizerAudioProcessor::bandColours [i]);
        g.strokePath (p, PathStrokeType (1.0));
    }
    Path p;
    processor.createFrequencyPlot (p, plot);
    g.setColour (Colours::silver);
    g.strokePath (p, PathStrokeType (1.0));

}

void FrequalizerAudioProcessorEditor::resized()
{
    plot = getLocalBounds().reduced (3, 3);

    auto bandSpace = plot.removeFromBottom (getHeight() / 2);
    float width = static_cast<float> (bandSpace.getWidth()) / bandEditors.size();
    for (auto* band : bandEditors)
        band->setBounds (bandSpace.removeFromLeft (width));
}

void FrequalizerAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster* sender)
{
    ignoreUnused (sender);
    for (int i=0; i < bandEditors.size(); ++i) {
        bandEditors.getUnchecked (i)->updateControls (processor.getFilterType (i));
    }
    repaint();
}


//==============================================================================
FrequalizerAudioProcessorEditor::BandEditor::BandEditor (const int i, FrequalizerAudioProcessor& processor)
  : index (i),
    frequency (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow),
    quality   (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow),
    gain      (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow)
{
    frame.setText (FrequalizerAudioProcessor::bandNames [i]);
    frame.setTextLabelPosition (Justification::centred);
    frame.setColour (GroupComponent::textColourId, FrequalizerAudioProcessor::bandColours [i]);
    frame.setColour (GroupComponent::outlineColourId, FrequalizerAudioProcessor::bandColours [i]);
    addAndMakeVisible (frame);

    int bandCounter = 0;
    for (auto& type : FrequalizerAudioProcessor::filterTypeNames)
        filterType.addItem (type, ++bandCounter);

    addAndMakeVisible (filterType);
    boxAttachments.add (new AudioProcessorValueTreeState::ComboBoxAttachment (processor.getPluginState(), processor.getTypeParamName (index), filterType));

    addAndMakeVisible (frequency);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getFrequencyParamName (index), frequency));

    addAndMakeVisible (quality);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getQualityParamName (index), quality));

    addAndMakeVisible (gain);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getGainParamName (index), gain));

    updateControls (processor.getFilterType (index));
}

void FrequalizerAudioProcessorEditor::BandEditor::resized ()
{
    auto bounds = getLocalBounds();
    frame.setBounds (bounds);

    bounds.reduce (10, 20);

    filterType.setBounds (bounds.removeFromTop (20));

    frequency.setBounds (bounds.removeFromBottom (bounds.getHeight() * 2 / 3));
    quality.setBounds (bounds.removeFromLeft (bounds.getWidth() / 2));
    gain.setBounds (bounds);
}

void FrequalizerAudioProcessorEditor::BandEditor::updateControls (FrequalizerAudioProcessor::FilterType type)
{
    switch (type) {
        case FrequalizerAudioProcessor::LowPass:
            frequency.setEnabled (true); quality.setEnabled (false); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::LowPass1st:
            frequency.setEnabled (true); quality.setEnabled (false); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::LowShelf:
            frequency.setEnabled (true); quality.setEnabled (false); gain.setEnabled (true);
            break;
        case FrequalizerAudioProcessor::BandPass:
            frequency.setEnabled (true); quality.setEnabled (true); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::AllPass:
            frequency.setEnabled (true); quality.setEnabled (false); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::AllPass1st:
            frequency.setEnabled (true); quality.setEnabled (false); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::Notch:
            frequency.setEnabled (true); quality.setEnabled (true); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::Peak:
            frequency.setEnabled (true); quality.setEnabled (true); gain.setEnabled (true);
            break;
        case FrequalizerAudioProcessor::HighShelf:
            frequency.setEnabled (true); quality.setEnabled (true); gain.setEnabled (true);
            break;
        case FrequalizerAudioProcessor::HighPass1st:
            frequency.setEnabled (true); quality.setEnabled (false); gain.setEnabled (false);
            break;
        case FrequalizerAudioProcessor::HighPass:
            frequency.setEnabled (true); quality.setEnabled (true); gain.setEnabled (false);
            break;
        default:
            frequency.setEnabled (true);
            quality.setEnabled (true);
            gain.setEnabled (true);
            break;
    }
}
