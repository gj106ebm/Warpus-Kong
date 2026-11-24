#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include <atomic>

using namespace juce;

// Identifiants de parametres (stables : ne pas renommer, ils sont sauvegardes dans les projets)
namespace ID
{
    inline constexpr const char* loopMode  = "loopMode";
    inline constexpr const char* loopStart = "loopStart";
    inline constexpr const char* loopSpan  = "loopSpan";
    inline constexpr const char* loopHold  = "loopHold";
    inline constexpr const char* decay     = "decay";
    inline constexpr const char* feedback  = "feedback";
    inline constexpr const char* color     = "color";
    inline constexpr const char* stiff     = "stiff";
    inline constexpr const char* pick      = "pick";
    inline constexpr const char* noise     = "noise";
    inline constexpr const char* eqLow     = "eqLow";
    inline constexpr const char* eqMid     = "eqMid";
    inline constexpr const char* eqFreq    = "eqFreq";
    inline constexpr const char* eqHigh    = "eqHigh";
    inline constexpr const char* glide     = "glide";
    inline constexpr const char* unison    = "unison";
    inline constexpr const char* detune    = "detune";
    inline constexpr const char* wide      = "wide";
    inline constexpr const char* playMode  = "playMode";
    inline constexpr const char* aA  = "aA";  inline constexpr const char* aD  = "aD";
    inline constexpr const char* aS  = "aS";  inline constexpr const char* aR  = "aR";
    inline constexpr const char* aAC = "aAC"; inline constexpr const char* aDC = "aDC";
    inline constexpr const char* fType  = "fType";
    inline constexpr const char* cutoff = "cutoff";
    inline constexpr const char* reso   = "reso";
    inline constexpr const char* fAmt   = "fAmt";
    inline constexpr const char* fKey   = "fKey";
    inline constexpr const char* fA = "fA"; inline constexpr const char* fD = "fD";
    inline constexpr const char* fS = "fS"; inline constexpr const char* fR = "fR";
    inline constexpr const char* vibAmt  = "vibAmt";
    inline constexpr const char* vibRate = "vibRate";
    inline constexpr const char* vibAtt  = "vibAtt";
    inline constexpr const char* modType = "modType";
    inline constexpr const char* modAmt  = "modAmt";
    inline constexpr const char* sTune   = "sTune";
    inline constexpr const char* sFine   = "sFine";
    inline constexpr const char* sLevel  = "sLevel";
    inline constexpr const char* gain    = "gain";
    inline constexpr const char* osIdx   = "osIdx";
}

struct SampleData
{
    std::vector<float> data;   // mono, normalise
    double rate = 44100.0;
    String name;
};

struct EnvParams
{
    float a = 0.01f, d = 0.1f, s = 0.7f, r = 0.2f;
    float ac = 0.f, dc = 0.f;   // courbes -1..1
};

// Instantane des reglages, copie une fois par bloc (thread audio)
struct Settings
{
    int   loopMode = 0;          // 0 = Window (fenetre a vitesse native), 1 = Stretch
    float loopStart = 0.f, loopSpan = 1.f, hold = 0.f;
    float decay = 2.f, feedback = 0.f, color = 0.7f, stiff = 0.f, pick = 0.f, noise = 0.f;
    float eqLow = 0.f, eqMid = 0.f, eqFreq = 1500.f, eqHigh = 0.f;   // EQ appliquee au sample (excitation) seulement
    float glide = 0.f;
    int   unison = 2, playMode = 0;      // playMode : 0 Poly, 1 Mono, 2 Legato
    float detune = 8.f, wide = 0.6f;
    EnvParams amp, flt;
    int   fType = 0;             // 0 LP24, 1 LP12, 2 BP, 3 BR, 4 HP12, 5 HP24
    float cutoff = 8000.f, reso = 0.1f, fAmt = 0.f, fKey = 0.f;
    float vibAmt = 0.f, vibRate = 5.f, vibAtt = 0.3f;
    int   modType = 0;           // 0 off, 1 FM, 2 Wavefold
    float modAmt = 0.f, sineSemis = 0.f, sineLevel = 0.f;
    float bend = 0.f;            // demi-tons
};
