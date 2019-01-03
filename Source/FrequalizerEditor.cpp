/*
  ==============================================================================

    This is the Frequalizer UI editor implementation

  ==============================================================================
*/

#include "Analyser.h"
#include "FrequalizerProcessor.h"
#include "SocialButtons.h"
#include "FrequalizerEditor.h"

static int   clickRadius = 4;
static float maxDB       = 24.0f;

//==============================================================================
FrequalizerAudioProcessorEditor::FrequalizerAudioProcessorEditor (FrequalizerAudioProcessor& p)
  : AudioProcessorEditor (&p), processor (p),
    output (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow)
{
    tooltipWindow->setMillisecondsBeforeTipAppears (1000);

    addAndMakeVisible (socialButtons);

    for (size_t i=0; i < processor.getNumBands(); ++i) {
        auto* bandEditor = bandEditors.add (new BandEditor (i, processor));
        addAndMakeVisible (bandEditor);
    }

    frame.setText (TRANS ("Output"));
    frame.setTextLabelPosition (Justification::centred);
    addAndMakeVisible (frame);
    addAndMakeVisible (output);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), FrequalizerAudioProcessor::paramOutput, output));
    output.setTooltip (TRANS ("Overall Gain"));

    auto size = processor.getSavedSize();
    setResizable (true, true);
    setSize (size.x, size.y);
    setResizeLimits (800, 450, 2990, 1800);

    updateFrequencyResponses();

#ifdef JUCE_OPENGL
    openGLContext.attachTo (*getTopLevelComponent());
#endif

    processor.addChangeListener (this);

    startTimerHz (30);
}

FrequalizerAudioProcessorEditor::~FrequalizerAudioProcessorEditor()
{
    PopupMenu::dismissAllActiveMenus();

    processor.removeChangeListener (this);
#ifdef JUCE_OPENGL
    openGLContext.detach();
#endif
}

//==============================================================================
void FrequalizerAudioProcessorEditor::paint (Graphics& g)
{
    const Colour inputColour = Colours::greenyellow;
    const Colour outputColour = Colours::indianred;

    Graphics::ScopedSaveState state (g);

    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    auto logo = ImageCache::getFromMemory (FFAudioData::LogoFF_png, FFAudioData::LogoFF_pngSize);
    g.drawImage (logo, brandingFrame.toFloat(), RectanglePlacement (RectanglePlacement::fillDestination));

    g.setFont (12.0f);
    g.setColour (Colours::silver);
    g.drawRoundedRectangle (plotFrame.toFloat(), 5, 2);
    for (int i=0; i < 10; ++i) {
        g.setColour (Colours::silver.withAlpha (0.3f));
        auto x = plotFrame.getX() + plotFrame.getWidth() * i * 0.1f;
        if (i > 0) g.drawVerticalLine (roundToInt (x), plotFrame.getY(), plotFrame.getBottom());

        g.setColour (Colours::silver);
        auto freq = getFrequencyForPosition (i * 0.1f);
        g.drawFittedText ((freq < 1000) ? String (freq) + " Hz" : String (freq / 1000, 1) + " kHz",
                          roundToInt (x + 3), plotFrame.getBottom() - 18, 50, 15, Justification::left, 1);
    }

    g.setColour (Colours::silver.withAlpha (0.3f));
    g.drawHorizontalLine (roundToInt (plotFrame.getY() + 0.25 * plotFrame.getHeight()), plotFrame.getX(), plotFrame.getRight());
    g.drawHorizontalLine (roundToInt (plotFrame.getY() + 0.75 * plotFrame.getHeight()), plotFrame.getX(), plotFrame.getRight());

    g.setColour (Colours::silver);
    g.drawFittedText (String (maxDB) + " dB", plotFrame.getX() + 3, plotFrame.getY() + 2, 50, 14, Justification::left, 1);
    g.drawFittedText (String (maxDB / 2) + " dB", plotFrame.getX() + 3, roundToInt (plotFrame.getY() + 2 + 0.25 * plotFrame.getHeight()), 50, 14, Justification::left, 1);
    g.drawFittedText (" 0 dB", plotFrame.getX() + 3, roundToInt (plotFrame.getY() + 2 + 0.5  * plotFrame.getHeight()), 50, 14, Justification::left, 1);
    g.drawFittedText (String (- maxDB / 2) + " dB", plotFrame.getX() + 3, roundToInt (plotFrame.getY() + 2 + 0.75 * plotFrame.getHeight()), 50, 14, Justification::left, 1);

    g.reduceClipRegion (plotFrame);

    g.setFont (16.0f);
    processor.createAnalyserPlot (analyserPath, plotFrame, 20.0f, true);
    g.setColour (inputColour);
    g.drawFittedText ("Input", plotFrame.reduced (8), Justification::topRight, 1);
    g.strokePath (analyserPath, PathStrokeType (1.0));
    processor.createAnalyserPlot (analyserPath, plotFrame, 20.0f, false);
    g.setColour (outputColour);
    g.drawFittedText ("Output", plotFrame.reduced (8, 28), Justification::topRight, 1);
    g.strokePath (analyserPath, PathStrokeType (1.0));

    for (size_t i=0; i < processor.getNumBands(); ++i) {
        auto* bandEditor = bandEditors.getUnchecked (int (i));
        auto* band = processor.getBand (i);

        g.setColour (band->active ? band->colour : band->colour.withAlpha (0.3f));
        g.strokePath (bandEditor->frequencyResponse, PathStrokeType (1.0));
        g.setColour (draggingBand == int (i) ? band->colour : band->colour.withAlpha (0.3f));
        auto x = roundToInt (plotFrame.getX() + plotFrame.getWidth() * getPositionForFrequency (float (band->frequency)));
        auto y = roundToInt (getPositionForGain (float (band->gain), plotFrame.getY(), plotFrame.getBottom()));
        g.drawVerticalLine (x, plotFrame.getY(), y - 5);
        g.drawVerticalLine (x, y + 5, plotFrame.getBottom());
        g.fillEllipse (x - 3, y - 3, 6, 6);
    }
    g.setColour (Colours::silver);
    g.strokePath (frequencyResponse, PathStrokeType (1.0));
}

