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
  : juce::AudioProcessorEditor (&p), freqProcessor (p)
{
    tooltipWindow->setMillisecondsBeforeTipAppears (1000);

    addAndMakeVisible (socialButtons);

    for (size_t i=0; i < freqProcessor.getNumBands(); ++i) {
        auto* bandEditor = bandEditors.add (new BandEditor (i, freqProcessor));
        addAndMakeVisible (bandEditor);
    }

    frame.setText (TRANS ("Output"));
    frame.setTextLabelPosition (juce::Justification::centred);
    addAndMakeVisible (frame);
    addAndMakeVisible (output);
    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (freqProcessor.getPluginState(), FrequalizerAudioProcessor::paramOutput, output));
    output.setTooltip (TRANS ("Overall Gain"));

    auto size = freqProcessor.getSavedSize();
    setResizable (true, true);
    setSize (size.x, size.y);
    setResizeLimits (800, 450, 2990, 1800);

    updateFrequencyResponses();

#ifdef JUCE_OPENGL
    openGLContext.attachTo (*getTopLevelComponent());
#endif

    freqProcessor.addChangeListener (this);

    startTimerHz (30);
}

FrequalizerAudioProcessorEditor::~FrequalizerAudioProcessorEditor()
{
    juce::PopupMenu::dismissAllActiveMenus();

    freqProcessor.removeChangeListener (this);
#ifdef JUCE_OPENGL
    openGLContext.detach();
#endif
}

//==============================================================================
void FrequalizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto inputColour = juce::Colours::greenyellow;
    const auto outputColour = juce::Colours::indianred;

    juce::Graphics::ScopedSaveState state (g);

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto logo = juce::ImageCache::getFromMemory (FFAudioData::LogoFF_png, FFAudioData::LogoFF_pngSize);
    g.drawImage (logo, brandingFrame.toFloat(), juce::RectanglePlacement (juce::RectanglePlacement::fillDestination));

    g.setFont (12.0f);
    g.setColour (juce::Colours::silver);
    g.drawRoundedRectangle (plotFrame.toFloat(), 5, 2);
    for (int i=0; i < 10; ++i) {
        g.setColour (juce::Colours::silver.withAlpha (0.3f));
        auto x = plotFrame.getX() + plotFrame.getWidth() * i * 0.1f;
        if (i > 0) g.drawVerticalLine (juce::roundToInt (x), float (plotFrame.getY()), float (plotFrame.getBottom()));

        g.setColour (juce::Colours::silver);
        auto freq = getFrequencyForPosition (i * 0.1f);
        g.drawFittedText ((freq < 1000) ? juce::String (freq) + " Hz"
                                        : juce::String (freq / 1000, 1) + " kHz",
                          juce::roundToInt (x + 3), plotFrame.getBottom() - 18, 50, 15, juce::Justification::left, 1);
    }

    g.setColour (juce::Colours::silver.withAlpha (0.3f));
    g.drawHorizontalLine (juce::roundToInt (plotFrame.getY() + 0.25 * plotFrame.getHeight()), float (plotFrame.getX()), float (plotFrame.getRight()));
    g.drawHorizontalLine (juce::roundToInt (plotFrame.getY() + 0.75 * plotFrame.getHeight()), float (plotFrame.getX()), float (plotFrame.getRight()));

    g.setColour (juce::Colours::silver);
    g.drawFittedText (juce::String (maxDB) + " dB", plotFrame.getX() + 3, plotFrame.getY() + 2, 50, 14, juce::Justification::left, 1);
    g.drawFittedText (juce::String (maxDB / 2) + " dB", plotFrame.getX() + 3, juce::roundToInt (plotFrame.getY() + 2 + 0.25 * plotFrame.getHeight()), 50, 14, juce::Justification::left, 1);
    g.drawFittedText (" 0 dB", plotFrame.getX() + 3, juce::roundToInt (plotFrame.getY() + 2 + 0.5  * plotFrame.getHeight()), 50, 14, juce::Justification::left, 1);
    g.drawFittedText (juce::String (- maxDB / 2) + " dB", plotFrame.getX() + 3, juce::roundToInt (plotFrame.getY() + 2 + 0.75 * plotFrame.getHeight()), 50, 14, juce::Justification::left, 1);

    g.reduceClipRegion (plotFrame);

    g.setFont (16.0f);
    freqProcessor.createAnalyserPlot (analyserPath, plotFrame, 20.0f, true);
    g.setColour (inputColour);
    g.drawFittedText ("Input", plotFrame.reduced (8), juce::Justification::topRight, 1);
    g.strokePath (analyserPath, juce::PathStrokeType (1.0));
    freqProcessor.createAnalyserPlot (analyserPath, plotFrame, 20.0f, false);
    g.setColour (outputColour);
    g.drawFittedText ("Output", plotFrame.reduced (8, 28), juce::Justification::topRight, 1);
    g.strokePath (analyserPath, juce::PathStrokeType (1.0));

    for (size_t i=0; i < freqProcessor.getNumBands(); ++i) {
        auto* bandEditor = bandEditors.getUnchecked (int (i));
        auto* band = freqProcessor.getBand (i);

        g.setColour (band->active ? band->colour : band->colour.withAlpha (0.3f));
        g.strokePath (bandEditor->frequencyResponse, juce::PathStrokeType (1.0));
        g.setColour (draggingBand == int (i) ? band->colour : band->colour.withAlpha (0.3f));
        auto x = juce::roundToInt (plotFrame.getX() + plotFrame.getWidth() * getPositionForFrequency (float (band->frequency)));
        auto y = juce::roundToInt (getPositionForGain (float (band->gain), float (plotFrame.getY()), float (plotFrame.getBottom())));
        g.drawVerticalLine (x, float (plotFrame.getY()), float (y - 5));
        g.drawVerticalLine (x, float (y + 5), float (plotFrame.getBottom()));
        g.fillEllipse (float (x - 3), float (y - 3), 6.0f, 6.0f);
    }
    g.setColour (juce::Colours::silver);
    g.strokePath (frequencyResponse, juce::PathStrokeType (1.0f));
}

