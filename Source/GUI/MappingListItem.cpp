/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#ifndef TOUCHKEYS_NO_GUI
#include "MappingListComponent.h"
#include "MappingExtendedEditorWindow.h"
//[/Headers]

#include "MappingListItem.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MappingListItem::MappingListItem (MappingListComponent& listComponent)
    : factory_(0), 
    listComponent_(listComponent),
    bypassToggleButton{ "Bypass toggle button" },
    showDetailsButton{ "Show details button" },
    mappingTypeLabel{ "mapping type label", "MappingType" },
    mappingShortEditorComponent{ std::make_unique< MappingEditorComponent >() },
    noSettingsLabel{ "no settings label", "(no settings)" },
    deleteButton{ "delete button" }

{
    addAndMakeVisible (bypassToggleButton);
    bypassToggleButton.setButtonText ("Bypass");
    bypassToggleButton.addListener (this);

    addAndMakeVisible (showDetailsButton);
    showDetailsButton.setButtonText ("Details...");
    showDetailsButton.addListener (this);

    addAndMakeVisible (mappingTypeLabel);
    mappingTypeLabel.setFont (juce::Font (18.00f, juce::Font::plain));
    mappingTypeLabel.setJustificationType (juce::Justification::centred);
    mappingTypeLabel.setEditable (false, false, false);
    mappingTypeLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    mappingTypeLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible ( mappingShortEditorComponent.get() );
    mappingShortEditorComponent->setName ("mapping short editor component");

    addAndMakeVisible (noSettingsLabel);
    noSettingsLabel.setFont (juce::Font (15.00f, juce::Font::plain));
    noSettingsLabel.setJustificationType (juce::Justification::centred);
    noSettingsLabel.setEditable (false, false, false);
    noSettingsLabel.setColour (juce::TextEditor::textColourId, juce::Colours::black);
    noSettingsLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);

    addAndMakeVisible (deleteButton);
    deleteButton.setButtonText ("Delete...");
    deleteButton.addListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (544, 72);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

MappingListItem::~MappingListItem()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MappingListItem::paint (juce::Graphics& g)
{
    // NOTE white colour obscures the text of the label components
    //g.fillAll(juce::Colours::white);
    g.fillAll( juce::Colours::grey );

    g.setColour (juce::Colour (0xffa52a60));
    g.fillPath (internalPath1);
    g.setColour(juce::Colours::black);
    g.strokePath (internalPath1, juce::PathStrokeType (1.000f));

    g.setColour (juce::Colour (0xffa52a94));
    g.fillPath (internalPath2);
    g.setColour(juce::Colours::black);
    g.strokePath (internalPath2, juce::PathStrokeType (0.500f));

    //[UserPaint] Add your own custom painting code here..
    /*MappingListComponent *parent = static_cast<MappingListComponent*>(getParentComponent());
    if(parent->isComponentSelected(this)) {
        g.setColour(juce::Colours::lightblue);
        g.drawRect (0, 0, 544, 72, 5);
    }*/
    //[/UserPaint]
}

