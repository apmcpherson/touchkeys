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

#include "TouchkeyReleaseAngleMappingFactory.h"
#include <JuceHeader.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TouchkeyReleaseAngleMappingExtendedEditor  : public MappingEditorComponent,
                                                   public juce::TextEditor::Listener,
                                                   public juce::ComboBox::Listener,
                                                   public juce::Button::Listener
{
public:
    //==============================================================================
    TouchkeyReleaseAngleMappingExtendedEditor (TouchkeyReleaseAngleMappingFactory& factory);
    ~TouchkeyReleaseAngleMappingExtendedEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void textEditorTextChanged(juce::TextEditor &editor) {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor);
    void textEditorEscapeKeyPressed(juce::TextEditor &editor);
    void textEditorFocusLost(juce::TextEditor &editor);

    void synchronize();
    juce::String getDescription();
    //[/UserMethods]

    void paint (juce::Graphics& g);
    void resized();
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged);
    void buttonClicked (juce::Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    void intToString(char *st, int value);
    
    juce::String getDescriptionHelper(juce::String baseName);

    TouchkeyReleaseAngleMappingFactory& factory_;
    //[/UserVariables]

    //==============================================================================
    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::ComboBox presetComboBox;
    juce::Label presetLabel2;
    juce::TextEditor windowLengthEditor;
    juce::Label presetLabel3;
    juce::Label presetLabel4;
    juce::TextEditor upMinSpeedEditor;
    juce::Label presetLabel5;
    juce::TextEditor upNote1Editor;
    juce::Label presetLabel6;
    juce::TextEditor upNote2Editor;
    juce::TextEditor upNote3Editor;
    juce::Label presetLabel7;
    juce::TextEditor upVelocity1Editor;
    juce::TextEditor upVelocity2Editor;
    juce::TextEditor upVelocity3Editor;
    juce::Label presetLabel8;
    juce::TextEditor downMinSpeedEditor;
    juce::Label presetLabel9;
    juce::TextEditor downNote1Editor;
    juce::Label presetLabel10;
    juce::TextEditor downNote2Editor;
    juce::TextEditor downNote3Editor;
    juce::Label presetLabel11;
    juce::TextEditor downVelocity1Editor;
    juce::TextEditor downVelocity2Editor;
    juce::TextEditor downVelocity3Editor;
    juce::ToggleButton upEnableButton;
    juce::ToggleButton downEnableButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyReleaseAngleMappingExtendedEditor)
};

//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
