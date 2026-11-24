#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

// ------------------------------------------------------------------ parametres
static AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pid = [] (const char* s) { return ParameterID (s, 1); };

    auto F = [&] (const char* id, const char* name, float lo, float hi, float def,
                  float skew, const char* suffix)
    {
        const int dec = hi >= 1000.f ? 0 : (hi >= 100.f ? 1 : 2);
        String sfx (suffix);
        p.push_back (std::make_unique<AudioParameterFloat> (
            pid (id), name, NormalisableRange<float> (lo, hi, 0.f, skew), def,
            AudioParameterFloatAttributes().withStringFromValueFunction (
                [dec, sfx] (float v, int) { return String (v, dec) + sfx; })));
    };
    auto C = [&] (const char* id, const char* name, StringArray items, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (pid (id), name, items, def));
    };

    // sample / boucle
    C (ID::loopMode, "Loop Mode", { "Window", "Stretch" }, 0);
    F (ID::loopStart, "Loop Start", 0.f, 1.f, 0.f, 1.f, "");
    F (ID::loopSpan,  "Loop Span", 0.005f, 1.f, 0.25f, 0.5f, "");
    F (ID::loopHold,  "Loop Hold", 0.f, 1.f, 0.f, 1.f, "");

    // corde
    F (ID::decay,    "Decay Time", 0.05f, 30.f, 2.f, 0.35f, "s");
    F (ID::feedback, "Feedback", 0.f, 1.f, 0.f, 1.f, "");
    F (ID::color,    "Color", 0.f, 1.f, 0.7f, 1.f, "");
    F (ID::stiff,    "Stiffness", 0.f, 1.f, 0.f, 1.f, "");
    F (ID::pick,     "Pickup", 0.f, 1.f, 0.f, 1.f, "");
    F (ID::noise,    "Noise", 0.f, 1.f, 0.f, 1.f, "");

    // EQ de l'excitation (agit sur le sample, pas sur la sortie)
    F (ID::eqLow,  "Sample EQ Low",  -18.f, 18.f, 0.f, 1.f, "dB");
    F (ID::eqMid,  "Sample EQ Mid",  -18.f, 18.f, 0.f, 1.f, "dB");
    F (ID::eqFreq, "Sample EQ Mid Freq", 200.f, 8000.f, 1500.f, 0.4f, "Hz");
    F (ID::eqHigh, "Sample EQ High", -18.f, 18.f, 0.f, 1.f, "dB");

    // amp
    F (ID::aA, "Amp Attack",  0.001f, 10.f, 0.002f, 0.3f, "s");
    F (ID::aD, "Amp Decay",   0.001f, 10.f, 0.3f, 0.3f, "s");
    F (ID::aS, "Amp Sustain", 0.f, 1.f, 0.7f, 1.f, "");
    F (ID::aR, "Amp Release", 0.001f, 10.f, 0.2f, 0.3f, "s");
    F (ID::aAC, "Amp Attack Curve", -1.f, 1.f, 0.f, 1.f, "");
    F (ID::aDC, "Amp Decay Curve",  -1.f, 1.f, 0.5f, 1.f, "");

    // filtre
    C (ID::fType, "Filter Type", { "LP 24", "LP 12", "BP", "Band Reject", "HP 12", "HP 24" }, 0);
    F (ID::cutoff, "Cutoff", 20.f, 20000.f, 8000.f, 0.3f, "Hz");
    F (ID::reso,   "Resonance", 0.f, 1.f, 0.1f, 1.f, "");
    F (ID::fAmt,   "Filter Env Amount", -1.f, 1.f, 0.f, 1.f, "");
    F (ID::fKey,   "Filter Keytrack", 0.f, 1.f, 0.f, 1.f, "");
    F (ID::fA, "Filter Attack",  0.001f, 10.f, 0.001f, 0.3f, "s");
    F (ID::fD, "Filter Decay",   0.001f, 10.f, 0.3f, 0.3f, "s");
    F (ID::fS, "Filter Sustain", 0.f, 1.f, 0.5f, 1.f, "");
    F (ID::fR, "Filter Release", 0.001f, 10.f, 0.3f, 0.3f, "s");

    // vibrato
    F (ID::vibAmt,  "Vibrato Amount", 0.f, 100.f, 0.f, 1.f, "ct");
    F (ID::vibRate, "Vibrato Speed", 0.1f, 20.f, 5.5f, 0.5f, "Hz");
    F (ID::vibAtt,  "Vibrato Attack", 0.f, 5.f, 0.3f, 0.5f, "s");

    // sinus / modulation
    C (ID::modType, "Mod Type", { "Off", "FM", "Wavefold" }, 0);
    F (ID::modAmt, "Mod Amount", 0.f, 1.f, 0.f, 1.f, "");
    p.push_back (std::make_unique<AudioParameterInt> (pid (ID::sTune), "Sine Tune", -24, 24, 0));
    F (ID::sFine,  "Sine Fine", -100.f, 100.f, 0.f, 1.f, "ct");
    F (ID::sLevel, "Sine Level", 0.f, 1.f, 0.f, 1.f, "");

    // voix / stereo
    C (ID::playMode, "Play Mode", { "Poly", "Mono", "Legato" }, 0);
    F (ID::glide,  "Glide", 0.f, 2.f, 0.f, 0.4f, "s");
    p.push_back (std::make_unique<AudioParameterInt> (pid (ID::unison), "Unison Voices", 1, 5, 2));
    F (ID::detune, "Unison Detune", 0.f, 50.f, 8.f, 0.6f, "ct");
    F (ID::wide,   "Wide", 0.f, 1.f, 0.6f, 1.f, "");

    F (ID::gain, "Master Gain", -36.f, 6.f, -6.f, 1.f, "dB");
    C (ID::osIdx, "Oversampling", { "1x", "2x", "4x", "8x", "16x", "32x" }, 1);

    return { p.begin(), p.end() };
}

