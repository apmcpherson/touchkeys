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

  MappingFactorySplitter.cpp: MappingFactory subclass which in turn contains
  several factories, routing all incoming method calls to each of them.
*/

#include "MappingFactorySplitter.h"

// Look up a mapping with the given note number
Mapping* MappingFactorySplitter::mapping(int noteNumber) {
    // TODO: find a mapping in any of the factories
    if(factories_.empty())
        return 0;
    return (*factories_.begin())->mapping(noteNumber);
}

// Return a list of all active notes
std::vector<int> MappingFactorySplitter::activeMappings() {
    // TODO: merge all active factories
    if(factories_.empty()) {
        return std::vector<int>();
    }
    return (*factories_.begin())->activeMappings();
}

// Remove all active mappings
void MappingFactorySplitter::removeAllMappings() {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->removeAllMappings();
    }
}

// Suspend messages from a particular note
void MappingFactorySplitter::suspendMapping(int noteNumber) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->suspendMapping(noteNumber);
    }
}

// Suspend messages from all notes
void MappingFactorySplitter::suspendAllMappings() {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->suspendAllMappings();
    }
}

// Resume messages from a particular note
void MappingFactorySplitter::resumeMapping(int noteNumber, bool resend) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->resumeMapping(noteNumber, resend);
    }
}

// Resume messages from all notes
void MappingFactorySplitter::resumeAllMappings(bool resend) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->resumeAllMappings(resend);
    }
}

// Whether the component parts of this mapping are bypassed
int MappingFactorySplitter::bypassed() {
    if(factories_.empty())
        return bypassNewMappings_;
    bool bypassOn = false, bypassOff = false;
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        if((*it)->bypassed())
            bypassOn = true;
        else
            bypassOff = true;
    }
    
    // Three states: all on, all off, or some on + some off (mixed)
    if(bypassOn && !bypassOff)
        return kBypassOn;
    if(bypassOff && !bypassOn)
        return kBypassOff;
    return kBypassMixed;
}

// Set whether the component parts are bypassed
void MappingFactorySplitter::setBypassed(bool bypass) {

    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->setBypassed(bypass);
    }
    bypassNewMappings_ = bypass;
}

// Add a factory to the list
void MappingFactorySplitter::addFactory(MappingFactory* factory) {
    if(bypassNewMappings_)
        factory->setBypassed(true);
    factories_.push_back(factory);
}

// Remove a factory from the list
void MappingFactorySplitter::removeFactory(MappingFactory* factory) {
    // Assume allocation/deallocation takes place elsewhere
    auto it = factories_.begin();
    
    while(it != factories_.end()) {
        if(*it == factory)
            factories_.erase(it++);
        else
            it++;
    }
}

// Remove all factories from the list
void MappingFactorySplitter::removeAllFactories() {
    // Assume allocation/deallocation takes place elsewhere
    factories_.clear();
}

// Generate XML element with preset settings
std::unique_ptr< juce::XmlElement > MappingFactorySplitter::getPreset() {
    auto preset = std::make_unique< juce::XmlElement >("MappingFactory");
    preset->setAttribute("type", "Splitter");
    return preset;
}

bool MappingFactorySplitter::loadPreset(juce::XmlElement const* preset) {
    return true;
}

// Touch becomes active on a key where it wasn't previously
void MappingFactorySplitter::touchBegan(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                Node<KeyTouchFrame>* touchBuffer,
                Node<key_position>* positionBuffer,
                KeyPositionTracker* positionTracker) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->touchBegan(noteNumber, midiNoteIsOn, keyMotionActive,
                          touchBuffer, positionBuffer, positionTracker);
    }
}

// Touch ends on a key where it wasn't previously
void MappingFactorySplitter::touchEnded(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                Node<KeyTouchFrame>* touchBuffer,
                Node<key_position>* positionBuffer,
                KeyPositionTracker* positionTracker) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->touchEnded(noteNumber, midiNoteIsOn, keyMotionActive,
                          touchBuffer, positionBuffer, positionTracker);
    }
}

// MIDI note on for a key
void MappingFactorySplitter::midiNoteOn(int noteNumber, bool touchIsOn, bool keyMotionActive,
                Node<KeyTouchFrame>* touchBuffer,
                Node<key_position>* positionBuffer,
                KeyPositionTracker* positionTracker) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->midiNoteOn(noteNumber, touchIsOn, keyMotionActive,
                          touchBuffer, positionBuffer, positionTracker);
    }
}

// MIDI note off for a key
void MappingFactorySplitter::midiNoteOff(int noteNumber, bool touchIsOn, bool keyMotionActive,
                 Node<KeyTouchFrame>* touchBuffer,
                 Node<key_position>* positionBuffer,
                 KeyPositionTracker* positionTracker) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->midiNoteOff(noteNumber, touchIsOn, keyMotionActive,
                           touchBuffer, positionBuffer, positionTracker);
    }
}

// Key goes active from continuous key position
void MappingFactorySplitter::keyMotionActive(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                     Node<KeyTouchFrame>* touchBuffer,
                     Node<key_position>* positionBuffer,
                     KeyPositionTracker* positionTracker) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->keyMotionActive(noteNumber, midiNoteIsOn, touchIsOn,
                           touchBuffer, positionBuffer, positionTracker);
    }
}

// Key goes idle from continuous key position
void MappingFactorySplitter::keyMotionIdle(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                   Node<KeyTouchFrame>* touchBuffer,
                   Node<key_position>* positionBuffer,
                   KeyPositionTracker* positionTracker) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->keyMotionIdle(noteNumber, midiNoteIsOn, touchIsOn,
                               touchBuffer, positionBuffer, positionTracker);
    }
}

// MIDI note about to begin
void MappingFactorySplitter::noteWillBegin(int noteNumber, int midiChannel, int midiVelocity) {
    
    for( auto it = factories_.begin(); it != factories_.end(); ++it) {
        (*it)->noteWillBegin(noteNumber, midiChannel, midiVelocity);
    }
}