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

#include "TouchkeyReleaseAngleMappingFactory.h"
#include <JuceHeader.h>

class TouchkeyReleaseAngleMappingExtendedEditor  : public MappingEditorComponent,
                                                   public juce::TextEditor::Listener,
                                                   public juce::ComboBox::Listener,
                                                   public juce::Button::Listener
{
public:
    TouchkeyReleaseAngleMappingExtendedEditor (TouchkeyReleaseAngleMappingFactory& factory);
    ~TouchkeyReleaseAngleMappingExtendedEditor();

    void textEditorTextChanged(juce::TextEditor &editor) override {}
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    void textEditorFocusLost(juce::TextEditor &editor) override;

    void synchronize();
    juce::String getDescription();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

private:
    void intToString(char *st, int value);
    
    TouchKeysLookAndFeel lnf;

    juce::String getDescriptionHelper(juce::String baseName);

    TouchkeyReleaseAngleMappingFactory& factory_;

    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::ComboBox presetComboBox;
    juce::Label presetLabel2;
    juce::TextEditor windowLengthEditor;
    juce::Label presetLabel3;
    juce::Label presetLabel4;
    juce::TextEditor upMinSpeedEditor;
    juce::Label presetLabel5;
    juce::TextEditor upNote1Editor;
    juce::Label presetLabel6;
    juce::TextEditor upNote2Editor;
    juce::TextEditor upNote3Editor;
    juce::Label presetLabel7;
    juce::TextEditor upVelocity1Editor;
    juce::TextEditor upVelocity2Editor;
    juce::TextEditor upVelocity3Editor;
    juce::Label presetLabel8;
    juce::TextEditor downMinSpeedEditor;
    juce::Label presetLabel9;
    juce::TextEditor downNote1Editor;
    juce::Label presetLabel10;
    juce::TextEditor downNote2Editor;
    juce::TextEditor downNote3Editor;
    juce::Label presetLabel11;
    juce::TextEditor downVelocity1Editor;
    juce::TextEditor downVelocity2Editor;
    juce::TextEditor downVelocity3Editor;
    juce::ToggleButton upEnableButton;
    juce::ToggleButton downEnableButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyReleaseAngleMappingExtendedEditor)
};

#endif      // TOUCHKEYS_NO_GUI