void FrequalizerAudioProcessorEditor::resized()
{
    processor.setSavedSize ({ getWidth(), getHeight() });
    plotFrame = getLocalBounds().reduced (3, 3);

    socialButtons.setBounds (plotFrame.removeFromBottom (35));

    auto bandSpace = plotFrame.removeFromBottom (getHeight() / 2);
    auto width = roundToInt (bandSpace.getWidth()) / (bandEditors.size() + 1);
    for (auto* bandEditor : bandEditors)
        bandEditor->setBounds (bandSpace.removeFromLeft (width));

    frame.setBounds (bandSpace.removeFromTop (bandSpace.getHeight() / 2));
    output.setBounds (frame.getBounds().reduced (8));

    plotFrame.reduce (3, 3);
    brandingFrame = bandSpace.reduced (5);

    updateFrequencyResponses();
}

void FrequalizerAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster* sender)
{
    ignoreUnused (sender);
    updateFrequencyResponses();
    repaint();
}

void FrequalizerAudioProcessorEditor::timerCallback()
{
    if (processor.checkForNewAnalyserData())
        repaint (plotFrame);
}

void FrequalizerAudioProcessorEditor::mouseDown (const MouseEvent& e)
{
    if (! e.mods.isPopupMenu() || ! plotFrame.contains (e.x, e.y))
        return;

    for (int i=0; i < bandEditors.size(); ++i)
    {
        if (auto* band = processor.getBand (size_t (i)))
        {
            if (std::abs (plotFrame.getX() + getPositionForFrequency (int (band->frequency)) * plotFrame.getWidth()
                          - e.position.getX()) < clickRadius)
            {
                contextMenu.clear();
                const auto& names = FrequalizerAudioProcessor::getFilterTypeNames();
                for (int t=0; t < names.size(); ++t)
                    contextMenu.addItem (t + 1, names [t], true, band->type == t);

                contextMenu.showMenuAsync (PopupMenu::Options()
                                           .withTargetComponent (this)
                                           .withTargetScreenArea ({e.getScreenX(), e.getScreenY(), 1, 1})
                                           , [this, i](int selected)
                                           {
                                               if (selected > 0)
                                                   bandEditors.getUnchecked (i)->setType (selected - 1);
                                           });
                return;
            }
        }
    }
}

