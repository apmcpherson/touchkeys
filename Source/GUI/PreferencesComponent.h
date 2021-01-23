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
#include <JuceHeader.h>

class MainApplicationController;

class PreferencesComponent  : public juce::Component,
                              public juce::ComboBox::Listener,
                              public juce::Button::Listener
{
public:
    PreferencesComponent ();
    ~PreferencesComponent();

    void setMainApplicationController(MainApplicationController *controller) {
        // Attach the user interface to the controller and vice-versa
        controller_ = controller;
    }

    // Synchronize UI state to match underlying state of the back end
    void synchronize(bool forceUpdates = false);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

private:
    enum {
        kStartupPresetNone = 1,
        kStartupPresetVibratoPitchBend,
        kStartupPresetLastSaved,
        kStartupPresetChoose
    };

    MainApplicationController *controller_; // Pointer to the main application controller

    TouchKeysLookAndFeel lnf;

    juce::ComboBox startupPresetComboBox;
    juce::Label label4;
    juce::ToggleButton startTouchKeysButton;
    juce::ToggleButton autodetectButton;
    juce::TextButton defaultsButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};


#endif          // TOUCHKEYS_NO_GUI
