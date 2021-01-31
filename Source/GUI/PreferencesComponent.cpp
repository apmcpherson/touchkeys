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

#ifndef TOUCHKEYS_NO_GUI
#include "../MainApplicationController.h"

#include "PreferencesComponent.h"

//==============================================================================
PreferencesComponent::PreferencesComponent ()
    : controller_(0),
    startupPresetComboBox{ "Startup preset combo box" },
    label4{ "new label", "Load preset on startup:" },
    startTouchKeysButton{ "auto start TouchKeys button" },
    autodetectButton{ "Autodetect button" },
    defaultsButton{ "new button" }
{
    setLookAndFeel( &lnf );

    addAndMakeVisible (startupPresetComboBox);
    startupPresetComboBox.setEditableText (false);
    startupPresetComboBox.setJustificationType (juce::Justification::centredLeft);
    startupPresetComboBox.setTextWhenNothingSelected (juce::String{});
    startupPresetComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    startupPresetComboBox.addListener (this);

    addAndMakeVisible (label4);
    label4.setFont (juce::Font (15.00f, juce::Font::plain));
    label4.setJustificationType (juce::Justification::centredLeft);
    label4.setEditable (false, false, false);

    addAndMakeVisible (startTouchKeysButton);
    startTouchKeysButton.setButtonText ("Start TouchKeys on startup");
    startTouchKeysButton.addListener (this);

    addAndMakeVisible (autodetectButton);
    autodetectButton.setButtonText ("Autodetect TouchKeys octave on each start");
    autodetectButton.addListener (this);

    addAndMakeVisible (defaultsButton);
    defaultsButton.setButtonText ("Reset to Defaults...");
    defaultsButton.addListener (this);

    setSize (296, 152);

    // Initialise the combo box
    startupPresetComboBox.addItem("None", kStartupPresetNone);
    startupPresetComboBox.addItem("Vibrato and Pitch Bend", kStartupPresetVibratoPitchBend);
    startupPresetComboBox.addItem("Last Saved", kStartupPresetLastSaved);
    startupPresetComboBox.addItem("Choose...", kStartupPresetChoose);
}

PreferencesComponent::~PreferencesComponent()
{
    setLookAndFeel( nullptr );
}

void PreferencesComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffd2d2d2));
}

void PreferencesComponent::resized()
{
    startupPresetComboBox.setBounds (16, 32, 264, 24);
    label4.setBounds (16, 8, 160, 24);
    startTouchKeysButton.setBounds (16, 64, 208, 24);
    autodetectButton.setBounds (16, 88, 272, 24);
    defaultsButton.setBounds (16, 120, 144, 24);
}

void PreferencesComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if(controller_ == nullptr)
        return;

    if (comboBoxThatHasChanged == &startupPresetComboBox)
    {
        int selection = startupPresetComboBox.getSelectedId();
        if(selection == kStartupPresetNone) {
            controller_->setPrefsStartupPresetNone();
        }
        else if(selection == kStartupPresetVibratoPitchBend) {
            controller_->setPrefsStartupPresetVibratoPitchBend();
        }
        else if(selection == kStartupPresetLastSaved) {
            controller_->setPrefsStartupPresetLastSaved();
        }
        else if(selection == kStartupPresetChoose) {
            // Bring up window to choose a preset
            juce::FileChooser myChooser ("Select a preset...",
                                   juce::File{}, // File::getSpecialLocation (File::userHomeDirectory),
                                   "*.tkpreset");
            if(myChooser.browseForFileToOpen()) {
                controller_->setPrefsStartupPreset(myChooser.getResult().getFullPathName());
            }
            // Otherwise user clicked cancel and we go back to whatever was there before
        }
    }
}

void PreferencesComponent::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if(controller_ == nullptr)
        return;

    if (buttonThatWasClicked == &startTouchKeysButton)
    {
        controller_->setPrefsAutoStartTouchKeys(startTouchKeysButton.getToggleState());
    }
    else if (buttonThatWasClicked == &autodetectButton)
    {
        controller_->setPrefsAutodetectOctave(autodetectButton.getToggleState());
    }
    else if (buttonThatWasClicked == &defaultsButton)
    {
        controller_->resetPreferences();
    }
}


// Synchronize the UI state with the underlying state of the controller
void PreferencesComponent::synchronize(bool forceUpdates) {
    if(controller_ == 0)
        return;

    startTouchKeysButton.setToggleState(controller_->getPrefsAutoStartTouchKeys(), juce::NotificationType::dontSendNotification);
    autodetectButton.setToggleState(controller_->getPrefsAutodetectOctave(), juce::NotificationType::dontSendNotification);

    if(controller_->getPrefsStartupPresetNone())
        startupPresetComboBox.setSelectedId(kStartupPresetNone);
    else if(controller_->getPrefsStartupPresetVibratoPitchBend())
        startupPresetComboBox.setSelectedId(kStartupPresetVibratoPitchBend);
    else if(controller_->getPrefsStartupPresetLastSaved())
        startupPresetComboBox.setSelectedId(kStartupPresetLastSaved);
    else {
        juce::String path = controller_->getPrefsStartupPreset();
        startupPresetComboBox.setText(path);
    }
}

#endif      // TOUCHKEYS_NO_GUI
