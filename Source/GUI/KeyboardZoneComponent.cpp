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


//==============================================================================
KeyboardZoneComponent::KeyboardZoneComponent ()
    : controller_(nullptr), 
    keyboardSegment_( nullptr ),
    mappingListComponent{},
    midiOutputGroupComponent{ "MIDI input group", "MIDI Output" },
    midiOutputDeviceComboBox{ "MIDI input combo box" },
    label4{ "Device:" },
    label5{ "Mode:" },
    midiOutputModeComboBox{ "MIDI input combo box" },
    midiOutputVoiceStealingButton{ "Voice stealing button" },
    label2{ "Channels:" },
    midiOutputChannelLowEditor{ "new text editor" },
    midiOutputChannelHighEditor{ "new text editor" },
    label3{ "to" },
    midiOutputGroupComponent2{ "MIDI input group", "Range" },
    label7{ "to" },
    rangeLowComboBox{ "range low combo box" },
    rangeHighComboBox{ "range high combo box" },
    label6{ "Transpose:" },
    midiOutputTransposeEditor{ "transposition text editor" },
    label8{ "Mappings:" },
    addMappingButton{ "add mapping button" },
    label9{ "Pitchwheel range:" },
    pitchWheelRangeEditor{ "pitch wheel range editor" },
    keyboardControllersButton{ "keyboard controllers button" }
{
    setLookAndFeel( &lnf );

    addAndMakeVisible (mappingListComponent);
    addAndMakeVisible (midiOutputGroupComponent);

    addAndMakeVisible (midiOutputDeviceComboBox);
    midiOutputDeviceComboBox.setEditableText (false);
    midiOutputDeviceComboBox.setJustificationType (juce::Justification::centredLeft);
    midiOutputDeviceComboBox.setTextWhenNothingSelected (juce::String{});
    midiOutputDeviceComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    midiOutputDeviceComboBox.addListener (this);

    addAndMakeVisible (label4);
    addAndMakeVisible (label5);

    addAndMakeVisible (midiOutputModeComboBox);
    midiOutputModeComboBox.setEditableText (false);
    midiOutputModeComboBox.setJustificationType (juce::Justification::centredLeft);
    midiOutputModeComboBox.setTextWhenNothingSelected (juce::String{});
    midiOutputModeComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    midiOutputModeComboBox.addListener (this);

    addAndMakeVisible (midiOutputVoiceStealingButton);
    midiOutputVoiceStealingButton.setButtonText ("Voice stealing");
    midiOutputVoiceStealingButton.addListener (this);

    addAndMakeVisible (label2);
    addAndMakeVisible (midiOutputChannelLowEditor);
    addAndMakeVisible (midiOutputChannelHighEditor);

    addAndMakeVisible (label3);
    addAndMakeVisible (midiOutputGroupComponent2);

    addAndMakeVisible (label7);
    addAndMakeVisible (rangeLowComboBox);
    rangeLowComboBox.setEditableText (true);
    rangeLowComboBox.setJustificationType (juce::Justification::centredLeft);
    rangeLowComboBox.setTextWhenNothingSelected (juce::String{});
    rangeLowComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    rangeLowComboBox.addListener (this);

    addAndMakeVisible (rangeHighComboBox);
    rangeHighComboBox.setEditableText (true);
    rangeHighComboBox.setJustificationType (juce::Justification::centredLeft);
    rangeHighComboBox.setTextWhenNothingSelected (juce::String{});
    rangeHighComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    rangeHighComboBox.addListener (this);

    addAndMakeVisible (label6);
    addAndMakeVisible (midiOutputTransposeEditor);

    addAndMakeVisible (label8);

    addAndMakeVisible (addMappingButton);
    addMappingButton.setButtonText ("Add Mapping...");
    addMappingButton.addListener (this);

    addAndMakeVisible (label9);
    addAndMakeVisible (pitchWheelRangeEditor);

    addAndMakeVisible (keyboardControllersButton);
    keyboardControllersButton.setButtonText (" Controllers...");
    keyboardControllersButton.addListener (this);

    // Add modes to MIDI mode toggle box
    for( const auto& mode : MidiKeyboardSegment::modeNames )
        midiOutputModeComboBox.addItem( mode.second, static_cast< int >( mode.first ) + kMidiOutputModeComboBoxOffset );


    // Populate the range combo boxes with notes of the 88-key keyboard
    for(int note = 12; note <= 127; note++) {
        rangeLowComboBox.addItem(MainApplicationController::midiNoteName(note).c_str(), note);
        rangeHighComboBox.addItem(MainApplicationController::midiNoteName(note).c_str(), note);
    }

    lastSelectedMidiOutputID_ = kInvalidMidiOutputId;

    setSize (552, 400);

    midiOutputChannelLowEditor.addListener(this);
    midiOutputChannelHighEditor.addListener(this);
    midiOutputTransposeEditor.addListener(this);
    pitchWheelRangeEditor.addListener(this);
    addMappingButton.setTriggeredOnMouseDown(true);
    keyboardControllersButton.setTriggeredOnMouseDown(true);
}

