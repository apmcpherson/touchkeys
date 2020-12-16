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
#include "../MainApplicationController.h"
//[/Headers]

#include "PreferencesComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PreferencesComponent::PreferencesComponent ()
    : controller_(0),
    startupPresetComboBox{ "Startup preset combo box" },
    label4{ "new label", "Load preset on startup:" },
    startTouchKeysButton{ "auto start TouchKeys button" },
    autodetectButton{ "Autodetect button" },
    defaultsButton{ "new button" }
{
    addAndMakeVisible (startupPresetComboBox);
    startupPresetComboBox.setEditableText (false);
    startupPresetComboBox.setJustificationType (juce::Justification::centredLeft);
    startupPresetComboBox.setTextWhenNothingSelected (juce::String{});
    startupPresetComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    startupPresetComboBox.addListener (this);

    addAndMakeVisible (label4);
    label4.setFont (juce::Font (15.00f, juce::Font::plain));
    label4.setJustificationType (juce::Justification::centredLeft);
    label4.setEditable (false, false, false);
    label4.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label4.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (startTouchKeysButton);
    startTouchKeysButton.setButtonText ("Start TouchKeys on startup");
    startTouchKeysButton.addListener (this);

    addAndMakeVisible (autodetectButton);
    autodetectButton.setButtonText ("Autodetect TouchKeys octave on each start");
    autodetectButton.addListener (this);

    addAndMakeVisible (defaultsButton);
    defaultsButton.setButtonText ("Reset to Defaults...");
    defaultsButton.addListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (296, 152);


    //[Constructor] You can add your own custom stuff here..

    // Initialise the combo box
    startupPresetComboBox.addItem("None", kStartupPresetNone);
    startupPresetComboBox.addItem("Vibrato and Pitch Bend", kStartupPresetVibratoPitchBend);
    startupPresetComboBox.addItem("Last Saved", kStartupPresetLastSaved);
    startupPresetComboBox.addItem("Choose...", kStartupPresetChoose);

    //[/Constructor]
}

PreferencesComponent::~PreferencesComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PreferencesComponent::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xffd2d2d2));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PreferencesComponent::resized()
{
    startupPresetComboBox.setBounds (16, 32, 264, 24);
    label4.setBounds (16, 8, 160, 24);
    startTouchKeysButton.setBounds (16, 64, 208, 24);
    autodetectButton.setBounds (16, 88, 272, 24);
    defaultsButton.setBounds (16, 120, 144, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PreferencesComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    if(controller_ == 0)
        return;
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == &startupPresetComboBox)
    {
        //[UserComboBoxCode_startupPresetComboBox] -- add your combo box handling code here..
        int selection = startupPresetComboBox.getSelectedId();
        if(selection == kStartupPresetNone) {
            controller_->setPrefsStartupPresetNone();
        }
        else if(selection == kStartupPresetVibratoPitchBend) {
            controller_->setPrefsStartupPresetVibratoPitchBend();
        }
        else if(selection == kStartupPresetLastSaved) {
            controller_->setPrefsStartupPresetLastSaved();
        }
        else if(selection == kStartupPresetChoose) {
            // Bring up window to choose a preset
            juce::FileChooser myChooser ("Select a preset...",
                                   juce::File{}, // File::getSpecialLocation (File::userHomeDirectory),
                                   "*.tkpreset");
            if(myChooser.browseForFileToOpen()) {
                controller_->setPrefsStartupPreset(myChooser.getResult().getFullPathName());
            }
            // Otherwise user clicked cancel and we go back to whatever was there before
        }
        //[/UserComboBoxCode_startupPresetComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void PreferencesComponent::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    if(controller_ == 0)
        return;
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == &startTouchKeysButton)
    {
        //[UserButtonCode_startTouchKeysButton] -- add your button handler code here..
        controller_->setPrefsAutoStartTouchKeys(startTouchKeysButton.getToggleState());
        //[/UserButtonCode_startTouchKeysButton]
    }
    else if (buttonThatWasClicked == &autodetectButton)
    {
        //[UserButtonCode_autodetectButton] -- add your button handler code here..
        controller_->setPrefsAutodetectOctave(autodetectButton.getToggleState());
        //[/UserButtonCode_autodetectButton]
    }
    else if (buttonThatWasClicked == &defaultsButton)
    {
        //[UserButtonCode_defaultsButton] -- add your button handler code here..
        controller_->resetPreferences();
        //[/UserButtonCode_defaultsButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

// Synchronize the UI state with the underlying state of the controller
void PreferencesComponent::synchronize(bool forceUpdates) {
    if(controller_ == 0)
        return;

    startTouchKeysButton.setToggleState(controller_->getPrefsAutoStartTouchKeys(), juce::NotificationType::dontSendNotification);
    autodetectButton.setToggleState(controller_->getPrefsAutodetectOctave(), juce::NotificationType::dontSendNotification);

    if(controller_->getPrefsStartupPresetNone())
        startupPresetComboBox.setSelectedId(kStartupPresetNone);
    else if(controller_->getPrefsStartupPresetVibratoPitchBend())
        startupPresetComboBox.setSelectedId(kStartupPresetVibratoPitchBend);
    else if(controller_->getPrefsStartupPresetLastSaved())
        startupPresetComboBox.setSelectedId(kStartupPresetLastSaved);
    else {
        juce::String path = controller_->getPrefsStartupPreset();
        startupPresetComboBox.setText(path);
    }
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PreferencesComponent" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers="controller_(0)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="296" initialHeight="152">
  <BACKGROUND backgroundColour="ffd2d2d2"/>
  <COMBOBOX name="Startup preset combo box" id="244410f02f6c1c72" memberName="startupPresetComboBox"
            virtualName="" explicitFocusOrder="0" pos="16 32 264 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="e9b3daa69a8ac5c" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="16 8 160 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Load preset on startup:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <TOGGLEBUTTON name="auto start TouchKeys button" id="62c82600413ca060" memberName="startTouchKeysButton"
                virtualName="" explicitFocusOrder="0" pos="16 64 208 24" buttonText="Start TouchKeys on startup"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="Autodetect button" id="69a491dfca4ea997" memberName="autodetectButton"
                virtualName="" explicitFocusOrder="0" pos="16 88 272 24" buttonText="Autodetect TouchKeys octave on each start"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TEXTBUTTON name="new button" id="89690e14d6bf00c0" memberName="defaultsButton"
              virtualName="" explicitFocusOrder="0" pos="16 120 144 24" buttonText="Reset to Defaults..."
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
