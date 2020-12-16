/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#ifndef TOUCHKEYS_NO_GUI
//[/Headers]

#include "TouchkeyKeyDivisionMappingShortEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TouchkeyKeyDivisionMappingShortEditor::TouchkeyKeyDivisionMappingShortEditor (TouchkeyKeyDivisionMappingFactory& factory)
    : factory_(factory),
    tuningComboBox { "tuning combo box" },
    tuningLabel { "tuning label", TRANS( "Tuning:" ) },
    controlLabel { "control label", TRANS( "Control:" ) },
    controlComboBox { "control combo box" },
    retriggerButton { "retrigger button" }

{
    addAndMakeVisible (tuningComboBox);
    tuningComboBox.setEditableText (false);
    tuningComboBox.setJustificationType (juce::Justification::centredLeft);
    tuningComboBox.setTextWhenNothingSelected ( juce::String{} );
    tuningComboBox.setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    tuningComboBox.addListener (this);

    addAndMakeVisible (tuningLabel);
    tuningLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    tuningLabel.setJustificationType (juce::Justification::centredLeft);
    tuningLabel.setEditable (false, false, false);
    tuningLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    tuningLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

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
    controlComboBox.setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    controlComboBox.addListener (this);

    addAndMakeVisible (retriggerButton);
    retriggerButton.setButtonText (TRANS("Retriggerable"));
    retriggerButton.addListener (this);


    //[UserPreSize]
    controlComboBox.addItem("Position", TouchkeyKeyDivisionMapping::kDetectionParameterYPosition);
    controlComboBox.addItem("Number of Touches", TouchkeyKeyDivisionMapping::kDetectionParameterNumberOfTouches);
    controlComboBox.addItem("Both", TouchkeyKeyDivisionMapping::kDetectionParameterYPositionAndNumberOfTouches);
    
    tuningComboBox.addItem("19-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset19TET + 1);
    tuningComboBox.addItem("24-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset24TET + 1);
    tuningComboBox.addItem("31-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset31TET + 1);
    tuningComboBox.addItem("36-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset36TET + 1);
    tuningComboBox.addItem("Yarman-24c Maqam Tuning", TouchkeyKeyDivisionMappingFactory::kTuningPresetYarman24c + 1);
    //[/UserPreSize]

    setSize (328, 71);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

TouchkeyKeyDivisionMappingShortEditor::~TouchkeyKeyDivisionMappingShortEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void TouchkeyKeyDivisionMappingShortEditor::paint (juce::Graphics& g)
{
    // NOTE white colour obscures the text of the label components
    //g.fillAll(juce::Colours::white);
    g.fillAll( juce::Colours::grey );
}

void TouchkeyKeyDivisionMappingShortEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    tuningComboBox.setBounds (72, 8, 248, 24);
    tuningLabel.setBounds (8, 8, 72, 24);
    controlLabel.setBounds (8, 40, 72, 24);
    controlComboBox.setBounds (72, 40, 88, 24);
    retriggerButton.setBounds (176, 40, 136, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TouchkeyKeyDivisionMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == &tuningComboBox)
    {
        //[UserComboBoxCode_tuningComboBox] -- add your combo box handling code here..
        // Offset the value by 1 since preset numbering starts from 0 but combo box IDs start from 1...
        factory_.setTuningPreset(tuningComboBox.getSelectedId() - 1);
        //[/UserComboBoxCode_tuningComboBox]
    }
    else if (comboBoxThatHasChanged == &controlComboBox)
    {
        //[UserComboBoxCode_controlComboBox] -- add your combo box handling code here..
        factory_.setDetectionParameter(controlComboBox.getSelectedId());
        //[/UserComboBoxCode_controlComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void TouchkeyKeyDivisionMappingShortEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == &retriggerButton)
    {
        //[UserButtonCode_retriggerButton] -- add your button handler code here..
        factory_.setRetriggerable(retriggerButton.getToggleState());
        //[/UserButtonCode_retriggerButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void TouchkeyKeyDivisionMappingShortEditor::synchronize()
{
    retriggerButton.setToggleState(factory_.getRetriggerable(), juce::NotificationType::dontSendNotification);
    controlComboBox.setSelectedId(factory_.getDetectionParameter());
    tuningComboBox.setSelectedId(factory_.getTuningPreset() + 1);
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TouchkeyKeyDivisionMappingShortEditor"
                 componentName="" parentClasses="public MappingEditorComponent, public juce::TextEditor::Listener"
                 constructorParams="TouchkeyKeyDivisionMappingFactory&amp; factory"
                 variableInitialisers="factory_(factory)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="328"
                 initialHeight="71">
  <BACKGROUND backgroundColour="ffffffff"/>
  <COMBOBOX name="tuning combo box" id="11460b0e135fe122" memberName="tuningComboBox"
            virtualName="" explicitFocusOrder="0" pos="72 8 248 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="tuning label" id="864de4f55b5481ee" memberName="tuningLabel"
         virtualName="" explicitFocusOrder="0" pos="8 8 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Tuning:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="control label" id="163b8236fad72f38" memberName="controlLabel"
         virtualName="" explicitFocusOrder="0" pos="8 40 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Control:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <COMBOBOX name="control combo box" id="597816425fbf42ce" memberName="controlComboBox"
            virtualName="" explicitFocusOrder="0" pos="72 40 88 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="retrigger button" id="1b86153e19e7aa57" memberName="retriggerButton"
                virtualName="" explicitFocusOrder="0" pos="176 40 136 24" buttonText="Retriggerable"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
