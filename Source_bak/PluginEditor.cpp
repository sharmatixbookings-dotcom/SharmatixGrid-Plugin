#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>

//==============================================================================
// Fondo psicodelico (se dibuja una sola vez a una imagen y se reutiliza)
namespace
{
    struct BranchSet
    {
        std::array<juce::Path, 8> paths;
        std::vector<juce::Point<float>> tips;
    };

    void growBranch (BranchSet& set, juce::Point<float> from, float angle, float len, int depth, juce::Random& rnd)
    {
        if (depth >= 8 || len < 6.0f)
        {
            set.tips.push_back (from);
            return;
        }

        const juce::Point<float> to { from.x + std::cos (angle) * len, from.y + std::sin (angle) * len };
        const float bend = (rnd.nextFloat() - 0.5f) * len * 0.5f;
        const juce::Point<float> ctrl { (from.x + to.x) * 0.5f - std::sin (angle) * bend,
                                        (from.y + to.y) * 0.5f + std::cos (angle) * bend };

        auto& path = set.paths[(size_t) depth];
        path.startNewSubPath (from);
        path.quadraticTo (ctrl, to);

        const int kids = depth < 2 ? 3 : 2;
        for (int i = 0; i < kids; ++i)
        {
            const float spread = 0.35f + rnd.nextFloat() * 0.45f;
            const float a = angle + (kids == 3 ? (float) (i - 1) * spread : (i == 0 ? -spread : spread));
            growBranch (set, to, a, len * (0.72f + rnd.nextFloat() * 0.1f), depth + 1, rnd);
        }
    }

