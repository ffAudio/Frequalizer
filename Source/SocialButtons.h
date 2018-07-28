/*
  ==============================================================================

    SocialButtons.h
    Created: 25 Feb 2018 1:59:43pm
    Author:  Dobby

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class SocialButtons    : public Component,
                         public Button::Listener
{
public:
    SocialButtons()
    {
        setOpaque (false);

        auto* b = buttons.add (new ImageButton());
        b->addListener (this);
        auto ffLogo = ImageCache::getFromMemory (FFAudioData::LogoFF_png, FFAudioData::LogoFF_pngSize);
        b->setImages (false, true, true, ffLogo, 1.0f, Colours::transparentWhite, ffLogo, 0.7f, Colours::transparentWhite, ffLogo, 0.7f, Colours::transparentWhite);
        b->setComponentID ("https://foleysfinest.com/");
        b->setTooltip (TRANS ("Go to the Foley's Finest Audio Website \"foleysfinest.com\""));
        addAndMakeVisible (b);

        b = buttons.add (new ImageButton());
        b->addListener (this);
        auto fbLogo = ImageCache::getFromMemory (FFAudioData::FBlogo_png, FFAudioData::FBlogo_pngSize);
        b->setImages (false, true, true, fbLogo, 1.0f, Colours::transparentWhite, fbLogo, 0.7f, Colours::transparentWhite, fbLogo, 0.7f, Colours::transparentWhite);
        b->setComponentID ("https://www.fb.com/FoleysFinest/");
        b->setTooltip (TRANS ("Like or connect with us on Facebook"));
        addAndMakeVisible (b);

        b = buttons.add (new ImageButton());
        b->addListener (this);
        auto inLogo = ImageCache::getFromMemory (FFAudioData::Inlogo_png, FFAudioData::Inlogo_pngSize);
        b->setImages (false, true, true, inLogo, 1.0f, Colours::transparentWhite, inLogo, 0.7f, Colours::transparentWhite, inLogo, 0.7f, Colours::transparentWhite);
        b->setComponentID ("https://www.linkedin.com/in/daniel-walz/");
        b->setTooltip (TRANS ("See our profile on Linked.In (TM)"));
        addAndMakeVisible (b);

        b = buttons.add (new ImageButton());
        b->addListener (this);
        auto githubLogo = ImageCache::getFromMemory (FFAudioData::GitHublogo_png, FFAudioData::GitHublogo_pngSize);
        b->setImages (false, true, true, githubLogo, 1.0f, Colours::transparentWhite, githubLogo, 0.7f, Colours::transparentWhite, githubLogo, 0.7f, Colours::transparentWhite);
        b->setComponentID ("https://github.com/ffAudio/");
        b->setTooltip (TRANS ("Find resources on Github"));
        addAndMakeVisible (b);

    }

    ~SocialButtons()
    {
    }

    void paint (Graphics& g) override
    {
        auto renderedText = ImageCache::getFromMemory (FFAudioData::FFtext_png, FFAudioData::FFtext_pngSize);
        g.drawImageWithin (renderedText, 0, 0, getWidth(), getHeight(), RectanglePlacement (RectanglePlacement::xRight));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        for (auto* b : buttons)
            b->setBounds (bounds.removeFromLeft (bounds.getHeight()).reduced (3));
    }

    void buttonClicked (Button* b) override
    {
        URL url (b->getComponentID());
        if (url.isWellFormed()) {
            url.launchInDefaultBrowser();
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SocialButtons)

    OwnedArray<ImageButton> buttons;

};
