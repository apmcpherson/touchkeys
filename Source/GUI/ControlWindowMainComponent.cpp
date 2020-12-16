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
#include "KeyboardZoneComponent.h"
//[/Headers]

#include "ControlWindowMainComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ControlWindowMainComponent::ControlWindowMainComponent ()
    : controller_(0),
    midiInputGroupComponent{ "MIDI input group", "MIDI Input" },
    midiInputDeviceComboBox{ "MIDI input combo box" },
    label{ "new label", "Keyboard:" },
    groupComponent{ "new group", "TouchKeys" },
    label2{ "new label", "Device:\n" },
    touchkeyDeviceComboBox{ "TouchKeys combo box" },
    label3{ "new label", "Status:\n" },
    touchkeyStartButton{ "TouchKeys start button" },
    touchkeyStatusLabel{ "TouchKeys status label", "not running" },
    oscGroupComponent{ "OSC group", "OSC Output" },
    label7{ "new label", "Host:" },
    oscHostTextEditor{ "new text editor" },
    label8{ "new label", "Port:" },
    oscPortTextEditor{ "new text editor" },
    oscEnableButton{ "OSC enable button" },
    oscEnableRawButton{ "OSC enable raw button" },
    label4{ "new label", "Lowest Octave:" },
    touchkeyOctaveComboBox{ "TouchKeys octave box" },
    oscInputGroupComponent{ "MIDI input group", "OSC Input" },
    oscInputEnableButton{ "OSC input enable button" },
    label6{ "new label", "Port:" },
    oscInputPortTextEditor{ "new text editor" },
    keyboardZoneTabbedComponent{ juce::TabbedButtonBar::TabsAtTop },
    addZoneButton{ "add zone button" },
    removeZoneButton{ "remove zone button" },
    touchkeyAutodetectButton{ "TouchKeys autodetect button" },
    midiInputAuxDeviceComboBox{ "MIDI input aux combo box" },
    label5{ "new label", "Aux:" }

