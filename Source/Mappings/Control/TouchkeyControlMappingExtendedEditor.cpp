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

#include "TouchkeyControlMappingExtendedEditor.h"


//==============================================================================
TouchkeyControlMappingExtendedEditor::TouchkeyControlMappingExtendedEditor (TouchkeyControlMappingFactory& factory)
    : factory_(factory), 
    typeWasAbsolute_(false),
    inputRangeLowEditor{ "range low text editor" },
    rangeLabel{ "range label", "Input Range:" },
    controlLabel{ "control label", "To Control:" },
    controlComboBox{ "control combo box" },
    controlLabel2{ "control label", "Parameter:" },
    parameterComboBox{ "parameter combo box" },
    controlLabel3{ "control label", "Type:" },
    typeComboBox{ "type combo box" },
    inputRangeHighEditor{ "range hi text editor" },
    rangeLabel2{ "range label", "-" },
    rangeLabel3{ "range label", "Output Range:" },
    outputRangeLowEditor{ "output range low text editor" },
    outputRangeHighEditor{ "output range hi text editor" },
    rangeLabel4{ "range label", "-" },
    controlLabel4{ "control label", "Direction:" },
    directionComboBox{ "direction combo box" },
    titleLabel{ "title label", "Control Mapping (Zone N, #M)" },
    rangeLabel5{ "range label", "Threshold:" },
    thresholdEditor{ "threshold text editor" },
    cc14BitButton{ "new toggle button" },
    ignore2FingersButton{ "ignore 2 fingers toggle button" },
    ignore3FingersButton{ "ignore 3 fingers toggle button" },
    controlLabel6{ "control label", "Out of Range:" },
    outOfRangeComboBox{ "out of range combo box" },
    rangeLabel6{ "range label", "Default Output:" },
    outputDefaultEditor{ "output default text editor" }

