#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

//==============================================================================
/** Una nota del clip. Las posiciones se miden en "steps" (semicorcheas):
    el clip dura 16 steps = 4 negras = 1 compas de 4/4. */
struct GridNote
{
    int    pitch       = 60;
    double startStep   = 0.0;   // 0 .. 16
    double lengthSteps = 1.0;   // > 0, start + length <= 16
    int    velocity    = 100;   // 1 .. 127
};

//==============================================================================
class SharmatixGridAudioProcessor : public juce::AudioProcessor,
                                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int numSteps = 16;

    SharmatixGridAudioProcessor();
    ~SharmatixGridAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SharmatixGrid"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Escalas
    static const juce::StringArray& getScaleNames();
    static const juce::StringArray& getRootNames();
    static std::vector<int> getScaleIntervals (int scaleIndex);

    /** Todas las alturas permitidas (ordenadas de grave a agudo) segun escala,
        tonica, octava base y rango actuales. Es el "eje vertical" de la grilla. */
    std::vector<int> getPitchSet() const;

    //==========================================================================
    // Clip (llamar solo desde el hilo de mensajes / GUI)
    void generate();
    void clearClip();
    bool hasNoteAt (int step, int pitch) const;
    void setNoteAt (int step, int pitch, bool shouldBeOn);
    std::vector<GridNote> getClipCopy() const;
    int  getClipVersion() const noexcept { return clipVersion.load(); }

    /** Ajusta las notas del clip a la escala vigente (tras cambiar escala/tonica/octava). */
    void requantizeClip();
    bool consumeRequantizeRequest() noexcept { return requantizePending.exchange (false); }

    //==========================================================================
    // MIDI file: exportar / importar
    juce::MidiFile buildMidiFile (const std::vector<GridNote>& notes) const;
    bool writeMidiFile (const juce::File& destination) const;
    /** Escribe el clip en un .mid temporal y devuelve el archivo (File() si el clip esta vacio). */
    juce::File exportClipToTempMidiFile();
    bool importMidiFile (const juce::File& source);

    //==========================================================================
    /** Posicion de reproduccion en steps (0..16) o -1 si el host esta detenido. */
    double getCurrentStep() const noexcept { return currentStep.load(); }
    double getEffectiveBpm() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void flushSoundingNotes (juce::MidiBuffer& midi, int samplePosition);

    // Clip compartido entre hilo de GUI y hilo de audio
    mutable juce::CriticalSection clipLock;
    std::vector<GridNote> clip;
    std::atomic<int> clipVersion { 0 };

    juce::Random rng;
    std::atomic<bool> requantizePending { false };

    // Parametros (lectura lock-free desde el hilo de audio)
    std::atomic<float>* scaleParam   = nullptr;
    std::atomic<float>* rootParam    = nullptr;
    std::atomic<float>* octaveParam  = nullptr;
    std::atomic<float>* rangeParam   = nullptr;
    std::atomic<float>* densityParam = nullptr;
    std::atomic<float>* bpmParam     = nullptr;
    std::atomic<float>* playParam    = nullptr;

    // Estado de reproduccion (solo hilo de audio, salvo los atomics)
    std::atomic<double> currentStep { -1.0 };
    std::atomic<float>  lastHostBpm { 0.0f };
    bool   wasRunning = false;
    double expectedNextStart = 0.0;
    std::array<bool, 128>   sounding {};
    std::array<double, 128> offAt {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharmatixGridAudioProcessor)
};