KeyboardZoneComponent::~KeyboardZoneComponent()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void KeyboardZoneComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffd2d2d2));
}

void KeyboardZoneComponent::resized()
{
    mappingListComponent.setBounds (0, 168, 552, 260);
    midiOutputGroupComponent.setBounds (200, 8, 344, 128);
    midiOutputDeviceComboBox.setBounds (264, 32, 264, 24);
    label4.setBounds (208, 32, 55, 24);
    label5.setBounds (208, 64, 55, 24);
    midiOutputModeComboBox.setBounds (264, 64, 152, 24);
    midiOutputVoiceStealingButton.setBounds (424, 64, 112, 24);
    label2.setBounds (208, 96, 56, 24);
    midiOutputChannelLowEditor.setBounds (264, 96, 32, 24);
    midiOutputChannelHighEditor.setBounds (320, 96, 32, 24);
    label3.setBounds (296, 96, 32, 24);
    midiOutputGroupComponent2.setBounds (8, 8, 184, 128);
    label7.setBounds (88, 32, 32, 24);
    rangeLowComboBox.setBounds (24, 32, 64, 24);
    rangeHighComboBox.setBounds (112, 32, 64, 24);
    label6.setBounds (392, 96, 80, 24);
    midiOutputTransposeEditor.setBounds (472, 96, 56, 24);
    label8.setBounds (8, 144, 88, 24);
    addMappingButton.setBounds (440, 144, 104, 20);
    label9.setBounds (24, 68, 104, 24);
    pitchWheelRangeEditor.setBounds (128, 68, 48, 24);
    keyboardControllersButton.setBounds (24, 100, 152, 20);
        
    // Resize the mapping list to fit the bottom of the window
    juce::Rectangle<int> const& ourBounds = getBounds();
    juce::Rectangle<int> mappingBounds = mappingListComponent.getBounds();
    mappingBounds.setHeight(ourBounds.getHeight() - mappingBounds.getY());
    mappingListComponent.setBounds(mappingBounds);
}

void KeyboardZoneComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if(keyboardSegment_ == nullptr || controller_ == nullptr )
        return;

    if (comboBoxThatHasChanged == &midiOutputDeviceComboBox)
    {
        // Look up the selected ID, remembering that Juce indices start at 1 and the first of
        // these is "Disabled" followed by "Virtual Output Port"
        int selection = midiOutputDeviceComboBox.getSelectedId() - kMidiOutputDeviceComboBoxOffset;
        if(selection == 1 - kMidiOutputDeviceComboBoxOffset) {   // Disabled
            controller_->disableMIDIOutputPort(keyboardSegment_->outputPort());
        }
        else if(selection == 2 - kMidiOutputDeviceComboBoxOffset) { // Virtual output
#ifndef JUCE_WINDOWS
            char st[20];
            snprintf(st, 20, "TouchKeys %d", keyboardSegment_.outputPort());
            controller_->enableMIDIOutputVirtualPort(keyboardSegment_.outputPort(), st);
#endif
        }
        else if(selection >= 0 && selection < midiOutputDeviceIDs_.size()) {
            int deviceId = midiOutputDeviceIDs_[selection];
            controller_->enableMIDIOutputPort(keyboardSegment_->outputPort(), deviceId);
        }
    }
    else if (comboBoxThatHasChanged == &midiOutputModeComboBox)
    {
        int mode = midiOutputModeComboBox.getSelectedId() - kMidiOutputModeComboBoxOffset;
        
        // Set the MIDI output mode (e.g., polyphonic, or MPE etc)
        keyboardSegment_->setMode(mode);

        // TODO Call textEditorReturnKeyPressed(), which will internally call setPolyphony()
        // NOTE In the case of MPE, setMode() above will send an MPE Configuration Message, which will create
        // an MPE zone with a default range of 15 member channels. Then, setPolyphony() below sends another MCM, 
        // constructing a new MPE zone, this time with the user-specified polyphonic range; it also updates the Channel
        // text editors.
        //textEditorReturnKeyPressed( midiOutputChannelHighEditor );
        //repaint();
    }
    else if (comboBoxThatHasChanged == &rangeLowComboBox)
    {
        updateSegmentRange();
    }
    else if (comboBoxThatHasChanged == &rangeHighComboBox)
    {
        updateSegmentRange();
    }
}

void KeyboardZoneComponent::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if(keyboardSegment_ == nullptr)
        return;

    if (buttonThatWasClicked == &midiOutputVoiceStealingButton)
    {
        bool stealing = midiOutputVoiceStealingButton.getToggleState();
        keyboardSegment_->setVoiceStealingEnabled(stealing);
    }
    else if (buttonThatWasClicked == &addMappingButton)
    {
        createMappingListPopup();
    }
    else if (buttonThatWasClicked == &keyboardControllersButton)
    {
        createKeyboardControllerPopup();
    }
}


void KeyboardZoneComponent::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(keyboardSegment_ == nullptr )
        return;

    if(&editor == &midiOutputChannelLowEditor || &editor == &midiOutputChannelHighEditor) {
        // Change range of MIDI output channels
        int rangeLow = atoi(midiOutputChannelLowEditor.getText().toUTF8());
        int rangeHigh = atoi(midiOutputChannelHighEditor.getText().toUTF8());
        if(rangeHigh > 16)
            rangeHigh = 16;
        if(rangeLow > 16)
            rangeLow = 16;
        if(rangeHigh < 1)
            rangeHigh = 1;
        if(rangeLow < 1)
            rangeLow = 1;
        keyboardSegment_->setOutputChannelLowest(rangeLow - 1); // 1-16 --> 0-15 indexing

        int polyphony = rangeHigh - rangeLow + 1;
        if(polyphony < 1)
            polyphony = 1;
        keyboardSegment_->setPolyphony(polyphony);
    }
    else if(&editor == &midiOutputTransposeEditor) {
        // Change output transposition (limiting possible range to +/- 4 octaves)
        int transpose = atoi(midiOutputTransposeEditor.getText().toUTF8());
        if(transpose < -48)
            transpose = -48;
        if(transpose > 48)
            transpose = 48;
        keyboardSegment_->setOutputTransposition(transpose);
    }
    else if(&editor == &pitchWheelRangeEditor) {
        float range = atof(pitchWheelRangeEditor.getText().toUTF8());
        keyboardSegment_->setMidiPitchWheelRange(range);
    }
}

void KeyboardZoneComponent::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{

}

void KeyboardZoneComponent::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

