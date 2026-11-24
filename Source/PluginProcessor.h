#pragma once
#include "Common.h"
#include "Voice.h"
#include <array>

class WarpusKongProcessor : public AudioProcessor
{
public:
    static constexpr int numVoices = 32;   // voix reelles (chaque note en utilise `unison`)

    WarpusKongProcessor();
    ~WarpusKongProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const String getName() const override { return "Warpus Kong"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const String getProgramName (int) override { return {}; }
    void changeProgramName (int, const String&) override {}

    void getStateInformation (MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- sample (thread message)
    bool loadSampleFile (const File&);
    void setSample (std::vector<float>&& data, double rate, const String& name, bool embedInState);
    std::shared_ptr<const SampleData> getSample() const;
    void getLoopRegion (int note, double& startFrac, double& endFrac) const;
    String getInfoText() const;

    AudioProcessorValueTreeState apvts;
    std::atomic<int> lastNote { 60 }, sampleVersion { 0 };
    std::atomic<double> hostSampleRate { 44100.0 };

private:
    struct Ptrs
    {
        std::atomic<float> *loopMode, *loopStart, *loopSpan, *loopHold, *decay, *feedback, *color,
            *stiff, *pick, *noise, *eqLow, *eqMid, *eqFreq, *eqHigh,
            *aA, *aD, *aS, *aR, *aAC, *aDC, *fType, *cutoff, *reso, *fAmt, *fKey,
            *fA, *fD, *fS, *fR, *vibAmt, *vibRate, *vibAtt,
            *modType, *modAmt, *sTune, *sFine, *sLevel,
            *glide, *unison, *detune, *wide, *playMode, *gain, *osIdx;
    } pr;

    void readSettings();
    void applyOversampling (int idx);
    void renderChunk (AudioBuffer<float>&, MidiBuffer&, int start, int count);
    void renderRange (float* l, float* r, int count);
    void handleMidi (const MidiMessage&);
    void triggerNote (int note, float vel, bool legatoJoin);
    void noteOff (int note);
    void releaseAll();
    void loadSampleFromState();

    std::array<Voice, numVoices> voices;
    uint64_t voiceCounter = 0;
    Settings cfg;
    bool sustainDown = false;
    float bendSemis = 0.f, lastVel = 0.8f;
    double lastSemis = -1.0;           // derniere note jouee (pour le glide)
    int lastMode = -1;
    std::vector<int> noteStack;        // notes tenues (modes mono)

    std::array<std::unique_ptr<dsp::Oversampling<float>>, 5> oversamplers; // 2x..32x, stereo
    int curOsIdx = -1, maxBlock = 512;
    double hostSr = 44100.0;
    std::vector<float> bufL, bufR, dummy;
    SmoothedValue<float> gainSm;

    mutable SpinLock sampleLock;
    std::shared_ptr<const SampleData> currentSample;
    std::shared_ptr<const SampleData> audioSample;
    std::vector<std::shared_ptr<const SampleData>> retired;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpusKongProcessor)
};
