/*
  ==============================================================================

    Analyser.h
    Created: 12 Jul 2018 11:27:50pm
    Author:  Daniel Walz

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/*
*/
template<typename Type>
class Analyser : public juce::Thread
{
public:
    Analyser() : juce::Thread ("Frequaliser-Analyser")
    {
        averager.clear();
    }

    ~Analyser() override = default;

    void addAudioData (const juce::AudioBuffer<Type>& buffer, int startChannel, int numChannels)
    {
        if (abstractFifo.getFreeSpace() < buffer.getNumSamples())
            return;

        int start1, block1, start2, block2;
        abstractFifo.prepareToWrite (buffer.getNumSamples(), start1, block1, start2, block2);
        audioFifo.copyFrom (0, start1, buffer.getReadPointer (startChannel), block1);
        if (block2 > 0)
            audioFifo.copyFrom (0, start2, buffer.getReadPointer (startChannel, block1), block2);

        for (int channel = startChannel + 1; channel < startChannel + numChannels; ++channel)
        {
            if (block1 > 0) audioFifo.addFrom (0, start1, buffer.getReadPointer (channel), block1);
            if (block2 > 0) audioFifo.addFrom (0, start2, buffer.getReadPointer (channel, block1), block2);
        }
        abstractFifo.finishedWrite (block1 + block2);
        waitForData.signal();
    }

    void setupAnalyser (int audioFifoSize, Type sampleRateToUse)
    {
        sampleRate = sampleRateToUse;
        audioFifo.setSize (1, audioFifoSize);
        abstractFifo.setTotalSize (audioFifoSize);

        startThread (5);
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            if (abstractFifo.getNumReady() >= fft.getSize())
            {
                fftBuffer.clear();

                int start1, block1, start2, block2;
                abstractFifo.prepareToRead (fft.getSize(), start1, block1, start2, block2);
                if (block1 > 0) fftBuffer.copyFrom (0, 0, audioFifo.getReadPointer (0, start1), block1);
                if (block2 > 0) fftBuffer.copyFrom (0, block1, audioFifo.getReadPointer (0, start2), block2);
                abstractFifo.finishedRead ((block1 + block2) / 2);

                windowing.multiplyWithWindowingTable (fftBuffer.getWritePointer (0), size_t (fft.getSize()));
                fft.performFrequencyOnlyForwardTransform (fftBuffer.getWritePointer (0));

                juce::ScopedLock lockedForWriting (pathCreationLock);
                averager.addFrom (0, 0, averager.getReadPointer (averagerPtr), averager.getNumSamples(), -1.0f);
                averager.copyFrom (averagerPtr, 0, fftBuffer.getReadPointer (0), averager.getNumSamples(), 1.0f / (averager.getNumSamples() * (averager.getNumChannels() - 1)));
                averager.addFrom (0, 0, averager.getReadPointer (averagerPtr), averager.getNumSamples());
                if (++averagerPtr == averager.getNumChannels()) averagerPtr = 1;

                newDataAvailable = true;
            }

            if (abstractFifo.getNumReady() < fft.getSize())
                waitForData.wait (100);
        }
    }

    void createPath (juce::Path& p, const juce::Rectangle<float> bounds, float minFreq)
    {
        p.clear();
        p.preallocateSpace (8 + averager.getNumSamples() * 3);

        juce::ScopedLock lockedForReading (pathCreationLock);
        const auto* fftData = averager.getReadPointer (0);
        const auto  factor  = bounds.getWidth() / 10.0f;

        p.startNewSubPath (bounds.getX() + factor * indexToX (0, minFreq), binToY (fftData [0], bounds));
        for (int i = 0; i < averager.getNumSamples(); ++i)
            p.lineTo (bounds.getX() + factor * indexToX (float (i), minFreq), binToY (fftData [i], bounds));
    }

    bool checkForNewData()
    {
        auto available = newDataAvailable.load();
        newDataAvailable.store (false);
        return available;
    }

private:

    inline float indexToX (float index, float minFreq) const
    {
        const auto freq = (sampleRate * index) / fft.getSize();
        return (freq > 0.01f) ? std::log (freq / minFreq) / std::log (2.0f) : 0.0f;
    }

    inline float binToY (float bin, const juce::Rectangle<float> bounds) const
    {
        const float infinity = -80.0f;
        return juce::jmap (juce::Decibels::gainToDecibels (bin, infinity),
                           infinity, 0.0f, bounds.getBottom(), bounds.getY());
    }

    juce::WaitableEvent waitForData;
    juce::CriticalSection pathCreationLock;

    Type sampleRate {};

    juce::dsp::FFT fft                           { 12 };
    juce::dsp::WindowingFunction<Type> windowing { size_t (fft.getSize()), juce::dsp::WindowingFunction<Type>::hann, true };
    juce::AudioBuffer<float> fftBuffer           { 1, fft.getSize() * 2 };

    juce::AudioBuffer<float> averager            { 5, fft.getSize() / 2 };
    int averagerPtr = 1;

    juce::AbstractFifo abstractFifo              { 48000 };
    juce::AudioBuffer<Type> audioFifo;

    std::atomic<bool> newDataAvailable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Analyser)
};
