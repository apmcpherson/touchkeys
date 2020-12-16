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
 
  Trigger.cpp: interface for objects to notify one another that new data
  has arrived. TriggerSource can send triggers to any object inheriting
  from TriggerDestination. Methods of keeping sources and destinations
  in sync are included in each class.
*/

#include "Trigger.h"

#undef DEBUG_TRIGGERS

void TriggerSource::sendTrigger(timestamp_type timestamp) {
#ifdef DEBUG_TRIGGERS
    std::cerr << "sendTrigger (" << this << ")\n";
#endif
    
    if(triggerDestinationsModified_) {
        juce::ScopedLock sl(triggerSourceMutex_);
        processAddRemoveQueue();
    }
    
    auto it = triggerDestinations_.begin();
	TriggerDestination* target;
	while(it != triggerDestinations_.end()) {	// Advance the iterator before sending the trigger
		target = *it;							// in case the triggerReceived routine causes the object to unregister
#ifdef DEBUG_TRIGGERS
        std::cerr << " --> " << target << std::endl;
#endif
		target->triggerReceived(this, timestamp);
        it++;
	}
}

void TriggerSource::addTriggerDestination(TriggerDestination* dest) { 
#ifdef DEBUG_TRIGGERS
    std::cerr << "addTriggerDestination (" << this << "): " << dest << "\n";
#endif
	if(dest == 0 || (void*)dest == (void*)this)
		return;
    juce::ScopedLock sl(triggerSourceMutex_);
    // Make sure this trigger isn't already present
    if(triggerDestinations_.count(dest) == 0) {
        triggersToAdd_.insert(dest);
        triggerDestinationsModified_ = true;
    }
    // If the trigger is also slated to be removed, cancel that request
    if(triggersToRemove_.count(dest) != 0)
        triggersToRemove_.erase(dest);
}

void TriggerSource::removeTriggerDestination(TriggerDestination* dest) {
#ifdef DEBUG_TRIGGERS
    std::cerr << "removeTriggerDestination (" << this << "): " << dest << "\n";
#endif
    juce::ScopedLock sl(triggerSourceMutex_);
    // Check whether this trigger is actually present
    if(triggerDestinations_.count(dest) != 0) {
        triggersToRemove_.insert(dest);
        triggerDestinationsModified_ = true;
    }
    // If the trigger is also slated to be added, cancel that request
    if(triggersToAdd_.count(dest) != 0)
        triggersToAdd_.erase(dest);
}	

void TriggerSource::clearTriggerDestinations() {
#ifdef DEBUG_TRIGGERS
    std::cerr << "clearTriggerDestinations (" << this << ")\n";
#endif
    juce::ScopedLock sl(triggerSourceMutex_);
    processAddRemoveQueue();
	for( auto it = triggerDestinations_.begin(); it != triggerDestinations_.end(); ++it)
		(*it)->triggerSourceDeleted(this);		
	triggerDestinations_.clear();
}

// Process everything in the add and remove groups and transfer them
// into the main set of trigger destinations. Do this with mutex locked.
void TriggerSource::processAddRemoveQueue() {
#ifdef DEBUG_TRIGGERS
    std::cerr << "processAddRemoveQueue (" << this << ")\n";
#endif

    for( auto it = triggersToAdd_.begin(); it != triggersToAdd_.end(); ++it) {
        triggerDestinations_.insert(*it);
#ifdef DEBUG_TRIGGERS
        std::cerr << " --> added " << *it << std::endl;
#endif
    }
    for( auto it = triggersToRemove_.begin(); it != triggersToRemove_.end(); ++it) {
        triggerDestinations_.erase(*it);
#ifdef DEBUG_TRIGGERS
        std::cerr << " --> removed " << *it << std::endl;
#endif
    }
    triggersToAdd_.clear();
    triggersToRemove_.clear();
    triggerDestinationsModified_ = false;
}