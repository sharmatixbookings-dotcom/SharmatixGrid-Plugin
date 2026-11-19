#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PsyLookAndFeel.h"

//==============================================================================
/** Boton con borde luminoso (neon). */
class GlowButton : public juce::Button
{
public:
    GlowButton (const juce::String& text, juce::Colour neon, bool isLarge = false);

    void setPulse (float newPulse) { pulse = newPulse; repaint(); }
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    juce::Colour colour;
    float pulse = 0.0f;
    bool large = false;
};

//==============================================================================
/** Zona desde la que se arrastra el .mid al DAW. */
class DragPad : public juce::Component
{
public:
    DragPad();

    std::function<void()> onDragStart;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    bool dragging = false;
    bool hover = false;
};

//==============================================================================
/** Cuadricula 16 steps x notas de la escala. Click / arrastre para pintar o borrar bloques. */
class GridComponent : public juce::Component
{
public:
    explicit GridComponent (SharmatixGridAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    static constexpr int labelWidth = 46;

    juce::Rectangle<float> getGridArea() const;
    void paintAt (const juce::MouseEvent&, bool isFirstClick);

    SharmatixGridAudioProcessor& processor;
    bool paintAdds = true;
    int lastStep = -1, lastPitch = -1;
};

//==============================================================================
class SharmatixGridAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          public juce::FileDragAndDropTarget,
                                          private juce::Timer
{
public:
    explicit SharmatixGridAudioProcessorEditor (SharmatixGridAudioProcessor&);
    ~SharmatixGridAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Arrastrar un .mid DENTRO del plugin para importarlo
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Caption { juce::Rectangle<int> area; juce::String text; };

    void timerCallback() override;
    void renderBackground();
    void setStatus (const juce::String& text);
    void configureIncDec (juce::Slider&);

    SharmatixGridAudioProcessor& processor;
    PsyLookAndFeel lnf;

    GridComponent grid;
    juce::ComboBox scaleBox, rootBox;
    juce::Slider octaveSlider, rangeSlider, bpmSlider, densitySlider;
    juce::ToggleButton playButton { "MIDI OUT" };
    juce::Label stepsLabel, status;
    GlowButton generateButton { "GENERATE", psy::magenta, true };
    GlowButton clearButton    { "CLEAR",    psy::cyan };
    GlowButton saveButton     { "SAVE .MID", psy::violet };
    DragPad dragPad;

    std::unique_ptr<ComboBoxAttachment> scaleAttach, rootAttach;
    std::unique_ptr<SliderAttachment>   octaveAttach, rangeAttach, bpmAttach, densityAttach;
    std::unique_ptr<ButtonAttachment>   playAttach;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image background;
    std::vector<Caption> captions;
    bool  dropHighlight = false;
    float phase = 0.0f;
    int   lastVersion = -1, lastStepKey = -2, statusTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharmatixGridAudioProcessorEditor)
};
