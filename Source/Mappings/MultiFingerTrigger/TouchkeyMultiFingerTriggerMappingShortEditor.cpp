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

#include "TouchkeyMultiFingerTriggerMappingShortEditor.h"


//==============================================================================
TouchkeyMultiFingerTriggerMappingShortEditor::TouchkeyMultiFingerTriggerMappingShortEditor (TouchkeyMultiFingerTriggerMappingFactory& factory)
    : factory_(factory),
    controlLabel{ "control label", "Touches:" },
    touchesComboBox{ "control combo box" },
    controlLabel2{ "control label", "Repeat Taps:" },
    tapsComboBox{ "control combo box" },
    controlLabel3{ "control label", "Note:" },
    noteComboBox{ "control combo box" },
    sendOnReleaseButton{ "new toggle button" }
{
    setLookAndFeel( &lnf );

    addAndMakeVisible (controlLabel);
    controlLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (touchesComboBox);
    touchesComboBox.setEditableText (false);
    touchesComboBox.setJustificationType (juce::Justification::centredLeft);
    touchesComboBox.setTextWhenNothingSelected ( juce::String{} );
    touchesComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    touchesComboBox.addListener (this);

    addAndMakeVisible (controlLabel2);

    addAndMakeVisible (tapsComboBox);
    tapsComboBox.setEditableText (false);
    tapsComboBox.setJustificationType (juce::Justification::centredLeft);
    tapsComboBox.setTextWhenNothingSelected ( juce::String{} );
    tapsComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    tapsComboBox.addListener (this);

    addAndMakeVisible (controlLabel3);
    controlLabel3.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (noteComboBox);
    noteComboBox.setEditableText (false);
    noteComboBox.setJustificationType (juce::Justification::centredLeft);
    noteComboBox.setTextWhenNothingSelected ( juce::String{} );
    noteComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    noteComboBox.addListener (this);

    addAndMakeVisible (sendOnReleaseButton);
    sendOnReleaseButton.setButtonText ("Also send on release");
    sendOnReleaseButton.addListener (this);

    for(int i = 1; i <= 3; i++) {
        touchesComboBox.addItem(juce::String(i), i);
    }
    for(int i = 1; i <= 5; i++) {
        tapsComboBox.addItem(juce::String(i), i);
    }
    noteComboBox.addItem("Same", kNoteSame);
    for(int i = 0; i <= 127; i++) {
        noteComboBox.addItem(juce::String(i), i + kNoteOffset);
    }

    setSize (328, 71);
}

TouchkeyMultiFingerTriggerMappingShortEditor::~TouchkeyMultiFingerTriggerMappingShortEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void TouchkeyMultiFingerTriggerMappingShortEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);
}

void TouchkeyMultiFingerTriggerMappingShortEditor::resized()
{
    controlLabel.setBounds (8, 8, 64, 24);
    touchesComboBox.setBounds (72, 8, 80, 24);
    controlLabel2.setBounds (160, 8, 80, 24);
    tapsComboBox.setBounds (240, 8, 80, 24);
    controlLabel3.setBounds (8, 40, 64, 24);
    noteComboBox.setBounds (72, 40, 80, 24);
    sendOnReleaseButton.setBounds (168, 40, 152, 24);
}

void TouchkeyMultiFingerTriggerMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &touchesComboBox)
    {
        factory_.setTouchesForTrigger(touchesComboBox.getSelectedId());
    }
    else if (comboBoxThatHasChanged == &tapsComboBox)
    {
        factory_.setConsecutiveTapsForTrigger(tapsComboBox.getSelectedId());
    }
    else if (comboBoxThatHasChanged == &noteComboBox)
    {
        int note = noteComboBox.getSelectedId();
        if(note == kNoteSame)
            note = -1;
        else
            note -= kNoteOffset;
        factory_.setTriggerOnNoteNumber(note);
        factory_.setTriggerOffNoteNumber(note);
    }
}

void TouchkeyMultiFingerTriggerMappingShortEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == &sendOnReleaseButton)
    {
        if(sendOnReleaseButton.getToggleState()) {
            factory_.setTriggerOffAction(TouchkeyMultiFingerTriggerMapping::kActionNoteOn);
        }
        else {
            factory_.setTriggerOffAction(TouchkeyMultiFingerTriggerMapping::kActionNone);
        }
    }
}


void TouchkeyMultiFingerTriggerMappingShortEditor::synchronize() {
    using jNT = juce::NotificationType;

    touchesComboBox.setSelectedId(factory_.getTouchesForTrigger(), jNT::dontSendNotification);
    tapsComboBox.setSelectedId(factory_.getConsecutiveTapsForTrigger(), jNT::dontSendNotification);
    
    int note = factory_.getTriggerOnNoteNumber();
    if(note < 0)
        noteComboBox.setSelectedId(kNoteSame, jNT::dontSendNotification);
    else
        noteComboBox.setSelectedId(note + kNoteOffset, jNT::dontSendNotification);
    
    if(factory_.getTriggerOffAction() == TouchkeyMultiFingerTriggerMapping::kActionNoteOn)
        sendOnReleaseButton.setToggleState(true, jNT::dontSendNotification);
    else
        sendOnReleaseButton.setToggleState(false, jNT::dontSendNotification);
}

#endif      // TOUCHKEYS_NO_GUI
