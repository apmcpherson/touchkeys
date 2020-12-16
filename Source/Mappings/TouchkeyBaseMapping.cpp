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

  TouchkeyBaseMapping.cpp: base class from which all TouchKeys-specific
  mappings derive. Like its Mapping parent class, it handles a mapping
  for one specific note.
*/

#include "TouchkeyBaseMapping.h"

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
TouchkeyBaseMapping::TouchkeyBaseMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                                         Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker,
                                                         bool finishesAutomatically)
: Mapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
  controlName_(""), noteIsOn_(false), finished_(true), finishRequested_(false), finishesAutomatically_(finishesAutomatically)
{
    setOscController(&keyboard_);
}

// Copy constructor
/*TouchkeyBaseMapping::TouchkeyBaseMapping(TouchkeyBaseMapping const& obj)
: Mapping(obj), noteIsOn_(obj.noteIsOn_), finished_(obj.finished_), finishRequested_(obj.finishRequested_),
  finishesAutomatically_(obj.finishesAutomatically_)
{
    setOscController(&keyboard_);
}*/

TouchkeyBaseMapping::~TouchkeyBaseMapping() {
#ifndef NEW_MAPPING_SCHEDULER
    try {
        disengage();
    }
    catch(...) {
        std::cerr << "~TouchkeyBaseMapping(): exception during disengage()\n";
    }
#endif
}

// Turn on mapping of data.
void TouchkeyBaseMapping::engage() {
    Mapping::engage();
    
    // Register for OSC callbacks on MIDI note on/off
    addOscListener("/midi/noteon");
	addOscListener("/midi/noteoff");
    
    //std::cout << "TouchkeyBaseMapping::engage(): after TS " << keyboard_.schedulerCurrentTimestamp() << std::endl;
}

// Turn off mapping of data. Remove our callback from the scheduler
void TouchkeyBaseMapping::disengage(bool shouldDelete) {
    // Remove OSC listeners first
    removeOscListener("/midi/noteon");
	removeOscListener("/midi/noteoff");
    
    // Don't send any change in bend, let it stay where it is
    
    Mapping::disengage(shouldDelete);
    
    noteIsOn_ = false;
}

// Reset state back to defaults
void TouchkeyBaseMapping::reset() {
    Mapping::reset();
    noteIsOn_ = false;
}

// Set the name of the control
void TouchkeyBaseMapping::setName(const std::string& name) {
    controlName_ = name;
}

// OSC handler method. Called from PianoKeyboard when MIDI data comes in.
bool TouchkeyBaseMapping::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void* /*data*/) {
    if(!strcmp(path, "/midi/noteon") && !noteIsOn_ && numValues >= 1) {
        if(types[0] == 'i' && values[0]->i == noteNumber_) {
            // First notify the subclass of this event
            if(numValues >= 3)
                midiNoteOnReceived(values[1]->i, values[2]->i);
            
            // When the MIDI note goes on, if the mapping doesn't finish automatically,
            // make sure we indicate we're busy so the factory doesn't delete the mapping when
            // touch and MIDI both stop. In this case, the subclass is responsible for notifying
            // the factory when finished.
            if(!finishesAutomatically_)
                finished_ = false;
            noteIsOn_ = true;
            return false;
        }
    }
    else if(!strcmp(path, "/midi/noteoff") && noteIsOn_ && numValues >= 1) {
        if(types[0] == 'i' && values[0]->i == noteNumber_) {
            // MIDI note goes off.
            
            // First notify the subclass of this event
            if(numValues >= 2)
                midiNoteOffReceived(values[1]->i);
            
            noteIsOn_ = false;
            return false;
        }
    }
    
    return false;
}

// Override the requestFinish method to return whether the object
// can be deleted now or not. If not, delete it when we've finished the
// release processing
bool TouchkeyBaseMapping::requestFinish() {
    finishRequested_ = true;
    return finished_;
}

// Acknowledge that the mapping is finished. Only relevant if the mapping doesn't
// finish automatically.
void TouchkeyBaseMapping::acknowledgeFinish() {
    if(!finishRequested_ || finishesAutomatically_)
        return;
    factory_->mappingFinished(noteNumber_);
}
