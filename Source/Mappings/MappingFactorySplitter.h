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

  MappingFactorySplitter.h: MappingFactory subclass which in turn contains
  several factories, routing all incoming method calls to each of them.
*/

#pragma once

#include "MappingFactory.h"
#include <iostream>
#include <list>

// Factory class to produce Touchkey vibrato (pitch-bend) mappings
// This class keeps track of all the active mappings and responds
// whenever touches or notes begin or end

class MappingFactorySplitter : public MappingFactory {
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    MappingFactorySplitter(PianoKeyboard &keyboard) : MappingFactory(keyboard), bypassNewMappings_(false) {}
	
    // ***** Destructor *****
    
    ~MappingFactorySplitter() {
        removeAllFactories();
    }
    
    // ***** Accessors / Modifiers *****
    
    Mapping* mapping(int noteNumber);                  // Look up a mapping with the given note number
    std::vector<int> activeMappings();                 // Return a list of all active notes
    
    void removeAllMappings();                          // Remove all active mappings
    void mappingFinished(int /*noteNumber*/) {}            // Callback from a mapping to say it's finished.
                                                       // Nobody should ever call this since we don't have our own mappings
    
    void suspendMapping(int noteNumber);                // Suspend messages from a particular note
    void suspendAllMappings();                          // ... or all notes
    void resumeMapping(int noteNumber, bool resend);    // Resume messages from a particular note
    void resumeAllMappings(bool resend);                // ... or all notes
    
    int bypassed();                                     // Whether this mapping is bypassed
    void setBypassed(bool bypass);                      // Set whether the mapping is bypassed or not
    
    // ***** Specific Methods *****
    
    void addFactory(MappingFactory* factory);
    void removeFactory(MappingFactory* factory);
    void removeAllFactories();
    
    // ****** Preset Save/Load ******
    // These methods generate XML settings files and reload values from them
    // The specific implementation is up to the subclass
    
    std::unique_ptr< juce::XmlElement > getPreset();
    bool loadPreset(juce::XmlElement const* preset);

    // ***** State Updaters *****
    
    // These are called by PianoKey whenever certain events occur that might
    // merit the start and stop of a mapping. What is done with them depends on
    // the particular factory subclass.
    
    // Touch becomes active on a key where it wasn't previously
    void touchBegan(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker);
    // Touch ends on a key where it wasn't previously
    void touchEnded(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker);
    // MIDI note on for a key
    void midiNoteOn(int noteNumber, bool touchIsOn, bool keyMotionActive,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker);
    // MIDI note off for a key
    void midiNoteOff(int noteNumber, bool touchIsOn, bool keyMotionActive,
                     Node<KeyTouchFrame>* touchBuffer,
                     Node<key_position>* positionBuffer,
                     KeyPositionTracker* positionTracker);
    // Key goes active from continuous key position
    void keyMotionActive(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                         Node<KeyTouchFrame>* touchBuffer,
                         Node<key_position>* positionBuffer,
                         KeyPositionTracker* positionTracker);
    // Key goes idle from continuous key position
    void keyMotionIdle(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                       Node<KeyTouchFrame>* touchBuffer,
                       Node<key_position>* positionBuffer,
                       KeyPositionTracker* positionTracker);
    // MIDI note about to begin
    void noteWillBegin(int noteNumber, int midiChannel, int midiVelocity);

private:
    
    // State variables
    std::list<MappingFactory*> factories_;              // List of child factories
    bool bypassNewMappings_;                            // Whether to bypass mappings that are added
};
