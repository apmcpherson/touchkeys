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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#ifndef TOUCHKEYS_NO_GUI

#include "../MainApplicationController.h"
#include <JuceHeader.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ControlWindowMainComponent  : public juce::Component,
                                    public juce::TextEditor::Listener,
                                    public juce::ComboBox::Listener,
                                    public juce::Button::Listener
{
public:
    //==============================================================================
    ControlWindowMainComponent ();
    ~ControlWindowMainComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setMainApplicationController(MainApplicationController *controller) {
        // Attach the user interface to the controller and vice-versa
        controller_ = controller;
        lastControllerUpdateDeviceCount_ = controller_->devicesShouldUpdate();
        updateInputDeviceList();
    }

    // juce::TextEditor listener methods
    void textEditorTextChanged(juce::TextEditor &editor) {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor);
    void textEditorEscapeKeyPressed(juce::TextEditor &editor);
    void textEditorFocusLost(juce::TextEditor &editor);

    void synchronize();

    // Return the currently selected TouchKeys string
    juce::String currentTouchkeysSelectedPath();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
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
    //[/UserVariables]

    //==============================================================================
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


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlWindowMainComponent)
};

//[EndFile] You can add extra defines here...
#endif  // TOUCHKEYS_NO_GUI
//[/EndFile]
