#include "PluginEditor.h"

static const Colour kBlack = Colours::black, kWhite = Colours::white;

// ======================================================================= look & feel
Font BWLookAndFeel::mono (float h, bool bold)
{
    return Font (FontOptions (Font::getDefaultMonospacedFontName(), h, bold ? Font::bold : Font::plain));
}

BWLookAndFeel::BWLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, kWhite);
    setColour (Label::textColourId, kBlack);
    setColour (Label::outlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxTextColourId, kBlack);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (ComboBox::backgroundColourId, kWhite);
    setColour (ComboBox::textColourId, kBlack);
    setColour (ComboBox::outlineColourId, kBlack);
    setColour (ComboBox::arrowColourId, kBlack);
    setColour (PopupMenu::backgroundColourId, kWhite);
    setColour (PopupMenu::textColourId, kBlack);
    setColour (PopupMenu::highlightedBackgroundColourId, kBlack);
    setColour (PopupMenu::highlightedTextColourId, kWhite);
    setColour (TextButton::buttonColourId, kWhite);
    setColour (TextButton::textColourOffId, kBlack);
    setColour (TextButton::textColourOnId, kWhite);
}

void BWLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                      float a0, float a1, Slider&)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (7.f);
    const float d = jmin (b.getWidth(), b.getHeight());
    auto c = b.getCentre();
    const float r = d * 0.5f;

    g.setColour (kBlack);
    for (int i = 0; i <= 10; ++i) // graduations
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.f;
        g.drawLine (c.x + std::sin (a) * (r + 2.f), c.y - std::cos (a) * (r + 2.f),
                    c.x + std::sin (a) * (r + 5.f), c.y - std::cos (a) * (r + 5.f), 1.f);
    }
    g.setColour (kWhite);
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    g.setColour (kBlack);
    g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.5f);

    const float a = a0 + pos * (a1 - a0);
    g.drawLine (c.x + std::sin (a) * r * 0.2f, c.y - std::cos (a) * r * 0.2f,
                c.x + std::sin (a) * r * 0.92f, c.y - std::cos (a) * r * 0.92f, 2.f);
}

Label* BWLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (9.f));
    l->setColour (Label::textColourId, kBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    return l;
}

void BWLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&)
{
    g.fillAll (kWhite);
    g.setColour (kBlack);
    g.drawRect (0, 0, w, h, 1);
    g.fillRect (w - 2, 2, 2, h - 2);   // ombre
    g.fillRect (2, h - 2, w - 2, 2);
    Path p; // petit triangle plein
    p.addTriangle ((float) w - 14.f, (float) h * 0.4f, (float) w - 6.f, (float) h * 0.4f, (float) w - 10.f, (float) h * 0.65f);
    g.fillPath (p);
}

void BWLookAndFeel::positionComboBoxText (ComboBox& b, Label& l)
{
    l.setBounds (3, 1, b.getWidth() - 20, b.getHeight() - 3);
    l.setFont (mono (10.f));
}

void BWLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool, bool down)
{
    auto r = b.getLocalBounds();
    g.setColour (down ? kBlack : kWhite);
    g.fillRect (r.withTrimmedRight (2).withTrimmedBottom (2));
    g.setColour (kBlack);
    g.drawRect (r.withTrimmedRight (2).withTrimmedBottom (2), 1);
    g.fillRect (r.getRight() - 2, r.getY() + 2, 2, r.getHeight() - 2);
    g.fillRect (r.getX() + 2, r.getBottom() - 2, r.getWidth() - 2, 2);
}

void BWLookAndFeel::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (kWhite);
    g.setColour (kBlack);
    g.drawRect (0, 0, w, h, 1);
}

