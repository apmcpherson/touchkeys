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

  Mapping.h: base class for a single-note mapping. The mapping will take in
  MIDI, touch and (optionally) continuous key position data, and generate
  specific OSC or MIDI messages. TouchKeys-specific mappings generally
  inherit from the TouchkeyBaseMapping subclass, which provides other
  useful generic methods.
*/

#pragma once

#include "../TouchKeys/KeyTouchFrame.h"
#include "../TouchKeys/KeyPositionTracker.h"
#include "../TouchKeys/PianoKeyboard.h"
#include <map>
#include <boost/bind.hpp>

#define NEW_MAPPING_SCHEDULER

class MappingFactory;

// This virtual base class defines a mapping from keyboard data to OSC or
// other output information. Specific behavior is implemented by subclasses.

class Mapping : public TriggerDestination {
protected:
    // Default frequency of mapping data, in the absence of other triggers
    //const timestamp_diff_type kDefaultUpdateInterval = microseconds_to_timestamp(5500);
    static const timestamp_diff_type kDefaultUpdateInterval;
    
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
    Mapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                       Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);
	
	// Copy constructor
    Mapping(Mapping const& obj);
    
    // ***** Destructor *****
    
    virtual ~Mapping();
	
    // ***** Modifiers *****
    
    // Enable mappings to be sent
    virtual void engage();
    
    // Disable mappings from being sent
    virtual void disengage(bool shouldDelete = false);
	
    // Reset the state back initial values
	virtual void reset();
    
    // Set the interval between mapping actions
    virtual void setUpdateInterval(timestamp_diff_type interval) {
        if(interval <= 0)
            return;
        updateInterval_ = interval;
    }
    
    // Suspend any further messages from this mapping
    virtual void suspend() { suspended_ = true; }
    
    // Resume sending messages, optionally re-sending the current state
    virtual void resume(bool resendCurrentState) {
        if(resendCurrentState)
            resend();
        suspended_ = false;
    }
    
    // Resend the current state of all the managed parameters
    virtual void resend() {}

	// ***** Evaluators *****
	// These are the main mapping functions, and they need to be implemented
    // specifically in any subclass.
    
    // This method receives triggers whenever events occur in the touch data or the
    // continuous key position (state changes only).
	virtual void triggerReceived(TriggerSource* who, timestamp_type timestamp) = 0;
	
    // This method is run periodically the Scheduler provided by PianoKeyboard and
    // handles the actual work of performing the mapping.
    virtual timestamp_type performMapping() = 0;
    
    // Indicate whether the mapping has finished all of its processing and can be deleted.
    // By default a mapping can be deleted whenever the factory wants to, but some
    // may persist beyond note release for a limited period of time.
    virtual bool requestFinish() { return true; }
    
protected:
    
	// ***** Member Variables *****
	
    PianoKeyboard& keyboard_;                   // Reference to the main keyboard controller
    MappingFactory *factory_;                   // Factory that created this mapping
    int noteNumber_;                            // MIDI note number for this key
    Node<KeyTouchFrame>* touchBuffer_;          // Key touch location history
	Node<key_position>* positionBuffer_;		// Raw key position data
    KeyPositionTracker* positionTracker_;       // Object which manages states of key
    
    bool engaged_;                              // Whether we're actively mapping
    bool suspended_;                            // Whether we're suppressing messages
    timestamp_diff_type updateInterval_;        // How long between mapping calls
    timestamp_type nextScheduledTimestamp_;     // When we've asked for the next callback
    Scheduler::action mappingAction_;           // Action function which calls performMapping()
};
