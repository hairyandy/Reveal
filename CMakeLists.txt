cmake_minimum_required(VERSION 3.22)

project(Reveal VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Locate JUCE
#
# Option A (recommended): pass your local clone on the command line:
#   cmake -B build -G Xcode -DJUCE_PATH=/path/to/JUCE
#
# Option B: let CMake fetch JUCE 7.0.12 automatically via FetchContent.
#
# For AU validation you will also need the CoreAudio SDK headers that
# ship with the Xcode Command Line Tools – no extra steps required on
# a standard macOS dev machine.
# ============================================================
if(DEFINED JUCE_PATH)
    message(STATUS "Using local JUCE at: ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(STATUS "JUCE_PATH not set – fetching JUCE 7.0.12 via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ============================================================
# Plugin target
# ============================================================
juce_add_plugin(Reveal
    COMPANY_NAME            "YourStudio"
    COMPANY_WEBSITE         "https://yourstudio.com"
    BUNDLE_ID               "com.yourstudio.reveal"

    # Four-character codes – must be unique per plugin / manufacturer.
    # Change these if you plan to distribute.
    PLUGIN_MANUFACTURER_CODE YrSt
    PLUGIN_CODE              Rvl1

    FORMATS                 VST3 AU

    PRODUCT_NAME            "Reveal"
    PLUGIN_NAME             "Reveal"
    DESCRIPTION             "Clean boost pedal plug-in"

    # Audio I/O
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # AU-specific metadata
    AU_MAIN_TYPE            kAudioUnitType_Effect
    AU_EXPORT_PREFIX        RevealAU

    # VST3 category (shown in DAW browser)
    VST3_CATEGORIES         "Fx"
)

# ============================================================
# Source files
# ============================================================
target_sources(Reveal PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# ============================================================
# Compile definitions
# ============================================================
target_compile_definitions(Reveal
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Set to 1 and supply a splash image if you need to show one
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(Reveal
    PRIVATE
        juce::juce_audio_utils        # AudioProcessor, AudioProcessorEditor, etc.
        juce::juce_audio_processors   # APVTS, plugin formats
        juce::juce_dsp                # IIR filters, Gain, ProcessorChain
        juce::juce_gui_basics         # Component, Graphics, Slider, Label
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================================
# Xcode-specific tweaks
# ============================================================
if(APPLE)
    # Required for AU validation / codesigning in local development.
    # Remove or replace with your real Team ID for distribution builds.
    set_target_properties(Reveal PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    )
endif()
