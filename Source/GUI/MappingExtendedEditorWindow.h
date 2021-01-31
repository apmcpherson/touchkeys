/*
  ==============================================================================

    MappingExtendedEditorWindow.h
    Created: 18 Jun 2014 10:57:00am
    Author:  Andrew McPherson

  ==============================================================================
*/

#pragma once

#ifndef TOUCHKEYS_NO_GUI

#include "../TouchKeys/MidiKeyboardSegment.h"
#include "../Mappings/MappingFactory.h"
#include "MappingListComponent.h"
#include <JuceHeader.h>

//==============================================================================
/*
*/
class MappingExtendedEditorWindow    : public juce::DocumentWindow
{
public:
    MappingExtendedEditorWindow(MappingListComponent& listComponent,
                                MidiKeyboardSegment& segment, MappingFactory& factory)
    : juce::DocumentWindow("", juce::Colours::lightgrey, juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton),
      listComponent_(listComponent), segment_(segment), factory_(factory), editor_{ nullptr }
    {
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        
        if((segment_.indexOfMappingFactory(&factory_) >= 0) && factory_.hasExtendedEditor()) {
            editor_ = factory_.createExtendedEditor();
        
            // Set properties
            setContentOwned(editor_.get(), true);
            
            // Start interface in sync
            editor_->synchronize();
        }
        
        // Show window
        setTopLeftPosition(60,60);
        setVisible(true);
    }

    ~MappingExtendedEditorWindow()
    {
    }

    // Method used by Juce timer which we will use for periodic UI updates
    // from the underlying system state
    void synchronize() {
        if(editor_ == nullptr)
            return;
        editor_->synchronize();
        setName(editor_->getDescription());
    }
    
    // Check whether this window still points to a valid mapping
    bool isValid() {
        if(segment_.indexOfMappingFactory(&factory_) < 0)
            return false;
        return true;
    }
    
    // Return the factory associated with this window
    MappingFactory *factory() {
        return &factory_;
    }
    
    void closeButtonPressed() override {
        // Close the window and delete it
        listComponent_.closeExtendedEditorWindow(this);
    }
    
    void resized() override {
        // This method is where you should set the bounds of any child
        // components that your component contains..
    }

private:
    MappingListComponent& listComponent_;
    MidiKeyboardSegment& segment_;
    MappingFactory& factory_;
    std::unique_ptr< MappingEditorComponent > editor_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingExtendedEditorWindow)
};

#endif      // TOUCHKEYS_NO_GUI
