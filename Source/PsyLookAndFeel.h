#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class PsyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PsyLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0b0914));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff19142b));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xfff4f0ff));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff7c3aed));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff161126));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xfff4f0ff));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff6d28d9));
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff231b3d));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff7c3aed));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xfff4f0ff));
        setColour (juce::Slider::thumbColourId, juce::Colour (0xff22d3ee));
        setColour (juce::Slider::trackColourId, juce::Colour (0xff4c1d95));
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff19142b));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        auto base = button.getToggleState() ? findColour (juce::TextButton::buttonOnColourId) : backgroundColour;

        if (shouldDrawButtonAsDown)
            base = base.brighter (0.25f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter (0.12f);

        g.setColour (base);
        g.fillRoundedRectangle (bounds, 6.0f);

        g.setColour (juce::Colour (0xff8b5cf6).withAlpha (0.65f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
    }
};
