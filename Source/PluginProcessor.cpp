#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int maxNotesInClip = 256;
    constexpr int midiTicksPerQuarter = 960;

    int snapToSet (const std::vector<int>& set, int pitch)
    {
        if (set.empty())
            return pitch;

        int best = set.front();
        for (int p : set)
            if (std::abs (p - pitch) < std::abs (best - pitch))
                best = p;
        return best;
    }
}

//==============================================================================
const juce::StringArray& SharmatixGridAudioProcessor::getScaleNames()
{
    static const juce::StringArray names { "Natural Minor", "Minor Pentatonic",
                                           "Phrygian Dominant", "Phrygian", "Harmonic Minor" };
    return names;
}

const juce::StringArray& SharmatixGridAudioProcessor::getRootNames()
{
    static const juce::StringArray names { "C", "C#", "D", "D#", "E", "F",
                                           "F#", "G", "G#", "A", "A#", "B" };
    return names;
}

std::vector<int> SharmatixGridAudioProcessor::getScaleIntervals (int scaleIndex)
{
    switch (scaleIndex)
    {
        case 0:  return { 0, 2, 3, 5, 7, 8, 10 };   // Natural Minor (Eolico)
        case 1:  return { 0, 3, 5, 7, 10 };         // Pentatonica menor
        case 2:  return { 0, 1, 4, 5, 7, 8, 10 };   // Phrygian Dominant
        case 3:  return { 0, 1, 3, 5, 7, 8, 10 };   // Phrygian
        case 4:  return { 0, 2, 3, 5, 7, 8, 11 };   // Menor armonica
        default: return { 0, 2, 3, 5, 7, 8, 10 };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SharmatixGridAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "scale", 1 }, "Scale", getScaleNames(), 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "root", 1 }, "Root", getRootNames(), 9 /* A */));

    // Octava base con la convencion C3 = MIDI 60 (la de Ableton / FL Studio)
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "octave", 1 }, "Octave", 0, 5, 2));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "range", 1 }, "Range", 1, 3, 2));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "density", 1 }, "Density",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bpm", 1 }, "BPM",
        juce::NormalisableRange<float> (60.0f, 200.0f, 0.1f), 140.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "play", 1 }, "MIDI Out", true));

    return layout;
}

//==============================================================================
SharmatixGridAudioProcessor::SharmatixGridAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    scaleParam   = apvts.getRawParameterValue ("scale");
    rootParam    = apvts.getRawParameterValue ("root");
    octaveParam  = apvts.getRawParameterValue ("octave");
    rangeParam   = apvts.getRawParameterValue ("range");
    densityParam = apvts.getRawParameterValue ("density");
    bpmParam     = apvts.getRawParameterValue ("bpm");
    playParam    = apvts.getRawParameterValue ("play");

    for (auto id : { "scale", "root", "octave", "range" })
        apvts.addParameterListener (id, this);

    offAt.fill (0.0);
    clip.reserve (64);
    generate();   // arranca con un patron para que la grilla no este vacia
}

SharmatixGridAudioProcessor::~SharmatixGridAudioProcessor()
{
    for (auto id : { "scale", "root", "octave", "range" })
        apvts.removeParameterListener (id, this);
}

void SharmatixGridAudioProcessor::parameterChanged (const juce::String&, float)
{
    // Puede llamarse desde cualquier hilo: solo levantamos una bandera.
    // La GUI (hilo de mensajes) hace el re-cuantizado.
    requantizePending.store (true);
}

//==============================================================================
std::vector<int> SharmatixGridAudioProcessor::getPitchSet() const
{
    const int scale = juce::roundToInt (scaleParam->load());
    const int root  = juce::roundToInt (rootParam->load());
    const int oct   = juce::roundToInt (octaveParam->load());
    const int range = juce::roundToInt (rangeParam->load());

    const int base = 12 * (oct + 2) + root;   // C3 = 60
    const auto intervals = getScaleIntervals (scale);

    std::vector<int> out;
    for (int o = 0; o < range; ++o)
        for (int iv : intervals)
        {
            const int p = base + 12 * o + iv;
            if (p <= 127)
                out.push_back (p);
        }

    const int top = base + 12 * range;   // tonica una octava arriba, cierra la grilla
    if (top <= 127)
        out.push_back (top);

    return out;
}

