#pragma once
#include "PluginProcessor.h"
#include <map>

// Look & feel noir et blanc, facon Turbosynth (Mac OS classique)
struct BWLookAndFeel : public LookAndFeel_V4
{
    BWLookAndFeel();
    static Font mono (float h, bool bold = false);

    void drawRotarySlider (Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, Slider&) override;
    Label* createSliderTextBox (Slider&) override;
    Font getComboBoxFont (ComboBox&) override { return mono (10.f); }
    Font getPopupMenuFont() override { return mono (10.f); }
    Font getTextButtonFont (TextButton&, int) override { return mono (10.f, true); }
    void drawComboBox (Graphics&, int w, int h, bool, int, int, int, int, ComboBox&) override;
    void positionComboBoxText (ComboBox&, Label&) override;
    void drawButtonBackground (Graphics&, Button&, const Colour&, bool over, bool down) override;
    void drawPopupMenuBackground (Graphics&, int w, int h) override;
};

class WaveformView : public Component
{
public:
    WaveformView (WarpusKongProcessor& p, bool loopView) : proc (p), isLoop (loopView) {}
    void paint (Graphics&) override;
private:
    WarpusKongProcessor& proc;
    bool isLoop;
};

class KnobCell : public Component
{
public:
    KnobCell (AudioProcessorValueTreeState&, const String& id, const String& text);
    void resized() override;
private:
    Label label;
    Slider slider;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att;
};

class ComboCell : public Component
{
public:
    ComboCell (AudioProcessorValueTreeState&, const String& id, const String& text);
    void resized() override;
private:
    Label label;
    ComboBox box;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

class WarpusKongEditor : public AudioProcessorEditor,
                           public FileDragAndDropTarget,
                           private Timer
{
public:
    explicit WarpusKongEditor (WarpusKongProcessor&);
    ~WarpusKongEditor() override;

    void paint (Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const StringArray& files) override;
    void filesDropped (const StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void knob (const char* id, const char* text);
    void combo (const char* id, const char* text);

    BWLookAndFeel lnf;
    WarpusKongProcessor& proc;
    std::map<std::string, std::unique_ptr<Component>> cells;
    WaveformView fullView, loopView;
    TextButton loadButton { "LOAD SAMPLE..." };
    std::unique_ptr<FileChooser> chooser;

    struct Panel { String title; Rectangle<int> r; };
    std::vector<Panel> panels;
    Rectangle<int> fullRect, loopRect;
    double lastKey = -1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpusKongEditor)
};
