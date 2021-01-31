/*
  TouchKeys: multi-touch musical keyboard control software
  Copyright (c) 2013 Andrew McPherson

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TOUCHKEYS_NO_GUI

#include "TouchkeyVibratoMappingShortEditor.h"

//==============================================================================
TouchkeyVibratoMappingShortEditor::TouchkeyVibratoMappingShortEditor (TouchkeyVibratoMappingFactory& factory)
    : factory_(factory),
    rangeEditor{ "range text editor" },
    rangeLabel{ "range label", "Range:" },
    thresholdEditor{ "threshold text editor" },
    thresholdLabel{ "threshold label", "Threshold:" },
    controlLabel{ "control label", "Control:" },
    controlComboBox{ "control combo box" }

{
    setLookAndFeel( &lnf );

    addAndMakeVisible (rangeEditor);
    rangeEditor.setMultiLine (false);
    rangeEditor.setReturnKeyStartsNewLine (false);
    rangeEditor.setReadOnly (false);
    rangeEditor.setScrollbarsShown (true);
    rangeEditor.setCaretVisible (true);
    rangeEditor.setPopupMenuEnabled (true);
    rangeEditor.setText (juce::String{});

    addAndMakeVisible (rangeLabel);
    rangeLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    rangeLabel.setJustificationType (juce::Justification::centredLeft);
    rangeLabel.setEditable (false, false, false);

    addAndMakeVisible (thresholdEditor);
    thresholdEditor.setMultiLine (false);
    thresholdEditor.setReturnKeyStartsNewLine (false);
    thresholdEditor.setReadOnly (false);
    thresholdEditor.setScrollbarsShown (true);
    thresholdEditor.setCaretVisible (true);
    thresholdEditor.setPopupMenuEnabled (true);
    thresholdEditor.setText (juce::String{});

    addAndMakeVisible (thresholdLabel);
    thresholdLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    thresholdLabel.setJustificationType (juce::Justification::centredLeft);
    thresholdLabel.setEditable (false, false, false);

    addAndMakeVisible (controlLabel);
    controlLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel.setJustificationType (juce::Justification::centredLeft);
    controlLabel.setEditable (false, false, false);

    addAndMakeVisible (controlComboBox);
    controlComboBox.setEditableText (false);
    controlComboBox.setJustificationType (juce::Justification::centredLeft);
    controlComboBox.setTextWhenNothingSelected (juce::String{});
    controlComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    controlComboBox.addListener (this);

    // Populate controllers field
    controlComboBox.addItem("Pitch Wheel", MidiKeyboardSegment::kControlPitchWheel);
    for(int i = 1; i <= 120; i++) {
        controlComboBox.addItem(juce::String(i), i);
    }

    setSize (328, 71);

    rangeEditor.addListener(this);
    thresholdEditor.addListener(this);
}

TouchkeyVibratoMappingShortEditor::~TouchkeyVibratoMappingShortEditor()
{
    setLookAndFeel( nullptr );
}

void TouchkeyVibratoMappingShortEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
}

void TouchkeyVibratoMappingShortEditor::resized()
{
    rangeEditor.setBounds (64, 8, 88, 24);
    rangeLabel.setBounds (8, 8, 56, 24);
    thresholdEditor.setBounds (232, 8, 88, 24);
    thresholdLabel.setBounds (160, 8, 72, 24);
    controlLabel.setBounds (8, 40, 56, 24);
    controlComboBox.setBounds (64, 40, 88, 24);
}

void TouchkeyVibratoMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &controlComboBox)
    {
        int control = controlComboBox.getSelectedId();
        factory_.setVibratoControl(control);
    }
}


void TouchkeyVibratoMappingShortEditor::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(&editor == &rangeEditor) {
        float range = atof(rangeEditor.getText().toUTF8());
        factory_.setVibratoRange(range);
    }
    else if(&editor == &thresholdEditor) {
        float threshold = atof(thresholdEditor.getText().toUTF8());
        factory_.setVibratoThreshold(threshold);
    }
}

void TouchkeyVibratoMappingShortEditor::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{

}

void TouchkeyVibratoMappingShortEditor::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

void TouchkeyVibratoMappingShortEditor::synchronize()
{
    // Update the editors to reflect the current status
    if(!rangeEditor.hasKeyboardFocus(true)) {
        float value = factory_.getVibratoRange();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        rangeEditor.setText(st);
    }

    if(!thresholdEditor.hasKeyboardFocus(true)) {
        float value = factory_.getVibratoThreshold();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        thresholdEditor.setText(st);
    }

    controlComboBox.setSelectedId(factory_.getVibratoControl(), juce::NotificationType::dontSendNotification);
}

#endif      // TOUCHKEYS_NO_GUI
