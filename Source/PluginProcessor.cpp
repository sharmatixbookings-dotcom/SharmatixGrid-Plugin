#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

SharmatixMelodiesAudioProcessor::SharmatixMelodiesAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    initSpiritualPresets();
    applyPreset (0);
    generatePattern();
}

void SharmatixMelodiesAudioProcessor::initSpiritualPresets()
{
    presets = {
        { "Akasha Portal",     0, 6,  2, 2, 16, 0.72f, 0, 0.55f },
        { "Dharma Pulse",      1, 0,  2, 2, 16, 0.80f, 1, 0.45f },
        { "Prana Sequence",    2, 2,  3, 2, 16, 0.65f, 2, 0.60f },
        { "Samsara Drift",     3, 9,  2, 2, 16, 0.58f, 3, 0.75f },
        { "Third Eye Arp",     0, 7,  3, 2, 16, 0.85f, 1, 0.40f },
        { "Soma Waves",        4, 4,  2, 2, 16, 0.68f, 2, 0.65f },
        { "Omkar Resonance",   5, 5,  2, 1, 16, 0.60f, 0, 0.70f },
        { "Astral Gate",       1, 11, 3, 2, 16, 0.76f, 3, 0.50f },
        { "Kundalini Rising",  2, 1,  2, 2, 16, 0.88f, 1, 0.48f },
        { "Mandala Echo",      4, 8,  2, 2, 16, 0.62f, 2, 0.58f }
    };
}

std::vector<int> SharmatixMelodiesAudioProcessor::getScaleIntervals (int scaleIndex) const
{
    switch (scaleIndex)
    {
        case 0: return { 0, 1, 4, 5, 7, 8, 10 };
        case 1: return { 0, 2, 3, 7, 8 };
        case 2: return { 0, 2, 3, 5, 7, 8, 10 };
        case 3: return { 0, 3, 5, 6, 7, 10 };
        case 4: return { 0, 1, 5, 7, 8 };
        default:return { 0, 2, 4, 7, 9 };
    }
}

void SharmatixMelodiesAudioProcessor::applyPreset (int index)
{
    if (presets.empty())
        return;

    currentPresetIndex = juce::jlimit (0, (int) presets.size() - 1, index);
    const auto& p = presets[(size_t) currentPresetIndex];

    auto setParam = [this] (const juce::String& id, float rawValue)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (rawValue));
    };

    setParam ("scale",   (float) p.scaleIndex);
    setParam ("root",    (float) p.rootNote);
    setParam ("octave",  (float) p.octave);
    setParam ("range",   (float) p.rangeOctaves);
    setParam ("length",  (float) p.lengthSteps);
    setParam ("density", p.density);
    setParam ("arpMode", (float) p.arpMode);
    setParam ("gate",    p.gateRatio);

    generatePattern();
}

void SharmatixMelodiesAudioProcessor::generatePattern()
{
    const int scaleIdx = (int) *apvts.getRawParameterValue ("scale");
    const int root     = (int) *apvts.getRawParameterValue ("root");
    const int octave   = (int) *apvts.getRawParameterValue ("octave");
    const int rangeOct = juce::jlimit (1, 3, (int) *apvts.getRawParameterValue ("range"));
    const int len      = juce::jlimit (4, maxSteps, (int) *apvts.getRawParameterValue ("length"));
    const float dens   = *apvts.getRawParameterValue ("density");
    const int arpMode  = (int) *apvts.getRawParameterValue ("arpMode");

    for (auto& row : stepGrid)
        row.fill (false);
    stepVelocities.fill (100);

    const auto intervals = getScaleIntervals (scaleIdx);
    std::vector<int> candidateNotes;

    const int baseMidi = 12 * (octave + 1) + root;
    for (int oct = 0; oct < rangeOct; ++oct)
    {
        for (int interval : intervals)
        {
            int midiNote = baseMidi + (oct * 12) + interval;
            if (midiNote >= 24 && midiNote <= 96)
                candidateNotes.push_back (midiNote);
        }
    }

    if (candidateNotes.empty())
        candidateNotes.push_back (60);

    juce::Random rng;
    int idxCursor = 0;

    for (int step = 0; step < len; ++step)
    {
        bool isStrongBeat = (step % 4 == 0);
        float effectiveDensity = isStrongBeat ? juce::jmin (1.0f, dens + 0.20f) : dens;

        if (rng.nextFloat() <= effectiveDensity)
        {
            if (arpMode == 1)
                idxCursor = step % (int) candidateNotes.size();
            else if (arpMode == 2)
            {
                int period = juce::jmax (1, (int) candidateNotes.size() * 2 - 2);
                int pos = step % period;
                idxCursor = pos < (int) candidateNotes.size() ? pos : (period - pos);
            }
            else if (arpMode == 3)
            {
                int stepJump = (step % 2 == 0) ? 0 : juce::jmin ((int) candidateNotes.size() - 1, idxCursor + 2);
                idxCursor = stepJump;
            }
            else
            {
                int jump = rng.nextInt (5) - 2;
                idxCursor = juce::jlimit (0, (int) candidateNotes.size() - 1, idxCursor + jump);
            }

            int note = candidateNotes[(size_t) idxCursor];
            int row = juce::jlimit (0, maxPitchRows - 1, (maxPitchRows - 1) - ((note - 36) % maxPitchRows));
            stepGrid[(size_t) row][(size_t) step] = true;

            int vel = isStrongBeat ? rng.nextInt (20) + 105 : rng.nextInt (30) + 75;
            stepVelocities[(size_t) step] = juce::jlimit (1, 127, vel);
        }
    }
}

