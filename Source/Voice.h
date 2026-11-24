#pragma once
#include "Common.h"
#include <cmath>

// ---------------------------------------------------------------- utilitaires DSP
inline float fastSin (float x) // x dans [-pi, pi]
{
    float y = 1.27323954f * x - 0.405284735f * x * std::abs (x);
    return 0.225f * (y * std::abs (y) - y) + y;
}
inline float foldSin (float x)
{
    x -= 6.2831853f * std::floor ((x + 3.14159265f) * 0.15915494f);
    return fastSin (x);
}
inline float softClip (float x)
{
    x = jlimit (-3.f, 3.f, x);
    return x * (27.f + x * x) / (27.f + 9.f * x * x);
}
inline float hermite (const float* b, int mask, float pos)
{
    int i = (int) std::floor (pos);
    float f = pos - (float) i;
    float y0 = b[(i - 1) & mask], y1 = b[i & mask], y2 = b[(i + 1) & mask], y3 = b[(i + 2) & mask];
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + y1;
}
inline float readLin (const float* d, int len, double pos)
{
    if (pos < 0.0 || pos >= (double) (len - 1)) return 0.f;
    int i = (int) pos;
    float f = (float) (pos - i);
    return d[i] + f * (d[i + 1] - d[i]);
}

// Biquad (RBJ) pour l'EQ de l'excitation
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float tick (float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void set (float B0, float B1, float B2, float A0, float A1, float A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    void setPeak (double sr, double f, float dB, float Q)
    {
        const float A = std::pow (10.f, dB / 40.f), w = (float) (6.283185307 * f / sr);
        const float cs = std::cos (w), al = std::sin (w) / (2.f * Q);
        set (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }
    void setLowShelf (double sr, double f, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f), w = (float) (6.283185307 * f / sr);
        const float cs = std::cos (w), sn = std::sin (w);
        const float beta = 2.f * std::sqrt (A) * (sn * 0.5f * 1.41421356f);
        set (A * ((A + 1) - (A - 1) * cs + beta), 2 * A * ((A - 1) - (A + 1) * cs),
             A * ((A + 1) - (A - 1) * cs - beta),
             (A + 1) + (A - 1) * cs + beta, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - beta);
    }
    void setHighShelf (double sr, double f, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f), w = (float) (6.283185307 * f / sr);
        const float cs = std::cos (w), sn = std::sin (w);
        const float beta = 2.f * std::sqrt (A) * (sn * 0.5f * 1.41421356f);
        set (A * ((A + 1) + (A - 1) * cs + beta), -2 * A * ((A - 1) + (A + 1) * cs),
             A * ((A + 1) + (A - 1) * cs - beta),
             (A + 1) - (A - 1) * cs + beta, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - beta);
    }
};

// Enveloppe ADSR avec courbes d'attaque / decay
struct Env
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage st = Idle;
    double ph = 0.0;
    float level = 0.f, startLevel = 0.f, relLevel = 0.f;

    bool isIdle() const { return st == Idle; }
    void kill() { st = Idle; ph = 0; level = 0; }
    void noteOn()  { st = Attack; ph = 0; startLevel = level; }
    void noteOff() { if (st != Idle) { st = Release; ph = 0; relLevel = level; } }

    float advance (const EnvParams& p, double dt)
    {
        switch (st)
        {
            case Attack:
                ph += dt / jmax (p.a, 0.0005f);
                if (ph >= 1.0) { st = Decay; ph = 0; level = 1.f; }
                else level = startLevel + (1.f - startLevel) * (float) std::pow (ph, std::pow (4.0, (double) p.ac));
                break;
            case Decay:
                ph += dt / jmax (p.d, 0.001f);
                if (ph >= 1.0) { st = Sustain; level = p.s; }
                else level = p.s + (1.f - p.s) * (float) std::pow (1.0 - ph, std::pow (4.0, (double) p.dc));
                break;
            case Sustain: level = p.s; break;
            case Release:
                ph += dt / jmax (p.r, 0.001f);
                if (ph >= 1.0) { st = Idle; level = 0.f; }
                else level = relLevel * (float) ((1.0 - ph) * (1.0 - ph));
                break;
            default: level = 0.f; break;
        }
        return level;
    }
};

