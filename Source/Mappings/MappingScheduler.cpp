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

  MappingScheduler.cpp: implements a thread in which mapping actions are
  performed. Each Mapping object implements a triggerReceived() method
  which is called by the hardware I/O thread. This method should do a
  minimal amount of work but pass the real work off to the performMapping()
  method which is called by the MappingScheduler thread. The scheduler
  also allows mapping calls to be performed in the absence of received data,
  for example to cause a parameter to ramp down over time if no touch data
  is received.
*/

#include "MappingScheduler.h"

#undef DEBUG_MAPPING_SCHEDULER


// Constructor
MappingScheduler::MappingScheduler(PianoKeyboard& keyboard, juce::String threadName)
: juce::Thread(threadName), keyboard_(keyboard),
  waitableEvent_(true), isRunning_(false), counter_(0)
#ifdef DEBUG_MAPPING_SCHEDULER_STATISTICS
  ,lastDebugStatisticsTimestamp_(0)
#endif
{
}

// Destructor
MappingScheduler::~MappingScheduler() {
    // Stop the thread
    stop();
    
    // Now go through and delete any mappings awaiting deletion
    // so these objects don't leak
    MappingAction nextAction;
    
    //while(actionsNow_.Consume(nextAction)) {
    while(actionsNow_.pop(nextAction)) {
        if(nextAction.who != nullptr && nextAction.action == kActionUnregisterAndDelete) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "~MappingScheduler(): Deleting mapping " << who << " (actionsNow)\n";
#endif
            delete nextAction.who;
        }
    }
    
    while(!actionsLater_.empty()) {
        nextAction = actionsLater_.begin()->second;
        
        if(nextAction.who != nullptr && nextAction.action == kActionUnregisterAndDelete) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "~MappingScheduler(): Deleting mapping " << who << " (actionsLater)\n";
#endif
            delete nextAction.who;
        }
        actionsLater_.erase(actionsLater_.begin());
    }
}

// Start the thread handling the scheduling.
void MappingScheduler::start() {
	if(isRunning_)
		return;
    startThread();
}

// Stop the scheduler thread if it is currently running.
void MappingScheduler::stop() {
	if(!isRunning_)
		return;
    
    // Tell the thread to quit and signal the event it waits on
    signalThreadShouldExit();
    waitableEvent_.signal();
    stopThread(-1);
    
	isRunning_ = false;
}

// Register a mapping to be called by the scheduler
void MappingScheduler::registerMapping(Mapping *who) {
    // Lock the mutex for insertions to ensure that only a single
    // thread can act as producer at any given time.
    juce::ScopedLock sl(actionsInsertionMutex_);
    
    actionsNow_.push(MappingAction(who, counter_, kActionRegister));
    
    // Increment the counter so each insertion gets a unique label
    counter_++;
    
    // Wake up the consumer thread
    waitableEvent_.signal();
}

// Schedule a mapping action to happen as soon as possible
void MappingScheduler::scheduleNow(Mapping *who) {
    // Lock the mutex for insertions to ensure that only a single
    // thread can act as producer at any given time.
    juce::ScopedLock sl(actionsInsertionMutex_);
    
    actionsNow_.push(MappingAction(who, counter_, kActionPerformMapping));
    
    // Increment the counter so each insertion gets a unique label
    counter_++;
    
    // Wake up the consumer thread
    waitableEvent_.signal();
}

// Schedule a mapping action to happen in the future at a specified timestamp
void MappingScheduler::scheduleLater(Mapping *who, timestamp_type timestamp) {
    juce::ScopedLock sl(actionsLaterMutex_);
    juce::ScopedLock sl2(actionsInsertionMutex_);
    
    bool newActionWillComeFirst = false;
    if(actionsLater_.empty())
        newActionWillComeFirst = true;
    else if(timestamp < actionsLater_.begin()->first)
        newActionWillComeFirst = true;
    
    actionsLater_.insert(std::pair<timestamp_type, MappingAction>(timestamp,
                                                                  MappingAction(who,
                                                                                counter_,
                                                                                kActionPerformMapping)));
    
    // Increment the counter so each insertion gets a unique label
    counter_++;
    
    // Wake up the consumer thread if what we inserted is the next
    // upcoming event
    if(newActionWillComeFirst)
        waitableEvent_.signal();
}