void MappingListItem::resized()
{
    bypassToggleButton.setBounds (24, 44, 72, 24);
    showDetailsButton.setBounds (456, 8, 80, 24);
    mappingTypeLabel.setBounds (8, 4, 104, 40);
    mappingShortEditorComponent->setBounds (120, 0, 328, 71);
    noSettingsLabel.setBounds (208, 24, 150, 24);
    deleteButton.setBounds (456, 44, 80, 20);
    internalPath1.clear();
    internalPath1.startNewSubPath (544.0f, 72.0f);
    internalPath1.lineTo (0.0f, 72.0f);
    internalPath1.closeSubPath();

    internalPath2.clear();
    internalPath2.startNewSubPath (119.0f, 16.0f);
    internalPath2.lineTo (119.0f, 56.0f);
    internalPath2.closeSubPath();

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MappingListItem::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    if(factory_ == nullptr)
        return;
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == &bypassToggleButton)
    {
        //[UserButtonCode_bypassToggleButton] -- add your button handler code here..
        bool bypass = bypassToggleButton.getToggleState();
        factory_->setBypassed(bypass);
        //[/UserButtonCode_bypassToggleButton]
    }
    else if (buttonThatWasClicked == &showDetailsButton)
    {
        //[UserButtonCode_showDetailsButton] -- add your button handler code here..
        // Create an extended editor window
        MappingExtendedEditorWindow *window = listComponent_.extendedEditorWindowForFactory(factory_);
        if(window != 0) {
            window->setVisible(true);
            window->toFront(true);
        }
        else if(factory_->hasExtendedEditor())
            listComponent_.openExtendedEditorWindow(factory_);
        //[/UserButtonCode_showDetailsButton]
    }
    else if (buttonThatWasClicked == &deleteButton)
    {
        //[UserButtonCode_deleteButton] -- add your button handler code here..
        // Display an alert to confirm the user wants to delete this mapping
        juce::AlertWindow::showOkCancelBox ( juce::AlertWindow::QuestionIcon,
                                      "Delete mapping",
                                      "Are you sure you want to delete this mapping?",
                                      juce::String{},
                                      juce::String{},
                                      0,
            juce::ModalCallbackFunction::forComponent (alertBoxResultChosen, this));
        //[/UserButtonCode_deleteButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

// Called when user clicks a result in the alert box to confirm deletion
void MappingListItem::alertBoxResultChosen(int result, MappingListItem *item)
{
    if(result != 0) {
        item->deleteMapping();
    }
}

// Delete this mapping factory
void MappingListItem::deleteMapping()
{
    listComponent_.deleteMapping(factory_);
}

// Set the mapping factory and create any editor components it uses
void MappingListItem::setMappingFactory(MappingFactory *factory)
{
    factory_ = factory;

    if(factory_->hasBasicEditor()) {
        // Has a short editor: make one and add it to the window, using the same bounds
        // as before
        const juce::Rectangle<int>& bounds = mappingShortEditorComponent->getBounds();
        mappingShortEditorComponent = factory_->createBasicEditor();
        addAndMakeVisible(mappingShortEditorComponent.get());
        mappingShortEditorComponent->setBounds(bounds);
        noSettingsLabel.setVisible(false);
    }
    else {
        noSettingsLabel.setVisible(true);
        mappingShortEditorComponent->setVisible(false);
    }

    if(factory_->hasExtendedEditor()) {
        showDetailsButton.setEnabled(true);
    }
    else {
        showDetailsButton.setEnabled(false);
    }

    synchronize();
}

void MappingListItem::synchronize()
{
    if(factory_ == nullptr)
        return;

    // Update the label and the bypass button
    mappingTypeLabel.setText(factory_->factoryTypeName().c_str(), juce::NotificationType::dontSendNotification);
    if(factory_->bypassed() != MappingFactory::kBypassOff)
        bypassToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    else
        bypassToggleButton.setToggleState(false, juce::NotificationType::dontSendNotification);

    // Update the short and long components if present
    if(mappingShortEditorComponent != nullptr)
        mappingShortEditorComponent->synchronize();
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MappingListItem" componentName=""
                 parentClasses="public juce::Component" constructorParams="MappingListComponent&amp; listComponent"
                 variableInitialisers="factory_(0), listComponent_(listComponent)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="544" initialHeight="72">
  <BACKGROUND backgroundColour="ffffffff">
    <PATH pos="0 0 100 100" fill="solid: ffa52a60" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: ff000000" nonZeroWinding="1">s 544 72 l 0 72 x</PATH>
    <PATH pos="0 0 100 100" fill="solid: ffa52a94" hasStroke="1" stroke="0.5, mitered, butt"
          strokeColour="solid: ff000000" nonZeroWinding="1">s 119 16 l 119 56 x</PATH>
  </BACKGROUND>
  <TOGGLEBUTTON name="Bypass toggle button" id="cfe71c39a64f4704" memberName="bypassToggleButton"
                virtualName="" explicitFocusOrder="0" pos="24 44 72 24" buttonText="Bypass"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TEXTBUTTON name="Show details button" id="17ac5d15223ada90" memberName="showDetailsButton"
              virtualName="" explicitFocusOrder="0" pos="456 8 80 24" buttonText="Details..."
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <LABEL name="mapping type label" id="58b75e1d781dd4c6" memberName="mappingTypeLabel"
         virtualName="" explicitFocusOrder="0" pos="8 4 104 40" edTextCol="ff000000"
         edBkgCol="0" labelText="MappingType" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="18" bold="0" italic="0" justification="36"/>
  <GENERICCOMPONENT name="mapping short editor component" id="8cbc2e53072fcaa7" memberName="mappingShortEditorComponent"
                    virtualName="" explicitFocusOrder="0" pos="120 0 328 71" class="MappingEditorComponent"
                    params=""/>
  <LABEL name="no settings label" id="a8fb2694ebf4280b" memberName="noSettingsLabel"
         virtualName="" explicitFocusOrder="0" pos="208 24 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="(no settings)" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <TEXTBUTTON name="delete button" id="fced502f19d4fe5b" memberName="deleteButton"
              virtualName="" explicitFocusOrder="0" pos="456 44 80 20" buttonText="Delete..."
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
#endif // TOUCHKEYS_NO_GUI
//[/EndFile]