{
    setLookAndFeel( &lnf );

    addAndMakeVisible (inputRangeLowEditor);

    addAndMakeVisible (rangeLabel);
    addAndMakeVisible (controlLabel);
    controlLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (controlComboBox);
    controlComboBox.setEditableText (false);
    controlComboBox.setJustificationType (juce::Justification::centredLeft);
    controlComboBox.setTextWhenNothingSelected (juce::String{});
    controlComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    controlComboBox.addListener (this);

    addAndMakeVisible (controlLabel2);
    addAndMakeVisible (parameterComboBox);
    parameterComboBox.setEditableText (false);
    parameterComboBox.setJustificationType (juce::Justification::centredLeft);
    parameterComboBox.setTextWhenNothingSelected (juce::String{});
    parameterComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    parameterComboBox.addListener (this);

    addAndMakeVisible (controlLabel3);
    controlLabel3.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (typeComboBox);
    typeComboBox.setEditableText (false);
    typeComboBox.setJustificationType (juce::Justification::centredLeft);
    typeComboBox.setTextWhenNothingSelected (juce::String{});
    typeComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    typeComboBox.addListener (this);

    addAndMakeVisible (inputRangeHighEditor);

    addAndMakeVisible (rangeLabel2);
    addAndMakeVisible (rangeLabel3);

    addAndMakeVisible (outputRangeLowEditor);
    addAndMakeVisible (outputRangeHighEditor);

    addAndMakeVisible (rangeLabel4);
    addAndMakeVisible (controlLabel4);
    controlLabel4.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (directionComboBox);
    directionComboBox.setEditableText (false);
    directionComboBox.setJustificationType (juce::Justification::centredLeft);
    directionComboBox.setTextWhenNothingSelected (juce::String{});
    directionComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    directionComboBox.addListener (this);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (rangeLabel5);

    addAndMakeVisible (thresholdEditor);

    addAndMakeVisible (cc14BitButton);
    cc14BitButton.setButtonText ("Use 14-bit CC");
    cc14BitButton.addListener (this);

    addAndMakeVisible (ignore2FingersButton);
    ignore2FingersButton.setButtonText ("Ignore 2 Fingers");
    ignore2FingersButton.addListener (this);

    addAndMakeVisible (ignore3FingersButton);
    ignore3FingersButton.setButtonText ("Ignore 3 Fingers");
    ignore3FingersButton.addListener (this);

    addAndMakeVisible (controlLabel6);
    controlLabel6.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (outOfRangeComboBox);
    outOfRangeComboBox.setEditableText (false);
    outOfRangeComboBox.setJustificationType (juce::Justification::centredLeft);
    outOfRangeComboBox.setTextWhenNothingSelected (juce::String{});
    outOfRangeComboBox.setTextWhenNoChoicesAvailable ("(no choices)");
    outOfRangeComboBox.addListener (this);

    addAndMakeVisible (rangeLabel6);
    addAndMakeVisible (outputDefaultEditor);

    parameterComboBox.addItem("X Position", TouchkeyControlMapping::kInputParameterXPosition);
    parameterComboBox.addItem("Y Position", TouchkeyControlMapping::kInputParameterYPosition);
    parameterComboBox.addItem("Contact Area", TouchkeyControlMapping::kInputParameterTouchSize);
    parameterComboBox.addItem("2-Finger Mean", TouchkeyControlMapping::kInputParameter2FingerMean);
    parameterComboBox.addItem("2-Finger Distance", TouchkeyControlMapping::kInputParameter2FingerDistance);

    typeComboBox.addItem("Absolute", TouchkeyControlMapping::kTypeAbsolute);
    typeComboBox.addItem("1st Touch Relative", TouchkeyControlMapping::kTypeFirstTouchRelative);
    typeComboBox.addItem("Note Onset Relative", TouchkeyControlMapping::kTypeNoteOnsetRelative);

    controlComboBox.addItem("Pitch Wheel", MidiKeyboardSegment::kControlPitchWheel);
    controlComboBox.addItem("Channel Pressure", MidiKeyboardSegment::kControlChannelAftertouch);
    controlComboBox.addItem("Poly Aftertouch", MidiKeyboardSegment::kControlPolyphonicAftertouch);
    for(int i = 1; i <= 119; i++) {
        controlComboBox.addItem(juce::String(i), i);
    }

    directionComboBox.addItem("Normal", TouchkeyControlMapping::kDirectionPositive);
    directionComboBox.addItem("Reverse", TouchkeyControlMapping::kDirectionNegative);
    directionComboBox.addItem("Always Positive", TouchkeyControlMapping::kDirectionBoth);

    outOfRangeComboBox.addItem("Ignore", OscMidiConverter::kOutOfRangeIgnore);
    outOfRangeComboBox.addItem("Clip", OscMidiConverter::kOutOfRangeClip);
    outOfRangeComboBox.addItem("Extrapolate", OscMidiConverter::kOutOfRangeExtrapolate);

    setSize (448, 248);

    inputRangeLowEditor.addListener(this);
    inputRangeHighEditor.addListener(this);
    outputRangeLowEditor.addListener(this);
    outputRangeHighEditor.addListener(this);
    outputDefaultEditor.addListener(this);
    thresholdEditor.addListener(this);
}

TouchkeyControlMappingExtendedEditor::~TouchkeyControlMappingExtendedEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void TouchkeyControlMappingExtendedEditor::paint (juce::Graphics& g)
{
    g.fillAll ( juce::Colour (0xffd2d2d2));
}