double SharmatixGridAudioProcessor::getEffectiveBpm() const
{
    const float host = lastHostBpm.load();
    return host > 0.0f ? (double) host : (double) bpmParam->load();
}

//==============================================================================
// Motor de generacion: camina por los grados de la escala (random walk),
// asi el resultado es siempre armonico y ademas melodicamente coherente.
void SharmatixGridAudioProcessor::generate()
{
    const auto pitches = getPitchSet();
    if (pitches.empty())
        return;

    const int size = (int) pitches.size();
    const int scaleSize = (int) getScaleIntervals (juce::roundToInt (scaleParam->load())).size();
    const float density = juce::jlimit (0.05f, 1.0f, densityParam->load());

    std::vector<GridNote> out;
    int idx = 0;     // empieza en la tonica
    int step = 0;

    while (step < numSteps)
    {
        const bool strong  = (step % 4) == 0;
        const bool offbeat = (step % 2) == 1;
        const float prob   = density * (strong ? 1.25f : (offbeat ? 0.8f : 1.0f));
        const bool force   = (step == 0);

        if (force || rng.nextFloat() < prob)
        {
            if (! force)
            {
                const int r = rng.nextInt (100);
                int jump = 0;
                if      (r < 30) jump = 0;                       // repite nota (muy psytrance)
                else if (r < 52) jump = 1;
                else if (r < 74) jump = -1;
                else if (r < 84) jump = 2;
                else if (r < 94) jump = -2;
                else             jump = rng.nextBool() ? scaleSize : -scaleSize;   // salto de octava

                idx = juce::jlimit (0, size - 1, idx + jump);
            }

            const int rl = rng.nextInt (100);
            int len = rl < 60 ? 1 : (rl < 88 ? 2 : 3);
            len = juce::jmin (len, numSteps - step);

            GridNote n;
            n.pitch       = pitches[(size_t) idx];
            n.startStep   = (double) step;
            n.lengthSteps = (double) len;
            n.velocity    = strong ? 108 + rng.nextInt (20) : 76 + rng.nextInt (30);
            out.push_back (n);

            step += len;
        }
        else
        {
            ++step;
        }
    }

    {
        const juce::ScopedLock sl (clipLock);
        clip = std::move (out);
    }
    ++clipVersion;
}

void SharmatixGridAudioProcessor::clearClip()
{
    {
        const juce::ScopedLock sl (clipLock);
        clip.clear();
    }
    ++clipVersion;
}

bool SharmatixGridAudioProcessor::hasNoteAt (int step, int pitch) const
{
    const juce::ScopedLock sl (clipLock);
    for (const auto& n : clip)
        if (n.pitch == pitch && (double) step >= n.startStep && (double) step < n.startStep + n.lengthSteps)
            return true;
    return false;
}

void SharmatixGridAudioProcessor::setNoteAt (int step, int pitch, bool shouldBeOn)
{
    if (step < 0 || step >= numSteps)
        return;

    {
        const juce::ScopedLock sl (clipLock);

        if (shouldBeOn)
        {
            bool exists = false;
            for (const auto& n : clip)
                if (n.pitch == pitch && (double) step >= n.startStep && (double) step < n.startStep + n.lengthSteps)
                    exists = true;

            if (! exists && (int) clip.size() < maxNotesInClip)
            {
                GridNote n;
                n.pitch = pitch;
                n.startStep = (double) step;
                n.lengthSteps = 1.0;
                n.velocity = (step % 4 == 0) ? 112 : 96;
                clip.push_back (n);
            }
        }
        else
        {
            clip.erase (std::remove_if (clip.begin(), clip.end(), [=] (const GridNote& n)
            {
                return n.pitch == pitch && (double) step >= n.startStep
                       && (double) step < n.startStep + n.lengthSteps;
            }), clip.end());
        }
    }
    ++clipVersion;
}

