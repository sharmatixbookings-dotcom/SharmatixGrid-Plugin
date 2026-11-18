cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# Binarios universales en macOS (Apple Silicon + Intel).
# Estas variables deben definirse ANTES de project().
# ---------------------------------------------------------------------------
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# Runtime estatico en Windows: el plugin no exige instalar el VC++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(SharmatixGrid VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SHARMATIX_COPY_AFTER_BUILD
       "Copiar los plugins a las carpetas del sistema despues de compilar" OFF)

# ---------------------------------------------------------------------------
# JUCE (se descarga automaticamente la primera vez que se configura)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

# AU solo existe en macOS.
if(APPLE)
    set(SHARMATIX_FORMATS VST3 AU Standalone)
else()
    set(SHARMATIX_FORMATS VST3 Standalone)
endif()

# ---------------------------------------------------------------------------
# Plugin
# Se declara como instrumento (IS_SYNTH) con salida MIDI: asi Ableton Live y
# FL Studio lo cargan sin problemas. No genera audio, solo MIDI.
# ---------------------------------------------------------------------------
juce_add_plugin(SharmatixGrid
    COMPANY_NAME                Sharmatix
    PRODUCT_NAME                "SharmatixGrid"
    PLUGIN_MANUFACTURER_CODE    Shrm
    PLUGIN_CODE                 Sgrd
    FORMATS                     ${SHARMATIX_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${SHARMATIX_COPY_AFTER_BUILD})

target_sources(SharmatixGrid PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PsyLookAndFeel.h)

target_compile_definitions(SharmatixGrid PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SharmatixGrid
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
