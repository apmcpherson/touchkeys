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

#pragma once

#ifndef TOUCHKEYS_NO_GUI

#include "MappingListComponent.h"
#include "../MainApplicationController.h"
#include "../TouchKeys/MidiKeyboardSegment.h"
#include <JuceHeader.h>

class KeyboardZoneComponent  : public juce::Component,
                               public juce::TextEditor::Listener,
                               public juce::ComboBox::Listener,
                               public juce::Button::Listener
{
public:
    KeyboardZoneComponent ();
    ~KeyboardZoneComponent();

    void setMainApplicationController(MainApplicationController *controller) {
        // Attach the user interface to the controller and vice-versa
        controller_ = controller;
        mappingListComponent.setMainApplicationController(controller_);
        if(controller_ != nullptr) {
            synchronize(true);
        }
    }

    void setKeyboardSegment(MidiKeyboardSegment *segment, int zone) {
        keyboardSegment_ = segment;
        keyboardZone_ = zone;
        mappingListComponent.setKeyboardSegment(keyboardSegment_);
        if(controller_ != nullptr) {
            synchronize(true);
        }
    }

    MidiKeyboardSegment* keyboardSegment() {
        return keyboardSegment_;
    }
    int keyboardZone() { return keyboardZone_; }

    // juce::TextEditor listener methods
    void textEditorTextChanged(juce::TextEditor& /*editor*/) override {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    void textEditorFocusLost(juce::TextEditor &editor) override;

    // Synchronize UI state to match underlying state of the back end
    void synchronize(bool forceUpdates = false);

    // Update the range of the keyboard segment
    void updateSegmentRange();

    static void staticMappingChosenCallback(int result, KeyboardZoneComponent* component) {
        if (result != 0 && component != nullptr)
            component->mappingChosenCallback(result);
    }
    void mappingChosenCallback(int result);

    static void staticKeyboardControllerChosenCallback(int result, KeyboardZoneComponent* component) {
        if (result != 0 && component != nullptr)
            component->keyboardControllerChosenCallback(result);
    }
    void keyboardControllerChosenCallback(int result);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:

    enum {
        kInvalidMidiOutputId = -100
    };
    
    enum {
        // Offsets between Juce UI IDs and positions in vector
        kMidiOutputDeviceComboBoxOffset = 3,
        kMidiOutputModeComboBoxOffset = 1
    };

    enum {
        // Special commands for keyboard controller popup button
        kKeyboardControllerRetransmitOthers = 2000,
        kKeyboardControllerSendPitchWheelRange,
        kKeyboardControllerRetransmitPedals
    };

    // Update list of MIDI output devices
    void updateOutputDeviceList();

    // Create popup menu for mapping list
    void createMappingListPopup();

    // Create popup menu for keyboard controller retransmission
    void createKeyboardControllerPopup();

    MainApplicationController *controller_; // Pointer to the main application controller
    MidiKeyboardSegment *keyboardSegment_;  // Pointer to the segment this component controls
    int keyboardZone_;                      // Which zone this corresponds to (for UI sync purposes)
    std::vector<int> midiOutputDeviceIDs_;
    int lastSelectedMidiOutputID_;

    TouchKeysLookAndFeel lnf;

    MappingListComponent mappingListComponent;
    juce::GroupComponent midiOutputGroupComponent;
    juce::ComboBox midiOutputDeviceComboBox;
    juce::Label label4;
    juce::Label label5;
    juce::ComboBox midiOutputModeComboBox;
    juce::ToggleButton midiOutputVoiceStealingButton;
    juce::Label label2;
    juce::TextEditor midiOutputChannelLowEditor;
    juce::TextEditor midiOutputChannelHighEditor;
    juce::Label label3;
    juce::GroupComponent midiOutputGroupComponent2;
    juce::Label label7;
    juce::ComboBox rangeLowComboBox;
    juce::ComboBox rangeHighComboBox;
    juce::Label label6;
    juce::TextEditor midiOutputTransposeEditor;
    juce::Label label8;
    juce::TextButton addMappingButton;
    juce::Label label9;
    juce::TextEditor pitchWheelRangeEditor;
    juce::TextButton keyboardControllersButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardZoneComponent)
};

#endif      // TOUCHKEYS_NO_GUI
