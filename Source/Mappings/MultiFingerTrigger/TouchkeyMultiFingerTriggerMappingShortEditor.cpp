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
    controlLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel.setJustificationType (juce::Justification::centredRight);
    controlLabel.setEditable (false, false, false);

    addAndMakeVisible (touchesComboBox);
    touchesComboBox.setEditableText (false);
    touchesComboBox.setJustificationType (juce::Justification::centredLeft);
    touchesComboBox.setTextWhenNothingSelected ( juce::String{} );
    touchesComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    touchesComboBox.addListener (this);

    addAndMakeVisible (controlLabel2);
    controlLabel2.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel2.setJustificationType (juce::Justification::centredLeft);
    controlLabel2.setEditable (false, false, false);

    addAndMakeVisible (tapsComboBox);
    tapsComboBox.setEditableText (false);
    tapsComboBox.setJustificationType (juce::Justification::centredLeft);
    tapsComboBox.setTextWhenNothingSelected ( juce::String{} );
    tapsComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    tapsComboBox.addListener (this);

    addAndMakeVisible (controlLabel3);
    controlLabel3.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel3.setJustificationType (juce::Justification::centredRight);
    controlLabel3.setEditable (false, false, false);

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
    g.fillAll(juce::Colours::white);
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


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TouchkeyMultiFingerTriggerMappingShortEditor"
                 componentName="" parentClasses="public MappingEditorComponent, public juce::TextEditor::Listener"
                 constructorParams="TouchkeyMultiFingerTriggerMappingFactory&amp; factory"
                 variableInitialisers="factory_(factory)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="328"
                 initialHeight="71">
  <BACKGROUND backgroundColour="ffffffff"/>
  <LABEL name="control label" id="f953b12999632418" memberName="controlLabel"
         virtualName="" explicitFocusOrder="0" pos="8 8 64 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Touches:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="34"/>
  <COMBOBOX name="control combo box" id="f1c84bb5fd2730fb" memberName="touchesComboBox"
            virtualName="" explicitFocusOrder="0" pos="72 8 80 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="control label" id="e3b829a3e4774248" memberName="controlLabel2"
         virtualName="" explicitFocusOrder="0" pos="160 8 80 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Repeat Taps:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <COMBOBOX name="control combo box" id="26848818ea1ea5ea" memberName="tapsComboBox"
            virtualName="" explicitFocusOrder="0" pos="240 8 80 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="control label" id="858bbbef4bfb2c55" memberName="controlLabel3"
         virtualName="" explicitFocusOrder="0" pos="8 40 64 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Note:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="34"/>
  <COMBOBOX name="control combo box" id="cb809b358724b54b" memberName="noteComboBox"
            virtualName="" explicitFocusOrder="0" pos="72 40 80 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="new toggle button" id="f75c92be72563883" memberName="sendOnReleaseButton"
                virtualName="" explicitFocusOrder="0" pos="168 40 152 24" buttonText="Also send on release"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
