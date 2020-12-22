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
#include "TouchkeyKeyDivisionMappingFactory.h"

//==============================================================================
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
    void synchronize();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

private:
    TouchkeyKeyDivisionMappingFactory& factory_;

    juce::ComboBox tuningComboBox;
    juce::Label tuningLabel;
    juce::Label controlLabel;
    juce::ComboBox controlComboBox;
    juce::ToggleButton retriggerButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyKeyDivisionMappingShortEditor)
};

#endif  // TOUCHKEYS_NO_GUI
