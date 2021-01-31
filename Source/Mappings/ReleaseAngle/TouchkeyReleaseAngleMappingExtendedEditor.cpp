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

#include "TouchkeyReleaseAngleMappingExtendedEditor.h"

//==============================================================================
TouchkeyReleaseAngleMappingExtendedEditor::TouchkeyReleaseAngleMappingExtendedEditor (TouchkeyReleaseAngleMappingFactory& factory)
    : factory_(factory),
    titleLabel{ "title label", TRANS( "Release Angle Mapping (Zone N, #M)" ) },
    presetLabel{ "preset label", TRANS( "Preset:" ) },
    presetComboBox{ "parameter combo box" },
    presetLabel2{ "preset label", TRANS( "Window Length:" ) },
    windowLengthEditor{"range low text editor" },
    presetLabel3{ "preset label", TRANS( "ms. before release" ) },
    presetLabel4{ "preset label", TRANS( "Release Moving Up" ) },
    upMinSpeedEditor{ "up release speed editor" },
    presetLabel5{ "preset label", TRANS( "Min. release speed:" ) },
    upNote1Editor{ "up note 1 editor" },
    presetLabel6{ "preset label", TRANS( "Send notes:" ) },
    upNote2Editor{ "up note 2 editor" },
    upNote3Editor{ "up note 3 editor" },
    presetLabel7{ "preset label", TRANS( "With velocities:" ) },
    upVelocity1Editor{ "up velocity 1 editor" },
    upVelocity2Editor{ "up velocity 2 editor" },
    upVelocity3Editor{ "up velocity 3 editor" },
    presetLabel8{ "preset label", TRANS( "Release Moving Down" ) },
    downMinSpeedEditor{ "down release speed editor" },
    presetLabel9{ "preset label", TRANS( "Min. release speed:" ) },
    downNote1Editor{ "down note 1 editor" },
    presetLabel10{ "preset label", TRANS( "Send notes:" ) },
    downNote2Editor{ "down note 2 editor" },
    downNote3Editor{ "down note 3 editor" },
    presetLabel11{ "preset label", TRANS( "With velocities:" ) },
    downVelocity1Editor{ "down velocity 1 editor" },
    downVelocity2Editor{ "down velocity 2 editor" },
    downVelocity3Editor{ "down velocity 3 editor" },
    upEnableButton{ "up enable button" },
    downEnableButton{ "down enable button" }

{
    setLookAndFeel( &lnf );

    addAndMakeVisible (titleLabel);
    titleLabel.setFont (juce::Font (15.00f, juce::Font::bold));

    addAndMakeVisible (presetLabel);

    addAndMakeVisible (presetComboBox);
    presetComboBox.setEditableText (false);
    presetComboBox.setJustificationType (juce::Justification::centredLeft);
    presetComboBox.setTextWhenNothingSelected ( juce::String{} );
    presetComboBox.setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    presetComboBox.addListener (this);

    addAndMakeVisible (presetLabel2);

    addAndMakeVisible( windowLengthEditor );

    addAndMakeVisible( presetLabel3 );

    addAndMakeVisible( presetLabel4 );
    presetLabel4.setFont (juce::Font (15.00f, juce::Font::bold));

    addAndMakeVisible( upMinSpeedEditor );

    addAndMakeVisible( presetLabel5 );
    presetLabel5.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible( upNote1Editor );

    addAndMakeVisible( presetLabel6 );
    presetLabel6.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible( upNote2Editor );
    addAndMakeVisible( upNote3Editor );

    addAndMakeVisible( presetLabel7 );
    presetLabel7.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible( upVelocity1Editor );
    addAndMakeVisible( upVelocity2Editor );
    addAndMakeVisible( upVelocity3Editor );

    addAndMakeVisible( presetLabel8 );
    presetLabel8.setFont (juce::Font (15.00f, juce::Font::bold));

    addAndMakeVisible( downMinSpeedEditor );

    addAndMakeVisible( presetLabel9 );
    presetLabel9.setFont (juce::Font (15.00f, juce::Font::plain));
    presetLabel9.setJustificationType (juce::Justification::centredRight);
    presetLabel9.setEditable (false, false, false);

    addAndMakeVisible( downNote1Editor );

    addAndMakeVisible( presetLabel10 );
    presetLabel10.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible( downNote2Editor );

    addAndMakeVisible( downNote3Editor );

    addAndMakeVisible( presetLabel11 );
    presetLabel11.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible( downVelocity1Editor );

    addAndMakeVisible( downVelocity2Editor );

    addAndMakeVisible( downVelocity3Editor );

    addAndMakeVisible( upEnableButton );
    upEnableButton.setButtonText (TRANS("Enable"));
    upEnableButton.addListener (this);

    addAndMakeVisible( downEnableButton );
    downEnableButton.setButtonText (TRANS("Enable"));
    downEnableButton.addListener (this);

    for(int i = 0; i < factory_.getNumConfigurations(); i++) {
        presetComboBox.addItem(factory_.getConfigurationName(i).c_str(), i+1);
    }
    
    windowLengthEditor.addListener(this);
    upMinSpeedEditor.addListener(this);
    downMinSpeedEditor.addListener(this);
    upNote1Editor.addListener(this);
    upNote2Editor.addListener(this);
    upNote3Editor.addListener(this);
    upVelocity1Editor.addListener(this);
    upVelocity2Editor.addListener(this);
    upVelocity3Editor.addListener(this);
    downNote1Editor.addListener(this);
    downNote2Editor.addListener(this);
    downNote3Editor.addListener(this);
    downVelocity1Editor.addListener(this);
    downVelocity2Editor.addListener(this);
    downVelocity3Editor.addListener(this);

    setSize (342, 328);
}

