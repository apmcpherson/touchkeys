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

#include "TouchkeyPitchBendMappingShortEditor.h"

//==============================================================================
TouchkeyPitchBendMappingShortEditor::TouchkeyPitchBendMappingShortEditor (TouchkeyPitchBendMappingFactory& factory)
    : factory_(factory),
    rangeEditor{ "range text editor" },
    rangeLabel{ "range label", "Range:" },
    thresholdEditor{ "threshold text editor" },
    thresholdLabel{ "threshold label", "Threshold:" },
    controlLabel{ "control label", "Endpoints:" }
{
    setLookAndFeel( &lnf );

    addAndMakeVisible (rangeEditor);

    addAndMakeVisible (rangeLabel);

    addAndMakeVisible (thresholdEditor);

    addAndMakeVisible (thresholdLabel);

    addAndMakeVisible (controlLabel);

    addAndMakeVisible (endpointsComboBox);
    endpointsComboBox.setEditableText (false);
    endpointsComboBox.setJustificationType (juce::Justification::centredLeft);
    endpointsComboBox.setTextWhenNothingSelected ( juce::String{} );
    endpointsComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    endpointsComboBox.addListener (this);

    endpointsComboBox.addItem("Variable", TouchkeyPitchBendMapping::kPitchBendModeVariableEndpoints);
    endpointsComboBox.addItem("Fixed", TouchkeyPitchBendMapping::kPitchBendModeFixedEndpoints);

    setSize (328, 71);

    rangeEditor.addListener(this);
    thresholdEditor.addListener(this);
}

TouchkeyPitchBendMappingShortEditor::~TouchkeyPitchBendMappingShortEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void TouchkeyPitchBendMappingShortEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
}

void TouchkeyPitchBendMappingShortEditor::resized()
{
    rangeEditor.setBounds (80, 8, 80, 24);
    rangeLabel.setBounds (8, 8, 56, 24);
    thresholdEditor.setBounds (240, 8, 80, 24);
    thresholdLabel.setBounds (168, 8, 72, 24);
    controlLabel.setBounds (8, 40, 72, 24);
    endpointsComboBox.setBounds (80, 40, 80, 24);
}

void TouchkeyPitchBendMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &endpointsComboBox)
    {
        int control = endpointsComboBox.getSelectedId();
        if(control == TouchkeyPitchBendMapping::kPitchBendModeVariableEndpoints)
            factory_.setBendVariableEndpoints();
        else if(control == TouchkeyPitchBendMapping::kPitchBendModeFixedEndpoints)
            factory_.setBendFixedEndpoints(factory_.getBendThresholdKeyLength(), 0);
    }
}


void TouchkeyPitchBendMappingShortEditor::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(&editor == &rangeEditor) {
        float range = atof(rangeEditor.getText().toUTF8());
        factory_.setBendRange(range);
    }
    else if(&editor == &thresholdEditor) {
        float threshold = atof(thresholdEditor.getText().toUTF8());
        factory_.setBendThresholdKeyLength(threshold);
    }
}

void TouchkeyPitchBendMappingShortEditor::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{

}

void TouchkeyPitchBendMappingShortEditor::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

void TouchkeyPitchBendMappingShortEditor::synchronize()
{
    // Update the editors to reflect the current status
    if(!rangeEditor.hasKeyboardFocus(true)) {
        float value = factory_.getBendRange();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        rangeEditor.setText(st);
    }

    if(!thresholdEditor.hasKeyboardFocus(true)) {
        float value = factory_.getBendThresholdKeyLength();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        thresholdEditor.setText(st);
    }

    endpointsComboBox.setSelectedId(factory_.getBendMode(), juce::NotificationType::dontSendNotification);
}

#endif      // TOUCHKEYS_NO_GUI
