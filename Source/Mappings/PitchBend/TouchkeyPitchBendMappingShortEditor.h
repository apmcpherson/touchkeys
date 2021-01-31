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
#include "TouchkeyPitchBendMappingFactory.h"

class TouchkeyPitchBendMappingShortEditor  : public MappingEditorComponent,
                                             public juce::TextEditor::Listener,
                                             public juce::ComboBox::Listener
{
public:
    TouchkeyPitchBendMappingShortEditor (TouchkeyPitchBendMappingFactory& factory);
    ~TouchkeyPitchBendMappingShortEditor();

    // juce::TextEditor listener methods
    void textEditorTextChanged(juce::TextEditor& ) override {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    void textEditorFocusLost(juce::TextEditor &editor) override;

    void synchronize();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

private:
    TouchkeyPitchBendMappingFactory& factory_;

    TouchKeysLookAndFeel lnf;

    TKTextEditor rangeEditor;
    TKLabel rangeLabel;
    TKTextEditor thresholdEditor;
    TKLabel thresholdLabel;
    TKLabel controlLabel;
    juce::ComboBox endpointsComboBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyPitchBendMappingShortEditor)
};

#endif      // TOUCHKEYS_NO_GUI