    void drawPsychedelicBackground (juce::Graphics& g, int w, int h)
    {
        const float W = (float) w, H = (float) h;

        // Degradado base: negro -> purpura profundo
        juce::ColourGradient base (juce::Colour (0xff04020a), 0.0f, 0.0f,
                                   juce::Colour (0xff2a0b45), 0.0f, H, false);
        base.addColour (0.45, juce::Colour (0xff150630));
        g.setGradientFill (base);
        g.fillAll();

        // Resplandores neon sutiles
        auto glow = [&] (juce::Colour c, float cx, float cy, float r, float alpha)
        {
            g.setGradientFill (juce::ColourGradient (c.withAlpha (alpha), cx, cy,
                                                     c.withAlpha (0.0f), cx + r, cy, true));
            g.fillRect (0.0f, 0.0f, W, H);
        };
        glow (psy::cyan,    W * 0.10f, H * 0.10f, W * 0.55f, 0.17f);
        glow (psy::magenta, W * 0.92f, H * 0.90f, W * 0.60f, 0.20f);
        glow (psy::violet,  W * 0.50f, H * 0.50f, W * 0.55f, 0.16f);

        // Arbol de la vida (ramas recursivas, tenues)
        {
            juce::Random rnd (2024);
            BranchSet set;
            growBranch (set, { W * 0.5f, H + 10.0f }, -juce::MathConstants<float>::halfPi, H * 0.20f, 0, rnd);

            for (int d = 0; d < 8; ++d)
            {
                const float t = (float) d / 7.0f;
                g.setColour (psy::violet.interpolatedWith (psy::magenta, t).withAlpha (0.24f));
                g.strokePath (set.paths[(size_t) d],
                              juce::PathStrokeType ((float) (8 - d) * 0.9f + 0.4f,
                                                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            for (size_t i = 0; i < set.tips.size(); ++i)
            {
                const auto c = (i % 3 == 0) ? psy::lime : (i % 3 == 1 ? psy::yellow : psy::cyan);
                const auto p = set.tips[i];
                g.setColour (c.withAlpha (0.10f));
                g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
                g.setColour (c.withAlpha (0.45f));
                g.fillEllipse (p.x - 1.5f, p.y - 1.5f, 3.0f, 3.0f);
            }
        }

        // Mandala / geometria sagrada (centro inferior)
        {
            const float cx = W * 0.5f, cy = H * 0.84f;
            const float R = juce::jmin (W, H) * 0.62f;

            for (int i = 1; i <= 6; ++i)
            {
                g.setColour (psy::cyan.withAlpha (0.05f + 0.01f * (float) i));
                const float r = R * (float) i / 6.0f;
                g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
            }

            auto petals = [&] (int count, float distance, float len, float wid, juce::Colour c, float alpha)
            {
                for (int k = 0; k < count; ++k)
                {
                    juce::Path p;
                    p.addEllipse (cx - wid * 0.5f, cy - distance - len * 0.5f, wid, len);
                    p.applyTransform (juce::AffineTransform::rotation (
                        juce::MathConstants<float>::twoPi * (float) k / (float) count, cx, cy));
                    g.setColour (c.withAlpha (alpha));
                    g.strokePath (p, juce::PathStrokeType (1.2f));
                }
            };
            petals (16, R * 0.55f, R * 0.30f, R * 0.12f, psy::magenta, 0.14f);
            petals (24, R * 0.80f, R * 0.22f, R * 0.07f, psy::cyan,    0.12f);
            petals (12, R * 0.28f, R * 0.22f, R * 0.10f, psy::lime,    0.10f);

            // tercer ojo
            juce::Path eye;
            const float ew = R * 0.14f, eh = R * 0.07f;
            eye.startNewSubPath (cx - ew, cy);
            eye.quadraticTo (cx, cy - eh * 2.0f, cx + ew, cy);
            eye.quadraticTo (cx, cy + eh * 2.0f, cx - ew, cy);
            eye.closeSubPath();
            g.setColour (psy::yellow.withAlpha (0.28f));
            g.strokePath (eye, juce::PathStrokeType (1.6f));
            g.setColour (psy::cyan.withAlpha (0.35f));
            g.fillEllipse (cx - eh * 0.7f, cy - eh * 0.7f, eh * 1.4f, eh * 1.4f);
        }

        // Estrellas
        {
            juce::Random rnd (7);
            for (int i = 0; i < 170; ++i)
            {
                const float x = rnd.nextFloat() * W, y = rnd.nextFloat() * H;
                const float s = 0.6f + rnd.nextFloat() * 1.8f;
                const int pick = rnd.nextInt (3);
                const auto c = pick == 0 ? juce::Colours::white : (pick == 1 ? psy::yellow : psy::cyan);
                g.setColour (c.withAlpha (0.15f + rnd.nextFloat() * 0.45f));
                g.fillEllipse (x, y, s, s);
            }
        }

        // Vineta
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                                 juce::Colours::black.withAlpha (0.65f), 0.0f, 0.0f, true));
        g.fillAll();
    }
}

//==============================================================================
// GlowButton
GlowButton::GlowButton (const juce::String& text, juce::Colour neon, bool isLarge)
    : juce::Button (text), colour (neon), large (isLarge)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void GlowButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto b = getLocalBounds().toFloat().reduced (large ? 10.0f : 8.0f);
    const float corner = large ? 16.0f : 12.0f;
    const float boost = (isMouseOver ? 1.5f : 1.0f) * (isButtonDown ? 1.6f : 1.0f);
    const float p = large ? pulse : 0.0f;

    for (int i = 6; i >= 1; --i)
    {
        g.setColour (colour.withAlpha (juce::jmin (0.5f, (0.035f + 0.025f * p) * boost)));
        g.drawRoundedRectangle (b.expanded ((float) i * 1.6f), corner + (float) i, 2.0f);
    }

    g.setGradientFill (juce::ColourGradient (colour.withAlpha (isButtonDown ? 0.40f : 0.24f), b.getX(), b.getY(),
                                             colour.darker (0.8f).withAlpha (0.10f), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, corner);

    g.setColour (colour.brighter (0.25f));
    g.drawRoundedRectangle (b, corner, isButtonDown ? 3.0f : 2.2f);

    g.setColour (juce::Colours::white);
    g.setFont (psy::makeFont (large ? 22.0f : 14.0f, true));
    g.drawText (getButtonText(), b.toNearestInt(), juce::Justification::centred);
}

//==============================================================================
// DragPad
DragPad::DragPad()
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void DragPad::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (8.0f);
    const auto col = psy::lime;

    for (int i = 5; i >= 1; --i)
    {
        g.setColour (col.withAlpha (hover ? 0.07f : 0.04f));
        g.drawRoundedRectangle (b.expanded ((float) i * 1.5f), 12.0f + (float) i, 2.0f);
    }