// Filtre a variables d'etat (TPT)
struct Svf
{
    float ic1 = 0.f, ic2 = 0.f;
    void reset() { ic1 = ic2 = 0.f; }
    inline void tick (float x, float a1, float a2, float a3, float k,
                      float& lo, float& bp, float& hi, float& nt)
    {
        float v3 = x - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1;
        ic2 = 2.f * v2 - ic2;
        lo = v2; bp = v1; hi = x - k * v1 - v2; nt = x - k * v1;
    }
};

// ---------------------------------------------------------------- voix
struct Voice
{
    bool active = false, keyDown = false, sustained = false, wasIdle = false, mono = false;
    int note = -1;
    float vel = 1.f;
    uint64_t age = 0;
    std::shared_ptr<const SampleData> smp;

    std::vector<float> dl, fm;
    int dlMask = 0, fmMask = 0, Kmax = 0, written = 0;
    double sr = 44100.0, invSr = 1.0 / 44100.0;
    int osFactor = 1, interval = 16, ctr = 0;

    // pitch / unison / stereo
    double curSemis = 60.0, detuneCents = 0.0;
    float gL = 0.707f, gR = 0.707f;

    // corde
    int w = 0, fmW = 0, N = 10, pickM = 0;
    float lp = 0.f, apX = 0.f, apY = 0.f, apA = 0.f, c = 0.f, g = 0.99f, holdNorm = 1.f;
    float dX = 0.f, dY = 0.f, dA = 0.f; bool dispOn = false;
    float ph = 0.f, P = 100.f, invP = 0.01f;
    int periods = 0;
    double startPos = 0.0, ratio = 1.0, spanLen = 1.0;
    uint32_t rng = 0x12345678u;

    // EQ d'excitation
    Biquad eqLo, eqMi, eqHi;
    bool eqOn = false;
    float eqKey = -1e30f;

    // sinus / vibrato / filtre / enveloppes
    float sinePh = 0.f, sineInc = 0.f;
    double vibPh = 0.0, vibT = 0.0;
    Svf f1, f2;
    float fa1 = 0, fa2 = 0, fa3 = 0, fk = 1;
    Env ampEnv, fltEnv;
    float ampCur = 0.f, ampInc = 0.f;

    // ---- allocation (hors temps reel)
    void prepare (double hostSr, int os)
    {
        osFactor = os;
        sr = hostSr * os;
        invSr = 1.0 / sr;
        interval = 16 * os;
        const int dlSize = nextPowerOfTwo ((int) std::ceil (2.2 * sr / 20.0) + 64);
        const int fmSize = nextPowerOfTwo ((int) std::ceil (0.007 * sr) + 64);
        dl.assign ((size_t) dlSize, 0.f);  dlMask = dlSize - 1;
        fm.assign ((size_t) fmSize, 0.f);  fmMask = fmSize - 1;
        written = 0;
        eqKey = -1e30f;
        kill();
    }

    void kill()
    {
        active = keyDown = sustained = mono = false;
        ampEnv.kill(); fltEnv.kill();
        ampCur = ampInc = 0.f;
        smp.reset();
    }

    static double noteHz (double semis) { return 440.0 * std::pow (2.0, (semis - 69.0) / 12.0); }

    void noteOn (int n, float v, std::shared_ptr<const SampleData> s, uint64_t a,
                 double startSemis, bool keepString, float panL, float panR,
                 double detune, bool isMono)
    {
        note = n; vel = v; smp = std::move (s); age = a; mono = isMono;
        gL = panL; gR = panR; detuneCents = detune;
        curSemis = startSemis >= 0.0 ? startSemis : (double) n;
        active = true; keyDown = true; sustained = false; wasIdle = false;

        const double fmin = jmin (noteHz ((double) n), noteHz (curSemis));
        const double P0 = sr / jlimit (20.0, sr * 0.25, fmin);
        Kmax = jmin ((int) std::ceil (P0 * 2.0) + 16, dlMask - 8);

        // on garde la corde en cours (re-pincement) si son historique est assez long
        if (! (keepString && written >= Kmax))
        {
            std::fill (dl.begin(), dl.begin() + Kmax, 0.f);
            w = Kmax; written = Kmax;
            lp = apX = apY = dX = dY = 0.f;
        }
        std::fill (fm.begin(), fm.end(), 0.f);
        fmW = 0;
        ph = 0.f; periods = 0;
        sinePh = 0.f; vibPh = 0.0; vibT = 0.0;
        f1.reset(); f2.reset(); eqLo.reset(); eqMi.reset(); eqHi.reset();
        ampEnv.noteOn(); fltEnv.noteOn();
        ctr = 0;
    }