// ------------------------------------------------------------------ sweep par defaut
static std::vector<float> makeDefaultSweep (double rate)
{
    const double T = 0.30, f0 = 9000.0, f1 = 90.0;
    const int n = (int) (T * rate);
    const double lr = std::log (f1 / f0);
    std::vector<float> d ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / rate;
        const double phase = 6.283185307 * f0 * T / lr * (std::exp (t / T * lr) - 1.0);
        const double env = jmin (1.0, t / 0.002) * std::exp (-6.0 * t / T);
        d[(size_t) i] = (float) (std::tanh (2.5 * std::sin (phase)) * env);
    }
    return d;
}

// ------------------------------------------------------------------ construction
WarpusKongProcessor::WarpusKongProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
   #define BIND(x) pr.x = apvts.getRawParameterValue (ID::x);
    BIND(loopMode) BIND(loopStart) BIND(loopSpan) BIND(loopHold) BIND(decay) BIND(feedback) BIND(color)
    BIND(stiff) BIND(pick) BIND(noise) BIND(eqLow) BIND(eqMid) BIND(eqFreq) BIND(eqHigh)
    BIND(aA) BIND(aD) BIND(aS) BIND(aR) BIND(aAC) BIND(aDC)
    BIND(fType) BIND(cutoff) BIND(reso) BIND(fAmt) BIND(fKey) BIND(fA) BIND(fD) BIND(fS) BIND(fR)
    BIND(vibAmt) BIND(vibRate) BIND(vibAtt) BIND(modType) BIND(modAmt) BIND(sTune) BIND(sFine) BIND(sLevel)
    BIND(glide) BIND(unison) BIND(detune) BIND(wide) BIND(playMode) BIND(gain) BIND(osIdx)
   #undef BIND

    noteStack.reserve (256);
    setSample (makeDefaultSweep (44100.0), 44100.0, "Laser Sweep (default)", true);
}

bool WarpusKongProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::stereo() || out == AudioChannelSet::mono());
}

