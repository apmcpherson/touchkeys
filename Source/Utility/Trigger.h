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
 
  Trigger.h: interface for objects to notify one another that new data
  has arrived. TriggerSource can send triggers to any object inheriting
  from TriggerDestination. Methods of keeping sources and destinations
  in sync are included in each class.
*/

#pragma once

#include "Types.h"
#include <JuceHeader.h>
#include <iostream>
#include <set>

class TriggerDestination;

/*
 * TriggerSource
 *
 * Provides a set of routines for an object that sends triggers with an associated timestamp.  All Node
 * objects inherit from Trigger, but other objects may use these routines as well.
 */

class TriggerSource {
	friend class TriggerDestination;
protected:
	// Send a trigger event out to all our registered listeners.  The type of the accompanying
	// data will be set by the template of the subclass.
	void sendTrigger(timestamp_type timestamp);
	
public:
	// ***** Constructor *****
	
	TriggerSource() {}	// No instantiating this class directly!
	
	// ***** Destructor *****
	
	~TriggerSource() { clearTriggerDestinations(); }	
	
	// ***** Connection Management *****
	
	bool hasTriggerDestinations() { return triggerDestinations_.size() > 0; }

private:
	// For internal use or use by friend class NodeBase only
	
	// These methods manage the list of objects to whom the triggers should be sent.  All objects
	// will inherit from the Triggerable base class.  These shouldn't be called by the user directly;
	// rather, they're called by Triggerable when it registers itself.
	
	void addTriggerDestination(TriggerDestination* dest);
	void removeTriggerDestination(TriggerDestination* dest);
	void clearTriggerDestinations();
    
    // When sources are added or removed, they are first stored in separate locations to be updated
    // prior to each new call of sendTrigger(). This way, destinations which are updated from functions
    // called from sendTrigger() do not render the set inconsistent in the middle.
    void processAddRemoveQueue();
	
private:
	std::set<TriggerDestination*> triggerDestinations_;
    std::set<TriggerDestination*> triggersToAdd_;
    std::set<TriggerDestination*> triggersToRemove_;
    bool triggerDestinationsModified_;
	juce::CriticalSection triggerSourceMutex_;
};

/*
 * TriggerDestination
 *
 * This class accepts a Trigger event.  Designed to be inherited by more complex objects.
 */

class TriggerDestination {
	friend class TriggerSource;
public:
	// ***** Constructors *****
	
	TriggerDestination() {}
	TriggerDestination(TriggerDestination const& obj) : registeredTriggerSources_(obj.registeredTriggerSources_) {}
	
	// This defines what we actually do when a trigger is received.  It should be implemented
	// by the subclass.
	virtual void triggerReceived(TriggerSource* who, timestamp_type timestamp) { /*std::cout << "     received this = " << this << " who = " << who << std::endl;*/ }
	
	// These methods register and unregister sources of triggers.
	
	void registerForTrigger(TriggerSource* src) {
		//std::cout<<"registerForTrigger: this = " << this << " src = " << src << std::endl;
		
		if(src == 0 || (void*)src == (void*)this)
			return;
		juce::ScopedLock sl(triggerDestMutex_);
		src->addTriggerDestination(this);
		registeredTriggerSources_.insert(src);
	}
	
	void unregisterForTrigger(TriggerSource* src) {
		if(src == 0 || (void*)src == (void*)this)
			return;
        juce::ScopedLock sl(triggerDestMutex_);
		src->removeTriggerDestination(this);
		registeredTriggerSources_.erase(src);
	}
	
	void clearTriggers() {
		juce::ScopedLock sl(triggerDestMutex_);
		for( auto it = registeredTriggerSources_.begin(); it != registeredTriggerSources_.end(); it++)
			(*it)->removeTriggerDestination(this);
		registeredTriggerSources_.clear();
	}
	
protected:
	// This method is called by a TriggerBase object when it is deleted, so that we know not
	// to contact it later when this object is deleted.  This is different than unregisterForTrigger()
	// because it only removes the reference and does not call the TriggerBase object (which would create
	// an infinite loop).
	
	void triggerSourceDeleted(TriggerSource* src) { registeredTriggerSources_.erase(src); }
	
public:
	// ***** Destructor *****
	//
	// Remove all trigger sources before this object goes away
	
	virtual ~TriggerDestination() { clearTriggers(); }
	
private:
	// Keep an internal registry of who we've asked to send us triggers.  It's important to keep
	// a list of these so that when this object is destroyed, all triggers are automatically unregistered.
	std::set<TriggerSource*> registeredTriggerSources_;
    juce::CriticalSection triggerDestMutex_;
};