TouchkeyReleaseAngleMappingExtendedEditor::~TouchkeyReleaseAngleMappingExtendedEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void TouchkeyReleaseAngleMappingExtendedEditor::paint (juce::Graphics& g)
{
    g.fillAll ( juce::Colour (0xffd2d2d2));
}

void TouchkeyReleaseAngleMappingExtendedEditor::resized()
{
    titleLabel.setBounds (8, 8, 424, 24);
    presetLabel.setBounds (8, 40, 56, 24);
    presetComboBox.setBounds (64, 40, 264, 24);
    presetLabel2.setBounds (8, 80, 112, 24);
    windowLengthEditor.setBounds (120, 80, 56, 24);
    presetLabel3.setBounds (176, 80, 136, 24);
    presetLabel4.setBounds (8, 112, 136, 24);
    upMinSpeedEditor.setBounds (144, 136, 56, 24);
    presetLabel5.setBounds (8, 136, 136, 24);
    upNote1Editor.setBounds (144, 160, 56, 24);
    presetLabel6.setBounds (8, 160, 136, 24);
    upNote2Editor.setBounds (208, 160, 56, 24);
    upNote3Editor.setBounds (272, 160, 56, 24);
    presetLabel7.setBounds (8, 184, 136, 24);
    upVelocity1Editor.setBounds (144, 184, 56, 24);
    upVelocity2Editor.setBounds (208, 184, 56, 24);
    upVelocity3Editor.setBounds (272, 184, 56, 24);
    presetLabel8.setBounds (8, 216, 160, 24);
    downMinSpeedEditor.setBounds (144, 240, 56, 24);
    presetLabel9.setBounds (8, 240, 136, 24);
    downNote1Editor.setBounds (144, 264, 56, 24);
    presetLabel10.setBounds (8, 264, 136, 24);
    downNote2Editor.setBounds (208, 264, 56, 24);
    downNote3Editor.setBounds (272, 264, 56, 24);
    presetLabel11.setBounds (8, 288, 136, 24);
    downVelocity1Editor.setBounds (144, 288, 56, 24);
    downVelocity2Editor.setBounds (208, 288, 56, 24);
    downVelocity3Editor.setBounds (272, 288, 56, 24);
    upEnableButton.setBounds (208, 112, 72, 24);
    downEnableButton.setBounds (208, 216, 72, 24);
}

void TouchkeyReleaseAngleMappingExtendedEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &presetComboBox)
    {
        int index = presetComboBox.getSelectedItemIndex();
        factory_.setCurrentConfiguration(index);
    }
}

void TouchkeyReleaseAngleMappingExtendedEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == &upEnableButton)
    {
        factory_.setUpMessagesEnabled(upEnableButton.getToggleState());
    }
    else if (buttonThatWasClicked == &downEnableButton)
    {
        factory_.setDownMessagesEnabled(downEnableButton.getToggleState());
    }
}