    void retarget (int n) { note = n; keyDown = true; }   // legato : pas de nouveau pincement
    void release() { ampEnv.noteOff(); fltEnv.noteOff(); }
    void noteOff (bool pedalDown) { keyDown = false; if (pedalDown) sustained = true; else release(); }

    inline float noise()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (int32_t) rng * 4.656612875e-10f;
    }

    // ---- controle (toutes les `interval` samples)
    void updateControl (const Settings& s)
    {
        const double dt = interval * invSr;
        const float amp = ampEnv.advance (s.amp, dt);
        const float fe  = fltEnv.advance (s.flt, dt);
        ampInc = (amp - ampCur) / (float) interval;

        if (ampEnv.isIdle() && wasIdle) { active = false; return; }
        wasIdle = ampEnv.isIdle();

        // glide (portamento) : approche exponentielle de la note cible
        if (s.glide > 0.001f)
            curSemis += ((double) note - curSemis) * (1.0 - std::exp (-dt / (s.glide / 3.0)));
        else
            curSemis = (double) note;

        // vibrato
        vibT += dt;
        vibPh += 6.283185307 * s.vibRate * dt;
        if (vibPh > 6.283185307) vibPh -= 6.283185307;
        const float vl = s.vibAtt > 0.001f ? jmin (1.f, (float) vibT / s.vibAtt) : 1.f;
        const double cents = s.vibAmt * std::sin (vibPh) * vl + detuneCents;

        const double semis = curSemis + s.bend + cents * 0.01;
        const double freq = jlimit (20.0, sr * 0.25, noteHz (semis));

        // ---- corde Karplus-Strong
        P = (float) (sr / freq);
        invP = 1.f / P;
        c = (1.f - s.color) * 0.96f;                                   // COLOR : sombre <-> brillant
        const float gT = 0.99997f;
        const float gBase = jmin (gT, std::exp (-6.9077553f / (jmax (0.02f, s.decay) * (float) freq)));
        g = gBase + s.feedback * (gT - gBase);                         // FEEDBACK : vers le gain unitaire
        holdNorm = jmax (0.002f, 1.f - g);

        dispOn = s.stiff > 0.001f;                                     // STIFFNESS : dispersion (allpass)
        dA = -s.stiff * 0.8f;
        const float dispDelay = dispOn ? (1.f - dA) / (1.f + dA) : 0.f;

        float Dd = P - c / (1.f - c) - dispDelay;                      // compense les retards de boucle
        const int lim = jmin (written, dlMask - 8);
        Dd = jlimit (1.6f, (float) (lim - 4), Dd);
        N = (int) std::floor (Dd - 0.5f);
        const float frac = Dd - (float) N;
        apA = (1.f - frac) / (1.f + frac);
        pickM = s.pick > 0.001f ? jlimit (1, jmax (1, N - 1), (int) ((0.02f + 0.48f * s.pick) * (float) N)) : 0;

        if (smp)
        {
            const double len = (double) smp->data.size();
            startPos = s.loopStart * len;
            ratio = smp->rate / sr;
            spanLen = jmin ((double) s.loopSpan, 1.0 - (double) s.loopStart) * len;
        }

        // ---- EQ appliquee uniquement a l'excitation (le sample), pas au son final
        const float key = s.eqLow * 1.37f + s.eqMid * 3.1f + s.eqFreq * 0.001f + s.eqHigh * 7.7f + (float) sr * 1e-6f;
        if (key != eqKey)
        {
            eqKey = key;
            eqOn = std::abs (s.eqLow) > 0.05f || std::abs (s.eqMid) > 0.05f || std::abs (s.eqHigh) > 0.05f;
            if (eqOn)
            {
                eqLo.setLowShelf (sr, 250.0, s.eqLow);
                eqMi.setPeak (sr, jmin ((double) s.eqFreq, sr * 0.4), s.eqMid, 1.f);
                eqHi.setHighShelf (sr, jmin (4000.0, sr * 0.4), s.eqHigh);
            }
        }

        // ---- sinus
        double sf = freq * std::pow (2.0, s.sineSemis / 12.0);
        sineInc = sf < sr * 0.45 ? (float) (6.283185307 * sf / sr) : 0.f;

        // ---- filtre
        double fc = s.cutoff * std::pow (2.0, (double) (s.fAmt * 6.f * fe) + s.fKey * (note - 60) / 12.0);
        fc = jlimit (20.0, jmin (20000.0, sr * 0.45), fc);
        const float gg = (float) std::tan (3.14159265358979 * fc / sr);
        fk = 2.f * (1.f - s.reso) + 0.05f;
        fa1 = 1.f / (1.f + gg * (gg + fk));
        fa2 = gg * fa1;
        fa3 = gg * fa2;
    }

    // ---- rendu stereo (additionne dans L et R)
    void render (float* outL, float* outR, int n, const Settings& s)
    {
        const float* sd = smp ? smp->data.data() : nullptr;
        const int slen = smp ? (int) smp->data.size() : 0;
        const int cap = dlMask - 8;

        for (int i = 0; i < n; ++i)
        {
            if (ctr <= 0)
            {
                updateControl (s);
                ctr = interval;
                if (! active) return;
            }
            --ctr;
            ampCur += ampInc;

            // excitation : sample lu sur UNE periode + bruit optionnel, puis EQ d'excitation
            float exc = 0.f;
            if (sd != nullptr && (periods == 0 || s.hold > 0.0005f))
            {
                const double pos = (s.loopMode == 0) ? startPos + (double) ph * ratio
                                                     : startPos + (double) (ph * invP) * spanLen;
                exc = readLin (sd, slen, pos) * (periods == 0 ? 1.f : s.hold * holdNorm) * vel;
            }
            if (periods == 0 && s.noise > 0.001f) exc += s.noise * noise() * vel;
            if (eqOn) exc = eqLo.tick (eqMi.tick (eqHi.tick (exc)));

            ph += 1.f;
            while (ph >= P) { ph -= P; if (periods < 1000000) ++periods; }

            // boucle Karplus-Strong
            const float x = dl[(size_t) ((w - N) & dlMask)];
            float y = apA * (x - apY) + apX;
            apX = x; apY = y;
            lp = (1.f - c) * y + c * lp;
            float loopOut = lp;
            if (dispOn) { const float yd = dA * (loopOut - dY) + dX; dX = loopOut; dY = yd; loopOut = yd; }

            float yo = y;
            if (pickM > 0) yo = (y - dl[(size_t) ((w - pickM) & dlMask)]) * 0.7071f;   // position du micro

            dl[(size_t) w] = exc + loopOut * g;
            w = (w + 1) & dlMask;
            if (written < cap) ++written;
            y = yo;

            // sinus + modulation
            const float sn = fastSin (sinePh);
            sinePh += sineInc;
            if (sinePh > 3.14159265f) sinePh -= 6.2831853f;

            if (s.modType == 1)
            {
                fm[(size_t) fmW] = y;
                fmW = (fmW + 1) & fmMask;
                const float depth = s.modAmt * s.modAmt * 0.003f * (float) sr;
                y = hermite (fm.data(), fmMask, (float) fmW - (depth * (1.f + sn) + 4.f));
            }
            else if (s.modType == 2)
            {
                y = foldSin (y * (1.f + 6.f * s.modAmt) + sn * s.modAmt * 2.f);
            }
            y += sn * s.sineLevel;

            // filtre
            float lo, bp, hi, nt, o = y;
            switch (s.fType)
            {
                case 0: f1.tick (y, fa1, fa2, fa3, fk, lo, bp, hi, nt);
                        f2.tick (lo, fa1, fa2, fa3, fk, lo, bp, hi, nt); o = lo; break;
                case 1: f1.tick (y, fa1, fa2, fa3, fk, lo, bp, hi, nt); o = lo; break;
                case 2: f1.tick (y, fa1, fa2, fa3, fk, lo, bp, hi, nt); o = bp * fk; break;
                case 3: f1.tick (y, fa1, fa2, fa3, fk, lo, bp, hi, nt); o = nt; break;
                case 4: f1.tick (y, fa1, fa2, fa3, fk, lo, bp, hi, nt); o = hi; break;
                default: f1.tick (y, fa1, fa2, fa3, fk, lo, bp, hi, nt);
                         f2.tick (hi, fa1, fa2, fa3, fk, lo, bp, hi, nt); o = hi; break;
            }

            const float v = softClip (o) * ampCur;
            outL[i] += v * gL;
            outR[i] += v * gR;
        }
    }
};