// Update state of GUI to reflect underlying controller
void KeyboardZoneComponent::synchronize(bool forceUpdates)
{
    if(keyboardSegment_ == nullptr || controller_ == nullptr )
        return;

    if(forceUpdates) {
        // Update the controls to reflect the current state
        updateOutputDeviceList();
    }

    // Update note ranges
    std::pair<int, int> range = keyboardSegment_->noteRange();
    if(!rangeLowComboBox.hasKeyboardFocus(true) || forceUpdates) {
        if(range.first < 12 || range.first > 127) {
            rangeLowComboBox.setText(juce::String(range.first));
        }
        else
            rangeLowComboBox.setSelectedId(range.first, juce::NotificationType::dontSendNotification);
    }
    if(!rangeHighComboBox.hasKeyboardFocus(true) || forceUpdates) {
        if(range.second < 12 || range.second > 127) {
            rangeHighComboBox.setText(juce::String(range.second));
        }
        else
            rangeHighComboBox.setSelectedId(range.second, juce::NotificationType::dontSendNotification);
    }

    // Update MIDI output status
    int selectedMidiOutputDevice = controller_->selectedMIDIOutputPort(keyboardSegment_->outputPort());
    if(selectedMidiOutputDevice != lastSelectedMidiOutputID_ || forceUpdates) {
        if(selectedMidiOutputDevice == MidiOutputController::kMidiOutputNotOpen)
            midiOutputDeviceComboBox.setSelectedId(1, juce::NotificationType::dontSendNotification);
#ifndef JUCE_WINDOWS
        else if(selectedMidiOutputDevice == MidiOutputController::kMidiVirtualOutputPortNumber)
            midiOutputDeviceComboBox.setSelectedId(2, dontSendNotification);
#endif
        else {
            // Find the output device in the vector
            for(int i = 0; i < midiOutputDeviceIDs_.size(); i++) {
                if(midiOutputDeviceIDs_[i] == selectedMidiOutputDevice) {
                    midiOutputDeviceComboBox.setSelectedId(i + kMidiOutputDeviceComboBoxOffset, juce::NotificationType::dontSendNotification);
                    break;
                }
            }
        }
        lastSelectedMidiOutputID_ = selectedMidiOutputDevice;
    }

    // Update the mode and the peripheral controls that go with it
    // NOTE selectedMidiOutputMode is an enum class
    const auto selectedMidiOutputMode = keyboardSegment_->mode();
    midiOutputModeComboBox.setSelectedId( static_cast< int >( selectedMidiOutputMode ) + kMidiOutputModeComboBoxOffset, juce::NotificationType::dontSendNotification);

    switch( selectedMidiOutputMode )
    {
        case MidiKeyboardSegment::Mode::Polyphonic :
            midiOutputChannelLowEditor.setEnabled(true);
            midiOutputChannelHighEditor.setEnabled(true);
            midiOutputVoiceStealingButton.setEnabled(true);
            midiOutputVoiceStealingButton.setToggleState(keyboardSegment_->voiceStealingEnabled(), juce::NotificationType::dontSendNotification);
            break;
        case MidiKeyboardSegment::Mode::MPE :
            midiOutputChannelLowEditor.setEnabled(false);
            midiOutputChannelHighEditor.setEnabled(true);
            break;
        default :
            midiOutputChannelLowEditor.setEnabled(true);
            midiOutputChannelHighEditor.setEnabled(true);
            midiOutputVoiceStealingButton.setEnabled(false);
            midiOutputVoiceStealingButton.setToggleState(false, juce::NotificationType::dontSendNotification);
            break;
    }

    // Update text editors
    if(!pitchWheelRangeEditor.hasKeyboardFocus(true) || forceUpdates) {
        float value = keyboardSegment_->midiPitchWheelRange();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.1f", value);
#else
        snprintf(st, 16, "%.1f", value);
#endif
        pitchWheelRangeEditor.setText(st);
    }

    if(!midiOutputChannelLowEditor.hasKeyboardFocus(true) || forceUpdates) {
        const int rangeLow = keyboardSegment_->outputChannelLowest() + 1; // 0-15 --> 1-16
        midiOutputChannelLowEditor.setText(juce::String(rangeLow));
    }

    if(!midiOutputTransposeEditor.hasKeyboardFocus(true) || forceUpdates) {
        const int transpose = keyboardSegment_->outputTransposition();
        midiOutputTransposeEditor.setText(juce::String(transpose));
    }

    if(selectedMidiOutputMode == MidiKeyboardSegment::Mode::Polyphonic || selectedMidiOutputMode == MidiKeyboardSegment::Mode::MPE ) {
        midiOutputChannelHighEditor.setEnabled(true);

        if(!midiOutputChannelHighEditor.hasKeyboardFocus(true) || forceUpdates) {
            const int rangeHigh = keyboardSegment_->polyphony() + keyboardSegment_->outputChannelLowest();
            midiOutputChannelHighEditor.setText(juce::String(rangeHigh));
        }
    }
    else {
        midiOutputChannelHighEditor.setEnabled(false);
        midiOutputChannelHighEditor.setText("", false);
    }

    // Update buttons
    //useAftertouchButton.setToggleState(keyboardSegment_->usesKeyboardChannnelPressure(), dontSendNotification);
    //usePitchWheelButton.setToggleState(keyboardSegment_->usesKeyboardPitchWheel(), dontSendNotification);
    //useControllersButton.setToggleState(keyboardSegment_->usesKeyboardMIDIControllers(), dontSendNotification);

    // Update the mapping list
    mappingListComponent.synchronize();
}