void WarpusKongProcessor::prepareToPlay (double sr, int block)
{
    hostSr = sr;
    hostSampleRate = sr;
    maxBlock = jmax (16, block);
    bufL.assign ((size_t) maxBlock, 0.f);
    bufR.assign ((size_t) maxBlock, 0.f);
    dummy.assign ((size_t) maxBlock, 0.f);
    for (int i = 0; i < 5; ++i)
    {
        oversamplers[(size_t) i] = std::make_unique<dsp::Oversampling<float>> (
            2, (size_t) (i + 1), dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversamplers[(size_t) i]->initProcessing ((size_t) maxBlock);
    }
    sustainDown = false;
    noteStack.clear();
    lastSemis = -1.0;
    applyOversampling ((int) pr.osIdx->load());
    gainSm.reset (sr, 0.02);
    gainSm.setCurrentAndTargetValue (Decibels::decibelsToGain (pr.gain->load()));
}

void WarpusKongProcessor::applyOversampling (int idx)
{
    curOsIdx = idx;
    const int f = 1 << idx;
    for (auto& v : voices) v.prepare (hostSr, f);
    noteStack.clear();
    if (idx > 0)
    {
        oversamplers[(size_t) (idx - 1)]->reset();
        setLatencySamples (roundToInt (oversamplers[(size_t) (idx - 1)]->getLatencyInSamples()));
    }
    else
        setLatencySamples (0);
}

// ------------------------------------------------------------------ reglages
void WarpusKongProcessor::readSettings()
{
    cfg.loopMode  = (int) pr.loopMode->load();
    cfg.loopStart = pr.loopStart->load();
    cfg.loopSpan  = pr.loopSpan->load();
    cfg.hold      = pr.loopHold->load();
    cfg.decay     = pr.decay->load();
    cfg.feedback  = pr.feedback->load();
    cfg.color     = pr.color->load();
    cfg.stiff     = pr.stiff->load();
    cfg.pick      = pr.pick->load();
    cfg.noise     = pr.noise->load();
    cfg.eqLow     = pr.eqLow->load();
    cfg.eqMid     = pr.eqMid->load();
    cfg.eqFreq    = pr.eqFreq->load();
    cfg.eqHigh    = pr.eqHigh->load();
    cfg.amp = { pr.aA->load(), pr.aD->load(), pr.aS->load(), pr.aR->load(), pr.aAC->load(), pr.aDC->load() };
    cfg.flt = { pr.fA->load(), pr.fD->load(), pr.fS->load(), pr.fR->load(), 0.f, 0.f };
    cfg.fType  = (int) pr.fType->load();
    cfg.cutoff = pr.cutoff->load();
    cfg.reso   = pr.reso->load();
    cfg.fAmt   = pr.fAmt->load();
    cfg.fKey   = pr.fKey->load();
    cfg.vibAmt  = pr.vibAmt->load();
    cfg.vibRate = pr.vibRate->load();
    cfg.vibAtt  = pr.vibAtt->load();
    cfg.modType = (int) pr.modType->load();
    cfg.modAmt  = pr.modAmt->load();
    cfg.sineSemis = pr.sTune->load() + pr.sFine->load() * 0.01f;
    cfg.sineLevel = pr.sLevel->load();
    cfg.glide    = pr.glide->load();
    cfg.unison   = jlimit (1, 5, (int) pr.unison->load());
    cfg.detune   = pr.detune->load();
    cfg.wide     = pr.wide->load();
    cfg.playMode = (int) pr.playMode->load();
}

// ------------------------------------------------------------------ MIDI / voix
// legatoJoin : une note est deja tenue (modes Mono / Legato)
void WarpusKongProcessor::triggerNote (int note, float vel, bool legatoJoin)
{
    const int u = cfg.unison;
    const bool glideOn = cfg.glide > 0.001f;
    const bool monoMode = cfg.playMode != 0;
    lastNote = note;
    lastVel = vel;

    std::array<Voice*, 5> chosen {};
    std::array<bool, 5> reused {};
    int nc = 0;
    auto taken = [&] (Voice* v) { for (int k = 0; k < nc; ++k) if (chosen[(size_t) k] == v) return true; return false; };
    auto findFree = [&]() -> Voice*
    {
        Voice* t = nullptr;
        for (auto& v : voices) if (! v.active && ! taken (&v)) return &v;
        for (auto& v : voices)                      // vol : voix la plus faible en release
            if (! taken (&v) && ! v.keyDown && ! v.sustained && (t == nullptr || v.ampCur < t->ampCur)) t = &v;
        if (t == nullptr)
            for (auto& v : voices) if (! taken (&v) && (t == nullptr || v.age < t->age)) t = &v;
        return t;
    };

    if (! monoMode)
    {
        for (int i = 0; i < u; ++i)
        {
            Voice* t = nullptr;
            bool re = false;
            for (auto& v : voices)
                if (v.active && ! v.mono && v.note == note && ! taken (&v)) { t = &v; re = true; break; }
            if (t == nullptr) t = findFree();
            chosen[(size_t) nc] = t; reused[(size_t) nc] = re; ++nc;
        }
    }
    else
    {
        Voice* monoV[5]; int nm = 0;
        for (auto& v : voices)
            if (v.mono && v.active)
            {
                if (nm < u) monoV[nm++] = &v;
                else { v.keyDown = false; v.sustained = false; v.release(); }
            }

        if (legatoJoin && cfg.playMode == 2 && nm > 0)   // LEGATO : on glisse sans re-pincer
        {
            for (int i = 0; i < nm; ++i) monoV[i]->retarget (note);
            lastSemis = (double) note;
            return;
        }
        for (int i = 0; i < nm; ++i) { chosen[(size_t) nc] = monoV[i]; reused[(size_t) nc] = true; ++nc; }
        while (nc < u) { chosen[(size_t) nc] = findFree(); reused[(size_t) nc] = false; ++nc; }
    }

    for (int i = 0; i < u; ++i)
    {
        Voice* v = chosen[(size_t) i];
        if (v == nullptr) continue;
        // repartition des copies d'unison : detune et panoramique de -1 a +1
        const float pos = u == 1 ? 0.f : ((float) i / (float) (u - 1)) * 2.f - 1.f;
        const float th = (pos * cfg.wide + 1.f) * 0.78539816f;          // 0..pi/2 (loi de panoramique constante)
        const float trim = 1.41421356f / std::sqrt ((float) u);
        const double startSemis = reused[(size_t) i] && v->active ? v->curSemis
                                : (glideOn && lastSemis >= 0.0 ? lastSemis : -1.0);
        v->noteOn (note, vel, audioSample, ++voiceCounter, glideOn ? startSemis : -1.0, reused[(size_t) i],
                   std::cos (th) * trim, std::sin (th) * trim, (double) pos * cfg.detune, monoMode);
    }
    lastSemis = (double) note;
}

void WarpusKongProcessor::noteOff (int note)
{
    if (cfg.playMode == 0)
    {
        for (auto& v : voices)
            if (v.active && v.keyDown && ! v.mono && v.note == note) v.noteOff (sustainDown);
        return;
    }

    const bool wasTop = ! noteStack.empty() && noteStack.back() == note;
    noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), note), noteStack.end());

    if (noteStack.empty())
    {
        for (auto& v : voices) if (v.active && v.mono && v.keyDown) v.noteOff (sustainDown);
    }
    else if (wasTop)    // on revient a la note precedemment tenue
        triggerNote (noteStack.back(), lastVel, true);
}

