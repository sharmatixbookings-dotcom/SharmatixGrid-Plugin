#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

//==============================================================================
// Paleta y utilidades del tema psicodelico oscuro
namespace psy
{
    inline const juce::Colour ink     { 0xff05030c };
    inline const juce::Colour panel   { 0xff160a2e };
    inline const juce::Colour cyan    { 0xff00e5ff };
    inline const juce::Colour violet  { 0xff9b30ff };
    inline const juce::Colour magenta { 0xffff2bd6 };
    inline const juce::Colour lime    { 0xffc6ff00 };
    inline const juce::Colour yellow  { 0xffffd400 };

    /** Centraliza la creacion de fuentes (API de fuentes de JUCE 8). */
    inline juce::Font makeFont (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }

    /** Gradiente de color grave -> agudo: cian, violeta, magenta, amarillo. */
    inline juce::Colour rowColour (float t)
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        if (t < 0.4f)  return cyan.interpolatedWith (violet, t / 0.4f);
        if (t < 0.75f) return violet.interpolatedWith (magenta, (t - 0.4f) / 0.35f);
        return magenta.interpolatedWith (yellow, (t - 0.75f) / 0.25f);
    }
}

//==============================================================================
class PsyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PsyLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, psy::ink);

        setColour (juce::ComboBox::backgroundColourId, psy::panel);
        setColour (juce::ComboBox::outlineColourId,    psy::violet.withAlpha (0.7f));
        setColour (juce::ComboBox::textColourId,       juce::Colours::white);
        setColour (juce::ComboBox::arrowColourId,      psy::cyan);

        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff10071f));
        setColour (juce::PopupMenu::textColourId,                  juce::Colours::white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, psy::violet.withAlpha (0.55f));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

        setColour (juce::Slider::textBoxTextColourId,       juce::Colours::white);
        setColour (juce::Slider::textBoxBackgroundColourId, psy::panel);
        setColour (juce::Slider::textBoxOutlineColourId,    psy::violet.withAlpha (0.5f));
        setColour (juce::Slider::textBoxHighlightColourId,  psy::magenta.withAlpha (0.5f));

        setColour (juce::TextButton::buttonColourId,   psy::panel);
        setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
        setColour (juce::TextButton::textColourOnId,   juce::Colours::white);

        setColour (juce::Label::textColourId, juce::Colours::white);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return psy::makeFont (14.0f, true); }

    //--------------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float thickness = 5.0f;
        const float arcR = radius - thickness;

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff1d0f3a));
        g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);

        for (int i = 3; i >= 1; --i)   // resplandor
        {
            g.setColour (psy::magenta.withAlpha (0.10f));
            g.strokePath (value, juce::PathStrokeType (thickness + (float) i * 3.0f,
                                                       juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setGradientFill (juce::ColourGradient (psy::cyan, centre.x - radius, centre.y,
                                                 psy::magenta, centre.x + radius, centre.y, false));
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float knobR = arcR * 0.62f;
        g.setColour (juce::Colour (0xff0d0620));
        g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
        g.setColour (psy::violet.withAlpha (0.8f));
        g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);

        const juce::Point<float> tip (centre.x + std::sin (angle) * knobR * 0.9f,
                                      centre.y - std::cos (angle) * knobR * 0.9f);
        g.setColour (psy::lime);
        g.drawLine (centre.x, centre.y, tip.x, tip.y, 2.5f);
    }

    //--------------------------------------------------------------------------
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool highlighted, bool /*down*/) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (3.0f);
        const bool on = button.getToggleState();
        const auto col = on ? psy::lime : psy::violet;

        if (on)
            for (int i = 3; i >= 1; --i)
            {
                g.setColour (col.withAlpha (0.09f));
                g.drawRoundedRectangle (b.expanded ((float) i * 1.5f), 10.0f, 2.0f);
            }

        g.setColour (on ? col.withAlpha (0.20f) : psy::panel);
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (col.withAlpha (highlighted ? 1.0f : 0.85f));
        g.drawRoundedRectangle (b, 10.0f, 1.6f);

        g.setColour (juce::Colours::white.withAlpha (on ? 1.0f : 0.65f));
        g.setFont (psy::makeFont (13.0f, true));
        g.drawText (button.getButtonText(), b.toNearestInt(), juce::Justification::centred);
    }
};