std::vector<GridNote> SharmatixGridAudioProcessor::getClipCopy() const
{
    const juce::ScopedLock sl (clipLock);
    return clip;
}

void SharmatixGridAudioProcessor::requantizeClip()
{
    const auto set = getPitchSet();
    {
        const juce::ScopedLock sl (clipLock);
        for (auto& n : clip)
            n.pitch = snapToSet (set, n.pitch);
    }
    ++clipVersion;
}

//==============================================================================
// MIDI file
juce::MidiFile SharmatixGridAudioProcessor::buildMidiFile (const std::vector<GridNote>& notes) const
{
    const double bpm = juce::jlimit (20.0, 400.0, getEffectiveBpm());
    const double ticksPerStep = (double) midiTicksPerQuarter / 4.0;

    juce::MidiMessageSequence seq;

    auto add = [&seq] (juce::MidiMessage m, double tick)
    {
        m.setTimeStamp (tick);
        seq.addEvent (m);
    };

    add (juce::MidiMessage::textMetaEvent (3, "SharmatixGrid"), 0.0);
    add (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / bpm)), 0.0);
    add (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);

    for (const auto& n : notes)
    {
        const double on  = std::round (n.startStep * ticksPerStep);
        const double off = std::round ((n.startStep + n.lengthSteps) * ticksPerStep);
        const auto vel = (juce::uint8) juce::jlimit (1, 127, n.velocity);

        add (juce::MidiMessage::noteOn (1, n.pitch, vel), on);
        add (juce::MidiMessage::noteOff (1, n.pitch), off);
    }

    add (juce::MidiMessage::endOfTrack(), (double) numSteps * ticksPerStep);

    seq.sort();
    seq.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (midiTicksPerQuarter);
    file.addTrack (seq);
    return file;
}

bool SharmatixGridAudioProcessor::writeMidiFile (const juce::File& destination) const
{
    const auto notes = getClipCopy();
    if (notes.empty())
        return false;

    const auto midiFile = buildMidiFile (notes);

    destination.getParentDirectory().createDirectory();
    destination.deleteFile();

    juce::FileOutputStream out (destination);
    if (! out.openedOk())
        return false;

    const bool ok = midiFile.writeTo (out, 1);
    out.flush();
    return ok;
}

juce::File SharmatixGridAudioProcessor::exportClipToTempMidiFile()
{
    if (getClipCopy().empty())
        return {};

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("SharmatixGrid");
    dir.createDirectory();

    // Limpia exportaciones antiguas (> 24 h)
    const auto limit = juce::Time::getCurrentTime() - juce::RelativeTime::hours (24);
    for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "SharmatixGrid_*.mid"))
        if (f.getLastModificationTime() < limit)
            f.deleteFile();

    const auto scaleName = getScaleNames()[juce::roundToInt (scaleParam->load())].replaceCharacter (' ', '-');
    const auto rootName  = getRootNames()[juce::roundToInt (rootParam->load())].replace ("#", "s");
    const auto stamp     = juce::Time::getCurrentTime().formatted ("%H%M%S");

    const auto file = dir.getChildFile ("SharmatixGrid_" + rootName + "_" + scaleName + "_" + stamp + ".mid");

    return writeMidiFile (file) ? file : juce::File();
}

