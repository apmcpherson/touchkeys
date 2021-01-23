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

#include "TouchkeyKeyDivisionMappingShortEditor.h"

//==============================================================================
TouchkeyKeyDivisionMappingShortEditor::TouchkeyKeyDivisionMappingShortEditor (TouchkeyKeyDivisionMappingFactory& factory)
    : factory_(factory),
    tuningComboBox { "tuning combo box" },
    tuningLabel { "tuning label", TRANS( "Tuning:" ) },
    controlLabel { "control label", TRANS( "Control:" ) },
    controlComboBox { "control combo box" },
    retriggerButton { "retrigger button" }

{
    setLookAndFeel( &lnf );

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

    addAndMakeVisible (controlLabel);
    controlLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    controlLabel.setJustificationType (juce::Justification::centredLeft);
    controlLabel.setEditable (false, false, false);

    addAndMakeVisible (controlComboBox);
    controlComboBox.setEditableText (false);
    controlComboBox.setJustificationType (juce::Justification::centredLeft);
    controlComboBox.setTextWhenNothingSelected ( juce::String{} );
    controlComboBox.setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    controlComboBox.addListener (this);

    addAndMakeVisible (retriggerButton);
    retriggerButton.setButtonText (TRANS("Retriggerable"));
    retriggerButton.addListener (this);


    controlComboBox.addItem("Position", TouchkeyKeyDivisionMapping::kDetectionParameterYPosition);
    controlComboBox.addItem("Number of Touches", TouchkeyKeyDivisionMapping::kDetectionParameterNumberOfTouches);
    controlComboBox.addItem("Both", TouchkeyKeyDivisionMapping::kDetectionParameterYPositionAndNumberOfTouches);
    
    tuningComboBox.addItem("19-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset19TET + 1);
    tuningComboBox.addItem("24-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset24TET + 1);
    tuningComboBox.addItem("31-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset31TET + 1);
    tuningComboBox.addItem("36-tone Equal Temperament", TouchkeyKeyDivisionMappingFactory::kTuningPreset36TET + 1);
    tuningComboBox.addItem("Yarman-24c Maqam Tuning", TouchkeyKeyDivisionMappingFactory::kTuningPresetYarman24c + 1);

    setSize (328, 71);
}

TouchkeyKeyDivisionMappingShortEditor::~TouchkeyKeyDivisionMappingShortEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void TouchkeyKeyDivisionMappingShortEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void TouchkeyKeyDivisionMappingShortEditor::resized()
{
    tuningComboBox.setBounds (72, 8, 248, 24);
    tuningLabel.setBounds (8, 8, 72, 24);
    controlLabel.setBounds (8, 40, 72, 24);
    controlComboBox.setBounds (72, 40, 88, 24);
    retriggerButton.setBounds (176, 40, 136, 24);
}

void TouchkeyKeyDivisionMappingShortEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &tuningComboBox)
    {
        // Offset the value by 1 since preset numbering starts from 0 but combo box IDs start from 1...
        factory_.setTuningPreset(tuningComboBox.getSelectedId() - 1);
    }
    else if (comboBoxThatHasChanged == &controlComboBox)
    {
        factory_.setDetectionParameter(controlComboBox.getSelectedId());
    }
}

void TouchkeyKeyDivisionMappingShortEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == &retriggerButton)
    {
        factory_.setRetriggerable(retriggerButton.getToggleState());
    }
}


void TouchkeyKeyDivisionMappingShortEditor::synchronize()
{
    retriggerButton.setToggleState(factory_.getRetriggerable(), juce::NotificationType::dontSendNotification);
    controlComboBox.setSelectedId(factory_.getDetectionParameter());
    tuningComboBox.setSelectedId(factory_.getTuningPreset() + 1);
}


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
