/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#ifndef TOUCHKEYS_NO_GUI
//[/Headers]

#include "TouchkeyControlMappingShortEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

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
    rangeLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    rangeLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (controlLabel);
    controlLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel.setJustificationType (juce::Justification::centredLeft);
    controlLabel.setEditable (false, false, false);
    controlLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    controlLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

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
    controlLabel2.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    controlLabel2.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

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
    controlLabel3.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    controlLabel3.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

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
    rangeLabel2.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    rangeLabel2.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);


    //[UserPreSize]
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
    //[/UserPreSize]

    setSize (328, 71);


    //[Constructor] You can add your own custom stuff here..
    inputRangeLowEditor.addListener(this);
    inputRangeHighEditor.addListener(this);
    //[/Constructor]
}

TouchkeyControlMappingShortEditor::~TouchkeyControlMappingShortEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void TouchkeyControlMappingShortEditor::paint (juce::Graphics& g)
{
    // NOTE white colour obscures the text of the label components
    //g.fillAll(juce::Colours::white);
    g.fillAll( juce::Colours::grey );
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
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TouchkeyControlMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == &controlComboBox)
    {
        //[UserComboBoxCode_controlComboBox] -- add your combo box handling code here..
        int controller = controlComboBox.getSelectedId();
        factory_.setController(controller);
        //[/UserComboBoxCode_controlComboBox]
    }
    else if (comboBoxThatHasChanged == &parameterComboBox)
    {
        //[UserComboBoxCode_parameterComboBox] -- add your combo box handling code here..
        int param = parameterComboBox.getSelectedId();
        factory_.setInputParameter(param);
        //[/UserComboBoxCode_parameterComboBox]
    }
    else if (comboBoxThatHasChanged == &typeComboBox)
    {
        //[UserComboBoxCode_typeComboBox] -- add your combo box handling code here..
        int type = typeComboBox.getSelectedId();
        factory_.setInputType(type);
        //[/UserComboBoxCode_typeComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

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
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TouchkeyControlMappingShortEditor"
                 componentName="" parentClasses="public MappingEditorComponent, public juce::TextEditor::Listener"
                 constructorParams="TouchkeyControlMappingFactory&amp; factory"
                 variableInitialisers="factory_(factory)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="1" initialWidth="328"
                 initialHeight="71">
  <BACKGROUND backgroundColour="ffffffff"/>
  <TEXTEDITOR name="range low text editor" id="db0f62c03a58af03" memberName="inputRangeLowEditor"
              virtualName="" explicitFocusOrder="0" pos="80 40 40 24" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="range label" id="1ca2d422f4c37b7f" memberName="rangeLabel"
         virtualName="" explicitFocusOrder="0" pos="0 40 80 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Input Range:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="control label" id="f953b12999632418" memberName="controlLabel"
         virtualName="" explicitFocusOrder="0" pos="176 8 56 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Control:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="control combo box" id="f1c84bb5fd2730fb" memberName="controlComboBox"
            virtualName="" explicitFocusOrder="0" pos="232 8 88 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="control label" id="5ef7c1b78fdcf616" memberName="controlLabel2"
         virtualName="" explicitFocusOrder="0" pos="0 8 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Parameter:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="parameter combo box" id="f12f6f6e31042be1" memberName="parameterComboBox"
            virtualName="" explicitFocusOrder="0" pos="72 8 104 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="control label" id="9ded92e82db31777" memberName="controlLabel3"
         virtualName="" explicitFocusOrder="0" pos="184 40 56 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Type:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="type combo box" id="82d38054016f6c4f" memberName="typeComboBox"
            virtualName="" explicitFocusOrder="0" pos="232 40 88 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <TEXTEDITOR name="range hi text editor" id="c34ac3e87db289d1" memberName="inputRangeHighEditor"
              virtualName="" explicitFocusOrder="0" pos="136 40 40 24" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="range label" id="19e0ad80306cc4c0" memberName="rangeLabel2"
         virtualName="" explicitFocusOrder="0" pos="120 40 16 24" edTextCol="ff000000"
         edBkgCol="0" labelText="-" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif  // TOUCHKEYS_NO_GUI
//[/EndFile]
