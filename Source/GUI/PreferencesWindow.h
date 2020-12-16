/*
  ==============================================================================

    PreferencesWindow.h
    Created: 17 Jun 2014 11:22:44pm
    Author:  Andrew McPherson

  ==============================================================================
*/

#pragma once

#ifndef TOUCHKEYS_NO_GUI

#include "PreferencesComponent.h"
#include "../MainApplicationController.h"

//==============================================================================
/*
*/
class PreferencesWindow : public juce::DocumentWindow, public juce::Timer
{
public:
    PreferencesWindow(MainApplicationController& controller)
    : juce::DocumentWindow("Preferences", juce::Colours::lightgrey, juce::DocumentWindow::allButtons, false)
    {
        // Make a new preferences component
        preferencesComponent_ = new PreferencesComponent();
        preferencesComponent_->setMainApplicationController(&controller);
        
        // Set properties
        setContentOwned(preferencesComponent_, true);
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        
        // Don't show window yet
        setTopLeftPosition(60,60);
        setVisible(false);
        
        // Start a timer that will keep the interface in sync with the application
        startTimer(50);
    }

    ~PreferencesWindow()
    {
    }
    
    // Method used by Juce timer which we will use for periodic UI updates
    // from the underlying system state
    void timerCallback() override {
        preferencesComponent_->synchronize();
    }
    
    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    PreferencesComponent *preferencesComponent_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesWindow)
};

#endif      // TOUCHKEYS_NO_GUI