void WarpusKongProcessor::releaseAll()
{
    for (auto& v : voices) if (v.active) { v.keyDown = false; v.sustained = false; v.release(); }
    noteStack.clear();
}

void WarpusKongProcessor::handleMidi (const MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        if (cfg.playMode == 0) triggerNote (n, m.getFloatVelocity(), false);
        else
        {
            const bool held = ! noteStack.empty();
            noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), n), noteStack.end());
            noteStack.push_back (n);
            triggerNote (n, m.getFloatVelocity(), held);
        }
    }
    else if (m.isNoteOff())      noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) releaseAll();
    else if (m.isPitchWheel())   bendSemis = (float) (m.getPitchWheelValue() - 8192) / 8192.f * 2.f;
    else if (m.isController() && m.getControllerNumber() == 64)
    {
        const bool down = m.getControllerValue() >= 64;
        if (sustainDown && ! down)
            for (auto& v : voices) if (v.active && v.sustained) { v.sustained = false; v.release(); }
        sustainDown = down;
    }
}

void WarpusKongProcessor::renderRange (float* l, float* r, int count)
{
    if (count <= 0) return;
    FloatVectorOperations::clear (l, count);
    FloatVectorOperations::clear (r, count);
    cfg.bend = bendSemis;
    for (auto& v : voices)
        if (v.active) v.render (l, r, count, cfg);
}

