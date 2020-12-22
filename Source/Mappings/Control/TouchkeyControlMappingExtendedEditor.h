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
#include "TouchkeyControlMappingFactory.h"


class TouchkeyControlMappingExtendedEditor  : public MappingEditorComponent,
                                              public juce::TextEditor::Listener,
                                              public juce::ComboBox::Listener,
                                              public juce::Button::Listener
{
public:
    TouchkeyControlMappingExtendedEditor (TouchkeyControlMappingFactory& factory);
    ~TouchkeyControlMappingExtendedEditor();

    void textEditorTextChanged(juce::TextEditor &editor) {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor);
    void textEditorEscapeKeyPressed(juce::TextEditor &editor);
    void textEditorFocusLost(juce::TextEditor &editor);

    void synchronize();
    juce::String getDescription();

    void paint (juce::Graphics& g);
    void resized();
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged);
    void buttonClicked (juce::Button* buttonThatWasClicked);

private:
    juce::String getDescriptionHelper(juce::String baseName);

    TouchkeyControlMappingFactory& factory_;
    bool typeWasAbsolute_;

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
    juce::Label rangeLabel3;
    juce::TextEditor outputRangeLowEditor;
    juce::TextEditor outputRangeHighEditor;
    juce::Label rangeLabel4;
    juce::Label controlLabel4;
    juce::ComboBox directionComboBox;
    juce::Label titleLabel;
    juce::Label rangeLabel5;
    juce::TextEditor thresholdEditor;
    juce::ToggleButton cc14BitButton;
    juce::ToggleButton ignore2FingersButton;
    juce::ToggleButton ignore3FingersButton;
    juce::Label controlLabel6;
    juce::ComboBox outOfRangeComboBox;
    juce::Label rangeLabel6;
    juce::TextEditor outputDefaultEditor;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyControlMappingExtendedEditor)
};

#endif      // TOUCHKEYS_NO_GUI
