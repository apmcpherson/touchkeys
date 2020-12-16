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

#include <JuceHeader.h>
#include "TouchkeyControlMappingFactory.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TouchkeyControlMappingShortEditor  : public MappingEditorComponent,
                                           public juce::TextEditor::Listener,
                                           public juce::ComboBox::Listener
{
public:
    //==============================================================================
    TouchkeyControlMappingShortEditor (TouchkeyControlMappingFactory& factory);
    ~TouchkeyControlMappingShortEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    // juce::TextEditor listener methods
    void textEditorTextChanged(juce::TextEditor &editor) {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor);
    void textEditorEscapeKeyPressed(juce::TextEditor &editor);
    void textEditorFocusLost(juce::TextEditor &editor);

    void synchronize();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    TouchkeyControlMappingFactory& factory_;
    //[/UserVariables]

    //==============================================================================
    juce::TextEditor inputRangeLowEditor;
    juce::Label rangeLabel;
    juce::Label controlLabel;
    juce::ComboBox controlComboBox;
    juce::Label controlLabel2;
    juce::ComboBox parameterComboBox;
    juce::Label controlLabel3;
    juce::ComboBox typeComboBox;
    juce::TextEditor inputRangeHighEditor;
    juce::Label rangeLabel2;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyControlMappingShortEditor)
};

//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]