void FrequalizerAudioProcessorEditor::resized()
{
    freqProcessor.setSavedSize ({ getWidth(), getHeight() });
    plotFrame = getLocalBounds().reduced (3, 3);

    socialButtons.setBounds (plotFrame.removeFromBottom (35));

    auto bandSpace = plotFrame.removeFromBottom (getHeight() / 2);
    auto width = juce::roundToInt (bandSpace.getWidth()) / (bandEditors.size() + 1);
    for (auto* bandEditor : bandEditors)
        bandEditor->setBounds (bandSpace.removeFromLeft (width));

    frame.setBounds (bandSpace.removeFromTop (bandSpace.getHeight() / 2));
    output.setBounds (frame.getBounds().reduced (8));

    plotFrame.reduce (3, 3);
    brandingFrame = bandSpace.reduced (5);

    updateFrequencyResponses();
}

void FrequalizerAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* sender)
{
    ignoreUnused (sender);
    updateFrequencyResponses();
    repaint();
}

void FrequalizerAudioProcessorEditor::timerCallback()
{
    if (freqProcessor.checkForNewAnalyserData())
        repaint (plotFrame);
}

void FrequalizerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() || ! plotFrame.contains (e.x, e.y))
        return;

    for (int i=0; i < bandEditors.size(); ++i)
    {
        if (auto* band = freqProcessor.getBand (size_t (i)))
        {
            if (std::abs (plotFrame.getX() + getPositionForFrequency (float (int (band->frequency)) * plotFrame.getWidth())
                          - e.position.getX()) < clickRadius)
            {
                contextMenu.clear();
                const auto& names = FrequalizerAudioProcessor::getFilterTypeNames();
                for (int t=0; t < names.size(); ++t)
                    contextMenu.addItem (t + 1, names [t], true, band->type == t);

                contextMenu.showMenuAsync (juce::PopupMenu::Options()
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

void FrequalizerAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    if (plotFrame.contains (e.x, e.y))
    {
        for (int i=0; i < bandEditors.size(); ++i)
        {
            if (auto* band = freqProcessor.getBand (size_t (i)))
            {
                auto pos = plotFrame.getX() + getPositionForFrequency (float (band->frequency)) * plotFrame.getWidth();

                if (std::abs (pos - e.position.getX()) < clickRadius)
                {
                    if (std::abs (getPositionForGain (float (band->gain), float (plotFrame.getY()), float (plotFrame.getBottom()))
                                  - e.position.getY()) < clickRadius)
                    {
                        draggingGain = freqProcessor.getPluginState().getParameter (freqProcessor.getGainParamName (size_t (i)));
                        setMouseCursor (juce::MouseCursor (juce::MouseCursor::UpDownLeftRightResizeCursor));
                    }
                    else
                    {
                        setMouseCursor (juce::MouseCursor (juce::MouseCursor::LeftRightResizeCursor));
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
    setMouseCursor (juce::MouseCursor (juce::MouseCursor::NormalCursor));
    repaint (plotFrame);
}

void FrequalizerAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (juce::isPositiveAndBelow (draggingBand, bandEditors.size()))
    {
        auto pos = (e.position.getX() - plotFrame.getX()) / plotFrame.getWidth();
        bandEditors [draggingBand]->setFrequency (getFrequencyForPosition (pos));
        if (draggingGain)
            bandEditors [draggingBand]->setGain (getGainForPosition (e.position.getY(), float (plotFrame.getY()), float (plotFrame.getBottom())));
    }
}

void FrequalizerAudioProcessorEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (plotFrame.contains (e.x, e.y))
    {
        for (size_t i=0; i < size_t (bandEditors.size()); ++i)
        {
            if (auto* band = freqProcessor.getBand (i))
            {
                if (std::abs (plotFrame.getX() + getPositionForFrequency (float (band->frequency)) * plotFrame.getWidth()
                              - e.position.getX()) < clickRadius)
                {
                    if (auto* param = freqProcessor.getPluginState().getParameter (freqProcessor.getActiveParamName (i)))
                        param->setValueNotifyingHost (param->getValue() < 0.5f ? 1.0f : 0.0f);
                }
            }
        }
    }
}

void FrequalizerAudioProcessorEditor::updateFrequencyResponses ()
{
    auto pixelsPerDouble = 2.0f * plotFrame.getHeight() / juce::Decibels::decibelsToGain (maxDB);

    for (int i=0; i < bandEditors.size(); ++i)
    {
        auto* bandEditor = bandEditors.getUnchecked (i);

        if (auto* band = freqProcessor.getBand (size_t (i)))
        {
            bandEditor->updateControls (band->type);
            bandEditor->frequencyResponse.clear();
            freqProcessor.createFrequencyPlot (bandEditor->frequencyResponse, band->magnitudes, plotFrame.withX (plotFrame.getX() + 1), pixelsPerDouble);
        }
        bandEditor->updateSoloState (freqProcessor.getBandSolo (i));
    }
    frequencyResponse.clear();
    freqProcessor.createFrequencyPlot (frequencyResponse, freqProcessor.getMagnitudes(), plotFrame, pixelsPerDouble);
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
    return juce::jmap (juce::Decibels::gainToDecibels (gain, -maxDB), -maxDB, maxDB, bottom, top);
}

float FrequalizerAudioProcessorEditor::getGainForPosition (float pos, float top, float bottom)
{
    return juce::Decibels::decibelsToGain (juce::jmap (pos, bottom, top, -maxDB, maxDB), -maxDB);
}


//==============================================================================
FrequalizerAudioProcessorEditor::BandEditor::BandEditor (size_t i, FrequalizerAudioProcessor& p)
  : index (i),
    processor (p)
{
    frame.setText (processor.getBandName (index));
    frame.setTextLabelPosition (juce::Justification::centred);
    frame.setColour (juce::GroupComponent::textColourId, processor.getBandColour (index));
    frame.setColour (juce::GroupComponent::outlineColourId, processor.getBandColour (index));
    addAndMakeVisible (frame);

    if (auto* choiceParameter = dynamic_cast<juce::AudioParameterChoice*>(processor.getPluginState().getParameter (processor.getTypeParamName (index))))
        filterType.addItemList (choiceParameter->choices, 1);

    addAndMakeVisible (filterType);
    boxAttachments.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (processor.getPluginState(), processor.getTypeParamName (index), filterType));

    addAndMakeVisible (frequency);
    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getFrequencyParamName (index), frequency));
    frequency.setTooltip (TRANS ("Filter's frequency"));

    addAndMakeVisible (quality);
    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getQualityParamName (index), quality));
    quality.setTooltip (TRANS ("Filter's steepness (Quality)"));

    addAndMakeVisible (gain);
    attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (processor.getPluginState(), processor.getGainParamName (index), gain));
    gain.setTooltip (TRANS ("Filter's gain"));

    solo.setClickingTogglesState (true);
    solo.addListener (this);
    solo.setColour (juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    addAndMakeVisible (solo);
    solo.setTooltip (TRANS ("Listen only through this filter (solo)"));

    activate.setClickingTogglesState (true);
    activate.setColour (juce::TextButton::buttonOnColourId, juce::Colours::green);
    buttonAttachments.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (processor.getPluginState(), processor.getActiveParamName (index), activate));
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
        case FrequalizerAudioProcessor::LastFilterID:
        case FrequalizerAudioProcessor::NoFilter:
        default:
            frequency.setEnabled (true);
            quality.setEnabled (true);
            gain.setEnabled (true);
            break;
    }
}

void FrequalizerAudioProcessorEditor::BandEditor::updateSoloState (bool isSolo)
{
    solo.setToggleState (isSolo, juce::dontSendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::setFrequency (float freq)
{
    frequency.setValue (freq, juce::sendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::setGain (float gainToUse)
{
    gain.setValue (gainToUse, juce::sendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::setType (int type)
{
    filterType.setSelectedId (type + 1, juce::sendNotification);
}

void FrequalizerAudioProcessorEditor::BandEditor::buttonClicked (juce::Button* b)
{
    if (b == &solo) {
        processor.setBandSolo (solo.getToggleState() ? int (index) : -1);
    }
}