    g.setColour (col.withAlpha (hover ? 0.22f : 0.12f));
    g.fillRoundedRectangle (b, 12.0f);
    g.setColour (col.withAlpha (0.9f));
    g.drawRoundedRectangle (b, 12.0f, 2.0f);

    // Asa de agarre (2 x 3 puntos)
    g.setColour (col);
    const float gx = b.getX() + 16.0f, gy = b.getCentreY() - 9.0f;
    for (int cx = 0; cx < 2; ++cx)
        for (int cy = 0; cy < 3; ++cy)
            g.fillEllipse (gx + (float) cx * 7.0f, gy + (float) cy * 8.0f, 3.5f, 3.5f);

    g.setColour (juce::Colours::white);
    g.setFont (psy::makeFont (17.0f, true));
    g.drawText ("DRAG MIDI", b.withTrimmedLeft (36.0f).toNearestInt(), juce::Justification::centred);
}

void DragPad::mouseDown (const juce::MouseEvent&)
{
    dragging = false;
}

void DragPad::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging && e.getDistanceFromDragStart() > 6)
    {
        dragging = true;
        if (onDragStart)
            onDragStart();
    }
}

void DragPad::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

//==============================================================================
// GridComponent
juce::Rectangle<float> GridComponent::getGridArea() const
{
    return getLocalBounds().toFloat().withTrimmedLeft ((float) labelWidth).reduced (4.0f);
}

void GridComponent::paint (juce::Graphics& g)
{
    const auto pitches = processor.getPitchSet();
    const int rows = (int) pitches.size();
    if (rows == 0)
        return;

    const auto area = getGridArea();
    const float cw = area.getWidth() / (float) SharmatixGridAudioProcessor::numSteps;
    const float ch = area.getHeight() / (float) rows;
    const int rootPc = pitches.front() % 12;
    const double cur = processor.getCurrentStep();
    const auto notes = processor.getClipCopy();

    // Panel
    g.setColour (juce::Colour (0xff0a0614).withAlpha (0.74f));
    g.fillRoundedRectangle (area.expanded (4.0f), 12.0f);
    g.setColour (psy::violet.withAlpha (0.40f));
    g.drawRoundedRectangle (area.expanded (4.0f), 12.0f, 1.5f);

    // Celdas vacias + etiquetas de nota
    for (int r = 0; r < rows; ++r)
    {
        const int pitchIdx = rows - 1 - r;
        const float y = area.getY() + (float) r * ch;
        const bool isRoot = (pitches[(size_t) pitchIdx] % 12) == rootPc;

        g.setColour (isRoot ? psy::cyan : juce::Colours::white.withAlpha (0.45f));
        g.setFont (psy::makeFont (juce::jmin (12.0f, ch * 0.8f), isRoot));
        g.drawText (juce::MidiMessage::getMidiNoteName (pitches[(size_t) pitchIdx], true, true, 3),
                    juce::Rectangle<float> (0.0f, y, (float) labelWidth - 6.0f, ch).toNearestInt(),
                    juce::Justification::centredRight);

        for (int s = 0; s < SharmatixGridAudioProcessor::numSteps; ++s)
        {
            const juce::Rectangle<float> cell (area.getX() + (float) s * cw, y, cw, ch);
            const bool beat = (s % 4) == 0;
            g.setColour (isRoot ? psy::violet.withAlpha (0.16f)
                                : juce::Colours::white.withAlpha (beat ? 0.06f : 0.03f));
            g.fillRoundedRectangle (cell.reduced (1.5f), 3.0f);
        }
    }

    // Separadores de tiempo
    g.setColour (psy::cyan.withAlpha (0.14f));
    for (int s = 0; s <= SharmatixGridAudioProcessor::numSteps; s += 4)
    {
        const float x = area.getX() + (float) s * cw;
        g.drawLine (x, area.getY(), x, area.getBottom(), 1.0f);
    }

    // Columna del playhead
    if (cur >= 0.0)
    {
        const juce::Rectangle<float> col (area.getX() + (float) std::floor (cur) * cw, area.getY(), cw, area.getHeight());
        g.setGradientFill (juce::ColourGradient (psy::cyan.withAlpha (0.22f), col.getX(), col.getY(),
                                                 psy::magenta.withAlpha (0.12f), col.getX(), col.getBottom(), false));
        g.fillRect (col);
    }

    // Bloques
    for (const auto& n : notes)
    {
        const auto it = std::lower_bound (pitches.begin(), pitches.end(), n.pitch);
        if (it == pitches.end() || *it != n.pitch)
            continue;

        const int idx = (int) std::distance (pitches.begin(), it);
        const int r = rows - 1 - idx;
        const auto nr = juce::Rectangle<float> (area.getX() + (float) n.startStep * cw,
                                                area.getY() + (float) r * ch,
                                                (float) n.lengthSteps * cw, ch).reduced (1.5f);

        const bool active = cur >= n.startStep && cur < n.startStep + n.lengthSteps;
        const auto colour = psy::rowColour (rows > 1 ? (float) idx / (float) (rows - 1) : 0.0f);
        const float bright = 0.6f + 0.4f * (float) n.velocity / 127.0f;

        for (int i = 3; i >= 1; --i)
        {
            g.setColour (colour.withAlpha (active ? 0.18f : 0.08f));
            g.fillRoundedRectangle (nr.expanded ((float) i * 2.0f), 5.0f + (float) i);
        }

        g.setGradientFill (juce::ColourGradient (colour.brighter (active ? 0.7f : 0.25f).withMultipliedBrightness (bright),
                                                 nr.getX(), nr.getY(),
                                                 colour.darker (0.5f).withMultipliedBrightness (bright),
                                                 nr.getX(), nr.getBottom(), false));
        g.fillRoundedRectangle (nr, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (active ? 0.95f : 0.55f));
        g.drawRoundedRectangle (nr, 4.0f, 1.2f);
    }
}

