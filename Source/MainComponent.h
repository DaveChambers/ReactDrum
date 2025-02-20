#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

//#include "SamplePlayerProcessor.h"

#if JUCE_ANDROID
namespace juce
{
#include "../JuceModules/juce_core/native/juce_android_JNIHelpers.h"
}
#endif

class MainContentComponent   : public Component,
                               public AudioSource,
                               public ChangeListener,
                               private ButtonListener,
                               private Timer
{
public:
    MainContentComponent ()
//            : timeSliceThread ("Player Thread"),
            : thumbnailCache (5),
              thumbnail (512, formatManager, thumbnailCache),
              thumbnailBackground (Colours::white),
              thumbnailForeground (Colours::grey),
              audioPositionColour (Colours::green),
              backgroundColour (Colours::lightcoral)
    {
        setLookAndFeel (&lookAndFeel);
        
        addAndMakeVisible (&playButton);
        playButton.setButtonText ("Play");
        playButton.addListener (this);
        playButton.setColour (TextButton::buttonColourId, Colours::green);
        playButton.setColour (TextButton::textColourOnId, Colours::lightgrey);
        playButton.setColour (TextButton::textColourOffId, Colours::white);
        playButton.setEnabled (false);
        playButton.setTriggeredOnMouseDown (true);
        
        setSize (600, 400);
        
        formatManager.registerBasicFormats();
//        transportSource.addChangeListener (this);
#if JUCE_IOS
        transportSource.setGain(0.6);
#endif
//        thumbnail.addChangeListener (this);

        // TODO: Below to make into React logo ?
//        Path proAudioPath;
//        proAudioPath.loadPathFromData (BinaryData::proaudio_path, BinaryData::proaudio_pathSize);
//        proAudioIcon.setPath (proAudioPath);
//        addAndMakeVisible (proAudioIcon);
//
//        Colour proAudioIconColour = findColour (TextButton::buttonColourId);
//        proAudioIcon.setFill (FillType (proAudioIconColour));

        //loadSampleFromName ("Chhhhaah");

//#if JUCE_ANDROID
//        timeSliceThread.startThread(9);
//#endif
        
        player.setSource(this);
        deviceManager->addAudioCallback(&player);
        
        startTimerHz (25);
    }
    
    ~MainContentComponent()
    {
//        shutdownAudio();
        DBG ("destructor called");
        deviceManager->removeAudioCallback(&player);
    }
    
    void loadSampleFromName (String name)
    {
        DBG ("loadSampleFromName: " << name);
        int dataSizeInBytes;
        name = name.replace(" ", "_");
        String resourceNameString (name + "_" + String (audioFormatExtension));
        const char* resourceName = resourceNameString.toRawUTF8();
        const char* fileData = BinaryData::getNamedResource (resourceName, dataSizeInBytes);
        
        loadNewSample (fileData, dataSizeInBytes, audioFormatExtension);
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        if (readerSource == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        transportSource.getNextAudioBlock (bufferToFill);
    }

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (backgroundColour);
        const Rectangle<int> thumbnailBounds (10, 100, getWidth() - 20, getHeight() - 120);
        
        if (thumbnail.getNumChannels() == 0)
            paintIfNoFileLoaded (g, thumbnailBounds);
        else
            paintIfFileLoaded (g, thumbnailBounds);
    }
    
    void resized() override
    {
        playButton.setBounds (10, 10, getWidth() - 20, 80);
    }
    
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &thumbnail)       thumbnailChanged();
        if (source == &transportSource)
        {
            DBG ("isPlaying " << transportSource.isPlaying());
            DBG ("postion " << transportSource.getCurrentPosition());
        }
    }
    
    void buttonClicked (Button* button) override
    {
        if (button == &playButton)  playButtonClicked();
    }

    void setBackgroundColour(String colourString)
    {
        backgroundColour = Colour::fromString (colourString);
        repaint();
    }

    void setPlayButtonColour(String colourString)
    {
        playButton.setColour (TextButton::buttonColourId, Colour::fromString (colourString));
        repaint();
    }
    
    void setPlayButtonTextColour(String colourString)
    {
        playButton.setColour (TextButton::textColourOnId, Colour::fromString (colourString));
        playButton.setColour (TextButton::textColourOffId, Colour::fromString (colourString));
        repaint();
    }
    
    void setThumbnailBackground(String colourString)
    {
        thumbnailBackground = Colour::fromString (colourString);
        repaint();
    }
    
    void setThumbnailForeground(String colourString)
    {
        thumbnailForeground = Colour::fromString (colourString);
        repaint();
    }
    
    void setAudioPositionColour(String colourString)
    {
        audioPositionColour = Colour::fromString (colourString);
        repaint();
    }
    
