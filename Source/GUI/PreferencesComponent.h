/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.0.8

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

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
                              public juce::Button::Listener,
                              public juce::Slider::Listener
{
public:
    //==============================================================================
    PreferencesComponent ();
    ~PreferencesComponent() override;

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
    void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;



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
    std::unique_ptr<juce::ComboBox> startupPresetComboBox;
    std::unique_ptr<juce::Label> label4;
    std::unique_ptr<juce::ToggleButton> startTouchKeysButton;
    std::unique_ptr<juce::ToggleButton> autodetectButton;
    std::unique_ptr<juce::TextButton> defaultsButton;
    std::unique_ptr<juce::Slider> suppressStrayTouchSlider;
    std::unique_ptr<juce::ToggleButton> suppressStrayTouchButton;
    std::unique_ptr<juce::Label> strayTouchThresholdLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};

//[EndFile] You can add extra defines here...
#endif          // TOUCHKEYS_NO_GUI
//[/EndFile]