// ======================================================================= waveform
void WaveformView::paint (Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    g.fillAll (kWhite);
    g.setColour (kBlack);
    for (int x = 0; x < W; x += 4) g.fillRect (x, H / 2, 1, 1);   // ligne zero pointillee

    auto smp = proc.getSample();
    if (smp && ! smp->data.empty())
    {
        const auto& d = smp->data;
        const double len = (double) d.size();
        double rs, re;
        proc.getLoopRegion (proc.lastNote.load(), rs, re);

        double lo = 0.0, hi = len;
        if (isLoop) { lo = rs * len; hi = jmax (re * len, lo + 2.0); }

        auto at = [&] (double t) -> float
        {
            if (t < 0.0 || t > len - 1.0) return 0.f;
            const int i = (int) t;
            if (i >= (int) d.size() - 1) return d.back();
            const float f = (float) (t - i);
            return d[(size_t) i] + f * (d[(size_t) i + 1] - d[(size_t) i]);
        };

        const float mid = (float) H * 0.5f, amp = (float) H * 0.45f;
        g.setColour (kBlack);
        for (int x = 0; x < W; ++x)
        {
            const double t0 = lo + (hi - lo) * x / W, t1 = lo + (hi - lo) * (x + 1) / W;
            float a = at (t0), mn = a, mx = a;
            const float bb = at (t1);
            mn = jmin (mn, bb); mx = jmax (mx, bb);
            for (int i = (int) std::ceil (t0); i <= (int) std::floor (t1); ++i)
                if (i >= 0 && i < (int) d.size()) { mn = jmin (mn, d[(size_t) i]); mx = jmax (mx, d[(size_t) i]); }
            g.drawVerticalLine (x, mid - mx * amp, mid - mn * amp + 1.f);
        }

        if (! isLoop) // zone de boucle : barres + hachures
        {
            const int x0 = jlimit (0, W - 1, (int) (rs * W));
            const int x1 = jlimit (x0 + 2, W, (int) (jmin (re, 1.0) * W));
            g.setColour (kBlack);
            g.fillRect (x0, 0, x1 - x0, 3);
            g.fillRect (x0, H - 3, x1 - x0, 3);
            g.drawVerticalLine (x0, 0.f, (float) H);
            g.drawVerticalLine (x1 - 1, 0.f, (float) H);
            g.setColour (kBlack.withAlpha (0.18f));
            for (int x = x0; x < x1; x += 3) g.drawVerticalLine (x, 3.f, (float) H - 3.f);
        }
    }
    g.setColour (kBlack);
    g.drawRect (getLocalBounds(), 1);
}

// ======================================================================= cellules
KnobCell::KnobCell (AudioProcessorValueTreeState& s, const String& id, const String& text)
{
    label.setText (text, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (BWLookAndFeel::mono (9.f, true));
    addAndMakeVisible (label);

    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 70, 12);
    addAndMakeVisible (slider);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);

    if (auto* p = s.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}
void KnobCell::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (12));
    slider.setBounds (r);
}

