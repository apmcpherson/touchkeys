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

#include "TouchkeyPitchBendMappingShortEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TouchkeyPitchBendMappingShortEditor::TouchkeyPitchBendMappingShortEditor (TouchkeyPitchBendMappingFactory& factory)
    : factory_(factory),
    rangeEditor{ "range text editor" },
    rangeLabel{ "range label", "Range:" },
    thresholdEditor{ "threshold text editor" },
    thresholdLabel{ "threshold label", "Threshold:" },
    controlLabel{ "control label", "Endpoints:" }
{
    addAndMakeVisible (rangeEditor);
    rangeEditor.setMultiLine (false);
    rangeEditor.setReturnKeyStartsNewLine (false);
    rangeEditor.setReadOnly (false);
    rangeEditor.setScrollbarsShown (true);
    rangeEditor.setCaretVisible (true);
    rangeEditor.setPopupMenuEnabled (true);
    rangeEditor.setText ( juce::String{} );

    addAndMakeVisible (rangeLabel);
    rangeLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    rangeLabel.setJustificationType (juce::Justification::centredLeft);
    rangeLabel.setEditable (false, false, false);
    rangeLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    rangeLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (thresholdEditor);
    thresholdEditor.setMultiLine (false);
    thresholdEditor.setReturnKeyStartsNewLine (false);
    thresholdEditor.setReadOnly (false);
    thresholdEditor.setScrollbarsShown (true);
    thresholdEditor.setCaretVisible (true);
    thresholdEditor.setPopupMenuEnabled (true);
    thresholdEditor.setText ( juce::String{} );

    addAndMakeVisible (thresholdLabel);
    thresholdLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    thresholdLabel.setJustificationType (juce::Justification::centredLeft);
    thresholdLabel.setEditable (false, false, false);
    thresholdLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    thresholdLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (controlLabel);
    controlLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel.setJustificationType (juce::Justification::centredLeft);
    controlLabel.setEditable (false, false, false);
    controlLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    controlLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (endpointsComboBox);
    endpointsComboBox.setEditableText (false);
    endpointsComboBox.setJustificationType (juce::Justification::centredLeft);
    endpointsComboBox.setTextWhenNothingSelected ( juce::String{} );
    endpointsComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    endpointsComboBox.addListener (this);


    //[UserPreSize]
    endpointsComboBox.addItem("Variable", TouchkeyPitchBendMapping::kPitchBendModeVariableEndpoints);
    endpointsComboBox.addItem("Fixed", TouchkeyPitchBendMapping::kPitchBendModeFixedEndpoints);
    //[/UserPreSize]

    setSize (328, 71);


    //[Constructor] You can add your own custom stuff here..
    rangeEditor.addListener(this);
    thresholdEditor.addListener(this);
    //[/Constructor]
}

TouchkeyPitchBendMappingShortEditor::~TouchkeyPitchBendMappingShortEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void TouchkeyPitchBendMappingShortEditor::paint (juce::Graphics& g)
{
    // NOTE white colour obscures the text of the label components
    //g.fillAll(juce::Colours::white);
    g.fillAll(juce::Colours::grey);
}

void TouchkeyPitchBendMappingShortEditor::resized()
{
    rangeEditor.setBounds (80, 8, 80, 24);
    rangeLabel.setBounds (8, 8, 56, 24);
    thresholdEditor.setBounds (240, 8, 80, 24);
    thresholdLabel.setBounds (168, 8, 72, 24);
    controlLabel.setBounds (8, 40, 72, 24);
    endpointsComboBox.setBounds (80, 40, 80, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TouchkeyPitchBendMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == &endpointsComboBox)
    {
        //[UserComboBoxCode_endpointsComboBox] -- add your combo box handling code here..
        int control = endpointsComboBox.getSelectedId();
        if(control == TouchkeyPitchBendMapping::kPitchBendModeVariableEndpoints)
            factory_.setBendVariableEndpoints();
        else if(control == TouchkeyPitchBendMapping::kPitchBendModeFixedEndpoints)
            factory_.setBendFixedEndpoints(factory_.getBendThresholdKeyLength(), 0);
        //[/UserComboBoxCode_endpointsComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

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
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TouchkeyPitchBendMappingShortEditor"
                 componentName="" parentClasses="public MappingEditorComponent, public juce::TextEditor::Listener"
                 constructorParams="TouchkeyPitchBendMappingFactory&amp; factory"
                 variableInitialisers="factory_(factory)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="1" initialWidth="328"
                 initialHeight="71">
  <BACKGROUND backgroundColour="ffffffff"/>
  <TEXTEDITOR name="range text editor" id="db0f62c03a58af03" memberName="rangeEditor"
              virtualName="" explicitFocusOrder="0" pos="80 8 80 24" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="range label" id="1ca2d422f4c37b7f" memberName="rangeLabel"
         virtualName="" explicitFocusOrder="0" pos="8 8 56 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Range:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="threshold text editor" id="854a054d84eaf552" memberName="thresholdEditor"
              virtualName="" explicitFocusOrder="0" pos="240 8 80 24" initialText=""
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="threshold label" id="864de4f55b5481ee" memberName="thresholdLabel"
         virtualName="" explicitFocusOrder="0" pos="168 8 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Threshold:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="control label" id="f953b12999632418" memberName="controlLabel"
         virtualName="" explicitFocusOrder="0" pos="8 40 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Endpoints:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="control combo box" id="f1c84bb5fd2730fb" memberName="endpointsComboBox"
            virtualName="" explicitFocusOrder="0" pos="80 40 80 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