void FrequalizerAudioProcessorEditor::mouseMove (const MouseEvent& e)
{
    if (plotFrame.contains (e.x, e.y))
    {
        for (int i=0; i < bandEditors.size(); ++i)
        {
            if (auto* band = processor.getBand (size_t (i)))
            {
                auto pos = plotFrame.getX() + getPositionForFrequency (float (band->frequency)) * plotFrame.getWidth();

                if (std::abs (pos - e.position.getX()) < clickRadius)
                {
                    if (std::abs (getPositionForGain (float (band->gain), plotFrame.getY(), plotFrame.getBottom())
                                  - e.position.getY()) < clickRadius)
                    {
                        draggingGain = processor.getPluginState().getParameter (processor.getGainParamName (size_t (i)));
                        setMouseCursor (MouseCursor (MouseCursor::UpDownLeftRightResizeCursor));
                    }
                    else
                    {
                        setMouseCursor (MouseCursor (MouseCursor::LeftRightResizeCursor));
                    }

                    if (i != draggingBand)
                    {
                        draggingBand = i;
                        repaint (plotFrame);
                    }
                    return;
                }
            }
        }
    }
    draggingBand = -1;
    draggingGain = false;
    setMouseCursor (MouseCursor (MouseCursor::NormalCursor));
    repaint (plotFrame);
}

void FrequalizerAudioProcessorEditor::mouseDrag (const MouseEvent& e)
{
    if (isPositiveAndBelow (draggingBand, bandEditors.size()))
    {
        auto pos = (e.position.getX() - plotFrame.getX()) / plotFrame.getWidth();
        bandEditors [draggingBand]->setFrequency (getFrequencyForPosition (pos));
        if (draggingGain)
            bandEditors [draggingBand]->setGain (getGainForPosition (e.position.getY(), plotFrame.getY(), plotFrame.getBottom()));
    }
}

void FrequalizerAudioProcessorEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (plotFrame.contains (e.x, e.y))
    {
        for (size_t i=0; i < size_t (bandEditors.size()); ++i)
        {
            if (auto* band = processor.getBand (i))
            {
                if (std::abs (plotFrame.getX() + getPositionForFrequency (float (band->frequency)) * plotFrame.getWidth()
                              - e.position.getX()) < clickRadius)
                {
                    if (auto* param = processor.getPluginState().getParameter (processor.getActiveParamName (i)))
                        param->setValueNotifyingHost (param->getValue() < 0.5f ? 1.0f : 0.0f);
                }
            }
        }
    }
}

void FrequalizerAudioProcessorEditor::updateFrequencyResponses ()
{
    auto pixelsPerDouble = 2.0f * plotFrame.getHeight() / Decibels::decibelsToGain (maxDB);

    for (int i=0; i < bandEditors.size(); ++i)
    {
        auto* bandEditor = bandEditors.getUnchecked (i);

        if (auto* band = processor.getBand (size_t (i)))
        {
            bandEditor->updateControls (band->type);
            bandEditor->frequencyResponse.clear();
            processor.createFrequencyPlot (bandEditor->frequencyResponse, band->magnitudes, plotFrame.withX (plotFrame.getX() + 1), pixelsPerDouble);
        }
        bandEditor->updateSoloState (processor.getBandSolo (i));
    }
    frequencyResponse.clear();
    processor.createFrequencyPlot (frequencyResponse, processor.getMagnitudes(), plotFrame, pixelsPerDouble);
}

float FrequalizerAudioProcessorEditor::getPositionForFrequency (float freq)
{
    return (std::log (freq / 20.0f) / std::log (2.0f)) / 10.0f;
}

float FrequalizerAudioProcessorEditor::getFrequencyForPosition (float pos)
{
    return 20.0f * std::pow (2.0f, pos * 10.0f);
}

float FrequalizerAudioProcessorEditor::getPositionForGain (float gain, float top, float bottom)
{
    return jmap (Decibels::gainToDecibels (gain, -maxDB), -maxDB, maxDB, bottom, top);
}