private:
    void timerCallback() override
    {
        repaint();
    }
    
    
    void thumbnailChanged()
    {
        repaint();
    }
    
    void paintIfNoFileLoaded (Graphics& g, const Rectangle<int>& thumbnailBounds)
    {
        g.setColour (Colours::darkgrey);
        g.fillRect (thumbnailBounds);
        g.setColour (Colours::white);
        g.drawFittedText ("No File Loaded", thumbnailBounds, Justification::centred, 1.0f);
    }
    
    void paintIfFileLoaded (Graphics& g, const Rectangle<int>& thumbnailBounds)
    {
        // TODO: Optimise paint by drawing thumbnail to Image first
        g.setColour (thumbnailBackground);
        g.fillRoundedRectangle (thumbnailBounds.toFloat(), 5.0f);

        g.setColour (thumbnailForeground);                                     // [8]

        const double audioLength (thumbnail.getTotalLength());
        thumbnail.drawChannels (g,                                      // [9]
                                thumbnailBounds,
                                0.0,                                    // start time
                                thumbnail.getTotalLength(),             // end time
                                1.0f);                                  // vertical zoom
        
        g.setColour (audioPositionColour);
        
        const double audioPosition (transportSource.getCurrentPosition());
        const float drawPosition ((audioPosition / audioLength) * thumbnailBounds.getWidth()
                                  + thumbnailBounds.getX());                                        // [13]
        g.drawLine (drawPosition, thumbnailBounds.getY(), drawPosition,
                    thumbnailBounds.getBottom(), 2.0f);
    }
    
    StringArray getSampleNames()
    {
        StringArray list;
        String formatExtensionUnderscore = "_" + String(audioFormatExtension);
        
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            String filename = BinaryData::namedResourceList[i];
            if (filename.endsWith (formatExtensionUnderscore))
                list.add (filename.upToLastOccurrenceOf (formatExtensionUnderscore, false, true));
        }
        return list;
    }
    

    
    void loadNewSample (const void* data, int dataSize, const char* format)
    {
        DBG ("loadNewSample");
#if JUCE_IOS
        MessageManagerLock mm;
        DBG ("MessageManagerLock");
#endif

        transportSource.stop();
        DBG ("transportSource.stop()");

        MemoryInputStream* soundBuffer = new MemoryInputStream (data, static_cast<std::size_t> (dataSize), false);
        DBG ("soundBuffer");

        AudioFormatReader* reader = formatManager.findFormatForFileExtension (format)->createReaderFor (soundBuffer, true);

        DBG ("got new reader");

        if (reader == nullptr)
        {
            DBG ("loadNewSample: reader == nullptr");
            return;
        }
        
        ScopedPointer<AudioFormatReaderSource> newSource = new AudioFormatReaderSource (reader, false);
        transportSource.setSource (newSource, 0, nullptr, reader->sampleRate);
        thumbnail.setReader (reader, generateHashForSample (data, 128));
        playButton.setEnabled (true);
        transportSource.setPosition(0.0);
        readerSource = newSource.release();
        thumbnailChanged();
    }
    
    int generateHashForSample(const void* data, const int upperLimit)
    {
        return (int)(((pointer_sized_uint) data) % ((pointer_sized_uint) upperLimit));
    }
    
    void playButtonClicked()
    {
        transportSource.stop();
        transportSource.setPosition (0.0);
        transportSource.start();
    }
    
    
    //==========================================================================
    TextButton playButton;

    AudioFormatManager formatManager;
    ScopedPointer<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;
    HashMap<String, String> sampleHashMap;
    AudioThumbnailCache thumbnailCache;
    AudioThumbnail thumbnail;

    SharedResourcePointer<AudioDeviceManager> deviceManager;
    AudioSourcePlayer player;

    LookAndFeel_V3 lookAndFeel;
    Colour thumbnailBackground, thumbnailForeground, audioPositionColour;
    Colour backgroundColour;
    
    const char* audioFormatExtension = "ogg";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED

