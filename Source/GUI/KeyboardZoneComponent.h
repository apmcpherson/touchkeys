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

#include "MappingListComponent.h"
#include "../MainApplicationController.h"
#include "../TouchKeys/MidiKeyboardSegment.h"
#include <JuceHeader.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class KeyboardZoneComponent  : public juce::Component,
                               public juce::TextEditor::Listener,
                               public juce::ComboBox::Listener,
                               public juce::Button::Listener
{
public:
    //==============================================================================
    KeyboardZoneComponent ();
    ~KeyboardZoneComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setMainApplicationController(MainApplicationController *controller) {
        // Attach the user interface to the controller and vice-versa
        controller_ = controller;
        mappingListComponent.setMainApplicationController(controller_);
        if(controller_ != 0) {
            synchronize(true);
        }
    }

    void setKeyboardSegment(MidiKeyboardSegment *segment, int zone) {
        keyboardSegment_ = segment;
        keyboardZone_ = zone;
        mappingListComponent.setKeyboardSegment(keyboardSegment_);
        if(controller_ != 0) {
            synchronize(true);
        }
    }

    MidiKeyboardSegment* keyboardSegment() {
        return keyboardSegment_;
    }
    int keyboardZone() { return keyboardZone_; }

    // juce::TextEditor listener methods
    void textEditorTextChanged(juce::TextEditor& /*editor*/) {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor);
    void textEditorEscapeKeyPressed(juce::TextEditor &editor);
    void textEditorFocusLost(juce::TextEditor &editor);

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
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

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
    //[/UserVariables]

    //==============================================================================
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


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardZoneComponent)
};

//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
