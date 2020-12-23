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
#include "TouchkeyMultiFingerTriggerMappingFactory.h"


class TouchkeyMultiFingerTriggerMappingShortEditor  : public MappingEditorComponent,
                                                      public juce::TextEditor::Listener,
                                                      public juce::ComboBox::Listener,
                                                      public juce::Button::Listener
{
private:
    static constexpr int kNoteSame = 256;
    static constexpr int kNoteOffset = 1;
public:
    TouchkeyMultiFingerTriggerMappingShortEditor (TouchkeyMultiFingerTriggerMappingFactory& factory);
    ~TouchkeyMultiFingerTriggerMappingShortEditor();

    void synchronize();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:
    TouchkeyMultiFingerTriggerMappingFactory& factory_;

    juce::Label controlLabel;
    juce::ComboBox touchesComboBox;
    juce::Label controlLabel2;
    juce::ComboBox tapsComboBox;
    juce::Label controlLabel3;
    juce::ComboBox noteComboBox;
    juce::ToggleButton sendOnReleaseButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyMultiFingerTriggerMappingShortEditor)
};

#endif      // TOUCHKEYS_NO_GUI
