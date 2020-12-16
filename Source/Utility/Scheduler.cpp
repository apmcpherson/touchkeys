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

  Scheduler.cpp: allows actions to be scheduled at future times. Runs a
  thread in which these actions are executed.
*/

#include "Scheduler.h"
#undef DEBUG_SCHEDULER

using std::cout;

const timestamp_diff_type Scheduler::kAllowableAdvanceExecutionTime = milliseconds_to_timestamp(1.0);

// Start the thread handling the scheduling.  Pass it an initial timestamp.
void Scheduler::start(timestamp_type where) {
	if(isRunning_)
		return;
    startingTimestamp_ = where;
    startThread();
}

// Stop the scheduler thread if it is currently running.  Events will remain
// in the queue unless explicitly cleared.
void Scheduler::stop() {
	if(!isRunning_)
		return;
    
    // Tell the thread to quit and signal the event it waits on
    signalThreadShouldExit();
    waitableEvent_.signal();
    stopThread(-1);
    
	isRunning_ = false;
}

// Return the current timestamp, relative to this class's start time.
timestamp_type Scheduler::currentTimestamp() {
	if(!isRunning_)
		return 0;
    return milliseconds_to_timestamp( juce::Time::getMillisecondCounterHiRes() - startTimeMilliseconds_);
	//return ptime_to_timestamp(microsec_clock::universal_time() - startTime_);
}

// Schedule a new event
void Scheduler::schedule(void *who, action func, timestamp_type timestamp) {
    juce::ScopedLock sl(eventMutex_);
    
#ifdef DEBUG_SCHEDULER
    std::cerr << "Scheduler::schedule: " << who << ", " << timestamp << " (" << timestamp - currentTimestamp() << " from now)\n";
#endif
    
    // Check if this timestamp will become the next thing in the queue
    bool newActionWillComeFirst = false;
    if(events_.empty())
        newActionWillComeFirst = true;
    else if(timestamp < events_.begin()->first)
        newActionWillComeFirst = true;
    events_.insert(std::pair<timestamp_type,std::pair<void*, action> >
					(timestamp, std::pair<void*, action>(who, func)));

	// Tell the thread to wake up and recheck its status if the
    // time of the next event has changed
    if(newActionWillComeFirst)
        waitableEvent_.signal();
}

// Remove an existing event
void Scheduler::unschedule(void *who, timestamp_type timestamp) {
#ifdef DEBUG_SCHEDULER
    std::cerr << "Scheduler::unschedule: " << who << ", " << timestamp << std::endl;
#endif
    juce::ScopedLock sl(eventMutex_);
    
	// Find all events with this timestamp, and remove only the ones matching the given source
    
    if(timestamp == 0) {
        // Remove all events from this source
        auto it = events_.begin();
        while(it != events_.end()) {
#ifdef DEBUG_SCHEDULER
            std::cerr << "| (" << it->first << ", " << it->second.first << ")\n";
#endif
            if(it->second.first == who) {
#ifdef DEBUG_SCHEDULER
                std::cerr << "--> erased " << it->first << ", " << it->second.first << ")\n";
#endif
                events_.erase(it++);
            }
            else
                it++;
        }
    }
    else {
        // Remove only a specific event from this source with the given timestmap
        auto it = events_.find(timestamp);
        while(it != events_.end()) {
            if(it->second.first == who) {
#ifdef DEBUG_SCHEDULER
                std::cerr << "--> erased " << it->first << ", " << it->second.first << ")\n";
#endif
                events_.erase(it++);
            }
            else
                it++;
        }
    }

#ifdef DEBUG_SCHEDULER
    std::cerr << "Scheduler::unschedule: done\n";
#endif
	// No need to wake up the thread...
}

// Clear all events from the queue
void Scheduler::clear() {
    juce::ScopedLock sl(eventMutex_);
    
	events_.clear();
	
	// No need to signal the condition variable.  If the thread is waiting, it can keep waiting.
}

// This function runs in its own thread (from the Juce parent class).  It looks for the next event
// in the queue.  When its time arrives, the event is executed and removed from the queue.
// When the queue is empty, or the next event has not arrived yet, the thread sleeps.

void Scheduler::run() {
    // Start with the mutex locked.  The wait() methods will unlock it.
    eventMutex_.enter();
    
	// Find the start time, against which our offsets will be measured.
	//startTime_ = microsec_clock::universal_time();
    startTimeMilliseconds_ = juce::Time::getMillisecondCounterHiRes();
	isRunning_ = true;
	
    // This will run until the thread is interrupted (in the stop() method)
    // events_ is ordered by increasing timestamp, so the next event to execute is always the first item.
    while(!threadShouldExit()) {
        if(events_.empty())	{					// If there are no events in the queue, wait until we're signaled
            eventMutex_.exit();                 // that a new one comes in.  Unlock the mutex and wait.
            waitableEvent_.wait();
            eventMutex_.enter();
        }
        else {
            timestamp_type t = events_.begin()->first;				// Find the timestamp of the first event
            double targetTimeMilliseconds = startTimeMilliseconds_ + timestamp_to_milliseconds(t);
            
            // Wait until that time arrives, provided it hasn't already
            int timeDifferenceMilliseconds = (int)floor(targetTimeMilliseconds - juce::Time::getMillisecondCounterHiRes() + 0.5);
#ifdef DEBUG_SCHEDULER
            std::cerr << "Scheduler::run: waiting for " << timeDifferenceMilliseconds << "ms\n";
#endif
            if(timeDifferenceMilliseconds > 0) {
                eventMutex_.exit();                                    
                waitableEvent_.wait(timeDifferenceMilliseconds);
                eventMutex_.enter();
            }
        }
        
        waitableEvent_.reset();             // Clear the signal
        
        if(threadShouldExit())
            break;
        
        // At this point, the mutex is locked.  We can change the contents of events_ without worrying about disrupting anything.
        
        if(events_.empty())				// Double check that we actually have an event to execute
            continue;
        if(currentTimestamp() + kAllowableAdvanceExecutionTime < events_.begin()->first) {
#ifdef DEBUG_SCHEDULER
            std::cerr << "Scheduler::run: next event hasn't arrived (currently " << currentTimestamp() << ", waiting for " << events_.begin()->first << "\n";
#endif
            continue;
        }
        
        // Run the function that's stored, which takes no arguments and returns a timestamp
        // of the next time this particular function should run.
        auto it = events_.begin();
        action actionFunction = (it->second).second;
        //timestamp_type testingTimestamp = it->first;
        void *who = it->second.first;
        
#ifdef DEBUG_SCHEDULER
        std::cerr << "Scheduler::run: " << who << ", " << it->first << std::endl;
#endif
        
        timestamp_type timeOfNextEvent = actionFunction();
        
        // Remove the last event from the queue
        events_.erase(it);
        
        if(timeOfNextEvent > 0) {
            // Reschedule the same event for some (hopefully) future time.
            events_.insert(std::pair<timestamp_type,std::pair<void*, action> >
                           (timeOfNextEvent,
                            std::pair<void*, action>(who, actionFunction)));
        }
    }
    
    eventMutex_.exit();
}