void GridComponent::mouseDown (const juce::MouseEvent& e)
{
    paintAt (e, true);
}

void GridComponent::mouseDrag (const juce::MouseEvent& e)
{
    paintAt (e, false);
}

void GridComponent::paintAt (const juce::MouseEvent& e, bool isFirstClick)
{
    const auto pitches = processor.getPitchSet();
    const int rows = (int) pitches.size();
    if (rows == 0)
        return;

    const auto area = getGridArea();
    const auto pos = e.position;
    if (! area.contains (pos))
        return;

    const float cw = area.getWidth() / (float) SharmatixGridAudioProcessor::numSteps;
    const float ch = area.getHeight() / (float) rows;

    const int step = juce::jlimit (0, SharmatixGridAudioProcessor::numSteps - 1, (int) ((pos.x - area.getX()) / cw));
    const int row  = juce::jlimit (0, rows - 1, (int) ((pos.y - area.getY()) / ch));
    const int pitch = pitches[(size_t) (rows - 1 - row)];

    if (isFirstClick)
    {
        // El primer click decide si este gesto pinta o borra
        paintAdds = ! processor.hasNoteAt (step, pitch);
        lastStep = -1;
        lastPitch = -1;
    }

    if (step == lastStep && pitch == lastPitch)
        return;

    lastStep = step;
    lastPitch = pitch;
    processor.setNoteAt (step, pitch, paintAdds);
}

