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

#include "TouchkeyControlMappingShortEditor.h"


//==============================================================================
TouchkeyControlMappingShortEditor::TouchkeyControlMappingShortEditor (TouchkeyControlMappingFactory& factory)
    : factory_(factory),
    inputRangeLowEditor{ "range low text editor" },
    rangeLabel{ "range label", "Input Range:" },
    controlLabel{ "control label", "Control:" },
    controlComboBox{ "control combo box" },
    controlLabel2{ "control label", "Parameter:" },
    parameterComboBox{ "parameter combo box" },
    controlLabel3{ "control label", "Type:" },
    typeComboBox{ "type combo box" },
    inputRangeHighEditor{ "range hi text editor" },
    rangeLabel2{ "range label", "-" }
{
    setLookAndFeel( &lnf );

    addAndMakeVisible (inputRangeLowEditor);
    inputRangeLowEditor.setMultiLine (false);
    inputRangeLowEditor.setReturnKeyStartsNewLine (false);
    inputRangeLowEditor.setReadOnly (false);
    inputRangeLowEditor.setScrollbarsShown (true);
    inputRangeLowEditor.setCaretVisible (true);
    inputRangeLowEditor.setPopupMenuEnabled (true);
    inputRangeLowEditor.setText ( juce::String{} );

    addAndMakeVisible (rangeLabel);
    rangeLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    rangeLabel.setJustificationType (juce::Justification::centredLeft);
    rangeLabel.setEditable (false, false, false);

    addAndMakeVisible (controlLabel);
    controlLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel.setJustificationType (juce::Justification::centredLeft);
    controlLabel.setEditable (false, false, false);

    addAndMakeVisible (controlComboBox);
    controlComboBox.setEditableText (false);
    controlComboBox.setJustificationType (juce::Justification::centredLeft);
    controlComboBox.setTextWhenNothingSelected ( juce::String{} );
    controlComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    controlComboBox.addListener (this);

    addAndMakeVisible (controlLabel2);
    controlLabel2.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel2.setJustificationType (juce::Justification::centredLeft);
    controlLabel2.setEditable (false, false, false);

    addAndMakeVisible (parameterComboBox);
    parameterComboBox.setEditableText (false);
    parameterComboBox.setJustificationType (juce::Justification::centredLeft);
    parameterComboBox.setTextWhenNothingSelected ( juce::String{} );
    parameterComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    parameterComboBox.addListener (this);

    addAndMakeVisible (controlLabel3);
    controlLabel3.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel3.setJustificationType (juce::Justification::centredLeft);
    controlLabel3.setEditable (false, false, false);

    addAndMakeVisible (typeComboBox);
    typeComboBox.setEditableText (false);
    typeComboBox.setJustificationType (juce::Justification::centredLeft);
    typeComboBox.setTextWhenNothingSelected ( juce::String{} );
    typeComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    typeComboBox.addListener (this);

    addAndMakeVisible (inputRangeHighEditor);
    inputRangeHighEditor.setMultiLine (false);
    inputRangeHighEditor.setReturnKeyStartsNewLine (false);
    inputRangeHighEditor.setReadOnly (false);
    inputRangeHighEditor.setScrollbarsShown (true);
    inputRangeHighEditor.setCaretVisible (true);
    inputRangeHighEditor.setPopupMenuEnabled (true);
    inputRangeHighEditor.setText ( juce::String{} );

    addAndMakeVisible (rangeLabel2);
    rangeLabel2.setFont (juce::Font (15.00f, juce::Font::plain));
    rangeLabel2.setJustificationType (juce::Justification::centredLeft);
    rangeLabel2.setEditable (false, false, false);

    parameterComboBox.addItem("X Position", TouchkeyControlMapping::kInputParameterXPosition);
    parameterComboBox.addItem("Y Position", TouchkeyControlMapping::kInputParameterYPosition);
    parameterComboBox.addItem("Contact Area", TouchkeyControlMapping::kInputParameterTouchSize);
    parameterComboBox.addItem("2-Finger Mean", TouchkeyControlMapping::kInputParameter2FingerMean);
    parameterComboBox.addItem("2-Finger Distance", TouchkeyControlMapping::kInputParameter2FingerDistance);

    typeComboBox.addItem("Absolute", TouchkeyControlMapping::kTypeAbsolute);
    typeComboBox.addItem("1st Touch Relative", TouchkeyControlMapping::kTypeFirstTouchRelative);
    typeComboBox.addItem("Note Onset Relative", TouchkeyControlMapping::kTypeNoteOnsetRelative);

    controlComboBox.addItem("Pitch Wheel", MidiKeyboardSegment::kControlPitchWheel);
    controlComboBox.addItem("Channel Pressure", MidiKeyboardSegment::kControlChannelAftertouch);
    controlComboBox.addItem("Poly Aftertouch", MidiKeyboardSegment::kControlPolyphonicAftertouch);
    for(int i = 1; i <= 119; i++) {
        controlComboBox.addItem(juce::String(i), i);
    }

    setSize (328, 71);

    inputRangeLowEditor.addListener(this);
    inputRangeHighEditor.addListener(this);

}

TouchkeyControlMappingShortEditor::~TouchkeyControlMappingShortEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void TouchkeyControlMappingShortEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void TouchkeyControlMappingShortEditor::resized()
{
    inputRangeLowEditor.setBounds (80, 40, 40, 24);
    rangeLabel.setBounds (0, 40, 80, 24);
    controlLabel.setBounds (176, 8, 56, 24);
    controlComboBox.setBounds (232, 8, 88, 24);
    controlLabel2.setBounds (0, 8, 72, 24);
    parameterComboBox.setBounds (72, 8, 104, 24);
    controlLabel3.setBounds (184, 40, 56, 24);
    typeComboBox.setBounds (232, 40, 88, 24);
    inputRangeHighEditor.setBounds (136, 40, 40, 24);
    rangeLabel2.setBounds (120, 40, 16, 24);

}

void TouchkeyControlMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &controlComboBox)
    {
        int controller = controlComboBox.getSelectedId();
        factory_.setController(controller);

    }
    else if (comboBoxThatHasChanged == &parameterComboBox)
    {
        int param = parameterComboBox.getSelectedId();
        factory_.setInputParameter(param);
    }
    else if (comboBoxThatHasChanged == &typeComboBox)
    {
        int type = typeComboBox.getSelectedId();
        factory_.setInputType(type);
    }
}


void TouchkeyControlMappingShortEditor::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(&editor == &inputRangeLowEditor) {
        float range = atof(inputRangeLowEditor.getText().toUTF8());
        factory_.setRangeInputMin(range);
        factory_.setRangeInputCenter(range);
    }
    else if(&editor == &inputRangeHighEditor) {
        float range = atof(inputRangeHighEditor.getText().toUTF8());
        factory_.setRangeInputMax(range);
    }
}

void TouchkeyControlMappingShortEditor::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{

}

void TouchkeyControlMappingShortEditor::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

void TouchkeyControlMappingShortEditor::synchronize()
{
    // Update the editors to reflect the current status
    if(!inputRangeLowEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeInputMin();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif
        inputRangeLowEditor.setText(st);
    }

    if(!inputRangeHighEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeInputMax();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        inputRangeHighEditor.setText(st);
    }

    parameterComboBox.setSelectedId(factory_.getInputParameter(), juce::NotificationType::dontSendNotification);
    typeComboBox.setSelectedId(factory_.getInputType(), juce::NotificationType::dontSendNotification);
    controlComboBox.setSelectedId(factory_.getController(), juce::NotificationType::dontSendNotification);
}

#endif  // TOUCHKEYS_NO_GUI
