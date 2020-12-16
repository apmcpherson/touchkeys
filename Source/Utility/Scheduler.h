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

  Scheduler.h: allows actions to be scheduled at future times. Runs a
  thread in which these actions are executed.
*/

#pragma once

#include "Types.h"
#include <JuceHeader.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <map>

/*
 * Scheduler
 *
 * This class allows function calls to be scheduled for arbitrary points in the future.
 * It maintains a list of future events, ordered by timestamp.  A dedicated thread scans the
 * list, and when it is time for an event to occur, the thread wakes up, executes it, deletes
 * it from the list, and goes back to sleep.
 */

class Scheduler : public juce::Thread {
public:	
	typedef boost::function<timestamp_type ()> action;
    
private:
    static const timestamp_diff_type kAllowableAdvanceExecutionTime;
	
public:	
	// ***** Constructor *****
	//
	// Note: This class is not copy-constructable.
	
	Scheduler(juce::String threadName = "Scheduler") : juce::Thread(threadName), waitableEvent_(true), isRunning_(false) {}
	
	// ***** Destructor *****
	
	~Scheduler() noexcept { stop(); }
	
	// ***** juce::Timer Methods *****
	//
	// These start and stop the thread that handles the scheduling of events.
	
	void start(timestamp_type where = 0);
	void stop();
	
	bool isRunning() { return isRunning_; }
	timestamp_type currentTimestamp();
	
	// ***** Event Management Methods *****
	//
	// This interface provides the ability to schedule and unschedule events for
	// future times.
	
	void schedule(void *who, action func, timestamp_type timestamp);
	void unschedule(void *who, timestamp_type timestamp = 0);
	void clear();
	
	//static void staticRunLoop(Scheduler* sch, timestamp_type starting_timestamp) { sch->runLoop(starting_timestamp); }
	
    // The main Thread run loop
	void run();

private:    
	// These variables keep track of the status of the separate thread running the events
    juce::CriticalSection eventMutex_;
	juce::WaitableEvent waitableEvent_;
    timestamp_type startingTimestamp_;
	bool isRunning_;
	
	// Collection of future events to execute
	//boost::posix_time::ptime startTime_;
    double startTimeMilliseconds_;
	std::multimap<timestamp_type, std::pair<void*, action> > events_;
};