ComboCell::ComboCell (AudioProcessorValueTreeState& s, const String& id, const String& text)
{
    label.setText (text, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (BWLookAndFeel::mono (9.f, true));
    addAndMakeVisible (label);

    if (auto* cp = dynamic_cast<AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (cp->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}
void ComboCell::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (12));
    box.setBounds (r.withSizeKeepingCentre (jmin (r.getWidth() - 8, 110), 20).withY (r.getY() + 24));
}

// ======================================================================= editeur
WarpusKongEditor::WarpusKongEditor (WarpusKongProcessor& p)
    : AudioProcessorEditor (&p), proc (p), fullView (p, false), loopView (p, true)
{
    setLookAndFeel (&lnf);

    combo (ID::loopMode, "MODE");   knob (ID::loopStart, "START");
    knob (ID::loopSpan, "SPAN");    knob (ID::loopHold, "HOLD");

    knob (ID::decay, "DECAY");      knob (ID::feedback, "FEEDBACK"); knob (ID::color, "COLOR");
    knob (ID::stiff, "STIFFNESS");  knob (ID::pick, "PICKUP");       knob (ID::noise, "NOISE");

    knob (ID::eqLow, "LOW");  knob (ID::eqMid, "MID");  knob (ID::eqFreq, "MID FREQ");  knob (ID::eqHigh, "HIGH");

    combo (ID::playMode, "PLAY");   knob (ID::glide, "GLIDE");   knob (ID::unison, "UNISON");
    knob (ID::detune, "DETUNE");    knob (ID::wide, "WIDE");
    knob (ID::gain, "GAIN");        combo (ID::osIdx, "OVERSAMPLE");

    knob (ID::aA, "A"); knob (ID::aD, "D"); knob (ID::aS, "S"); knob (ID::aR, "R");
    knob (ID::aAC, "A CURVE"); knob (ID::aDC, "D CURVE");

    combo (ID::fType, "TYPE"); knob (ID::cutoff, "CUTOFF"); knob (ID::reso, "RESO");
    knob (ID::fAmt, "ENV AMT"); knob (ID::fKey, "KEYTRK");

    knob (ID::fA, "A"); knob (ID::fD, "D"); knob (ID::fS, "S"); knob (ID::fR, "R");

    knob (ID::vibAmt, "DEPTH"); knob (ID::vibRate, "SPEED"); knob (ID::vibAtt, "ATTACK");

    combo (ID::modType, "MOD"); knob (ID::modAmt, "AMOUNT"); knob (ID::sTune, "TUNE");
    knob (ID::sFine, "FINE"); knob (ID::sLevel, "LEVEL");

    addAndMakeVisible (fullView);
    addAndMakeVisible (loopView);
    addAndMakeVisible (loadButton);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> ("Load a sample", File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this] (const FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f.existsAsFile()) proc.loadSampleFile (f);
                              });
    };

    setSize (960, 700);
    startTimerHz (10);
}

WarpusKongEditor::~WarpusKongEditor() { setLookAndFeel (nullptr); }

void WarpusKongEditor::knob (const char* id, const char* text)
{
    auto c = std::make_unique<KnobCell> (proc.apvts, id, text);
    addAndMakeVisible (*c);
    cells[id] = std::move (c);
}
void WarpusKongEditor::combo (const char* id, const char* text)
{
    auto c = std::make_unique<ComboCell> (proc.apvts, id, text);
    addAndMakeVisible (*c);
    cells[id] = std::move (c);
}

void WarpusKongEditor::resized()
{
    panels = { { "WARPUS KONG  -  SAMPLE / LOOP", { 8, 8, 640, 240 } },
               { "STRING",                          { 656, 8, 296, 240 } },
               { "SAMPLE EQ  (excitation only)",    { 8, 256, 336, 140 } },
               { "VOICES / STEREO / OUTPUT",        { 352, 256, 600, 140 } },
               { "AMP ENVELOPE",                    { 8, 404, 470, 140 } },
               { "FILTER",                          { 486, 404, 466, 140 } },
               { "FILTER ENVELOPE",                 { 8, 552, 336, 140 } },
               { "VIBRATO",                         { 352, 552, 250, 140 } },
               { "SINE OSC / MODULATION",           { 610, 552, 342, 140 } } };

    auto row = [this] (Rectangle<int> r, std::initializer_list<const char*> ids)
    {
        const int n = (int) ids.size();
        int i = 0;
        for (auto id : ids)
        {
            cells[id]->setBounds (r.getX() + i * r.getWidth() / n, r.getY(), r.getWidth() / n, r.getHeight());
            ++i;
        }
    };
    auto inner = [] (Rectangle<int> r) { return r.withTrimmedTop (20).reduced (4); };

    const auto s = panels[0].r;
    fullRect = { s.getX() + 8, s.getY() + 34, 440, 84 };
    loopRect = { s.getX() + 8, s.getY() + 136, 440, 84 };
    fullView.setBounds (fullRect);
    loopView.setBounds (loopRect);
    loadButton.setBounds (s.getX() + 456, s.getY() + 26, 176, 20);
    row ({ s.getX() + 456, s.getY() + 52, 176, 92 },  { ID::loopMode, ID::loopStart });
    row ({ s.getX() + 456, s.getY() + 144, 176, 92 }, { ID::loopSpan, ID::loopHold });

    const auto st = panels[1].r;
    row ({ st.getX() + 4, st.getY() + 22, st.getWidth() - 8, 104 },  { ID::decay, ID::feedback, ID::color });
    row ({ st.getX() + 4, st.getY() + 128, st.getWidth() - 8, 104 }, { ID::stiff, ID::pick, ID::noise });

    row (inner (panels[2].r), { ID::eqLow, ID::eqMid, ID::eqFreq, ID::eqHigh });
    row (inner (panels[3].r), { ID::playMode, ID::glide, ID::unison, ID::detune, ID::wide, ID::gain, ID::osIdx });
    row (inner (panels[4].r), { ID::aA, ID::aD, ID::aS, ID::aR, ID::aAC, ID::aDC });
    row (inner (panels[5].r), { ID::fType, ID::cutoff, ID::reso, ID::fAmt, ID::fKey });
    row (inner (panels[6].r), { ID::fA, ID::fD, ID::fS, ID::fR });
    row (inner (panels[7].r), { ID::vibAmt, ID::vibRate, ID::vibAtt });
    row (inner (panels[8].r), { ID::modType, ID::modAmt, ID::sTune, ID::sFine, ID::sLevel });
}