void WarpusKongProcessor::renderChunk (AudioBuffer<float>& buffer, MidiBuffer& midi, int start, int count)
{
    const int factor = 1 << curOsIdx;
    const bool stereoOut = buffer.getNumChannels() > 1;
    float* ch0 = buffer.getWritePointer (0) + start;
    float* ch1 = stereoOut ? buffer.getWritePointer (1) + start : dummy.data();

    float* wl = bufL.data();
    float* wr = bufR.data();
    dsp::AudioBlock<float> up;

    if (curOsIdx > 0)
    {
        std::fill (bufL.begin(), bufL.begin() + count, 0.f);
        std::fill (bufR.begin(), bufR.begin() + count, 0.f);
        const float* in[2] = { bufL.data(), bufR.data() };
        dsp::AudioBlock<const float> inBlock (in, 2, (size_t) count);
        up = oversamplers[(size_t) (curOsIdx - 1)]->processSamplesUp (inBlock);
        wl = up.getChannelPointer (0);
        wr = up.getChannelPointer (1);
    }

    const int total = count * factor;
    int cur = 0;
    for (auto it = midi.findNextSamplePosition (start); it != midi.end(); ++it)
    {
        const auto meta = *it;
        if (meta.samplePosition >= start + count) break;
        const int pos = (meta.samplePosition - start) * factor;
        renderRange (wl + cur, wr + cur, pos - cur);
        cur = pos;
        handleMidi (meta.getMessage());
    }
    renderRange (wl + cur, wr + cur, total - cur);

    if (curOsIdx > 0)
    {
        float* out[2] = { ch0, ch1 };
        dsp::AudioBlock<float> outBlock (out, 2, (size_t) count);
        oversamplers[(size_t) (curOsIdx - 1)]->processSamplesDown (outBlock);
    }
    else
    {
        FloatVectorOperations::copy (ch0, wl, count);
        FloatVectorOperations::copy (ch1, wr, count);
    }

    if (! stereoOut)   // sortie mono : somme des deux canaux
        for (int i = 0; i < count; ++i) ch0[i] = 0.5f * (ch0[i] + ch1[i]);

    for (int i = 0; i < count; ++i)
    {
        const float gn = gainSm.getNextValue();
        ch0[i] *= gn;
        if (stereoOut) ch1[i] *= gn;
    }
}

void WarpusKongProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    buffer.clear();

    {
        SpinLock::ScopedTryLockType l (sampleLock);
        if (l.isLocked()) audioSample = currentSample;
    }

    readSettings();
    if (cfg.playMode != lastMode)           // changement poly <-> mono : on relache tout proprement
    {
        if (lastMode != -1) releaseAll();
        lastMode = cfg.playMode;
    }
    const int osIdx = jlimit (0, 5, (int) pr.osIdx->load());
    if (osIdx != curOsIdx) applyOversampling (osIdx);
    gainSm.setTargetValue (Decibels::decibelsToGain (pr.gain->load()));

    for (int start = 0; start < total; start += maxBlock)
        renderChunk (buffer, midi, start, jmin (maxBlock, total - start));

    midi.clear();
}

// ------------------------------------------------------------------ sample
std::shared_ptr<const SampleData> WarpusKongProcessor::getSample() const
{
    const SpinLock::ScopedLockType l (sampleLock);
    return currentSample;
}

