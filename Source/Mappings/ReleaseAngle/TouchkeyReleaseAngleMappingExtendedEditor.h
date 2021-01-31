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

    TKLabel titleLabel;
    TKLabel presetLabel;
    juce::ComboBox presetComboBox;
    TKLabel presetLabel2;
    TKTextEditor windowLengthEditor;
    TKLabel presetLabel3;
    TKLabel presetLabel4;
    TKTextEditor upMinSpeedEditor;
    TKLabel presetLabel5;
    TKTextEditor upNote1Editor;
    TKLabel presetLabel6;
    TKTextEditor upNote2Editor;
    TKTextEditor upNote3Editor;
    TKLabel presetLabel7;
    TKTextEditor upVelocity1Editor;
    TKTextEditor upVelocity2Editor;
    TKTextEditor upVelocity3Editor;
    TKLabel presetLabel8;
    TKTextEditor downMinSpeedEditor;
    TKLabel presetLabel9;
    TKTextEditor downNote1Editor;
    TKLabel presetLabel10;
    TKTextEditor downNote2Editor;
    TKTextEditor downNote3Editor;
    TKLabel presetLabel11;
    TKTextEditor downVelocity1Editor;
    TKTextEditor downVelocity2Editor;
    TKTextEditor downVelocity3Editor;
    juce::ToggleButton upEnableButton;
    juce::ToggleButton downEnableButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyReleaseAngleMappingExtendedEditor)
};

#endif      // TOUCHKEYS_NO_GUI