// Unschedule any further mappings from this object. Immediate mappings
// already in the queue may still be executed.
void MappingScheduler::unschedule(Mapping *who) {
    // Unscheduling works by inserting an action in the "now" queue
    // which preempts any further actions by this object.
    juce::ScopedLock sl(actionsInsertionMutex_);
    
    actionsNow_.push(MappingAction(who, counter_, kActionUnschedule));
    
    // Increment the counter to indicate we're at another cycle
    counter_++;
    
    // Wake up the consumer thread
    waitableEvent_.signal();
}

// Unregister a mapping which prevents it from being called by future events
void MappingScheduler::unregisterMapping(Mapping *who) {
    // Lock the mutex for insertions to ensure that only a single
    // thread can act as producer at any given time.
    juce::ScopedLock sl(actionsInsertionMutex_);
    
    actionsNow_.push(MappingAction(who, counter_, kActionUnregister));
    
    // Increment the counter so each insertion gets a unique label
    counter_++;
    
    // Wake up the consumer thread
    waitableEvent_.signal();
}


// Unschedule any further mappings from this object. Once any currently
// scheduled "now" mappings have been executed, delete the object in question.
void MappingScheduler::unregisterAndDelete(Mapping *who) {
    // Unscheduling works by inserting an action in the "now" queue
    // which preempts any further actions by this object. Deletion
    // will be handled by the consumer thread.
    juce::ScopedLock sl(actionsInsertionMutex_);
    
    actionsNow_.push(MappingAction(who, counter_, kActionUnregisterAndDelete));
    
    // Increment the counter to indicate we're at another cycle
    counter_++;
    
    // Wake up the consumer thread
    waitableEvent_.signal();
}

// This function runs in its own thread (from the Juce::Thread parent class). Every time
// it is signaled, it executes all the Mapping actions in the actionsNow_ category and then
// looks for the next delayed action.

void MappingScheduler::run() {
	isRunning_ = true;
	
    // This will run until the thread is interrupted (in the stop() method)
    while(!threadShouldExit()) {
        MappingAction nextAction;
        
        // Go through the accumulated actions in the "now" queue
        while(actionsNow_.pop(nextAction)) {
            if(nextAction.who != nullptr) {
#ifdef DEBUG_MAPPING_SCHEDULER
                std::cout << "Performing immediate mapping\n";
#endif
                performAction(nextAction);
            }
            
#ifdef DEBUG_MAPPING_SCHEDULER_STATISTICS
            printDebugStatistics();
#endif
        }
        
        // Next, grab the first upcoming action in the later category
        bool foundAction = true;
        timestamp_diff_type timeToNextAction = 0;
        
        while(foundAction && !threadShouldExit()) {
            // Lock the future actions mutex to examine the contents
            // of the future actions collection
            actionsLaterMutex_.enter();
            foundAction = false;
            
            if(!actionsLater_.empty()) {
                auto it = actionsLater_.begin();
                timestamp_type t = it->first;
                
                timeToNextAction = t - keyboard_.schedulerCurrentTimestamp();
                if(timeToNextAction <= 0) {
                    // If we get here, we have a non-empty collection fo future actions, the first
                    // of which should happen by now. Copy the action, erase it from the collection
                    // and unlock the mutex before proceeding.
                    nextAction = it->second;
                    actionsLater_.erase(it);
                    foundAction = true;
                }
            }
            else
                timeToNextAction = 0;
            
            actionsLaterMutex_.exit();
        
            if(foundAction) {
                // If this is set, we found a future action which is supposed to happen by now.
                // Execute it and check the next one.
#ifdef DEBUG_MAPPING_SCHEDULER
                std::cout << "Performing delayed mapping\n";
#endif
                performAction(nextAction);
            }
            else {
#ifdef DEBUG_MAPPING_SCHEDULER
                std::cout << "Found no further actions\n";
#endif
            }
#ifdef DEBUG_MAPPING_SCHEDULER_STATISTICS
            printDebugStatistics();
#endif
        }
        
        if(timeToNextAction > 0) {
            // If we complete the above loop with timeToNextAction set greater than 0, it means
            // we found an action that's supposed to happen in the future, but isn't ready yet.
            // The alternative is that there were no further actions, in which case the loop will
            // terminate with timeToNextAction set to 0.
            
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Waiting for next action in " << timestamp_to_milliseconds(timeToNextAction) << "ms\n";
#elif defined(DEBUG_MAPPING_SCHEDULER_STATISTICS)
            if(timestamp_to_milliseconds(timeToNextAction) > 100)
                std::cout << "Waiting for next action in " << timestamp_to_milliseconds(timeToNextAction) << "ms\n";
#endif
            
            // Wait for the next action to arrive (unless signaled)
            waitableEvent_.wait(timestamp_to_milliseconds(timeToNextAction));
        }
        else {
            // No future actions found; wait for a signal
            
#if defined(DEBUG_MAPPING_SCHEDULER) || defined(DEBUG_MAPPING_SCHEDULER_STATISTICS)
            std::cout << "Waiting for next action\n";
#endif
            waitableEvent_.wait();
        }
        
        waitableEvent_.reset();             // Clear the signal
    }
}

