/*
  ==============================================================================

    LabeledSlider.h
    Created: 24 Oct 2017 10:10:19pm
    Author:  Dobby

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class TextFormattedSlider : public Slider
{
public:
    enum {
        RawNumber = 0,
        LevelDB,
        GainDB,
        MiliSeconds,
        Hertz,
        Percent,
        Ratio
    };

    TextFormattedSlider (SliderStyle style, TextEntryBoxPosition textBoxPosition, int numberType = 0)
      : Slider (style, textBoxPosition),
        type (numberType)
    {}

    String getTextFromValue (double value) override
    {
        switch (type) {
            case LevelDB:       // level in dB
                return String (value, 1) + " dB";
            case GainDB:        // gain in dB
                return String (Decibels::gainToDecibels (value, -80.0), 1) + " dB";
            case MiliSeconds:   // msecs
                if (value >= 1.0)
                    return String (value, 2) + " s";
                else
                    return String (roundToInt (value * 1000.0)) + " ms";
            case Hertz:         // Hz
                if (value >= 1000.0)
                    return String (value * 0.001, 2) + " kHz";
                else
                    return String (value, 0) + " Hz";
            case Percent:
                return String (roundToInt (value * 100.0)) + " %";
            case Ratio:
                return "1 : " + String (value, 1);
            default:
                return Slider::getTextFromValue (value);
        }
    }
    double getValueFromText (const String &text) override
    {
        switch (type) {
            case LevelDB:
                return text.trimCharactersAtEnd (" dB").getFloatValue();
            case GainDB:
                return Decibels::decibelsToGain (text.trimCharactersAtEnd (" dB").getFloatValue(), -80.0f);
            case MiliSeconds:
                if (text.endsWith ("ms"))
                    return text.trimCharactersAtEnd (" ms").getFloatValue() * 0.001f;
                else
                    return text.trimCharactersAtEnd (" s").getFloatValue();
            case Hertz:
                if (text.endsWith ("kHz"))
                    return text.trimCharactersAtEnd (" kHz").getFloatValue() * 1000.0;
                else
                    return text.trimCharactersAtEnd (" Hz").getFloatValue();
            case Percent:
                return text.getFloatValue() * 0.01;
            case Ratio:
                return text.trimCharactersAtStart ("1 : ").getFloatValue();
            default:
                return Slider::getValueFromText (text.trimCharactersAtEnd (" %"));
        }
    }
    void setNumberType (const int t)
    {
        type = t;
    }

private:
    int type = RawNumber;
};


class LabeledSlider : public Component
{

public:

    LabeledSlider (const String& name, Slider::SliderStyle style, Slider::TextEntryBoxPosition textBoxPosition, const int numberType = 0) :
    slider (style, textBoxPosition, numberType)
    {
        frame.setText (name);
        frame.setTextLabelPosition (Justification::centred);
        addAndMakeVisible (frame);
        addAndMakeVisible (slider);
    }

    LabeledSlider () :
    slider (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow)
    {
        addAndMakeVisible (frame);
        addAndMakeVisible (slider);
    }

    void resized () override
    {
        auto b = getLocalBounds();
        frame.setBounds (b);
        slider.setBounds (b.reduced (15));
    }

    GroupComponent      frame;
    TextFormattedSlider slider;


};
