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

  TouchkeyBaseMapping.h: base class from which all TouchKeys-specific
  mappings derive. Like its Mapping parent class, it handles a mapping
  for one specific note.
*/

#pragma once

#include "MappingFactory.h"
#include "../TouchKeys/MidiOutputController.h"

// This class is a virtual base class for mappings which work specifically with TouchKeys
// and MIDI data (not continuous key angle), designed to work with TouchkeyBaseMappingFactory.
// Specific behaviors will be implemented in subclasses.

class TouchkeyBaseMapping : public Mapping, public OscHandler {
    friend class MappingFactory;
    
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	TouchkeyBaseMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                        Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker, bool finishesAutomatically = true);
    
    // ***** Destructor *****
    
    virtual ~TouchkeyBaseMapping();
	
    // ***** Modifiers *****
    
    // Enable mappings to be sent
    virtual void engage();
    
    // Disable mappings from being sent
    virtual void disengage(bool shouldDelete = false);
	
    // Reset the state back initial values
	virtual void reset();
    
    // Resend the current state of all parameters
    virtual void resend() = 0;
    
    // ***** Parameters *****
    
    // Name for this control, used in the OSC path
    virtual void setName(const std::string& name);
    
	// ***** Evaluators *****
    
    // OSC Handler Method: called by PianoKeyboard (or other OSC source)
	virtual bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
	
    // This method receives triggers whenever events occur in the touch data or the
    // continuous key position (state changes only). It alters the behavior and scheduling
    // of the mapping but does not itself send OSC messages
	virtual void triggerReceived(TriggerSource* who, timestamp_type timestamp) = 0;
	
    // This method handles the OSC message transmission. It should be run in the Scheduler
    // thread provided by PianoKeyboard.
    virtual timestamp_type performMapping() = 0;
    
    // Override the finished() method to give the note time for a post-release action.
    virtual bool requestFinish();
    
    // Acknowledge the finish when the mapping is done, removing the mapping
    virtual void acknowledgeFinish();
    
protected:
    // ***** Internal Methods for MIDI *****
    
    // These methods are called when an OSC message is received indicating
    // a MIDI note on/off message took place. They let the subclass insert
    // its own code right before the main MIDI processing.
    
    virtual void midiNoteOnReceived(int /*channel*/, int velocity) {}
    virtual void midiNoteOffReceived(int channel) {}
    
	// ***** Member Variables *****
    
    std::string controlName_;                   // Name of this control, used in the OSC message
    bool noteIsOn_;                             // Whether the MIDI note is active or not
    bool finished_;                             // Whether the note is finished
    bool finishRequested_;                      // Whether the factory has requested a finish
    bool finishesAutomatically_;                // Whether the mapping finishes automatically when requested
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchkeyBaseMapping)
};
