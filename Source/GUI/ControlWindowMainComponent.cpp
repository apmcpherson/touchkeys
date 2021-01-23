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
#include "KeyboardZoneComponent.h"
#include "ControlWindowMainComponent.h"

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
    setLookAndFeel( &lnf );

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

    addAndMakeVisible (groupComponent);

    addAndMakeVisible (label2);
    label2.setFont (juce::Font (15.00f, juce::Font::plain));
    label2.setJustificationType (juce::Justification::centredLeft);
    label2.setEditable (false, false, false);

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

    addAndMakeVisible (touchkeyStartButton);
    touchkeyStartButton.setButtonText ("Start");
    touchkeyStartButton.addListener (this);

    addAndMakeVisible (touchkeyStatusLabel);
    touchkeyStatusLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    touchkeyStatusLabel.setJustificationType (juce::Justification::centredLeft);
    touchkeyStatusLabel.setEditable (false, false, false);

    addAndMakeVisible (oscGroupComponent);

    addAndMakeVisible (label7);
    label7.setFont (juce::Font (15.00f, juce::Font::plain));
    label7.setJustificationType (juce::Justification::centredLeft);
    label7.setEditable (false, false, false);

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

    lastSelectedMidiInputID_ = -1;
    lastSelectedMidiAuxInputID_ = -1;
    lastSegmentUniqueIdentifier_ = -1;

    // Add octave labels to combo box
    for(int i = 0; i <= kTouchkeysMaxOctave; i++) {
        touchkeyOctaveComboBox.addItem("C" + juce::String(i), i + kTouchkeysComponentComboBoxOffset);
    }

    setSize (872, 444);

    oscHostTextEditor.addListener(this);
    oscPortTextEditor.addListener(this);
    oscInputPortTextEditor.addListener(this);
}

ControlWindowMainComponent::~ControlWindowMainComponent()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void ControlWindowMainComponent::paint (juce::Graphics& g)
{
    g.fillAll ( juce::Colour (0xffd2d2d2));
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
    
    // Resize KeyboardZoneComponent to fit new bounds
    juce::Rectangle<int> const& ourBounds = getBounds();
    juce::Rectangle<int> keyboardZoneBounds = keyboardZoneTabbedComponent.getBounds();
    keyboardZoneBounds.setHeight(ourBounds.getHeight() - keyboardZoneBounds.getY());
    keyboardZoneTabbedComponent.setBounds(keyboardZoneBounds);
}

void ControlWindowMainComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if(controller_ == nullptr)
        return;

    if (comboBoxThatHasChanged == &midiInputDeviceComboBox)
    {
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
    }
    else if (comboBoxThatHasChanged == &touchkeyDeviceComboBox)
    {
        // Nothing to do here right away -- wait until start button is pressed
    }
    else if (comboBoxThatHasChanged == &touchkeyOctaveComboBox)
    {
        int octave = touchkeyOctaveComboBox.getSelectedId() - kTouchkeysComponentComboBoxOffset;

        // Convert octave number to MIDI note (C4 = 60)
        if(controller_ != nullptr)
            controller_->touchkeyDeviceSetLowestMidiNote((octave + 1)*12);
    }
    else if (comboBoxThatHasChanged == &midiInputAuxDeviceComboBox)
    {
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
    }
}

void ControlWindowMainComponent::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if(controller_ == nullptr)
        return;

    if (buttonThatWasClicked == &touchkeyStartButton)
    {
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
    }
    else if (buttonThatWasClicked == &oscEnableButton)
    {
        controller_->oscTransmitSetEnabled(oscEnableButton.getToggleState());
    }
    else if (buttonThatWasClicked == &oscEnableRawButton)
    {
        controller_->oscTransmitSetRawDataEnabled(oscEnableRawButton.getToggleState());
    }
    else if (buttonThatWasClicked == &oscInputEnableButton)
    {
        controller_->oscReceiveSetEnabled(oscInputEnableButton.getToggleState());
    }
    else if (buttonThatWasClicked == &addZoneButton)
    {
        controller_->midiSegmentAdd();
    }
    else if (buttonThatWasClicked == &removeZoneButton)
    {
        int tabIndex = keyboardZoneTabbedComponent.getCurrentTabIndex();
        if(tabIndex != 0) {
            KeyboardZoneComponent* selectedComponent = static_cast<KeyboardZoneComponent*> (keyboardZoneTabbedComponent.getTabContentComponent(tabIndex));
            controller_->midiSegmentRemove(selectedComponent->keyboardSegment());
        }
    }
    else if (buttonThatWasClicked == &touchkeyAutodetectButton)
    {
        if(controller_->touchkeyDeviceIsAutodetecting())
            controller_->touchkeyDeviceStopAutodetecting();
        else
            controller_->touchkeyDeviceAutodetectLowestMidiNote();
    }
}


void ControlWindowMainComponent::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(controller_ == nullptr)
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
    if(controller_ == nullptr)
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
    if(controller_ == nullptr)
        return;

    juce::String oscHost = oscHostTextEditor.getText();
    juce::String oscPort = oscPortTextEditor.getText();
    controller_->oscTransmitClearAddresses();
    controller_->oscTransmitAddAddress(oscHost.toUTF8(), oscPort.toUTF8());
}

// Synchronize the UI state with the underlying state of the controller
void ControlWindowMainComponent::synchronize() {
    if(controller_ == nullptr)
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
    if(controller_ == nullptr)
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



#endif  // TOUCHKEYS_NO_GUI
