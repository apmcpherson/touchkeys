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

  TimerNode.cpp: creates a Node object which runs its own thread to generate
  timestamps.
*/

#include "TimerNode.h"

using std::cout;
using std::endl;

// Start the timer, if it isn't already running.  The update rate is set elsewhere.
void TimerNode::start(timestamp_type where) {
	if(isRunning_)
		return;
    startingTimestamp_ = where;
    startThread();
	isRunning_ = true;
}

// Stop the timer if it is currently running.  This kills the associated thread.
void TimerNode::stop() {
	if(!isRunning_)
		return;
    signalThreadShouldExit();
    notify();
    stopThread(-1); // Ask the thread to stop; no timeout
	isRunning_ = false;
}

// This function runs in its own thread, as managed by the Juce Thread parent class.  It produces
// data points approximately separated in time by intervalMicros_.  The resolution of the system
// timer affects how precise the spacing will be.  Though the ticks may jitter, there shouldn't
// be any systematic drift unless intervalMicros_ is smaller than the execution time of the loop.
// (For example, 1000 will be fine; 1 is too short)

void TimerNode::run() {
	unsigned long long targetMicros = 0;
	
	// Find the start time, against which our offsets will be measured.
    double startTime = juce::Time::getMillisecondCounterHiRes();
    double nextTime;
    
	while(!threadShouldExit()) {
		// Get the current time relative to when we started, using it as the "official" timestamp 
		// for the data source.
		insert(startingTimestamp_ + microseconds_to_timestamp(targetMicros), milliseconds_to_timestamp( juce::Time::getMillisecondCounterHiRes() - startTime));
		targetMicros += intervalMicros_;
        
        // Next millisecond time according to Juce timer
        nextTime = startTime + (double)targetMicros/1000.0;
		
		// Sleep until we get to the next tick.
		// thread_.sleep(startTime + microseconds(target_micros));
        wait((int)(nextTime - juce::Time::getMillisecondCounterHiRes()));
	}
}