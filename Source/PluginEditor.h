#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PsyLookAndFeel.h"

class SharmatixMelodiesAudioEditor : public juce::AudioProcessorEditor
{
public:
    explicit SharmatixMelodiesAudioEditor (SharmatixMelodiesAudioProcessor&);
    ~SharmatixMelodiesAudioEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SharmatixMelodiesAudioProcessor& audioProcessor;
    PsyLookAndFeel psyLookAndFeel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::ComboBox presetBox;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };

    juce::ComboBox scaleBox;
    juce::ComboBox rootBox;
    juce::ComboBox modeBox;

    juce::Slider octaveSlider;
    juce::Slider rangeSlider;
    juce::Slider lengthSlider;
    juce::Slider densitySlider;
    juce::Slider gateSlider;

    juce::TextButton generateButton { "GENERATE" };
    juce::TextButton clearButton { "CLEAR" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboAttachment> scaleAttachment;
    std::unique_ptr<ComboAttachment> rootAttachment;
    std::unique_ptr<ComboAttachment> modeAttachment;

    std::unique_ptr<SliderAttachment> octaveAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment;
    std::unique_ptr<SliderAttachment> lengthAttachment;
    std::unique_ptr<SliderAttachment> densityAttachment;
    std::unique_ptr<SliderAttachment> gateAttachment;

    void refreshPresetList();
    void configureSlider (juce::Slider& s, const juce::String& suffix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharmatixMelodiesAudioEditor)
};