{
    addAndMakeVisible (midiInputGroupComponent);

    addAndMakeVisible (midiInputDeviceComboBox);
    midiInputDeviceComboBox.setEditableText (false);
    midiInputDeviceComboBox.setJustificationType (juce::Justification::centredLeft);
    midiInputDeviceComboBox.setTextWhenNothingSelected (juce::String{});
    midiInputDeviceComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    midiInputDeviceComboBox.addListener (this);

    addAndMakeVisible (label);
    label.setFont (juce::Font (15.00f, juce::Font::plain));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setEditable (false, false, false);
    label.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (groupComponent);

    addAndMakeVisible (label2);
    label2.setFont (juce::Font (15.00f, juce::Font::plain));
    label2.setJustificationType (juce::Justification::centredLeft);
    label2.setEditable (false, false, false);
    label2.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label2.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (touchkeyDeviceComboBox);
    touchkeyDeviceComboBox.setEditableText (false);
    touchkeyDeviceComboBox.setJustificationType (juce::Justification::centredLeft);
    touchkeyDeviceComboBox.setTextWhenNothingSelected (juce::String{});
    touchkeyDeviceComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    touchkeyDeviceComboBox.addListener (this);

    addAndMakeVisible (label3);
    label3.setFont (juce::Font (15.00f, juce::Font::plain));
    label3.setJustificationType (juce::Justification::centredLeft);
    label3.setEditable (false, false, false);
    label3.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label3.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (touchkeyStartButton);
    touchkeyStartButton.setButtonText ("Start");
    touchkeyStartButton.addListener (this);

    addAndMakeVisible (touchkeyStatusLabel);
    touchkeyStatusLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    touchkeyStatusLabel.setJustificationType (juce::Justification::centredLeft);
    touchkeyStatusLabel.setEditable (false, false, false);
    touchkeyStatusLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    touchkeyStatusLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (oscGroupComponent);

    addAndMakeVisible (label7);
    label7.setFont (juce::Font (15.00f, juce::Font::plain));
    label7.setJustificationType (juce::Justification::centredLeft);
    label7.setEditable (false, false, false);
    label7.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label7.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (oscHostTextEditor);
    oscHostTextEditor.setMultiLine (false);
    oscHostTextEditor.setReturnKeyStartsNewLine (false);
    oscHostTextEditor.setReadOnly (false);
    oscHostTextEditor.setScrollbarsShown (true);
    oscHostTextEditor.setCaretVisible (true);
    oscHostTextEditor.setPopupMenuEnabled (true);
    oscHostTextEditor.setText ("127.0.0.1");

    addAndMakeVisible (label8);
    label8.setFont (juce::Font (15.00f, juce::Font::plain));
    label8.setJustificationType (juce::Justification::centredLeft);
    label8.setEditable (false, false, false);
    label8.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label8.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (oscPortTextEditor);
    oscPortTextEditor.setMultiLine (false);
    oscPortTextEditor.setReturnKeyStartsNewLine (false);
    oscPortTextEditor.setReadOnly (false);
    oscPortTextEditor.setScrollbarsShown (true);
    oscPortTextEditor.setCaretVisible (true);
    oscPortTextEditor.setPopupMenuEnabled (true);
    oscPortTextEditor.setText ("8000");

    addAndMakeVisible (oscEnableButton);
    oscEnableButton.setButtonText ("Enable OSC output");
    oscEnableButton.addListener (this);

    addAndMakeVisible (oscEnableRawButton);
    oscEnableRawButton.setButtonText ("Send raw frames");
    oscEnableRawButton.addListener (this);

    addAndMakeVisible (label4);
    label4.setFont (juce::Font (15.00f, juce::Font::plain));
    label4.setJustificationType (juce::Justification::centredLeft);
    label4.setEditable (false, false, false);
    label4.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label4.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (touchkeyOctaveComboBox);
    touchkeyOctaveComboBox.setEditableText (false);
    touchkeyOctaveComboBox.setJustificationType (juce::Justification::centredLeft);
    touchkeyOctaveComboBox.setTextWhenNothingSelected (juce::String{});
    touchkeyOctaveComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    touchkeyOctaveComboBox.addListener (this);

    addAndMakeVisible (oscInputGroupComponent);

    addAndMakeVisible (oscInputEnableButton);
    oscInputEnableButton.setButtonText ("Enable OSC input");
    oscInputEnableButton.addListener (this);

    addAndMakeVisible (label6);
    label6.setFont (juce::Font (15.00f, juce::Font::plain));
    label6.setJustificationType (juce::Justification::centredLeft);
    label6.setEditable (false, false, false);
    label6.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label6.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (oscInputPortTextEditor);
    oscInputPortTextEditor.setMultiLine (false);
    oscInputPortTextEditor.setReturnKeyStartsNewLine (false);
    oscInputPortTextEditor.setReadOnly (false);
    oscInputPortTextEditor.setScrollbarsShown (true);
    oscInputPortTextEditor.setCaretVisible (true);
    oscInputPortTextEditor.setPopupMenuEnabled (true);
    oscInputPortTextEditor.setText ("8001");

    addAndMakeVisible (keyboardZoneTabbedComponent);
    keyboardZoneTabbedComponent.setTabBarDepth (30);
    keyboardZoneTabbedComponent.setCurrentTabIndex (-1);

    addAndMakeVisible (addZoneButton);
    addZoneButton.setButtonText ("Add");
    addZoneButton.addListener (this);

    addAndMakeVisible (removeZoneButton);
    removeZoneButton.setButtonText ("Del");
    removeZoneButton.addListener (this);

    addAndMakeVisible (touchkeyAutodetectButton);
    touchkeyAutodetectButton.setButtonText ("Detect");
    touchkeyAutodetectButton.addListener (this);

    addAndMakeVisible (midiInputAuxDeviceComboBox);
    midiInputAuxDeviceComboBox.setEditableText (false);
    midiInputAuxDeviceComboBox.setJustificationType (juce::Justification::centredLeft);
    midiInputAuxDeviceComboBox.setTextWhenNothingSelected (juce::String{});
    midiInputAuxDeviceComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    midiInputAuxDeviceComboBox.addListener (this);

    addAndMakeVisible (label5);
    label5.setFont (juce::Font (15.00f, juce::Font::plain));
    label5.setJustificationType (juce::Justification::centredRight);
    label5.setEditable (false, false, false);
    label5.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label5.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);


    //[UserPreSize]
    lastSelectedMidiInputID_ = -1;
    lastSelectedMidiAuxInputID_ = -1;
    lastSegmentUniqueIdentifier_ = -1;

    // Add octave labels to combo box
    for(int i = 0; i <= kTouchkeysMaxOctave; i++) {
        touchkeyOctaveComboBox.addItem("C" + juce::String(i), i + kTouchkeysComponentComboBoxOffset);
    }
    //[/UserPreSize]

    setSize (872, 444);


    //[Constructor] You can add your own custom stuff here..
    oscHostTextEditor.addListener(this);
    oscPortTextEditor.addListener(this);
    oscInputPortTextEditor.addListener(this);
    //[/Constructor]
}