// Update the range of the keyboard segment
void KeyboardZoneComponent::updateSegmentRange()
{
    int selectionLow = rangeLowComboBox.getSelectedId();
    int noteLow = -1;
    if(selectionLow == 0) {
        // Not one of the predefined values that's selected. Parse the string.
        noteLow = MainApplicationController::midiNoteNumberForName((const char *)(rangeLowComboBox.getText().toUTF8()));
    }
    else {
        noteLow = selectionLow;
    }

    if(noteLow < 0 || noteLow > 127) {
        // Out of range: keep the old value
        noteLow = keyboardSegment_->noteRange().first;
    }

    int selectionHigh = rangeHighComboBox.getSelectedId();
    int noteHigh = -1;
    if(selectionHigh == 0) {
        // Not one of the predefined values that's selected. Parse the string.
        noteHigh = MainApplicationController::midiNoteNumberForName((const char *)(rangeHighComboBox.getText().toUTF8()));
    }
    else {
        noteHigh = selectionHigh;
    }

    if(noteHigh < 0 || noteHigh > 127) {
        // Out of range: keep the old value
        noteHigh = keyboardSegment_->noteRange().second;
    }

    if(noteHigh < noteLow)
        noteHigh = noteLow;
    keyboardSegment_->setNoteRange(noteLow, noteHigh);
}

// Update the combo box with the current output devices
void KeyboardZoneComponent::updateOutputDeviceList()
{
    if(controller_ == nullptr || keyboardSegment_ == nullptr )
        return;

    // *** MIDI output devices ***
    std::vector<std::pair<int, std::string> > devices = controller_->availableMIDIOutputDevices();

    midiOutputDeviceComboBox.clear();
    midiOutputDeviceIDs_.clear();
    midiOutputDeviceComboBox.addItem("Disabled", 1);
#ifndef JUCE_WINDOWS
    char virtualPortName[24];
    snprintf(virtualPortName, 24, "Virtual Port (%d)", keyboardSegment_->outputPort());
	midiOutputDeviceComboBox.addItem(virtualPortName, 2);
#endif

    // Check whether the currently selected ID still exists while
    // we build the list
    bool lastSelectedDeviceExists = false;
    int counter = kMidiOutputDeviceComboBoxOffset;
    for( auto it = devices.begin(); it != devices.end(); ++it) {
        if(it->first < 0)
            continue;
        midiOutputDeviceComboBox.addItem((*it).second.c_str(), counter);
        midiOutputDeviceIDs_.push_back(it->first);
        if(it->first == lastSelectedMidiOutputID_)
            lastSelectedDeviceExists = true;
        counter++;
    }

    if(!lastSelectedDeviceExists && lastSelectedMidiOutputID_ != kInvalidMidiOutputId) {
#ifndef JUCE_WINDOWS
        if(lastSelectedMidiOutputID_ != MidiOutputController::kMidiVirtualOutputPortNumber)
            controller_->disableMIDIOutputPort(keyboardSegment_->outputPort());
#else   // No virtual port on Windows
        controller_->disableMIDIOutputPort(keyboardSegment_->outputPort());
#endif
    }
}

