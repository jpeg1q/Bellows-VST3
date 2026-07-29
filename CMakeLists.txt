cmake_minimum_required(VERSION 3.22)

project(Bellows VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BELLOWS_BUILD_PLUGIN "Build the JUCE plug-in and standalone app" ON)
option(BELLOWS_BUILD_TESTS "Build the dependency-free DSP smoke tests" ON)
option(BELLOWS_BUILD_DEMO "Build the dependency-free WAV renderer" ON)

if(BELLOWS_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(Bellows
        COMPANY_NAME "jpeg1q Audio"
        BUNDLE_ID "com.jpeg1q.bellows"
        PLUGIN_MANUFACTURER_CODE JpgQ
        PLUGIN_CODE Blws
        FORMATS VST3 Standalone
        PRODUCT_NAME "Bellows"
        DESCRIPTION "Expressive physically-inspired accordion instrument"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Synth
    )

    juce_generate_juce_header(Bellows)

    target_sources(Bellows PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DSP/AccordionVoice.cpp
        Source/DSP/AccordionVoice.h
        Source/DSP/ParameterIDs.h
        Source/DSP/ReedOscillator.h
    )

    target_compile_definitions(Bellows PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(Bellows PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    )

    if(MSVC)
        target_compile_options(Bellows PRIVATE /W4 /permissive-)
    else()
        target_compile_options(Bellows PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(BELLOWS_BUILD_TESTS)
    enable_testing()
    add_executable(BellowsCoreTests Tests/CoreDspSmoke.cpp)
    target_include_directories(BellowsCoreTests PRIVATE Source)
    if(MSVC)
        target_compile_options(BellowsCoreTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(BellowsCoreTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME BellowsCoreSmoke COMMAND BellowsCoreTests)
endif()

if(BELLOWS_BUILD_DEMO)
    add_executable(BellowsRenderDemo tools/render_demo.cpp)
    target_include_directories(BellowsRenderDemo PRIVATE Source)
    if(MSVC)
        target_compile_options(BellowsRenderDemo PRIVATE /W4 /permissive-)
    else()
        target_compile_options(BellowsRenderDemo PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