ControlWindowMainComponent::~ControlWindowMainComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ControlWindowMainComponent::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll ( juce::Colour (0xffd2d2d2));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ControlWindowMainComponent::resized()
{
    midiInputGroupComponent.setBounds (8, 144, 304, 96);
    midiInputDeviceComboBox.setBounds (80, 168, 216, 24);
    label.setBounds (16, 168, 64, 24);
    groupComponent.setBounds (8, 8, 304, 128);
    label2.setBounds (16, 32, 60, 24);
    touchkeyDeviceComboBox.setBounds (72, 32, 224, 24);
    label3.setBounds (16, 96, 60, 24);
    touchkeyStartButton.setBounds (216, 96, 79, 24);
    touchkeyStatusLabel.setBounds (72, 96, 136, 24);
    oscGroupComponent.setBounds (8, 320, 304, 96);
    label7.setBounds (16, 376, 55, 24);
    oscHostTextEditor.setBounds (64, 376, 128, 24);
    label8.setBounds (200, 376, 40, 24);
    oscPortTextEditor.setBounds (240, 376, 56, 24);
    oscEnableButton.setBounds (24, 344, 144, 24);
    oscEnableRawButton.setBounds (176, 344, 144, 24);
    label4.setBounds (16, 64, 104, 24);
    touchkeyOctaveComboBox.setBounds (120, 64, 88, 24);
    oscInputGroupComponent.setBounds (8, 248, 304, 64);
    oscInputEnableButton.setBounds (24, 272, 152, 24);
    label6.setBounds (200, 272, 40, 24);
    oscInputPortTextEditor.setBounds (240, 272, 56, 24);
    keyboardZoneTabbedComponent.setBounds (320, 0, 552, 464);
    addZoneButton.setBounds (776, 4, 38, 20);
    removeZoneButton.setBounds (824, 4, 38, 20);
    touchkeyAutodetectButton.setBounds (216, 64, 79, 24);
    midiInputAuxDeviceComboBox.setBounds (80, 200, 216, 24);
    label5.setBounds (24, 200, 55, 24);
    //[UserResized] Add your own custom resize handling here..
    
    // Resize KeyboardZoneComponent to fit new bounds
    juce::Rectangle<int> const& ourBounds = getBounds();
    juce::Rectangle<int> keyboardZoneBounds = keyboardZoneTabbedComponent.getBounds();
    keyboardZoneBounds.setHeight(ourBounds.getHeight() - keyboardZoneBounds.getY());
    keyboardZoneTabbedComponent.setBounds(keyboardZoneBounds);
    //[/UserResized]
}

void ControlWindowMainComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    if(controller_ == 0)
        return;
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == &midiInputDeviceComboBox)
    {
        //[UserComboBoxCode_midiInputDeviceComboBox] -- add your combo box handling code here..

        // Look up the selected ID, remembering that Juce indices start at 1 and the first of
        // these is "Disabled"
        int selection = midiInputDeviceComboBox.getSelectedId() - kMidiInputDeviceComboBoxOffset;
        if(selection == 1 - kMidiInputDeviceComboBoxOffset) {   // Disabled
            if(controller_->midiTouchkeysStandaloneModeIsEnabled())
                controller_->midiTouchkeysStandaloneModeDisable();
            controller_->disablePrimaryMIDIInputPort();
        }
        else if(selection == 2 - kMidiInputDeviceComboBoxOffset) {  // Standalone mode
            controller_->disablePrimaryMIDIInputPort();
            controller_->midiTouchkeysStandaloneModeEnable();
        }
        else if(selection >= 0 && selection < midiInputDeviceIDs_.size()) {
            int deviceId = midiInputDeviceIDs_[selection];
            if(controller_->midiTouchkeysStandaloneModeIsEnabled())
                controller_->midiTouchkeysStandaloneModeDisable();
            controller_->enableMIDIInputPort(deviceId, true);
        }
        //[/UserComboBoxCode_midiInputDeviceComboBox]
    }
    else if (comboBoxThatHasChanged == &touchkeyDeviceComboBox)
    {
        //[UserComboBoxCode_touchkeyDeviceComboBox] -- add your combo box handling code here..
        // Nothing to do here right away -- wait until start button is pressed
        //[/UserComboBoxCode_touchkeyDeviceComboBox]
    }
    else if (comboBoxThatHasChanged == &touchkeyOctaveComboBox)
    {
        //[UserComboBoxCode_touchkeyOctaveComboBox] -- add your combo box handling code here..
        int octave = touchkeyOctaveComboBox.getSelectedId() - kTouchkeysComponentComboBoxOffset;

        // Convert octave number to MIDI note (C4 = 60)
        if(controller_ != 0)
            controller_->touchkeyDeviceSetLowestMidiNote((octave + 1)*12);
        //[/UserComboBoxCode_touchkeyOctaveComboBox]
    }
    else if (comboBoxThatHasChanged == &midiInputAuxDeviceComboBox)
    {
        //[UserComboBoxCode_midiInputAuxDeviceComboBox] -- add your combo box handling code here..
        
        // Look up the selected ID, remembering that Juce indices start at 1 and the first of
        // these is "Disabled"
        int selection = midiInputAuxDeviceComboBox.getSelectedId() - kMidiInputDeviceComboBoxOffset;
        if(selection == 1 - kMidiInputDeviceComboBoxOffset) {   // Disabled
            // Disable all aux ports
            controller_->disableAllMIDIInputPorts(true);
        }
        else if(selection == 2 - kMidiInputDeviceComboBoxOffset) {
            // Shouldn't happen; standalone mode not an aux feature
            controller_->disableAllMIDIInputPorts(true);
        }
        else if(selection >= 0 && selection < midiInputDeviceIDs_.size()) {
            int deviceId = midiInputDeviceIDs_[selection];
            // Enable this aux port
            controller_->disableAllMIDIInputPorts(true);
            controller_->enableMIDIInputPort(deviceId, false);
        }
        //[/UserComboBoxCode_midiInputAuxDeviceComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void ControlWindowMainComponent::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    if(controller_ == 0)
        return;
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == &touchkeyStartButton)
    {
        //[UserButtonCode_touchkeyStartButton] -- add your button handler code here..
#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST
        if(controller_->touchkeySensorTestIsRunning()) {
            // TouchKeys were performing a sensor test. Stop the test.
            controller_->touchkeySensorTestStop();
        }
        else if(controller_->touchkeyDeviceIsRunning()) {
#else
        if(controller_->touchkeyDeviceIsRunning()) {
#endif
            // TouchKeys were running. Stop and close.
            controller_->closeTouchkeyDevice();
        }
        else {
            // TouchKeys weren't running. Open and start.
            juce::String devName = controller_->touchkeyDevicePrefix().c_str();
            devName += touchkeyDeviceComboBox.getText();

            // This will attempt to start the device and update the state accordingly
            controller_->touchkeyDeviceStartupSequence(devName.toUTF8());
        }
        //[/UserButtonCode_touchkeyStartButton]
    }
    else if (buttonThatWasClicked == &oscEnableButton)
    {
        //[UserButtonCode_oscEnableButton] -- add your button handler code here..
        controller_->oscTransmitSetEnabled(oscEnableButton.getToggleState());
        //[/UserButtonCode_oscEnableButton]
    }
    else if (buttonThatWasClicked == &oscEnableRawButton)
    {
        //[UserButtonCode_oscEnableRawButton] -- add your button handler code here..
        controller_->oscTransmitSetRawDataEnabled(oscEnableRawButton.getToggleState());
        //[/UserButtonCode_oscEnableRawButton]
    }
    else if (buttonThatWasClicked == &oscInputEnableButton)
    {
        //[UserButtonCode_oscInputEnableButton] -- add your button handler code here..
        controller_->oscReceiveSetEnabled(oscInputEnableButton.getToggleState());
        //[/UserButtonCode_oscInputEnableButton]
    }
    else if (buttonThatWasClicked == &addZoneButton)
    {
        //[UserButtonCode_addZoneButton] -- add your button handler code here..
        controller_->midiSegmentAdd();
        //[/UserButtonCode_addZoneButton]
    }
    else if (buttonThatWasClicked == &removeZoneButton)
    {
        //[UserButtonCode_removeZoneButton] -- add your button handler code here..
        int tabIndex = keyboardZoneTabbedComponent.getCurrentTabIndex();
        if(tabIndex != 0) {
            KeyboardZoneComponent* selectedComponent = static_cast<KeyboardZoneComponent*> (keyboardZoneTabbedComponent.getTabContentComponent(tabIndex));
            controller_->midiSegmentRemove(selectedComponent->keyboardSegment());
        }
        //[/UserButtonCode_removeZoneButton]
    }
    else if (buttonThatWasClicked == &touchkeyAutodetectButton)
    {
        //[UserButtonCode_touchkeyAutodetectButton] -- add your button handler code here..
        if(controller_->touchkeyDeviceIsAutodetecting())
            controller_->touchkeyDeviceStopAutodetecting();
        else
            controller_->touchkeyDeviceAutodetectLowestMidiNote();
        //[/UserButtonCode_touchkeyAutodetectButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void ControlWindowMainComponent::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(controller_ == 0)
        return;
    if(&editor == &oscHostTextEditor || &editor == &oscPortTextEditor)
        updateOscHostPort();
    else if(&editor == &oscInputPortTextEditor) {
        int port = atoi(oscInputPortTextEditor.getText().toUTF8());
        controller_->oscReceiveSetPort(port);
    }
}

void ControlWindowMainComponent::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{
    // Nothing to do here
}

void ControlWindowMainComponent::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

// Update list of TouchKeys and MIDI input devices
void ControlWindowMainComponent::updateInputDeviceList()
{
    if(controller_ == 0)
        return;

    // *** TouchKeys devices ***
    std::vector<std::string> tkdevices = controller_->availableTouchkeyDevices();
    std::vector<std::string>::iterator tkit;
    int counter;

    touchkeyDeviceComboBox.clear();

    if(tkdevices.size() == 0) {
        touchkeyDeviceComboBox.addItem("No devices", 1);
        touchkeyDeviceComboBox.setSelectedId(1, juce::NotificationType::dontSendNotification);
        touchkeyDeviceComboBox.setEnabled(false);
        touchkeyStartButton.setEnabled(false);
    }
    else {
        counter = 1;
        for(tkit = tkdevices.begin(); tkit != tkdevices.end(); ++tkit) {
            touchkeyDeviceComboBox.addItem(tkit->c_str(), counter++);
        }
        touchkeyDeviceComboBox.setSelectedId(1, juce::NotificationType::dontSendNotification);
        touchkeyDeviceComboBox.setEnabled(true);
        touchkeyStartButton.setEnabled(true);
    }

    // *** MIDI input devices ***
    std::vector<std::pair<int, std::string> > devices = controller_->availableMIDIInputDevices();

    midiInputDeviceComboBox.clear();
    midiInputDeviceIDs_.clear();
    midiInputDeviceComboBox.addItem("Disabled", 1);
    midiInputDeviceComboBox.addItem("TouchKeys Standalone", 2);
    
    midiInputAuxDeviceComboBox.clear();
    midiInputAuxDeviceComboBox.addItem("Disabled", 1);
    
    counter = kMidiInputDeviceComboBoxOffset;

    // Check whether the currently selected ID still exists while
    // we build the list
    bool lastSelectedDeviceExists = false;
    bool lastSelectedAuxDeviceExists = false;
    for( auto it = devices.begin(); it != devices.end(); ++it) {
        midiInputDeviceComboBox.addItem((*it).second.c_str(), counter);
        midiInputAuxDeviceComboBox.addItem((*it).second.c_str(), counter);
        midiInputDeviceIDs_.push_back(it->first);
        if(it->first == lastSelectedMidiInputID_)
            lastSelectedDeviceExists = true;
        if(it->first == lastSelectedMidiAuxInputID_)
            lastSelectedAuxDeviceExists = true;
        counter++;
    }

    if(!lastSelectedDeviceExists && lastSelectedMidiInputID_ >= 0)
        controller_->disablePrimaryMIDIInputPort();
    if(!lastSelectedAuxDeviceExists && lastSelectedMidiAuxInputID_ >= 0)
        controller_->disableAllMIDIInputPorts(true);
}

void ControlWindowMainComponent::updateOscHostPort()
{
    if(controller_ == 0)
        return;

    juce::String oscHost = oscHostTextEditor.getText();
    juce::String oscPort = oscPortTextEditor.getText();
    controller_->oscTransmitClearAddresses();
    controller_->oscTransmitAddAddress(oscHost.toUTF8(), oscPort.toUTF8());
}

// Synchronize the UI state with the underlying state of the controller
void ControlWindowMainComponent::synchronize() {
    if(controller_ == 0)
        return;

    bool devicesUpdated = false;

    if(controller_->devicesShouldUpdate() != lastControllerUpdateDeviceCount_) {
        lastControllerUpdateDeviceCount_ = controller_->devicesShouldUpdate();
        updateInputDeviceList();
        devicesUpdated = true;
    }

    // Update TouchKeys status
#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST
    if(controller_->touchkeySensorTestIsRunning()) {
        touchkeyStartButton.setButtonText("Stop");
        touchkeyStatusLabel.setText("Testing", dontSendNotification);
    }
    else if(controller_->touchkeyDeviceIsRunning()) {
#else
    if(controller_->touchkeyDeviceIsRunning()) {
#endif
        touchkeyStartButton.setButtonText("Stop");
        touchkeyStatusLabel.setText("Running", juce::NotificationType::dontSendNotification);
    }
    else if(controller_->touchkeyDeviceErrorOccurred()) {
        touchkeyStartButton.setButtonText("Start");
        touchkeyStatusLabel.setText(controller_->touchkeyDeviceErrorMessage().c_str(), juce::NotificationType::dontSendNotification);
    }
    else {
        touchkeyStartButton.setButtonText("Start");
        touchkeyStatusLabel.setText("Not running", juce::NotificationType::dontSendNotification);
    }

    // Update MIDI input status
    if(controller_->midiTouchkeysStandaloneModeIsEnabled()) {
        midiInputDeviceComboBox.setSelectedId(2, juce::NotificationType::dontSendNotification);
    }
    else {
        // First query the primary port
        int selectedPrimaryPort = controller_->selectedMIDIPrimaryInputPort();
        if(selectedPrimaryPort < 0) {
            midiInputDeviceComboBox.setSelectedId(1, juce::NotificationType::dontSendNotification);
        }
        else if(selectedPrimaryPort != lastSelectedMidiInputID_ || devicesUpdated){
            // Input has changed from before. Find it in vector
            // If there is more than one selected ID, we will only take the first one for
            // the current UI. This affects the display but not the functionality.
            for(int i = 0; i < midiInputDeviceIDs_.size(); i++) {
                if(midiInputDeviceIDs_[i] == selectedPrimaryPort) {
                    midiInputDeviceComboBox.setSelectedId(i + kMidiInputDeviceComboBoxOffset, juce::NotificationType::dontSendNotification);
                    break;
                }
            }
            // ...and cache this as the last ID so we don't search again next time
            lastSelectedMidiInputID_ = selectedPrimaryPort;
            
            // Now disable this item in the auxiliary combo box
            for(int i = 0; i < midiInputAuxDeviceComboBox.getNumItems(); i++) {
                int itemId = midiInputAuxDeviceComboBox.getItemId(i) - kMidiInputDeviceComboBoxOffset;
                if(itemId >= 0) {
                    midiInputAuxDeviceComboBox.setItemEnabled(midiInputAuxDeviceComboBox.getItemId(i),
                                                               (itemId != selectedPrimaryPort));
                }
            }
        }
    }
        
    // Then get all aux ports and display the first one
    const std::vector<int>& selectedMidiInputDevices(controller_->selectedMIDIAuxInputPorts());
    if(selectedMidiInputDevices.empty()) {
        midiInputAuxDeviceComboBox.setSelectedId(1, juce::NotificationType::dontSendNotification);
    }
    else if(selectedMidiInputDevices.front() != lastSelectedMidiAuxInputID_ || devicesUpdated){
        // Input has changed from before. Find it in vector
        // If there is more than one selected ID, we will only take the first one for
        // the current UI. This affects the display but not the functionality.
        for(int i = 0; i < midiInputDeviceIDs_.size(); i++) {
            if(midiInputDeviceIDs_[i] == selectedMidiInputDevices.front()) {
                midiInputAuxDeviceComboBox.setSelectedId(i + kMidiInputDeviceComboBoxOffset, juce::NotificationType::dontSendNotification);
                break;
            }
        }
        // ...and cache this as the last ID so we don't search again next time
        lastSelectedMidiAuxInputID_ = selectedMidiInputDevices.front();
    }

    // Update OSC status
    oscEnableButton.setToggleState(controller_->oscTransmitEnabled(), juce::NotificationType::dontSendNotification);
    oscEnableRawButton.setToggleState(controller_->oscTransmitRawDataEnabled(), juce::NotificationType::dontSendNotification);
    oscInputEnableButton.setToggleState(controller_->oscReceiveEnabled(), juce::NotificationType::dontSendNotification);

    // Update the OSC fields only if the text editors aren't active
    if(!oscHostTextEditor.hasKeyboardFocus(true) && !oscPortTextEditor.hasKeyboardFocus(true)) {
        const std::vector<lo_address>& oscAddresses = controller_->oscTransmitAddresses();
        if(oscAddresses.empty()) {
            oscHostTextEditor.setText("", false);
            oscPortTextEditor.setText("", false);
        }
        else {
            // Take the first address to display in the text editor. As with MIDI input,
            // this doesn't affect the functionality, only the UI display.
            lo_address firstAddress = oscAddresses.front();

            oscHostTextEditor.setText(lo_address_get_hostname(firstAddress), false);
            oscPortTextEditor.setText(lo_address_get_port(firstAddress), false);
        }
    }
    if(!oscInputPortTextEditor.hasKeyboardFocus(true)) {
        int port = controller_->oscReceivePort();
        oscInputPortTextEditor.setText(juce::String(port), false);
    }

    // Set the octave button
    int octave = (controller_->touchkeyDeviceLowestMidiNote() / 12) - 1;
    if(octave >= 0 && octave <= kTouchkeysMaxOctave)
        touchkeyOctaveComboBox.setSelectedId(octave + kTouchkeysComponentComboBoxOffset, juce::NotificationType::dontSendNotification);

    // Enable or disable the autodetect button depending on the device status
    if(!controller_->touchkeyDeviceIsRunning()) {
        touchkeyAutodetectButton.setEnabled(false);
    }
    else if(controller_->touchkeyDeviceIsAutodetecting()) {
        touchkeyAutodetectButton.setEnabled(true);
        touchkeyAutodetectButton.setButtonText("Cancel");
    }
    else {
        touchkeyAutodetectButton.setEnabled(true);
        touchkeyAutodetectButton.setButtonText("Detect");
    }

    // Update segments list if it has changed
    if(lastSegmentUniqueIdentifier_ != controller_->midiSegmentUniqueIdentifier())
        updateKeyboardSegments();

    // Synchronize every tab component
    for(int tab = 0; tab < keyboardZoneTabbedComponent.getNumTabs(); tab++) {
        KeyboardZoneComponent *component = static_cast<KeyboardZoneComponent*> (keyboardZoneTabbedComponent.getTabContentComponent(tab));
        component->synchronize(devicesUpdated);
    }

    // Update add/remove buttons
    if(keyboardZoneTabbedComponent.getCurrentTabIndex() <= 0) {
        removeZoneButton.setEnabled(false);
    }
    else {
        removeZoneButton.setEnabled(true);
    }
    if(controller_->midiSegmentsCount() >= 8)
        addZoneButton.setEnabled(false);
    else
        addZoneButton.setEnabled(true);
}

// Return the currently selected TouchKeys string
juce::String ControlWindowMainComponent::currentTouchkeysSelectedPath()
{
    juce::String devName = controller_->touchkeyDevicePrefix().c_str();
    devName += touchkeyDeviceComboBox.getText();

    return devName;
}

// Update the state of the keyboard segment tab bar. Called only when segments change
void ControlWindowMainComponent::updateKeyboardSegments()
{
    if(controller_ == 0)
        return;
    // Update the identifier to say we've matched the current state of the segments
    lastSegmentUniqueIdentifier_ = controller_->midiSegmentUniqueIdentifier();
    
    // Save the current selected index in case we later remove it
    int currentlySelectedIndex = keyboardZoneTabbedComponent.getCurrentTabIndex();
    
    KeyboardZoneComponent* currentlySelectedComponent = static_cast<KeyboardZoneComponent*> (keyboardZoneTabbedComponent.getTabContentComponent(currentlySelectedIndex));
    MidiKeyboardSegment* currentlySelectedSegment = 0;
    if(currentlySelectedComponent != 0)
        currentlySelectedSegment = currentlySelectedComponent->keyboardSegment();
    bool selectedNewTab = false;
    
    // First, go through the segments and create tabs as needed
    int maxNumSegments = controller_->midiSegmentsCount();
    for(int i = 0; i < maxNumSegments; i++) {
        MidiKeyboardSegment* segment = controller_->midiSegment(i);
        bool matched = false;
        if(segment == 0)
            continue;
        // Look for this segment among the tabs we already have
        for(int tab = 0; tab < keyboardZoneTabbedComponent.getNumTabs(); tab++) {
            KeyboardZoneComponent *component = static_cast<KeyboardZoneComponent*> (keyboardZoneTabbedComponent.getTabContentComponent(tab));
            if(component->keyboardSegment() == segment && component->keyboardZone() == segment->outputPort()) {
                // Found it...
                matched = true;
                break;
            }
        }
        // If we didn't find it, add a tab for this segment
        if(!matched) {
            KeyboardZoneComponent *newComponent = new KeyboardZoneComponent();
            newComponent->setMainApplicationController(controller_);
            newComponent->setKeyboardSegment(segment, segment->outputPort());

            char name[16];
#ifdef _MSC_VER
			_snprintf_s(name, 16, _TRUNCATE, "Zone %d", segment->outputPort());
#else
            snprintf(name, 16, "Zone %d", segment->outputPort());
#endif

            // Add the component, telling the tab manager to take charge of deleting it at the end
            keyboardZoneTabbedComponent.addTab(name, juce::Colours::lightgrey, newComponent, true);
            keyboardZoneTabbedComponent.setCurrentTabIndex(keyboardZoneTabbedComponent.getNumTabs() - 1);
            selectedNewTab = true;

            //std::cout << "Adding tab for segment " << segment << '\n';
        }
    }

    // Now go through the other way and remove tabs that are no longer needed
    // Iterate through each tab: find a match in the segments
    int tab = 0;
    while(tab < keyboardZoneTabbedComponent.getNumTabs()) {
        KeyboardZoneComponent *component = static_cast<KeyboardZoneComponent*> (keyboardZoneTabbedComponent.getTabContentComponent(tab));
        MidiKeyboardSegment *segment = component->keyboardSegment();
        bool matched = false;

        for(int i = 0; i < maxNumSegments; i++) {
            if(segment == controller_->midiSegment(i) && component->keyboardZone() == segment->outputPort()) {
                matched = true;
                break;
            }
        }
        if(segment == 0 || !matched) {
            // This tab holds a nonexistent segment and should be removed
            keyboardZoneTabbedComponent.removeTab(tab);
            
            if(currentlySelectedSegment == segment) {
                // The currently selected tab has been removed. Select the prior one.
                if(currentlySelectedIndex > 0) {
                    int indexToSelect = currentlySelectedIndex - 1;
                    if(indexToSelect >= keyboardZoneTabbedComponent.getNumTabs())
                        indexToSelect = keyboardZoneTabbedComponent.getNumTabs() - 1;
                    if(indexToSelect < 0)
                        indexToSelect = 0;
                    keyboardZoneTabbedComponent.setCurrentTabIndex(indexToSelect);
                }
                else
                    keyboardZoneTabbedComponent.setCurrentTabIndex(0);
            }

            // And we have to start over again since the tab indexing has changed
            tab = 0;
        }
        else    // Found a match: check the next tab
            tab++;
        // Eventually, we get to the end of the list of tabs an we know every existing tab matches a segment
    }
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ControlWindowMainComponent"
                 componentName="" parentClasses="public juce::Component, public juce::TextEditor::Listener"
                 constructorParams="" variableInitialisers="controller_(0)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="0"
                 initialWidth="872" initialHeight="444">
  <BACKGROUND backgroundColour="ffd2d2d2"/>
  <GROUPCOMPONENT name="MIDI input group" id="ce80a86ee6475cd9" memberName="midiInputGroupComponent"
                  virtualName="" explicitFocusOrder="0" pos="8 144 304 96" title="MIDI Input"/>
  <COMBOBOX name="MIDI input combo box" id="def32c74505cfa50" memberName="midiInputDeviceComboBox"
            virtualName="" explicitFocusOrder="0" pos="80 168 216 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="ad7bc4640d8023b7" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="16 168 64 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Keyboard:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <GROUPCOMPONENT name="new group" id="9106305fd2211185" memberName="groupComponent"
                  virtualName="" explicitFocusOrder="0" pos="8 8 304 128" title="TouchKeys"/>
  <LABEL name="new label" id="944877a84dcfc602" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="16 32 60 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Device:&#10;" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <COMBOBOX name="TouchKeys combo box" id="871223bdcad0e693" memberName="touchkeyDeviceComboBox"
            virtualName="" explicitFocusOrder="0" pos="72 32 224 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="1cdf89082d95c72c" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="16 96 60 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Status:&#10;" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <TEXTBUTTON name="TouchKeys start button" id="1bb1c69c957fc984" memberName="touchkeyStartButton"
              virtualName="" explicitFocusOrder="0" pos="216 96 79 24" buttonText="Start"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <LABEL name="TouchKeys status label" id="c91b132696e6ba1d" memberName="touchkeyStatusLabel"
         virtualName="" explicitFocusOrder="0" pos="72 96 136 24" edTextCol="ff000000"
         edBkgCol="0" labelText="not running" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <GROUPCOMPONENT name="OSC group" id="8268119e22809825" memberName="oscGroupComponent"
                  virtualName="" explicitFocusOrder="0" pos="8 320 304 96" title="OSC Output"/>
  <LABEL name="new label" id="896c0c48a1cf50a" memberName="label7" virtualName=""
         explicitFocusOrder="0" pos="16 376 55 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Host:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="new text editor" id="84778d0bbebedd36" memberName="oscHostTextEditor"
              virtualName="" explicitFocusOrder="0" pos="64 376 128 24" initialText="127.0.0.1"
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <LABEL name="new label" id="157c85bf83a7f936" memberName="label8" virtualName=""
         explicitFocusOrder="0" pos="200 376 40 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Port:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="new text editor" id="7c21f0c238812d11" memberName="oscPortTextEditor"
              virtualName="" explicitFocusOrder="0" pos="240 376 56 24" initialText="8000"
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <TOGGLEBUTTON name="OSC enable button" id="ccd52591cfd0b632" memberName="oscEnableButton"
                virtualName="" explicitFocusOrder="0" pos="24 344 144 24" buttonText="Enable OSC output"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="OSC enable raw button" id="4aaf8f80edaff24" memberName="oscEnableRawButton"
                virtualName="" explicitFocusOrder="0" pos="176 344 144 24" buttonText="Send raw frames"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="new label" id="c5873c6498f8156d" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="16 64 104 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Lowest Octave:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <COMBOBOX name="TouchKeys octave box" id="36ace32027c81d30" memberName="touchkeyOctaveComboBox"
            virtualName="" explicitFocusOrder="0" pos="120 64 88 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <GROUPCOMPONENT name="MIDI input group" id="bb54712f78382055" memberName="oscInputGroupComponent"
                  virtualName="" explicitFocusOrder="0" pos="8 248 304 64" title="OSC Input"/>
  <TOGGLEBUTTON name="OSC input enable button" id="22a196770a440560" memberName="oscInputEnableButton"
                virtualName="" explicitFocusOrder="0" pos="24 272 152 24" buttonText="Enable OSC input"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="new label" id="c680c2da87cdcbf2" memberName="label6" virtualName=""
         explicitFocusOrder="0" pos="200 272 40 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Port:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="new text editor" id="d4a91e8bff5b6bc9" memberName="oscInputPortTextEditor"
              virtualName="" explicitFocusOrder="0" pos="240 272 56 24" initialText="8001"
              multiline="0" retKeyStartsLine="0" readonly="0" scrollbars="1"
              caret="1" popupmenu="1"/>
  <TABBEDCOMPONENT name="keyboard zone tabbed component" id="33da3d6583cdacbf" memberName="keyboardZoneTabbedComponent"
                   virtualName="" explicitFocusOrder="0" pos="320 0 552 464" orientation="top"
                   tabBarDepth="30" initialTab="-1"/>
  <TEXTBUTTON name="add zone button" id="1d2fa7fd74f31315" memberName="addZoneButton"
              virtualName="" explicitFocusOrder="0" pos="776 4 38 20" buttonText="Add"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="remove zone button" id="7865f7787a191e0e" memberName="removeZoneButton"
              virtualName="" explicitFocusOrder="0" pos="824 4 38 20" buttonText="Del"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="TouchKeys autodetect button" id="6e19894bc11d0276" memberName="touchkeyAutodetectButton"
              virtualName="" explicitFocusOrder="0" pos="216 64 79 24" buttonText="Detect"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <COMBOBOX name="MIDI input aux combo box" id="1b77c934a4790942" memberName="midiInputAuxDeviceComboBox"
            virtualName="" explicitFocusOrder="0" pos="80 200 216 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="7409cb92cfa3b9f2" memberName="label5" virtualName=""
         explicitFocusOrder="0" pos="24 200 55 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Aux:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif  // TOUCHKEYS_NO_GUI
//[/EndFile]