void TouchkeyReleaseAngleMappingExtendedEditor::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(&editor == &windowLengthEditor) {
        float windowLength = atof(windowLengthEditor.getText().toUTF8());
        factory_.setWindowSize(windowLength);
    }
    else if(&editor == &upMinSpeedEditor) {
        float speed = atof(upMinSpeedEditor.getText().toUTF8());
        factory_.setUpMinimumAngle(speed);
    }
    else if(&editor == &downMinSpeedEditor) {
        float speed = atof(downMinSpeedEditor.getText().toUTF8());
        factory_.setDownMinimumAngle(speed);
    }
    else {
        // All the other editors are int values
        int value = atoi(editor.getText().toUTF8());

        if(&editor == &upNote1Editor)
            factory_.setUpNote(0, value);
        else if(&editor == &upNote2Editor)
            factory_.setUpNote(1, value);
        else if(&editor == &upNote3Editor)
            factory_.setUpNote(2, value);
        else if(&editor == &upVelocity1Editor)
            factory_.setUpVelocity(0, value);
        else if(&editor == &upVelocity2Editor)
            factory_.setUpVelocity(1, value);
        else if(&editor == &upVelocity3Editor)
            factory_.setUpVelocity(2, value);
        else if(&editor == &downNote1Editor)
            factory_.setDownNote(0, value);
        else if(&editor == &downNote2Editor)
            factory_.setDownNote(1, value);
        else if(&editor == &downNote3Editor)
            factory_.setDownNote(2, value);
        else if(&editor == &downVelocity1Editor)
            factory_.setDownVelocity(0, value);
        else if(&editor == &downVelocity2Editor)
            factory_.setDownVelocity(1, value);
        else if(&editor == &downVelocity3Editor)
            factory_.setDownVelocity(2, value);
    }
}

void TouchkeyReleaseAngleMappingExtendedEditor::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{

}

void TouchkeyReleaseAngleMappingExtendedEditor::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

void TouchkeyReleaseAngleMappingExtendedEditor::synchronize()
{
    // Set the title label
    titleLabel.setText(getDescriptionHelper("Release Angle Mapping"), juce::NotificationType::dontSendNotification);

    // Update the editors to reflect the current status
    if(!windowLengthEditor.hasKeyboardFocus(true)) {
        float value = factory_.getWindowSize();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.0f", value);
#else
        snprintf(st, 16, "%.0f", value);
#endif
        windowLengthEditor.setText(st);
    }

    if(!upMinSpeedEditor.hasKeyboardFocus(true)) {
        float value = factory_.getUpMinimumAngle();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        upMinSpeedEditor.setText(st);
    }

    if(!downMinSpeedEditor.hasKeyboardFocus(true)) {
        float value = factory_.getDownMinimumAngle();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        downMinSpeedEditor.setText(st);
    }
    
    char st[16];
    
    if(!upNote1Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getUpNote(0));
        upNote1Editor.setText(st);
    }
    if(!upNote2Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getUpNote(1));
        upNote2Editor.setText(st);
    }
    if(!upNote3Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getUpNote(2));
        upNote3Editor.setText(st);
    }
    if(!upVelocity1Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getUpVelocity(0));
        upVelocity1Editor.setText(st);
    }
    if(!upVelocity2Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getUpVelocity(1));
        upVelocity2Editor.setText(st);
    }
    if(!upVelocity3Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getUpVelocity(2));
        upVelocity3Editor.setText(st);
    }
    if(!downNote1Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getDownNote(0));
        downNote1Editor.setText(st);
    }
    if(!downNote2Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getDownNote(1));
        downNote2Editor.setText(st);
    }
    if(!downNote3Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getDownNote(2));
        downNote3Editor.setText(st);
    }
    if(!downVelocity1Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getDownVelocity(0));
        downVelocity1Editor.setText(st);
    }
    if(!downVelocity2Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getDownVelocity(1));
        downVelocity2Editor.setText(st);
    }
    if(!downVelocity3Editor.hasKeyboardFocus(true)) {
        intToString(st, factory_.getDownVelocity(2));
        downVelocity3Editor.setText(st);
    }

    upEnableButton.setToggleState(factory_.getUpMessagesEnabled(), juce::NotificationType::dontSendNotification);
    downEnableButton.setToggleState(factory_.getDownMessageEnabled(), juce::NotificationType::dontSendNotification);

    int configuration = factory_.getCurrentConfiguration();
    if(configuration >= 0)
        presetComboBox.setSelectedItemIndex(configuration, juce::NotificationType::dontSendNotification);
    else
        presetComboBox.setText( juce::String{} );
}

// Return a human-readable description of this mapping for the window
juce::String TouchkeyReleaseAngleMappingExtendedEditor::getDescription() {
    return getDescriptionHelper("Release Angle");
}

// Return a human-readable description of this mapping for the window
juce::String TouchkeyReleaseAngleMappingExtendedEditor::getDescriptionHelper(juce::String baseName) {
    juce::String desc = baseName;

    desc += " (Zone ";

    int zone = factory_.segment().outputPort();
    desc += zone;
    desc += ", #";

    int mappingNumber = factory_.segment().indexOfMappingFactory(&factory_);
    desc += mappingNumber;
    desc += ")";

    return desc;
}

// Cross-platform helper function to deal with weird Visual Studio definitions...
void TouchkeyReleaseAngleMappingExtendedEditor::intToString(char *st, int value) {
#ifdef _MSC_VER
    _snprintf_s(st, 16, _TRUNCATE, "%d", value);
#else
    snprintf(st, 16, "%d", value);
#endif
}


#endif      // TOUCHKEYS_NO_GUI
