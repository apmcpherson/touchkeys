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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#ifndef TOUCHKEYS_NO_GUI

#include "../Mappings/MappingFactory.h"

class MappingListComponent;
//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MappingListItem  : public juce::Component,
                         public juce::Button::Listener
{
public:
    //==============================================================================
    MappingListItem (MappingListComponent& listComponent);
    ~MappingListItem();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    static void alertBoxResultChosen(int result, MappingListItem *item);
    void deleteMapping();

    MappingFactory* mappingFactory() { return factory_; }
    void setMappingFactory(MappingFactory *factory);
    void synchronize();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MappingFactory *factory_;
    MappingListComponent& listComponent_;
    //[/UserVariables]

    //==============================================================================
    juce::ToggleButton bypassToggleButton;
    juce::TextButton showDetailsButton;
    juce::Label mappingTypeLabel;
    std::unique_ptr< MappingEditorComponent > mappingShortEditorComponent;
    juce::Label noSettingsLabel;
    juce::TextButton deleteButton;
    juce::Path internalPath1;
    juce::Path internalPath2;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingListItem)
};

//[EndFile] You can add extra defines here...
#endif // TOUCHKEYS_NO_GUI
//[/EndFile]
