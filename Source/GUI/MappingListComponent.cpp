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
 
  =====================================================================

  MappingListComponent.cpp: manages a ListBox to display the current
  mappigns associated with a keyboard segment.
*/

#ifndef TOUCHKEYS_NO_GUI

#include "MappingListComponent.h"
#include "MappingExtendedEditorWindow.h"

//==============================================================================
MappingListComponent::MappingListComponent() : controller_(0), keyboardSegment_(0),
  lastMappingFactoryIdentifier_(-1) {
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    addAndMakeVisible(&listBox_);
    listBox_.setModel(this);
    listBox_.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
    listBox_.setOutlineThickness(1);
    listBox_.setRowHeight(72);
}

MappingListComponent::~MappingListComponent() {
    clearExtendedEditorWindows();
}

void MappingListComponent::resized() {
    // This method is where you should set the bounds of any child
    // components that your component contains..
    listBox_.setBoundsInset (juce::BorderSize<int> (4));
}

// Delete the given factory from the mapping list
void MappingListComponent::deleteMapping(MappingFactory* factory) {
    if(keyboardSegment_ == nullptr || factory == nullptr)
        return;
    keyboardSegment_->removeMappingFactory(factory);
}

int MappingListComponent::getNumRows() {
    if(controller_ == nullptr || keyboardSegment_ == nullptr)
        return 0;
    return keyboardSegment_->mappingFactories().size();
}

// Given a row number and a possibly an existing component, return the component
// that should be drawn in this row of the list. Whenever a new component is created,
// the existing one should be deleted by this method (according to Juce docs).
juce::Component* MappingListComponent::refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component *existingComponentToUpdate) {
    if(keyboardSegment_ == nullptr)
        return nullptr;

    //std::cout << "refreshing component for row " << rowNumber << " (given " << existingComponentToUpdate << ")\n";
    if(rowNumber < 0 || rowNumber >= getNumRows()) {
        if(existingComponentToUpdate != 0)
            delete existingComponentToUpdate;
        return nullptr;
    }
    
    // Get the current component for the row, creating it if it doesn't exist
    MappingListItem *listItem = static_cast<MappingListItem*>(existingComponentToUpdate);
    if(listItem == nullptr) {
        listItem = new MappingListItem(*this);
        listItem->setMappingFactory(keyboardSegment_->mappingFactories()[rowNumber]);
        //std::cout << "item " << listItem << " was updated to factory " << keyboardSegment_->mappingFactories()[rowNumber] << std::endl;
    }
    else {
        // juce::Component exists; does it still point to a factory?
        if(rowNumber >= keyboardSegment_->mappingFactories().size()) {
            //std::cout << "Deleting component " << listItem << std::endl;
            delete listItem;
            return nullptr;
        }
        else if(keyboardSegment_->mappingFactories()[rowNumber] != listItem->mappingFactory()) {
            //std::cout << "Changing item " << listItem << " to point to factory " << keyboardSegment_->mappingFactories()[rowNumber] << std::endl;
            listItem->setMappingFactory(keyboardSegment_->mappingFactories()[rowNumber]);
        }
    }
    
    return listItem;
}

// Return whether a given component is selected or not (called by MappingListItem)
bool MappingListComponent::isComponentSelected(juce::Component *component) {
    int rowNumber = listBox_.getRowNumberOfComponent(component);
    if(rowNumber < 0)
        return false;
    return listBox_.isRowSelected(rowNumber);
}


void MappingListComponent::synchronize() {
    if(keyboardSegment_ != 0) {
        if(lastMappingFactoryIdentifier_ != keyboardSegment_->mappingFactoryUniqueIdentifier()) {
            lastMappingFactoryIdentifier_ = keyboardSegment_->mappingFactoryUniqueIdentifier();
            listBox_.updateContent();
            updateExtendedEditorWindows();
        }
    }
    
    for(int i = 0; i < getNumRows(); i++) {
        MappingListItem *listItem = static_cast<MappingListItem*>(listBox_.getComponentForRowNumber(i));
        if(listItem != 0)
            listItem->synchronize();
    }
    
    synchronizeExtendedEditorWindows();
}

// Open an extended editor window for the given component
// Store the new window in the list; it is deleted when it is closed
void MappingListComponent::openExtendedEditorWindow(MappingFactory *factory) {
    if(factory == nullptr)
        return;
    
    juce::ScopedLock sl(extendedEditorWindowsMutex_);
    
    MappingExtendedEditorWindow *window = new MappingExtendedEditorWindow(*this,
                                                                          *keyboardSegment_, *factory);
    extendedEditorWindows_.push_back(window);
}

// Close an extended editor window and remove it from the list
void MappingListComponent::closeExtendedEditorWindow(MappingExtendedEditorWindow *window) {
    juce::ScopedLock sl(extendedEditorWindowsMutex_);

    closeExtendedEditorWindowHelper(window);
}

MappingExtendedEditorWindow* MappingListComponent::extendedEditorWindowForFactory(MappingFactory *factory) {
    juce::ScopedLock sl(extendedEditorWindowsMutex_);
    
    for( auto it = extendedEditorWindows_.begin(); it != extendedEditorWindows_.end(); ++it) {
        if((*it)->factory() == factory)
            return *it;
    }
    
    return nullptr;
}

// Update extended editor windows
void MappingListComponent::synchronizeExtendedEditorWindows() {
    // Update extended editor windows
    juce::ScopedLock sl(extendedEditorWindowsMutex_);
    
    for( auto it = extendedEditorWindows_.begin(); it != extendedEditorWindows_.end(); ++it) {
        (*it)->synchronize();
    }
}

// Close an extended editor window and remove it from the list (interval version without lock)
void MappingListComponent::closeExtendedEditorWindowHelper(MappingExtendedEditorWindow *window) {
    window->setVisible(false);
    
    bool found = true;
    
    // Remove this window from the list (handling multiple entries just in case)
    while(found) {
        found = false;
        for( auto it = extendedEditorWindows_.begin(); it != extendedEditorWindows_.end(); ++it) {
            if(*it == window) {
                extendedEditorWindows_.erase(it);
                found = true;
                break;
            }
        }
    }
    
    // Delete the window which in turn deletes the editor component
    delete window;
}

// Find the invalid extended editor windows and close them
void MappingListComponent::updateExtendedEditorWindows() {
    juce::ScopedLock sl(extendedEditorWindowsMutex_);
    
    bool found = true;
    
    // Remove the window from the list if it is invalid
    while(found) {
        found = false;
        for( auto it = extendedEditorWindows_.begin(); it != extendedEditorWindows_.end(); ++it) {
            if(!(*it)->isValid()) {
                closeExtendedEditorWindowHelper(*it);
                found = true;
                break;
            }
        }
    }
}

// Remove all extend editor windows
void MappingListComponent::clearExtendedEditorWindows() {
    juce::ScopedLock sl(extendedEditorWindowsMutex_);
    
    for( auto it = extendedEditorWindows_.begin(); it != extendedEditorWindows_.end(); ++it) {
        delete *it;
    }
    
    extendedEditorWindows_.clear();
}

#endif  // TOUCHKEYS_NO_GUI
