cmake_minimum_required(VERSION 3.13.0)
project(frequalizer VERSION 1.1.0)
add_subdirectory(External/JUCE)

# check which formats we want to build
set(FORMATS "VST3")
if (AAX_PATH)
    list(APPEND FORMATS "AAX")
    juce_set_aax_sdk_path(${AAX_PATH})
endif()

if (VST2_PATH)
    list(APPEND FORMATS "VST2")
    juce_set_vst2_sdk_path(${VST2_PATH})
endif()

if (APPLE)
    list(APPEND FORMATS "AU")
endif()

# add the plugin targets
juce_add_plugin(frequalizer
                VERSION "1.1.0"
                COMPANY_NAME "foleysfinest"
                PLUGIN_MANUFACTURER_CODE "FFAU"
                PLUGIN_CODE "FqOS"
                FORMATS ${FORMATS}
                VST3_CATEGORIES "Fx" "Analyzer" "EQ"
                AAX_CATEGORY "AAX_ePlugInCategory_EQ"
                AU_MAIN_TYPE "kAudioUnitType_Effect"
                COMPANY_WEBSITE "https://foleysfinest.com"
                COMPANY_EMAIL "info@foleysfinest.com"
                BUNDLE_ID "com.foleysfinest.frequalizer"
                PLUGIN_NAME "Frequalizer Free"
                PRODUCT_NAME "Frequalizer")

# add the sources
add_subdirectory(Resources)
add_subdirectory(Source)

# add required flags
target_link_libraries(frequalizer PRIVATE juce::juce_recommended_warning_flags juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
target_link_libraries(frequalizer PRIVATE juce::juce_opengl juce::juce_dsp juce::juce_audio_utils)
target_compile_definitions(frequalizer 
    PUBLIC 
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0)

# setup the copying to the output folder
if (APPLE)
    set(COPY_FOLDER ${CMAKE_SOURCE_DIR}/Builds/MacOSX)
elseif(WIN32)
    set(COPY_FOLDER ${CMAKE_SOURCE_DIR}/Builds/VisualStudio2015)
endif()

foreach(FORMAT ${FORMATS})
    get_target_property(ARTEFACTS_DIR frequalizer_${FORMAT} LIBRARY_OUTPUT_DIRECTORY)
    add_custom_command(TARGET frequalizer_${FORMAT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARTEFACTS_DIR} ${COPY_FOLDER})
endforeach()