void WarpusKongProcessor::setSample (std::vector<float>&& d, double rate, const String& name, bool embed)
{
    auto s = std::make_shared<SampleData>();
    s->data = std::move (d);
    s->rate = rate;
    s->name = name;

    float peak = 0.f;
    for (auto v : s->data) peak = jmax (peak, std::abs (v));
    if (peak > 1.0e-6f) for (auto& v : s->data) v /= peak;

    if (embed) // le sample est integre a l'etat : le projet reste portable
    {
        MemoryBlock mb (s->data.size() * sizeof (int16));
        auto* o = static_cast<int16*> (mb.getData());
        for (size_t i = 0; i < s->data.size(); ++i)
            o[i] = (int16) jlimit (-32767, 32767, roundToInt (s->data[i] * 32767.f));
        apvts.state.setProperty ("sampleData", var (mb), nullptr);
        apvts.state.setProperty ("sampleRate", rate, nullptr);
        apvts.state.setProperty ("sampleName", name, nullptr);
    }

    {
        const SpinLock::ScopedLockType l (sampleLock);
        if (currentSample) { retired.push_back (currentSample); if (retired.size() > 4) retired.erase (retired.begin()); }
        currentSample = s;
    }
    ++sampleVersion;
}

bool WarpusKongProcessor::loadSampleFile (const File& f)
{
    AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr) return false;

    const int n = (int) jmin<int64> (r->lengthInSamples, (int64) (r->sampleRate * 20.0)); // 20 s max
    if (n < 16) return false;
    const int nch = (int) r->numChannels;
    AudioBuffer<float> b (nch, n);
    r->read (&b, 0, n, 0, true, true);

    std::vector<float> d ((size_t) n, 0.f);
    for (int ch = 0; ch < nch; ++ch)
    {
        auto* src = b.getReadPointer (ch);
        for (int i = 0; i < n; ++i) d[(size_t) i] += src[i] / (float) nch;
    }
    setSample (std::move (d), r->sampleRate, f.getFileNameWithoutExtension(), true);
    return true;
}

void WarpusKongProcessor::loadSampleFromState()
{
    if (auto* mb = apvts.state.getProperty ("sampleData").getBinaryData())
    {
        const size_t n = mb->getSize() / sizeof (int16);
        if (n < 16) return;
        auto* in = static_cast<const int16*> (mb->getData());
        std::vector<float> d (n);
        for (size_t i = 0; i < n; ++i) d[i] = (float) in[i] / 32767.f;
        setSample (std::move (d), (double) apvts.state.getProperty ("sampleRate", 44100.0),
                   apvts.state.getProperty ("sampleName", "Sample").toString(), false);
    }
}

void WarpusKongProcessor::getLoopRegion (int note, double& s, double& e) const
{
    auto smp = getSample();
    s = e = 0.0;
    if (! smp || smp->data.empty()) return;
    const double len = (double) smp->data.size();
    s = (double) apvts.getRawParameterValue (ID::loopStart)->load();
    if ((int) apvts.getRawParameterValue (ID::loopMode)->load() == 0)
        e = s + (smp->rate / MidiMessage::getMidiNoteInHertz (note)) / len;   // une periode, vitesse native
    else
        e = jmin (1.0, s + (double) apvts.getRawParameterValue (ID::loopSpan)->load());
}

String WarpusKongProcessor::getInfoText() const
{
    auto smp = getSample();
    if (! smp) return "NO SAMPLE";
    const int note = lastNote.load();
    const double f = MidiMessage::getMidiNoteInHertz (note);
    return smp->name + "  " + String ((double) smp->data.size() / smp->rate, 2) + "s  "
         + String (smp->rate / 1000.0, 1) + "kHz   |   "
         + MidiMessage::getMidiNoteName (note, true, true, 3) + " " + String (f, 1) + "Hz  loop = "
         + String (roundToInt (hostSampleRate.load() / f)) + " smp";
}

// ------------------------------------------------------------------ etat (tous les reglages + le sample)
void WarpusKongProcessor::getStateInformation (MemoryBlock& dest)
{
    MemoryOutputStream mos (dest, false);
    apvts.copyState().writeToStream (mos);
}

void WarpusKongProcessor::setStateInformation (const void* data, int size)
{
    auto tree = ValueTree::readFromData (data, (size_t) size);
    if (tree.isValid() && tree.hasType (apvts.state.getType()))
    {
        apvts.replaceState (tree);
        loadSampleFromState();
    }
}

AudioProcessorEditor* WarpusKongProcessor::createEditor() { return new WarpusKongEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WarpusKongProcessor(); }
