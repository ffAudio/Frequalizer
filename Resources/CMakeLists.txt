juce_add_binary_data(frequalizer_binary NAMESPACE FFAudioData SOURCES FB-logo.png
                                                FF-text.png
                                                GitHub-logo.png
                                                In-logo.png
                                                Logo-FF.png
                                                Screenshot.png)
target_link_libraries(frequalizer PRIVATE frequalizer_binary)