#include "PluginProcessor.h"
#include "PluginEditor.h"

SharmatixMelodiesAudioEditor::SharmatixMelodiesAudioEditor (SharmatixMelodiesAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&psyLookAndFeel);
    setSize (860, 540);

    titleLabel.setText ("SHARMATIX MELODIES", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (26.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff4f0ff));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("MIDI MELODY & ARP ENGINE", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (12.0f, juce::Font::plain));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff22d3ee));
    addAndMakeVisible (subtitleLabel);

    refreshPresetList();
    presetBox.onChange = [this]
    {
        int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0)
        {
            audioProcessor.setCurrentProgram (idx);
            repaint();
        }
    };
    addAndMakeVisible (presetBox);

    prevPresetButton.onClick = [this]
    {
        int nextIdx = (audioProcessor.getCurrentProgram() - 1 + audioProcessor.getNumPrograms())
                    % juce::jmax (1, audioProcessor.getNumPrograms());
        audioProcessor.setCurrentProgram (nextIdx);
        presetBox.setSelectedItemIndex (nextIdx, juce::dontSendNotification);
        repaint();
    };
    addAndMakeVisible (prevPresetButton);

    nextPresetButton.onClick = [this]
    {
        int nextIdx = (audioProcessor.getCurrentProgram() + 1)
                    % juce::jmax (1, audioProcessor.getNumPrograms());
        audioProcessor.setCurrentProgram (nextIdx);
        presetBox.setSelectedItemIndex (nextIdx, juce::dontSendNotification);
        repaint();
    };
    addAndMakeVisible (nextPresetButton);

    savePresetButton.onClick = [this]
    {
        audioProcessor.savePresetToFile();
        refreshPresetList();
    };
    addAndMakeVisible (savePresetButton);

    loadPresetButton.onClick = [this]
    {
        audioProcessor.loadPresetFromFile();
        repaint();
    };
    addAndMakeVisible (loadPresetButton);

    scaleBox.addItemList ({ "Phrygian Dom", "Hirajoshi", "Nat Minor", "Mystic Minor", "In Sen", "Harmonic Pent" }, 1);
    rootBox.addItemList  ({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    modeBox.addItemList  ({ "Psy Step", "Arp Up", "Wave", "Octave Jump" }, 1);

    addAndMakeVisible (scaleBox);
    addAndMakeVisible (rootBox);
    addAndMakeVisible (modeBox);

    scaleAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "scale", scaleBox);
    rootAttachment  = std::make_unique<ComboAttachment> (audioProcessor.apvts, "root", rootBox);
    modeAttachment  = std::make_unique<ComboAttachment> (audioProcessor.apvts, "arpMode", modeBox);

    configureSlider (octaveSlider, "");
    configureSlider (rangeSlider, " oct");
    configureSlider (lengthSlider, " stp");
    configureSlider (densitySlider, "%");
    configureSlider (gateSlider, "");

    octaveAttachment  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "octave", octaveSlider);
    rangeAttachment   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "range", rangeSlider);
    lengthAttachment  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "length", lengthSlider);
    densityAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "density", densitySlider);
    gateAttachment    = std::make_unique<SliderAttachment> (audioProcessor.apvts, "gate", gateSlider);

    generateButton.onClick = [this]
    {
        audioProcessor.generatePattern();
        repaint();
    };
    addAndMakeVisible (generateButton);

    clearButton.onClick = [this]
    {
        for (auto& row : audioProcessor.stepGrid)
            row.fill (false);
        repaint();
    };
    addAndMakeVisible (clearButton);
}

SharmatixMelodiesAudioEditor::~SharmatixMelodiesAudioEditor()
{
    setLookAndFeel (nullptr);
}

void SharmatixMelodiesAudioEditor::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetBox.addItem (audioProcessor.getProgramName (i), i + 1);

    presetBox.setSelectedItemIndex (audioProcessor.getCurrentProgram(), juce::dontSendNotification);
}

void SharmatixMelodiesAudioEditor::configureSlider (juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
    if (suffix.isNotEmpty())
        s.setTextValueSuffix (suffix);
    addAndMakeVisible (s);
}

void SharmatixMelodiesAudioEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0b0914));

    auto topBar = juce::Rectangle<int> (16, 12, getWidth() - 32, 66);
    g.setColour (juce::Colour (0xff140f24));
    g.fillRoundedRectangle (topBar.toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff5b21b6).withAlpha (0.6f));
    g.drawRoundedRectangle (topBar.toFloat(), 8.0f, 1.0f);

    auto gridArea = juce::Rectangle<int> (16, 132, getWidth() - 32, 342);
    g.setColour (juce::Colour (0xff100c1d));
    g.fillRoundedRectangle (gridArea.toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff4c1d95).withAlpha (0.7f));
    g.drawRoundedRectangle (gridArea.toFloat(), 8.0f, 1.0f);

    const int cols = SharmatixMelodiesAudioProcessor::maxSteps;
    const int rows = SharmatixMelodiesAudioProcessor::maxPitchRows;
    const float cellW = (float) gridArea.getWidth() / (float) cols;
    const float cellH = (float) gridArea.getHeight() / (float) rows;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            juce::Rectangle<float> cell ((float) gridArea.getX() + c * cellW,
                                         (float) gridArea.getY() + r * cellH,
                                         cellW, cellH);

            bool active = audioProcessor.stepGrid[(size_t) r][(size_t) c];
            if (active)
            {
                g.setColour (juce::Colour (0xff22d3ee).withAlpha (0.85f));
                g.fillRoundedRectangle (cell.reduced (1.5f), 3.0f);
            }
            else
            {
                g.setColour (juce::Colour (0xff241b3d).withAlpha (0.35f));
                g.drawRect (cell, 0.5f);
            }
        }
    }
}

void SharmatixMelodiesAudioEditor::resized()
{
    titleLabel.setBounds (28, 18, 320, 32);
    subtitleLabel.setBounds (30, 48, 260, 20);

    prevPresetButton.setBounds (370, 26, 30, 32);
    presetBox.setBounds        (404, 26, 210, 32);
    nextPresetButton.setBounds (618, 26, 30, 32);
    savePresetButton.setBounds (660, 26, 80, 32);
    loadPresetButton.setBounds (746, 26, 80, 32);

    scaleBox.setBounds (16, 88, 150, 32);
    rootBox.setBounds  (174, 88, 84, 32);
    modeBox.setBounds  (266, 88, 130, 32);

    octaveSlider.setBounds  (406, 90, 110, 28);
    lengthSlider.setBounds  (522, 90, 115, 28);
    densitySlider.setBounds (643, 90, 115, 28);
    gateSlider.setBounds    (764, 90, 80, 28);

    clearButton.setBounds    (16, 486, 110, 36);
    generateButton.setBounds (134, 486, 160, 36);
    rangeSlider.setBounds    (306, 490, 150, 28);
}
