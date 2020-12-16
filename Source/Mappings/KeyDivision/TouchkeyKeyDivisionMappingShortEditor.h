/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#ifndef TOUCHKEYS_NO_GUI

#include <JuceHeader.h>
#include "TouchkeyKeyDivisionMappingFactory.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TouchkeyKeyDivisionMappingShortEditor  : public MappingEditorComponent,
                                               public juce::TextEditor::Listener,
                                               public juce::ComboBox::Listener,
                                               public juce::Button::Listener
{
public:
    //==============================================================================
    TouchkeyKeyDivisionMappingShortEditor (TouchkeyKeyDivisionMappingFactory& factory);
    ~TouchkeyKeyDivisionMappingShortEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void synchronize();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    TouchkeyKeyDivisionMappingFactory& factory_;
    //[/UserVariables]

    //==============================================================================
    juce::ComboBox tuningComboBox;
    juce::Label tuningLabel;
    juce::Label controlLabel;
    juce::ComboBox controlComboBox;
    juce::ToggleButton retriggerButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyKeyDivisionMappingShortEditor)
};

//[EndFile] You can add extra defines here...
#endif  // TOUCHKEYS_NO_GUI
//[/EndFile]
