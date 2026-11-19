#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

struct SpiritualPreset
{
    juce::String name;
    int scaleIndex;
    int rootNote;
    int octave;
    int rangeOctaves;
    int lengthSteps;
    float density;
    int arpMode;
    float gateRatio;
};

class SharmatixMelodiesAudioProcessor : public juce::AudioProcessor
{
public:
    SharmatixMelodiesAudioProcessor();
    ~SharmatixMelodiesAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Sharmatix Melodies"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void generatePattern();
    void savePresetToFile();
    void loadPresetFromFile();
    juce::File getPresetsFolder() const;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int maxSteps = 16;
    static constexpr int maxPitchRows = 24;
    std::array<std::array<bool, maxSteps>, maxPitchRows> stepGrid {};
    std::array<int, maxSteps> stepVelocities {};

    std::vector<SpiritualPreset> presets;
    int currentPresetIndex = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void initSpiritualPresets();
    void applyPreset (int index);
    std::vector<int> getScaleIntervals (int scaleIndex) const;

    double currentSampleRate = 44100.0;
    double stepAccumulator = 0.0;
    int currentStep = 0;
    int lastNotePlayed = -1;
    double noteRemainingSamples = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharmatixMelodiesAudioProcessor)
};
