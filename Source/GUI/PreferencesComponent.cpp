/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.0.8

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#ifndef TOUCHKEYS_NO_GUI
#include "../MainApplicationController.h"
//[/Headers]

#include "PreferencesComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PreferencesComponent::PreferencesComponent ()
    : controller_(0)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    startupPresetComboBox.reset (new juce::ComboBox ("Startup preset combo box"));
    addAndMakeVisible (startupPresetComboBox.get());
    startupPresetComboBox->setEditableText (false);
    startupPresetComboBox->setJustificationType (juce::Justification::centredLeft);
    startupPresetComboBox->setTextWhenNothingSelected (juce::String());
    startupPresetComboBox->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    startupPresetComboBox->addListener (this);

    startupPresetComboBox->setBounds (16, 32, 264, 24);

    label4.reset (new juce::Label ("new label",
                                   TRANS("Load preset on startup:")));
    addAndMakeVisible (label4.get());
    label4->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    label4->setJustificationType (juce::Justification::centredLeft);
    label4->setEditable (false, false, false);
    label4->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    label4->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    label4->setBounds (16, 8, 160, 24);

    startTouchKeysButton.reset (new juce::ToggleButton ("auto start TouchKeys button"));
    addAndMakeVisible (startTouchKeysButton.get());
    startTouchKeysButton->setButtonText (TRANS("Start TouchKeys on startup"));
    startTouchKeysButton->addListener (this);

    startTouchKeysButton->setBounds (16, 64, 208, 24);

    autodetectButton.reset (new juce::ToggleButton ("Autodetect button"));
    addAndMakeVisible (autodetectButton.get());
    autodetectButton->setButtonText (TRANS("Autodetect TouchKeys octave on each start"));
    autodetectButton->addListener (this);

    autodetectButton->setBounds (16, 88, 272, 24);

    defaultsButton.reset (new juce::TextButton ("new button"));
    addAndMakeVisible (defaultsButton.get());
    defaultsButton->setButtonText (TRANS("Reset to Defaults..."));
    defaultsButton->addListener (this);

    defaultsButton->setBounds (16, 168, 144, 24);

    suppressStrayTouchSlider.reset (new juce::Slider ("suppress stray touch slider"));
    addAndMakeVisible (suppressStrayTouchSlider.get());
    suppressStrayTouchSlider->setRange (1, 5, 1);
    suppressStrayTouchSlider->setSliderStyle (juce::Slider::LinearHorizontal);
    suppressStrayTouchSlider->setTextBoxStyle (juce::Slider::NoTextBox, false, 80, 20);
    suppressStrayTouchSlider->addListener (this);

    suppressStrayTouchSlider->setBounds (104, 136, 120, 24);

    suppressStrayTouchButton.reset (new juce::ToggleButton ("suppress stray touches button"));
    addAndMakeVisible (suppressStrayTouchButton.get());
    suppressStrayTouchButton->setButtonText (TRANS("Suppress stray touches"));
    suppressStrayTouchButton->addListener (this);

    suppressStrayTouchButton->setBounds (16, 112, 168, 24);

    strayTouchThresholdLabel.reset (new juce::Label ("stray touch threshold label",
                                                     TRANS("Strength:")));
    addAndMakeVisible (strayTouchThresholdLabel.get());
    strayTouchThresholdLabel->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    strayTouchThresholdLabel->setJustificationType (juce::Justification::centredLeft);
    strayTouchThresholdLabel->setEditable (false, false, false);
    strayTouchThresholdLabel->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    strayTouchThresholdLabel->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    strayTouchThresholdLabel->setBounds (32, 136, 80, 24);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (296, 200);


    //[Constructor] You can add your own custom stuff here..

    // Initialise the combo box
    startupPresetComboBox->addItem("None", kStartupPresetNone);
    startupPresetComboBox->addItem("Vibrato and Pitch Bend", kStartupPresetVibratoPitchBend);
    startupPresetComboBox->addItem("Last Saved", kStartupPresetLastSaved);
    startupPresetComboBox->addItem("Choose...", kStartupPresetChoose);

    //[/Constructor]
}