float FrequalizerAudioProcessorEditor::getGainForPosition (float pos, float top, float bottom)
{
    return Decibels::decibelsToGain (jmap (pos, bottom, top, -maxDB, maxDB), -maxDB);
}


//==============================================================================
FrequalizerAudioProcessorEditor::BandEditor::BandEditor (size_t i, FrequalizerAudioProcessor& p)
  : index (i),
    processor (p),
    frequency (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow),
    quality   (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow),
    gain      (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow),
    solo      (TRANS ("S")),
    activate  (TRANS ("A"))
{
    frame.setText (processor.getBandName (index));
    frame.setTextLabelPosition (Justification::centred);
    frame.setColour (GroupComponent::textColourId, processor.getBandColour (index));
    frame.setColour (GroupComponent::outlineColourId, processor.getBandColour (index));
    addAndMakeVisible (frame);

    if (auto* choiceParameter = dynamic_cast<AudioParameterChoice*>(processor.getPluginState().getParameter (processor.getTypeParamName (index))))
        filterType.addItemList (choiceParameter->choices, 1);

    addAndMakeVisible (filterType);
    boxAttachments.add (new AudioProcessorValueTreeState::ComboBoxAttachment (processor.getPluginState(), processor.getTypeParamName (index), filterType));

    addAndMakeVisible (frequency);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getFrequencyParamName (index), frequency));
    frequency.setSkewFactorFromMidPoint (1000.0);
    frequency.setTooltip (TRANS ("Filter's frequency"));

    addAndMakeVisible (quality);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getQualityParamName (index), quality));
    quality.setSkewFactorFromMidPoint (1.0);
    quality.setTooltip (TRANS ("Filter's steepness (Quality)"));

    addAndMakeVisible (gain);
    attachments.add (new AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getGainParamName (index), gain));
    gain.setSkewFactorFromMidPoint (1.0);
    gain.setTooltip (TRANS ("Filter's gain"));

    solo.setClickingTogglesState (true);
    solo.addListener (this);
    solo.setColour (TextButton::buttonOnColourId, Colours::yellow);
    addAndMakeVisible (solo);
    solo.setTooltip (TRANS ("Listen only through this filter (solo)"));

    activate.setClickingTogglesState (true);
    activate.setColour (TextButton::buttonOnColourId, Colours::green);
    buttonAttachments.add (new AudioProcessorValueTreeState::ButtonAttachment (processor.getPluginState(), processor.getActiveParamName (index), activate));
    addAndMakeVisible (activate);
    activate.setTooltip (TRANS ("Activate or deactivate this filter"));
}

void FrequalizerAudioProcessorEditor::BandEditor::resized ()
{
    auto bounds = getLocalBounds();
    frame.setBounds (bounds);

    bounds.reduce (10, 20);

    filterType.setBounds (bounds.removeFromTop (20));

    auto freqBounds = bounds.removeFromBottom (bounds.getHeight() * 2 / 3);
    frequency.setBounds (freqBounds.withTop (freqBounds.getY() + 10));

    auto buttons = freqBounds.reduced (5).withHeight (20);
    solo.setBounds (buttons.removeFromLeft (20));
    activate.setBounds (buttons.removeFromRight (20));

    quality.setBounds (bounds.removeFromLeft (bounds.getWidth() / 2));
    gain.setBounds (bounds);
}

void FrequalizerAudioProcessorEditor::BandEditor::updateControls (FrequalizerAudioProcessor::FilterType type)
{
    switch (type) {
        case FrequalizerAudioProcessor::LowPass:
            frequency.setEnabled (true); quality.setEnabled (true); gain.setEnabled (false);
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

void FrequalizerAudioProcessorEditor::BandEditor::updateSoloState (bool isSolo)
{
    solo.setToggleState (isSolo, dontSendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::setFrequency (float freq)
{
    frequency.setValue (freq, sendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::setGain (float gainToUse)
{
    gain.setValue (gainToUse, sendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::setType (int type)
{
    filterType.setSelectedId (type + 1, sendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::buttonClicked (Button* b)
{
    if (b == &solo) {
        processor.setBandSolo (solo.getToggleState() ? int (index) : -1);
    }
}
