cmake_minimum_required(VERSION 3.16)
project(Patchwork VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)

add_subdirectory(JUCE)

juce_add_plugin(Patchwork
    COMPANY_NAME "Patchwork"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "Ptch"
    PLUGIN_CODE "Ptch"
    FORMATS VST3
    PRODUCT_NAME "Patchwork"
    DESCRIPTION "The Universal DJ Platform - Professional DJ plugin for any DAW"
    COMPANY_COPYRIGHT "2026 Patchwork Contributors"
    COMPANY_WEBSITE "https://patchwork.dj"
)

target_sources(Patchwork PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Modules/DeckProcessor.cpp
    Source/Modules/EffectsChain.cpp
    Source/Modules/SamplerEngine.cpp
    Source/Modules/MidiController.cpp
    Source/Modules/WaveformAnalyzer.cpp
    Source/Modules/NIDeviceActivator.cpp
    Source/UI/DeckComponent.cpp
    Source/UI/SamplerComponent.cpp
    Source/UI/EffectsComponent.cpp
    Source/UI/WaveformComponent.cpp
    Source/UI/PatchworkLookAndFeel.cpp
    Source/UI/MidiMonitorComponent.cpp
    Source/UI/NILEDEditorComponent.cpp
    Source/UI/LibraryComponent.cpp
    Source/UI/BroadcastComponent.cpp
    Source/Modules/MixRecorder.cpp
    Source/Modules/BroadcastEngine.cpp
    Source/Modules/TimecodeAnalyzer.cpp
    Source/UI/SpinningPlatterComponent.cpp
    Source/UI/MixerStripComponent.cpp
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(SOUNDTOUCH REQUIRED soundtouch)
pkg_check_modules(SHOUT      REQUIRED shout)
pkg_check_modules(LAME       REQUIRED lame)
pkg_check_modules(LIBUSB     REQUIRED libusb-1.0)

target_include_directories(Patchwork PRIVATE ${GTK3_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS} ${SOUNDTOUCH_INCLUDE_DIRS} ${SHOUT_INCLUDE_DIRS} ${LAME_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(Patchwork PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_core
    juce::juce_dsp
    juce::juce_audio_formats
    juce::juce_osc
    ${GTK3_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    curl
    ${SOUNDTOUCH_LIBRARIES}
    ${SHOUT_LIBRARIES}
    ${LAME_LIBRARIES}
    mp3lame
    ${LIBUSB_LIBRARIES}
)

target_compile_options(Patchwork PRIVATE
    ${GTK3_CFLAGS_OTHER}
    -Wall -Wextra -Wno-error=unused-parameter
)

target_compile_definitions(Patchwork PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)