PreferencesComponent::~PreferencesComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    startupPresetComboBox = nullptr;
    label4 = nullptr;
    startTouchKeysButton = nullptr;
    autodetectButton = nullptr;
    defaultsButton = nullptr;
    suppressStrayTouchSlider = nullptr;
    suppressStrayTouchButton = nullptr;
    strayTouchThresholdLabel = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PreferencesComponent::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xffd2d2d2));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PreferencesComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PreferencesComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    if(controller_ == 0)
        return;
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == startupPresetComboBox.get())
    {
        //[UserComboBoxCode_startupPresetComboBox] -- add your combo box handling code here..
        int selection = startupPresetComboBox->getSelectedId();
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
        //[/UserComboBoxCode_startupPresetComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void PreferencesComponent::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    if(controller_ == 0)
        return;
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == startTouchKeysButton.get())
    {
        //[UserButtonCode_startTouchKeysButton] -- add your button handler code here..
        controller_->setPrefsAutoStartTouchKeys(startTouchKeysButton->getToggleState());
        //[/UserButtonCode_startTouchKeysButton]
    }
    else if (buttonThatWasClicked == autodetectButton.get())
    {
        //[UserButtonCode_autodetectButton] -- add your button handler code here..
        controller_->setPrefsAutodetectOctave(autodetectButton->getToggleState());
        //[/UserButtonCode_autodetectButton]
    }
    else if (buttonThatWasClicked == defaultsButton.get())
    {
        //[UserButtonCode_defaultsButton] -- add your button handler code here..
        controller_->resetPreferences();
        //[/UserButtonCode_defaultsButton]
    }
    else if (buttonThatWasClicked == suppressStrayTouchButton.get())
    {
        //[UserButtonCode_suppressStrayTouchButton] -- add your button handler code here..
        if(suppressStrayTouchButton->getToggleState()) {
            controller_->setPrefsSuppressStrayTouches(suppressStrayTouchSlider->getValue());
        }
        else
            controller_->setPrefsSuppressStrayTouches(0);
        //[/UserButtonCode_suppressStrayTouchButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void PreferencesComponent::sliderValueChanged (juce::Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == suppressStrayTouchSlider.get())
    {
        //[UserSliderCode_suppressStrayTouchSlider] -- add your slider handling code here..
        int value = suppressStrayTouchSlider->getValue();
        if(suppressStrayTouchButton->getToggleState()) {
            controller_->setPrefsSuppressStrayTouches(value);
        }
        else
            controller_->setPrefsSuppressStrayTouches(0);
        //[/UserSliderCode_suppressStrayTouchSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

// Synchronize the UI state with the underlying state of the controller
void PreferencesComponent::synchronize(bool forceUpdates) {
    if(controller_ == 0)
        return;

    startTouchKeysButton->setToggleState(controller_->getPrefsAutoStartTouchKeys(), juce::NotificationType::dontSendNotification);
    autodetectButton->setToggleState(controller_->getPrefsAutodetectOctave(), juce::NotificationType::dontSendNotification);
    
    if(controller_->getPrefsSuppressStrayTouches() > 0) {
        suppressStrayTouchButton->setToggleState(true, juce::NotificationType::dontSendNotification);
        suppressStrayTouchSlider->setEnabled(true);
        suppressStrayTouchSlider->setValue(controller_->getPrefsSuppressStrayTouches(), juce::NotificationType::dontSendNotification);
    }
    else {
        suppressStrayTouchButton->setToggleState(false, juce::NotificationType::dontSendNotification);
        suppressStrayTouchSlider->setEnabled(false);
        suppressStrayTouchSlider->setValue(1, juce::NotificationType::dontSendNotification);
    }
    
    if(controller_->getPrefsStartupPresetNone())
        startupPresetComboBox->setSelectedId(kStartupPresetNone);
    else if(controller_->getPrefsStartupPresetVibratoPitchBend())
        startupPresetComboBox->setSelectedId(kStartupPresetVibratoPitchBend);
    else if(controller_->getPrefsStartupPresetLastSaved())
        startupPresetComboBox->setSelectedId(kStartupPresetLastSaved);
    else {
        juce::String path = controller_->getPrefsStartupPreset();
        startupPresetComboBox->setText(path);
    }
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PreferencesComponent" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers="controller_(0)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="296" initialHeight="200">
  <BACKGROUND backgroundColour="ffd2d2d2"/>
  <COMBOBOX name="Startup preset combo box" id="244410f02f6c1c72" memberName="startupPresetComboBox"
            virtualName="" explicitFocusOrder="0" pos="16 32 264 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="e9b3daa69a8ac5c" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="16 8 160 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Load preset on startup:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15.0" kerning="0.0" bold="0" italic="0" justification="33"/>
  <TOGGLEBUTTON name="auto start TouchKeys button" id="62c82600413ca060" memberName="startTouchKeysButton"
                virtualName="" explicitFocusOrder="0" pos="16 64 208 24" buttonText="Start TouchKeys on startup"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="Autodetect button" id="69a491dfca4ea997" memberName="autodetectButton"
                virtualName="" explicitFocusOrder="0" pos="16 88 272 24" buttonText="Autodetect TouchKeys octave on each start"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TEXTBUTTON name="new button" id="89690e14d6bf00c0" memberName="defaultsButton"
              virtualName="" explicitFocusOrder="0" pos="16 168 144 24" buttonText="Reset to Defaults..."
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <SLIDER name="suppress stray touch slider" id="c31cda5155c929b3" memberName="suppressStrayTouchSlider"
          virtualName="" explicitFocusOrder="0" pos="104 136 120 24" min="1.0"
          max="5.0" int="1.0" style="LinearHorizontal" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <TOGGLEBUTTON name="suppress stray touches button" id="8f678eb2daf99e09" memberName="suppressStrayTouchButton"
                virtualName="" explicitFocusOrder="0" pos="16 112 168 24" buttonText="Suppress stray touches"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="stray touch threshold label" id="fe2b73ef00e219fd" memberName="strayTouchThresholdLabel"
         virtualName="" explicitFocusOrder="0" pos="32 136 80 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Strength:" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif      // TOUCHKEYS_NO_GUI
//[/EndFile]