// Perform a mapping action: either execute the mapping or unschedule it,
// depending on the contents of the MappingAction object.
void MappingScheduler::performAction(MappingAction const& mappingAction) {
    Mapping* who = mappingAction.who;
    bool skip = true;
    
    
    // Check if this mapping action has been superseded by another
    // one already executed which was scheduled at the same time or later.
    // For example, if multiple actions have the same counter and the same
    // object, only the first one will run.
    if(countersForMappings_.count(who) != 0) {
        if(countersForMappings_[who] < mappingAction.counter) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Found counter " << countersForMappings_[who] << " for mapping " << who << std::endl;
#endif
            skip = false;
        }
    }
    else if(mappingAction.action == kActionRegister) {
        // Registration can happen if there is no previous
        // counter for the object. This is in fact the expected case.
#ifdef DEBUG_MAPPING_SCHEDULER
        std::cout << "No counter for mapping " << who << " but allowing for registration\n";
#endif
        skip = false;
    }
    
    if(!skip) {
        // Update the last counter for this object
        countersForMappings_[who] = mappingAction.counter;
        
        if(mappingAction.action == kActionRegister) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Registering object " << mappingAction.who << " with counter " << mappingAction.counter << std::endl;
#endif
        }
        else if(mappingAction.action == kActionPerformMapping) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Performing mapping for object " << mappingAction.who << " with counter " << mappingAction.counter << std::endl;
#endif
            timestamp_type nextTimestamp = who->performMapping();
            
            // Reschedule for later if next timestamp isn't 0
            if(nextTimestamp != 0) {
#ifdef DEBUG_MAPPING_SCHEDULER
                std::cout << "Rescheduling object " << mappingAction.who << " for timestamp " << nextTimestamp << std::endl;
#endif
                scheduleLater(who, nextTimestamp);
            }
        }
        else if(mappingAction.action == kActionUnschedule) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Unscheduling object " << who << " with counter " << mappingAction.counter << std::endl;
#endif
            // Nothing to do in fact; updating the counter will cause all future actions on this
            // object to be ignored.
        }
        else if(mappingAction.action == kActionUnregister) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Unregistering and deleting object " << who << " with counter " << mappingAction.counter << std::endl;
#endif
            // Remove the object from the counter registry
            countersForMappings_.erase(who);
        }
        else if(mappingAction.action == kActionUnregisterAndDelete) {
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Unregistering and deleting object " << who << " with counter " << mappingAction.counter << std::endl;
#endif
            // Remove the object from the counter registry
            countersForMappings_.erase(who);
            
            // Delete this object
            delete mappingAction.who;
        }
        else {
            // Shouldn't happen
#ifdef DEBUG_MAPPING_SCHEDULER
            std::cout << "Unknown action " << mappingAction.action << " for object " << who << " with counter " << mappingAction.counter << std::endl;
#endif
        }
    }
    else {
#ifdef DEBUG_MAPPING_SCHEDULER
        std::cout << "Skipping action " << mappingAction.action << " for object " << who << " with counter " << mappingAction.counter << std::endl;
#endif
    }
}

#ifdef DEBUG_MAPPING_SCHEDULER_STATISTICS
void MappingScheduler::printDebugStatistics() {
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    if(currentTimestamp - lastDebugStatisticsTimestamp_ < milliseconds_to_timestamp(500))
        return;
    lastDebugStatisticsTimestamp_ = currentTimestamp;
    std::cout << "MappingScheduler: " << actionsNow_.size() << " now, " << actionsLater_.size() << " later";
    if(!actionsLater_.empty()) {
        std::cout << ", time lag = " << timestamp_to_milliseconds(currentTimestamp - actionsLater_.begin()->first) << std::endl;
    }
    else
        std::cout << std::endl;
}
#endif