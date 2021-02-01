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
#include "MappingListComponent.h"
#include "MappingExtendedEditorWindow.h"

#include "MappingListItem.h"

//==============================================================================
MappingListItem::MappingListItem (MappingListComponent& listComponent)
    : factory_(nullptr), 
    listComponent_(listComponent),
    bypassToggleButton{ "Bypass toggle button" },
    showDetailsButton{ "Show details button" },
    mappingTypeLabel{ "mapping type label", "MappingType" },
    mappingShortEditorComponent{ std::make_unique< MappingEditorComponent >() },
    noSettingsLabel{ "no settings label", "(no settings)" },
    deleteButton{ "delete button" }

{
    setLookAndFeel( &lnf );

    addAndMakeVisible (bypassToggleButton);
    bypassToggleButton.setButtonText ("Bypass");
    bypassToggleButton.addListener (this);

    addAndMakeVisible (showDetailsButton);
    showDetailsButton.setButtonText ("Details...");
    showDetailsButton.addListener (this);

    addAndMakeVisible (mappingTypeLabel);
    mappingTypeLabel.setFont (juce::Font (18.00f, juce::Font::plain));
    mappingTypeLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible ( mappingShortEditorComponent.get() );
    mappingShortEditorComponent->setName ("mapping short editor component");

    addAndMakeVisible (noSettingsLabel);
    noSettingsLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (deleteButton);
    deleteButton.setButtonText ("Delete...");
    deleteButton.addListener (this);

    setSize (544, 72);
}

MappingListItem::~MappingListItem()
{
    setLookAndFeel( nullptr );
}

void MappingListItem::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);

    g.setColour (juce::Colour (0xffa52a60));
    g.fillPath (internalPath1);
    g.setColour(juce::Colours::black);
    g.strokePath (internalPath1, juce::PathStrokeType (1.000f));

    g.setColour (juce::Colour (0xffa52a94));
    g.fillPath (internalPath2);
    g.setColour(juce::Colours::black);
    g.strokePath (internalPath2, juce::PathStrokeType (0.500f));

    /*MappingListComponent *parent = static_cast<MappingListComponent*>(getParentComponent());
    if(parent->isComponentSelected(this)) {
        g.setColour(juce::Colours::lightblue);
        g.drawRect (0, 0, 544, 72, 5);
    }*/
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
}

void MappingListItem::buttonClicked (juce::Button* buttonThatWasClicked)
{
    if(factory_ == nullptr)
        return;

    if (buttonThatWasClicked == &bypassToggleButton)
    {
        bool bypass = bypassToggleButton.getToggleState();
        factory_->setBypassed(bypass);
    }
    else if (buttonThatWasClicked == &showDetailsButton)
    {
        // Create an extended editor window
        MappingExtendedEditorWindow *window = listComponent_.extendedEditorWindowForFactory(factory_);
        if(window != nullptr) {
            window->setVisible(true);
            window->toFront(true);
        }
        else if(factory_->hasExtendedEditor())
            listComponent_.openExtendedEditorWindow(factory_);
    }
    else if (buttonThatWasClicked == &deleteButton)
    {
        // Display an alert to confirm the user wants to delete this mapping
        juce::AlertWindow::showOkCancelBox ( juce::AlertWindow::QuestionIcon,
                                      "Delete mapping",
                                      "Are you sure you want to delete this mapping?",
                                      juce::String{},
                                      juce::String{},
                                      0,
            juce::ModalCallbackFunction::forComponent (alertBoxResultChosen, this));
    }
}


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

#endif // TOUCHKEYS_NO_GUI