void WarpusKongEditor::paint (Graphics& g)
{
    g.fillAll (kWhite);
    // fond tramé 50 % tres discret, comme le bureau du Mac classique
    g.setColour (kBlack.withAlpha (0.10f));
    for (int y = 0; y < getHeight(); y += 2)
        for (int x = (y / 2) % 2 ? 1 : 0; x < getWidth(); x += 2) g.fillRect (x, y, 1, 1);

    for (auto& p : panels)
    {
        auto r = p.r;
        g.setColour (kBlack);
        g.fillRect (r.translated (2, 2));       // ombre portee
        g.setColour (kWhite);
        g.fillRect (r);
        g.setColour (kBlack);
        g.drawRect (r, 1);

        auto bar = r.withHeight (16);           // barre de titre rayee
        g.drawHorizontalLine (bar.getBottom(), (float) r.getX(), (float) r.getRight());
        for (int y = bar.getY() + 3; y < bar.getBottom() - 2; y += 2)
            g.fillRect (bar.getX() + 3, y, bar.getWidth() - 6, 1);
        auto f = BWLookAndFeel::mono (10.f, true);
        const int tw = f.getStringWidth (p.title) + 14;
        auto tb = Rectangle<int> (tw, 14).withCentre (bar.getCentre());
        g.setColour (kWhite);
        g.fillRect (tb);
        g.setColour (kBlack);
        g.setFont (f);
        g.drawText (p.title, tb, Justification::centred);
    }

    const auto s = panels.empty() ? Rectangle<int>() : panels[0].r;
    g.setFont (BWLookAndFeel::mono (9.f));
    g.setColour (kBlack);
    g.drawText ("SAMPLE  (loop region marked)", s.getX() + 8, s.getY() + 20, 300, 12, Justification::left);
    g.drawText ("LOOP  (one string period, zoomed)", s.getX() + 8, s.getY() + 122, 300, 12, Justification::left);
    g.drawText (proc.getInfoText(), s.getX() + 8, s.getY() + 222, 440, 12, Justification::left);
}

bool WarpusKongEditor::isInterestedInFileDrag (const StringArray& files)
{
    if (files.size() != 1) return false;
    const auto ext = File (files[0]).getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg";
}

void WarpusKongEditor::filesDropped (const StringArray& files, int, int)
{
    if (files.size() == 1) proc.loadSampleFile (File (files[0]));
}

void WarpusKongEditor::timerCallback()
{
    auto v = [this] (const char* id) { return (double) proc.apvts.getRawParameterValue (id)->load(); };
    const double key = proc.sampleVersion.load() * 1000003.0 + proc.lastNote.load() * 1009.0
                     + v (ID::loopMode) * 97.0 + v (ID::loopStart) * 7919.0 + v (ID::loopSpan) * 104729.0;
    if (key != lastKey) { lastKey = key; repaint(); }
}
