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

#include "../MainApplicationController.h"
#include <JuceHeader.h>


class ControlWindowMainComponent  : public juce::Component,
                                    public juce::TextEditor::Listener,
                                    public juce::ComboBox::Listener,
                                    public juce::Button::Listener
{
public:
    ControlWindowMainComponent ();
    ~ControlWindowMainComponent();

    void setMainApplicationController(MainApplicationController *controller) {
        // Attach the user interface to the controller and vice-versa
        controller_ = controller;
        lastControllerUpdateDeviceCount_ = controller_->devicesShouldUpdate();
        updateInputDeviceList();
    }

    // juce::TextEditor listener methods
    void textEditorTextChanged(juce::TextEditor &editor) override {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    void textEditorFocusLost(juce::TextEditor &editor) override;

    void synchronize();

    // Return the currently selected TouchKeys string
    juce::String currentTouchkeysSelectedPath();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:
    enum {
        // Offsets between Juce UI IDs and positions in vector
        kMidiInputDeviceComboBoxOffset = 3,
        kTouchkeysComponentComboBoxOffset = 1,
        kTouchkeysMaxOctave = 6
    };

    void updateInputDeviceList();
    void updateOscHostPort();
    void updateKeyboardSegments();

    MainApplicationController *controller_; // Pointer to the main application controller
    std::vector<int> midiInputDeviceIDs_;
    int lastSelectedMidiInputID_;
    int lastSelectedMidiAuxInputID_;
    int lastSegmentUniqueIdentifier_;

    int lastControllerUpdateDeviceCount_;

    juce::GroupComponent midiInputGroupComponent;
    juce::ComboBox midiInputDeviceComboBox;
    juce::Label label;
    juce::GroupComponent groupComponent;
    juce::Label label2;
    juce::ComboBox touchkeyDeviceComboBox;
    juce::Label label3;
    juce::TextButton touchkeyStartButton;
    juce::Label touchkeyStatusLabel;
    juce::GroupComponent oscGroupComponent;
    juce::Label label7;
    juce::TextEditor oscHostTextEditor;
    juce::Label label8;
    juce::TextEditor oscPortTextEditor;
    juce::ToggleButton oscEnableButton;
    juce::ToggleButton oscEnableRawButton;
    juce::Label label4;
    juce::ComboBox touchkeyOctaveComboBox;
    juce::GroupComponent oscInputGroupComponent;
    juce::ToggleButton oscInputEnableButton;
    juce::Label label6;
    juce::TextEditor oscInputPortTextEditor;
    juce::TabbedComponent keyboardZoneTabbedComponent;
    juce::TextButton addZoneButton;
    juce::TextButton removeZoneButton;
    juce::TextButton touchkeyAutodetectButton;
    juce::ComboBox midiInputAuxDeviceComboBox;
    juce::Label label5;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlWindowMainComponent)
};

#endif  // TOUCHKEYS_NO_GUI