//==============================================================================
// Editor
SharmatixGridAudioProcessorEditor::SharmatixGridAudioProcessorEditor (SharmatixGridAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), grid (p)
{
    setLookAndFeel (&lnf);

    scaleBox.addItemList (SharmatixGridAudioProcessor::getScaleNames(), 1);
    rootBox.addItemList (SharmatixGridAudioProcessor::getRootNames(), 1);

    configureIncDec (octaveSlider);
    configureIncDec (rangeSlider);
    configureIncDec (bpmSlider);

    densitySlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    densitySlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);

    stepsLabel.setText ("16 STEPS", juce::dontSendNotification);
    stepsLabel.setJustificationType (juce::Justification::centred);
    stepsLabel.setColour (juce::Label::textColourId, psy::cyan);
    stepsLabel.setFont (psy::makeFont (15.0f, true));

    status.setJustificationType (juce::Justification::centred);
    status.setColour (juce::Label::textColourId, psy::lime);
    status.setFont (psy::makeFont (13.0f, true));

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &grid, &scaleBox, &rootBox, &octaveSlider, &rangeSlider, &bpmSlider, &densitySlider,
             &playButton, &stepsLabel, &status, &generateButton, &clearButton, &saveButton, &dragPad })
        addAndMakeVisible (c);

    auto& apvts = processor.apvts;
    scaleAttach   = std::make_unique<ComboBoxAttachment> (apvts, "scale", scaleBox);
    rootAttach    = std::make_unique<ComboBoxAttachment> (apvts, "root", rootBox);
    octaveAttach  = std::make_unique<SliderAttachment> (apvts, "octave", octaveSlider);
    rangeAttach   = std::make_unique<SliderAttachment> (apvts, "range", rangeSlider);
    bpmAttach     = std::make_unique<SliderAttachment> (apvts, "bpm", bpmSlider);
    densityAttach = std::make_unique<SliderAttachment> (apvts, "density", densitySlider);
    playAttach    = std::make_unique<ButtonAttachment> (apvts, "play", playButton);

    generateButton.onClick = [this]
    {
        processor.generate();
        setStatus ("New pattern generated");
    };

    clearButton.onClick = [this]
    {
        processor.clearClip();
        setStatus ("Clip cleared");
    };

    saveButton.onClick = [this]
    {
        if (processor.getClipCopy().empty())
        {
            setStatus ("Nothing to save - generate a pattern first");
            return;
        }

        chooser = std::make_unique<juce::FileChooser> (
            "Save MIDI clip",
            juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("SharmatixGrid.mid"),
            "*.mid");

        juce::Component::SafePointer<SharmatixGridAudioProcessorEditor> safe (this);

        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe.getComponent() == nullptr)
                                      return;

                                  auto f = fc.getResult();
                                  if (f == juce::File())
                                      return;

                                  f = f.withFileExtension ("mid");
                                  safe->setStatus (safe->processor.writeMidiFile (f)
                                                       ? "Saved " + f.getFileName()
                                                       : "Could not save the file");
                              });
    };

    dragPad.onDragStart = [this]
    {
        const auto file = processor.exportClipToTempMidiFile();
        if (! file.existsAsFile())
        {
            setStatus ("Generate a pattern first");
            return;
        }

        juce::StringArray files;
        files.add (file.getFullPathName());
        juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, &dragPad, nullptr);
    };

    setResizable (true, true);
    setResizeLimits (840, 580, 1800, 1200);
    setSize (980, 660);

    startTimerHz (30);
}

SharmatixGridAudioProcessorEditor::~SharmatixGridAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SharmatixGridAudioProcessorEditor::configureIncDec (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::IncDecButtons);
    s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 52, 28);
    s.setIncDecButtonsMode (juce::Slider::incDecButtonsNotDraggable);
}

void SharmatixGridAudioProcessorEditor::setStatus (const juce::String& text)
{
    status.setText (text, juce::dontSendNotification);
    statusTicks = 90;   // ~3 s a 30 Hz
}

void SharmatixGridAudioProcessorEditor::timerCallback()
{
    // Cambios de escala / tonica / octava: re-cuantizar el clip a la nueva escala
    if (processor.consumeRequantizeRequest())
        processor.requantizeClip();

    const int version = processor.getClipVersion();
    const double cur = processor.getCurrentStep();
    const int stepKey = cur < 0.0 ? -1 : (int) (cur * 4.0);

    if (version != lastVersion || stepKey != lastStepKey)
    {
        lastVersion = version;
        lastStepKey = stepKey;
        grid.repaint();
    }

    phase += 0.09f;
    generateButton.setPulse (0.5f + 0.5f * std::sin (phase));

    if (statusTicks > 0 && --statusTicks == 0)
        status.setText ({}, juce::dontSendNotification);
}

//==============================================================================
void SharmatixGridAudioProcessorEditor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    float scale = 1.0f;
    if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()))
        scale = juce::jmax (1.0f, (float) display->scale);

    background = juce::Image (juce::Image::ARGB,
                              juce::jmax (1, juce::roundToInt ((float) w * scale)),
                              juce::jmax (1, juce::roundToInt ((float) h * scale)), true);

    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    drawPsychedelicBackground (g, w, h);
}

void SharmatixGridAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (background, getLocalBounds().toFloat());
    }
    else
    {
        g.fillAll (psy::ink);
    }

    // Titulo con resplandor
    const juce::Rectangle<int> titleArea (20, 8, 520, 44);
    g.setFont (psy::makeFont (36.0f, true));
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (psy::magenta.withAlpha (0.10f));
        g.drawText ("SHARMATIX GRID", titleArea.expanded (i * 2, i), juce::Justification::centredLeft);
    }
    g.setGradientFill (juce::ColourGradient (psy::cyan, 20.0f, 0.0f, psy::magenta, 360.0f, 0.0f, false));
    g.drawText ("SHARMATIX GRID", titleArea, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (psy::makeFont (12.0f));
    g.drawText ("psychedelic riff generator", juce::Rectangle<int> (22, 50, 300, 16), juce::Justification::centredLeft);

    // Rotulos de los controles
    g.setColour (psy::cyan.withAlpha (0.85f));
    g.setFont (psy::makeFont (11.0f, true));
    for (const auto& c : captions)
        g.drawText (c.text, c.area, juce::Justification::centred);

    if (dropHighlight)
    {
        g.setColour (psy::magenta.withAlpha (0.9f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (4.0f), 14.0f, 3.0f);
        g.setColour (psy::magenta.withAlpha (0.12f));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (4.0f), 14.0f);
    }
}

void SharmatixGridAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);

    area.removeFromTop (68);   // titulo (dibujado en paint)

    // ---- Panel superior de controles ---------------------------------------
    auto controls = area.removeFromTop (92);
    captions.clear();

    struct Item { juce::Component* comp; const char* caption; float weight; int height; };
    const std::array<Item, 8> items {{
        { &scaleBox,      "SCALE",   170.0f, 34 },
        { &rootBox,       "ROOT",     70.0f, 34 },
        { &octaveSlider,  "OCTAVE",  112.0f, 34 },
        { &rangeSlider,   "RANGE",   112.0f, 34 },
        { &bpmSlider,     "BPM",     112.0f, 34 },
        { &stepsLabel,    "LENGTH",   78.0f, 34 },
        { &densitySlider, "DENSITY",  70.0f, 76 },
        { &playButton,    "OUTPUT",  100.0f, 34 }
    }};

    constexpr int gap = 12;
    float totalWeight = 0.0f;
    for (const auto& it : items)
        totalWeight += it.weight;

    const float available = (float) controls.getWidth() - (float) gap * (float) (items.size() - 1);
    int x = controls.getX();

    for (const auto& it : items)
    {
        const int w = juce::roundToInt (available * it.weight / totalWeight);
        const juce::Rectangle<int> slot (x, controls.getY(), w, controls.getHeight());

        captions.push_back ({ slot.withHeight (16), it.caption });

        auto body = slot.withTrimmedTop (16);
        it.comp->setBounds (body.withSizeKeepingCentre (body.getWidth(), juce::jmin (it.height, body.getHeight())));
        x += w + gap;
    }

    // ---- Barra inferior ----------------------------------------------------
    auto bottom = area.removeFromBottom (96);
    status.setBounds (bottom.removeFromBottom (22));

    auto row = bottom;
    generateButton.setBounds (row.withSizeKeepingCentre (270, row.getHeight()));

    auto left = row.removeFromLeft (250);
    clearButton.setBounds (left.removeFromLeft (112));
    saveButton.setBounds (left.removeFromLeft (136));
    dragPad.setBounds (row.removeFromRight (220));

    // ---- Cuadricula --------------------------------------------------------
    grid.setBounds (area.reduced (0, 6));

    renderBackground();
}

//==============================================================================
// Arrastrar un .mid hacia dentro del plugin
namespace
{
    bool isMidiFile (const juce::String& path)
    {
        const auto ext = juce::File (path).getFileExtension().toLowerCase();
        return ext == ".mid" || ext == ".midi";
    }
}

bool SharmatixGridAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (isMidiFile (f))
            return true;
    return false;
}

void SharmatixGridAudioProcessorEditor::fileDragEnter (const juce::StringArray&, int, int)
{
    dropHighlight = true;
    repaint();
}

void SharmatixGridAudioProcessorEditor::fileDragExit (const juce::StringArray&)
{
    dropHighlight = false;
    repaint();
}

void SharmatixGridAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dropHighlight = false;

    for (const auto& path : files)
        if (isMidiFile (path))
        {
            const juce::File f (path);
            setStatus (processor.importMidiFile (f) ? "Imported " + f.getFileName()
                                                    : "Could not read that MIDI file");
            break;
        }

    repaint();
}