// Create a popup menu containing a list of mapping factories
void KeyboardZoneComponent::createMappingListPopup()
{
    if(controller_ == nullptr)
        return;

    juce::PopupMenu menu;

    for(int i = 0; i < MidiKeyboardSegment::numberOfMappingFactories(); i++) {
        if(controller_->experimentalMappingsEnabled() || !MidiKeyboardSegment::mappingIsExperimental(i))
            menu.addItem(i + 1, MidiKeyboardSegment::mappingFactoryNameForIndex(i));
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(addMappingButton),
                       juce::ModalCallbackFunction::forComponent(staticMappingChosenCallback, this));
}

// Create a popup menu allowing selection of which controllers to retransmit
void KeyboardZoneComponent::createKeyboardControllerPopup()
{
    if(controller_ == nullptr || keyboardSegment_ == nullptr )
        return;

    juce::PopupMenu menu;

    menu.addItem(kKeyboardControllerSendPitchWheelRange, "Send Pitchwheel Range RPN", true, false);
    menu.addSeparator();
    menu.addItem(MidiKeyboardSegment::kControlPitchWheel, "Retransmit from Keyboard:", false);
    menu.addSeparator();
    menu.addItem(MidiKeyboardSegment::kControlPitchWheel, "Pitch Wheel", true, keyboardSegment_->usesKeyboardPitchWheel());
    menu.addItem(MidiKeyboardSegment::kControlChannelAftertouch, "Aftertouch", true, keyboardSegment_->usesKeyboardChannnelPressure());
    menu.addItem(1, "CC 1 (Mod Wheel)", true, keyboardSegment_->usesKeyboardModWheel());
    menu.addItem(kKeyboardControllerRetransmitPedals, "Pedals", true, keyboardSegment_->usesKeyboardPedals());
    menu.addItem(kKeyboardControllerRetransmitOthers, "Other Controllers", true, keyboardSegment_->usesKeyboardMIDIControllers());

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(keyboardControllersButton),
                       juce::ModalCallbackFunction::forComponent(staticKeyboardControllerChosenCallback, this));
}

// Called from the popup menu, indicating the selected item
void KeyboardZoneComponent::mappingChosenCallback(int result)
{
    if(controller_ == nullptr || keyboardSegment_ == nullptr)
        return;

    // Items are numbered from 1 in the menu but from 0 in the array in the controller
    if(result >= 1) {
        MappingFactory *newFactory = keyboardSegment_->createMappingFactoryForIndex(result - 1);

        if(newFactory != nullptr) {
            keyboardSegment_->addMappingFactory(newFactory, true);
        }
    }
}

// Called from the popup menu, indicated selected controller
void KeyboardZoneComponent::keyboardControllerChosenCallback(int result)
{
    if(controller_ == nullptr || keyboardSegment_ == nullptr )
        return;

    // Enable or disable retransmitting specific messages
    if(result == MidiKeyboardSegment::kControlPitchWheel) {
        keyboardSegment_->setUsesKeyboardPitchWheel(!keyboardSegment_->usesKeyboardPitchWheel());
    }
    else if(result == MidiKeyboardSegment::kControlChannelAftertouch) {
        keyboardSegment_->setUsesKeyboardChannelPressure(!keyboardSegment_->usesKeyboardChannnelPressure());
    }
    else if(result == 1) { // ModWheel == CC 1
        keyboardSegment_->setUsesKeyboardModWheel(!keyboardSegment_->usesKeyboardModWheel());
    }
    else if(result == kKeyboardControllerRetransmitPedals) {
        keyboardSegment_->setUsesKeyboardPedals(!keyboardSegment_->usesKeyboardPedals());
    }
    else if(result == kKeyboardControllerRetransmitOthers) {
        keyboardSegment_->setUsesKeyboardMIDIControllers(!keyboardSegment_->usesKeyboardMIDIControllers());
    }
    else if(result == kKeyboardControllerSendPitchWheelRange) {
        // Send a MIDI RPN message now to update the pitch wheel range
        keyboardSegment_->sendMidiPitchWheelRange();
    }
}

#endif      // TOUCHKEYS_NO_GUI