void TouchkeyControlMappingExtendedEditor::resized()
{
    inputRangeLowEditor.setBounds (112, 72, 56, 24);
    rangeLabel.setBounds (8, 72, 104, 24);
    controlLabel.setBounds (256, 40, 64, 24);
    controlComboBox.setBounds (320, 40, 112, 24);
    controlLabel2.setBounds (8, 40, 72, 24);
    parameterComboBox.setBounds (80, 40, 160, 24);
    controlLabel3.setBounds (264, 104, 56, 24);
    typeComboBox.setBounds (320, 104, 112, 24);
    inputRangeHighEditor.setBounds (184, 72, 56, 24);
    rangeLabel2.setBounds (168, 72, 16, 24);
    rangeLabel3.setBounds (8, 104, 96, 24);
    outputRangeLowEditor.setBounds (112, 104, 56, 24);
    outputRangeHighEditor.setBounds (184, 104, 56, 24);
    rangeLabel4.setBounds (168, 104, 16, 24);
    controlLabel4.setBounds (248, 136, 72, 24);
    directionComboBox.setBounds (320, 136, 112, 24);
    titleLabel.setBounds (8, 8, 424, 24);
    rangeLabel5.setBounds (8, 168, 72, 24);
    thresholdEditor.setBounds (112, 168, 56, 24);
    cc14BitButton.setBounds (320, 72, 112, 24);
    ignore2FingersButton.setBounds (8, 192, 128, 24);
    ignore3FingersButton.setBounds (8, 216, 128, 24);
    controlLabel6.setBounds (216, 168, 104, 24);
    outOfRangeComboBox.setBounds (320, 168, 112, 24);
    rangeLabel6.setBounds (8, 136, 96, 24);
    outputDefaultEditor.setBounds (112, 136, 56, 24);

}

void TouchkeyControlMappingExtendedEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{

    if (comboBoxThatHasChanged == &controlComboBox)
    {
        int controller = controlComboBox.getSelectedId();
        factory_.setController(controller);

    }
    else if (comboBoxThatHasChanged == &parameterComboBox)
    {
        int param = parameterComboBox.getSelectedId();
        factory_.setInputParameter(param);

    }
    else if (comboBoxThatHasChanged == &typeComboBox)
    {
        int type = typeComboBox.getSelectedId();
        factory_.setInputType(type);
    }
    else if (comboBoxThatHasChanged == &directionComboBox)
    {
        int direction = directionComboBox.getSelectedId();
        factory_.setDirection(direction);
    }
    else if (comboBoxThatHasChanged == &outOfRangeComboBox)
    {
        int behavior = outOfRangeComboBox.getSelectedId();
        factory_.setOutOfRangeBehavior(behavior);
    }
}

void TouchkeyControlMappingExtendedEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == &cc14BitButton)
    {
        factory_.setUses14BitControl(cc14BitButton.getToggleState());
    }
    else if (buttonThatWasClicked == &ignore2FingersButton)
    {
        factory_.setIgnoresTwoFingers(ignore2FingersButton.getToggleState());
    }
    else if (buttonThatWasClicked == &ignore3FingersButton)
    {
        factory_.setIgnoresThreeFingers(ignore3FingersButton.getToggleState());
    }
}

void TouchkeyControlMappingExtendedEditor::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if(&editor == &inputRangeLowEditor) {
        float range = atof(inputRangeLowEditor.getText().toUTF8());
        factory_.setRangeInputMin(range);
        factory_.setRangeInputCenter(range);
    }
    else if(&editor == &inputRangeHighEditor) {
        float range = atof(inputRangeHighEditor.getText().toUTF8());
        factory_.setRangeInputMax(range);
    }
    else if(&editor == &outputRangeLowEditor) {
        float range = atof(outputRangeLowEditor.getText().toUTF8());
        factory_.setRangeOutputMin(range);
    }
    else if(&editor == &outputRangeHighEditor) {
        float range = atof(outputRangeHighEditor.getText().toUTF8());
        factory_.setRangeOutputMax(range);
    }
    else if(&editor == &outputDefaultEditor) {
        float range = atof(outputDefaultEditor.getText().toUTF8());
        factory_.setRangeOutputDefault(range);
    }
    else if(&editor == &thresholdEditor) {
        float thresh = atof(thresholdEditor.getText().toUTF8());
        factory_.setThreshold(thresh);
    }
}

void TouchkeyControlMappingExtendedEditor::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{

}

void TouchkeyControlMappingExtendedEditor::textEditorFocusLost(juce::TextEditor &editor)
{
    textEditorReturnKeyPressed(editor);
}

