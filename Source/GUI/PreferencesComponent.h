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

class MainApplicationController;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PreferencesComponent  : public juce::Component,
                              public juce::ComboBox::Listener,
                              public juce::Button::Listener
{
public:
    //==============================================================================
    PreferencesComponent ();
    ~PreferencesComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    void setMainApplicationController(MainApplicationController *controller) {
        // Attach the user interface to the controller and vice-versa
        controller_ = controller;
    }

    // Synchronize UI state to match underlying state of the back end
    void synchronize(bool forceUpdates = false);

    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    enum {
        kStartupPresetNone = 1,
        kStartupPresetVibratoPitchBend,
        kStartupPresetLastSaved,
        kStartupPresetChoose
    };

    MainApplicationController *controller_; // Pointer to the main application controller

    //[/UserVariables]

    //==============================================================================
    juce::ComboBox startupPresetComboBox;
    juce::Label label4;
    juce::ToggleButton startTouchKeysButton;
    juce::ToggleButton autodetectButton;
    juce::TextButton defaultsButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};

//[EndFile] You can add extra defines here...
#endif          // TOUCHKEYS_NO_GUI
//[/EndFile]