bool SharmatixGridAudioProcessor::importMidiFile (const juce::File& source)
{
    juce::FileInputStream in (source);
    if (! in.openedOk())
        return false;

    juce::MidiFile midiFile;
    if (! midiFile.readFrom (in))
        return false;

    const int ticksPerQuarter = midiFile.getTimeFormat();
    if (ticksPerQuarter <= 0)
        return false;   // formato SMPTE, no soportado

    juce::MidiMessageSequence all;
    for (int t = 0; t < midiFile.getNumTracks(); ++t)
        if (auto* track = midiFile.getTrack (t))
            all.addSequence (*track, 0.0);

    all.updateMatchedPairs();

    const auto pitches = getPitchSet();
    const double stepsPerTick = 4.0 / (double) ticksPerQuarter;

    std::vector<GridNote> imported;

    for (int i = 0; i < all.getNumEvents(); ++i)
    {
        auto* ev = all.getEventPointer (i);
        if (! ev->message.isNoteOn())
            continue;

        const double startRaw = ev->message.getTimeStamp() * stepsPerTick;
        const double endRaw   = ev->noteOffObject != nullptr
                                  ? ev->noteOffObject->message.getTimeStamp() * stepsPerTick
                                  : startRaw + 1.0;

        const double start = std::round (startRaw);
        if (start < 0.0 || start >= (double) numSteps)
            continue;   // solo el primer compas

        GridNote n;
        n.pitch       = snapToSet (pitches, ev->message.getNoteNumber());
        n.startStep   = start;
        n.lengthSteps = juce::jlimit (1.0, (double) numSteps - start, std::round (endRaw - start));
        n.velocity    = juce::jlimit (1, 127, (int) ev->message.getVelocity());
        imported.push_back (n);

        if ((int) imported.size() >= maxNotesInClip)
            break;
    }

    if (imported.empty())
        return false;

    {
        const juce::ScopedLock sl (clipLock);
        clip = std::move (imported);
    }
    ++clipVersion;
    return true;
}

//==============================================================================
// Audio thread
void SharmatixGridAudioProcessor::prepareToPlay (double, int)
{
    wasRunning = false;
    sounding.fill (false);
    offAt.fill (0.0);
    currentStep.store (-1.0);
}

bool SharmatixGridAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SharmatixGridAudioProcessor::flushSoundingNotes (juce::MidiBuffer& midi, int samplePosition)
{
    for (int p = 0; p < 128; ++p)
        if (sounding[(size_t) p])
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, p), samplePosition);
            sounding[(size_t) p] = false;
        }
}

void SharmatixGridAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();   // este plugin no genera audio
    midi.clear();     // la entrada MIDI se descarta; solo emitimos el clip

    const int numSamples = buffer.getNumSamples();

    bool playing = false, havePpq = false;
    double ppq = 0.0, hostBpm = 0.0;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();

            if (auto b = pos->getBpm())
                hostBpm = *b;

            if (auto p = pos->getPpqPosition())
            {
                ppq = *p;
                havePpq = true;
            }
        }

    if (hostBpm > 0.0)
        lastHostBpm.store ((float) hostBpm);

    const double sampleRate = getSampleRate();
    const bool run = playing && havePpq && sampleRate > 0.0 && playParam->load() > 0.5f;

    if (! run)
    {
        if (wasRunning)
        {
            flushSoundingNotes (midi, 0);
            wasRunning = false;
        }
        currentStep.store (-1.0);
        return;
    }

    const double bpm = hostBpm > 0.0 ? hostBpm : (double) bpmParam->load();
    const double loopLen = (double) numSteps;
    const double stepsPerSample = (bpm / 60.0) * 4.0 / sampleRate;
    const double startAbs = ppq * 4.0;                       // en steps (semicorcheas)
    const double endAbs   = startAbs + (double) numSamples * stepsPerSample;

    // Arranque de transporte o salto de posicion: cortar lo que suena
    if (! wasRunning || std::abs (startAbs - expectedNextStart) > 0.5)
        flushSoundingNotes (midi, 0);

    wasRunning = true;
    expectedNextStart = endAbs;

    auto toSample = [&] (double t)
    {
        return juce::jlimit (0, juce::jmax (0, numSamples - 1), (int) ((t - startAbs) / stepsPerSample));
    };

    // Los note-off se programan al emitir el note-on (offAt), asi nunca quedan
    // notas colgadas aunque el usuario edite el clip mientras suena.
    auto emitDueOffs = [&] (double upTo)
    {
        for (int p = 0; p < 128; ++p)
            if (sounding[(size_t) p] && offAt[(size_t) p] <= upTo)
            {
                midi.addEvent (juce::MidiMessage::noteOff (1, p), toSample (juce::jmax (offAt[(size_t) p], startAbs)));
                sounding[(size_t) p] = false;
            }
    };

    struct OnEvent { double time; int pitch; int velocity; double length; };
    constexpr int maxEvents = 512;
    std::array<OnEvent, maxEvents> events;
    int numEvents = 0;

    {
        const juce::ScopedTryLock sl (clipLock);
        if (sl.isLocked())
        {
            const int k0 = (int) std::floor (startAbs / loopLen);
            const int k1 = (int) std::floor (endAbs / loopLen);

            for (int k = k0; k <= k1; ++k)
                for (const auto& n : clip)
                {
                    const double on = (double) k * loopLen + n.startStep;
                    if (on >= startAbs && on < endAbs && numEvents < maxEvents)
                        events[(size_t) numEvents++] = { on, n.pitch, n.velocity,
                                                         juce::jmin (n.lengthSteps, loopLen - n.startStep) };
                }
        }
    }

    std::sort (events.begin(), events.begin() + numEvents,
               [] (const OnEvent& a, const OnEvent& b) { return a.time < b.time; });

    for (int i = 0; i < numEvents; ++i)
    {
        const auto& e = events[(size_t) i];
        const int pitch = juce::jlimit (0, 127, e.pitch);
        const int sample = toSample (e.time);

        emitDueOffs (e.time);

        if (sounding[(size_t) pitch])   // misma nota solapada: re-disparo limpio
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, pitch), sample);
            sounding[(size_t) pitch] = false;
        }

        midi.addEvent (juce::MidiMessage::noteOn (1, pitch, (juce::uint8) juce::jlimit (1, 127, e.velocity)), sample);
        sounding[(size_t) pitch] = true;
        offAt[(size_t) pitch] = e.time + e.length;
    }

    emitDueOffs (endAbs - 1.0e-9);

    double cur = std::fmod (startAbs, loopLen);
    if (cur < 0.0)
        cur += loopLen;
    currentStep.store (cur);
}

//==============================================================================
juce::AudioProcessorEditor* SharmatixGridAudioProcessor::createEditor()
{
    return new SharmatixGridAudioProcessorEditor (*this);
}

void SharmatixGridAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    juce::ValueTree clipTree ("CLIP");
    for (const auto& n : getClipCopy())
    {
        juce::ValueTree v ("NOTE");
        v.setProperty ("p", n.pitch, nullptr);
        v.setProperty ("s", n.startStep, nullptr);
        v.setProperty ("l", n.lengthSteps, nullptr);
        v.setProperty ("v", n.velocity, nullptr);
        clipTree.appendChild (v, nullptr);
    }
    state.appendChild (clipTree, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SharmatixGridAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid() || ! tree.hasType (apvts.state.getType()))
        return;

    auto clipTree = tree.getChildWithName ("CLIP");
    std::vector<GridNote> loaded;

    for (auto v : clipTree)
    {
        GridNote n;
        n.pitch       = juce::jlimit (0, 127, (int) v.getProperty ("p", 60));
        n.startStep   = juce::jlimit (0.0, (double) numSteps - 1.0, (double) v.getProperty ("s", 0.0));
        n.lengthSteps = juce::jlimit (1.0, (double) numSteps - n.startStep, (double) v.getProperty ("l", 1.0));
        n.velocity    = juce::jlimit (1, 127, (int) v.getProperty ("v", 100));
        loaded.push_back (n);
    }

    const bool hadClip = clipTree.isValid();
    if (hadClip)
        tree.removeChild (clipTree, nullptr);

    apvts.replaceState (tree);

    if (hadClip)
    {
        {
            const juce::ScopedLock sl (clipLock);
            clip = std::move (loaded);
        }
        ++clipVersion;
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SharmatixGridAudioProcessor();
}