juce::File SharmatixMelodiesAudioProcessor::getPresetsFolder() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                 .getChildFile ("Sharmatix")
                 .getChildFile ("Sharmatix Melodies")
                 .getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

void SharmatixMelodiesAudioProcessor::savePresetToFile()
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
        return;

    auto folder = getPresetsFolder();
    auto file = folder.getChildFile ("Preset_" + juce::String (juce::Time::getCurrentTime().toMilliseconds()) + ".xml");
    xml->writeTo (file);
}

void SharmatixMelodiesAudioProcessor::loadPresetFromFile()
{
    auto folder = getPresetsFolder();
    auto files = folder.findChildFiles (juce::File::findFiles, false, "*.xml");
    if (files.isEmpty())
        return;

    files.sort();
    auto xml = juce::XmlDocument::parse (files.getLast());
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        generatePattern();
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SharmatixMelodiesAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "scale", "Scale",
        juce::StringArray { "Phrygian Dom", "Hirajoshi", "Nat Minor", "Mystic Minor", "In Sen", "Harmonic Pent" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "root", "Root",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 6));

    params.push_back (std::make_unique<juce::AudioParameterInt> ("octave", "Octave", 1, 5, 2));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("range", "Range", 1, 3, 2));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("length", "Length", 4, 16, 16));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("density", "Density", 0.0f, 1.0f, 0.72f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "arpMode", "Pattern Mode",
        juce::StringArray { "Psy Step", "Arp Up", "Wave", "Octave Jump" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("gate", "Gate", 0.15f, 0.95f, 0.55f));

    return { params.begin(), params.end() };
}

void SharmatixMelodiesAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    stepAccumulator = 0.0;
    currentStep = 0;
    lastNotePlayed = -1;
    noteRemainingSamples = 0.0;
}

void SharmatixMelodiesAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SharmatixMelodiesAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet().isDisabled())
        return true;

    return false;
}
#endif

void SharmatixMelodiesAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    double bpm = 145.0;
    bool isPlaying = true;

    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
            isPlaying = pos->getIsPlaying();
        }
    }

    if (! isPlaying)
    {
        if (lastNotePlayed >= 0)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, lastNotePlayed), 0);
            lastNotePlayed = -1;
        }
        return;
    }

    const int numSamples = buffer.getNumSamples();
    const int len = juce::jlimit (4, maxSteps, (int) *apvts.getRawParameterValue ("length"));
    const float gate = juce::jlimit (0.15f, 0.95f, static_cast<float> (*apvts.getRawParameterValue ("gate")));

    const double secondsPerBeat = 60.0 / juce::jmax (40.0, bpm);
    const double samplesPerStep = (secondsPerBeat * 0.25) * currentSampleRate;

    juce::MidiBuffer outputMidi;

    for (int i = 0; i < numSamples; ++i)
    {
        if (lastNotePlayed >= 0)
        {
            noteRemainingSamples -= 1.0;
            if (noteRemainingSamples <= 0.0)
            {
                outputMidi.addEvent (juce::MidiMessage::noteOff (1, lastNotePlayed), i);
                lastNotePlayed = -1;
            }
        }

        if (stepAccumulator <= 0.0)
        {
            int activeNote = -1;
            int activeVel = 100;

            for (int row = 0; row < maxPitchRows; ++row)
            {
                if (stepGrid[(size_t) row][(size_t) currentStep])
                {
                    int baseNote = 36 + ((maxPitchRows - 1) - row);
                    activeNote = juce::jlimit (24, 108, baseNote);
                    activeVel = stepVelocities[(size_t) currentStep];
                    break;
                }
            }

            if (activeNote >= 0)
            {
                if (lastNotePlayed >= 0 && lastNotePlayed != activeNote)
                    outputMidi.addEvent (juce::MidiMessage::noteOff (1, lastNotePlayed), i);

                outputMidi.addEvent (juce::MidiMessage::noteOn (1, activeNote, (juce::uint8) activeVel), i);
                lastNotePlayed = activeNote;
                noteRemainingSamples = samplesPerStep * gate;
            }

            stepAccumulator += samplesPerStep;
            currentStep = (currentStep + 1) % len;
        }

        stepAccumulator -= 1.0;
    }

    for (const auto metadata : midiMessages)
        outputMidi.addEvent (metadata.getMessage(), metadata.samplePosition);

    midiMessages.swapWith (outputMidi);
}

juce::AudioProcessorEditor* SharmatixMelodiesAudioProcessor::createEditor()
{
    return new SharmatixMelodiesAudioEditor (*this);
}

int SharmatixMelodiesAudioProcessor::getNumPrograms()
{
    return (int) presets.size();
}

int SharmatixMelodiesAudioProcessor::getCurrentProgram()
{
    return currentPresetIndex;
}

void SharmatixMelodiesAudioProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

const juce::String SharmatixMelodiesAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;
    return {};
}

void SharmatixMelodiesAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    if (index >= 0 && index < (int) presets.size())
        presets[(size_t) index].name = newName;
}

void SharmatixMelodiesAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SharmatixMelodiesAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        generatePattern();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SharmatixMelodiesAudioProcessor();
}