void TouchkeyControlMappingExtendedEditor::synchronize()
{
    // Set the title label
    titleLabel.setText(getDescriptionHelper("Control Mapping"), juce::NotificationType::dontSendNotification);

    // Update the editors to reflect the current status
    if(!inputRangeLowEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeInputMin();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif
        inputRangeLowEditor.setText(st);
    }

    if(!inputRangeHighEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeInputMax();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        inputRangeHighEditor.setText(st);
    }

    if(!outputRangeLowEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeOutputMin();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        outputRangeLowEditor.setText(st);
    }

    if(!outputRangeHighEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeOutputMax();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif

        outputRangeHighEditor.setText(st);
    }
    
    if(!outputDefaultEditor.hasKeyboardFocus(true)) {
        float value = factory_.getRangeOutputDefault();
        char st[16];
#ifdef _MSC_VER
		_snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
        snprintf(st, 16, "%.2f", value);
#endif
        
        outputDefaultEditor.setText(st);
    }

    if(factory_.getInputType() == TouchkeyControlMapping::kTypeFirstTouchRelative
       || factory_.getInputType() == TouchkeyControlMapping::kTypeNoteOnsetRelative) {
        thresholdEditor.setEnabled(true);
        if(!thresholdEditor.hasKeyboardFocus(true)) {
            float value = factory_.getThreshold();
            char st[16];
#ifdef _MSC_VER
            _snprintf_s(st, 16, _TRUNCATE, "%.2f", value);
#else
            snprintf(st, 16, "%.2f", value);
#endif

            thresholdEditor.setText(st);
        }

        if(typeWasAbsolute_) {
            // Add all three direction items
            directionComboBox.clear();
            directionComboBox.addItem("Normal", TouchkeyControlMapping::kDirectionPositive);
            directionComboBox.addItem("Reverse", TouchkeyControlMapping::kDirectionNegative);
            directionComboBox.addItem("Always Positive", TouchkeyControlMapping::kDirectionBoth);
        }

        typeWasAbsolute_ = false;
    }
    else {
        thresholdEditor.setEnabled(false);
        thresholdEditor.setText("", false);

        if(!typeWasAbsolute_) {
            // Add only one direction item
            directionComboBox.clear();
            directionComboBox.addItem("Normal", TouchkeyControlMapping::kDirectionPositive);
        }

        typeWasAbsolute_ = true;
    }

    if(factory_.getController() == MidiKeyboardSegment::kControlPitchWheel) {
        cc14BitButton.setEnabled(false);
        cc14BitButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    }
    else if(factory_.getController() == MidiKeyboardSegment::kControlPolyphonicAftertouch ||
            factory_.getController() == MidiKeyboardSegment::kControlChannelAftertouch) {
        cc14BitButton.setEnabled(false);
        cc14BitButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    }
    else {
        cc14BitButton.setEnabled(true);
        cc14BitButton.setToggleState(factory_.getUses14BitControl(), juce::NotificationType::dontSendNotification);
    }

    using jNT = juce::NotificationType;

    ignore2FingersButton.setToggleState(factory_.getIgnoresTwoFingers(), jNT::dontSendNotification);
    ignore3FingersButton.setToggleState(factory_.getIgnoresThreeFingers(), jNT::dontSendNotification);

    parameterComboBox.setSelectedId(factory_.getInputParameter(), jNT::dontSendNotification);
    typeComboBox.setSelectedId(factory_.getInputType(), jNT::dontSendNotification);
    controlComboBox.setSelectedId(factory_.getController(), jNT::dontSendNotification);
    directionComboBox.setSelectedId(factory_.getDirection(), jNT::dontSendNotification);
    outOfRangeComboBox.setSelectedId(factory_.getOutOfRangeBehavior(), jNT::dontSendNotification);
}

// Return a human-readable description of this mapping for the window
juce::String TouchkeyControlMappingExtendedEditor::getDescription() {
    return getDescriptionHelper("Control");
}

// Return a human-readable description of this mapping for the window
juce::String TouchkeyControlMappingExtendedEditor::getDescriptionHelper(juce::String baseName) {
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

#endif  // TOUCHKEYS_NO_GUI
