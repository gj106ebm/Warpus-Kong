cmake_minimum_required(VERSION 3.22)
project(WarpusKong VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# VST2 : Steinberg ne distribue plus le SDK VST2. Si (et seulement si) vous en possedez
# une copie sous licence, passez -DVST2_SDK_PATH=C:/chemin/vstsdk2.4
set(KS_FORMATS VST3 Standalone)
if(VST2_SDK_PATH)
    juce_set_vst2_sdk_path(${VST2_SDK_PATH})
    list(APPEND KS_FORMATS VST)
endif()

juce_add_plugin(WarpusKong
    COMPANY_NAME "SweepLab"
    PLUGIN_MANUFACTURER_CODE Swlb
    PLUGIN_CODE Wrpk
    FORMATS ${KS_FORMATS}
    PRODUCT_NAME "Warpus Kong"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(WarpusKong PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(WarpusKong PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(WarpusKong
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
