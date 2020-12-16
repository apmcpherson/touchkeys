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

  TimerNode.h: creates a Node object which runs its own thread to generate
  timestamps.
*/

#pragma once

#include "Node.h"

class TimerNode : public Node<timestamp_type>, public juce::Thread {
#if 0
    // ***** Class to implement the Juce thread *****
private:
    class TimerThread : public juce::Thread {
    public:
        TimerThread(Timer *timer, timestamp_type starting_timestamp)
        : timer_(timer), startingTimestamp_(starting_timestamp) {}
        
        ~TimerThread() {}
        
        void run() {
            timer_->runLoop(startingTimestamp_);
        }
        
    private:
        juce::Timer *timer_;
        timestamp_type startingTimestamp_;
    }
#endif
public:
	// ***** Constructor *****
	
	TimerNode(capacity_type capacity, unsigned long long interval_micros, juce::String threadName = "Timer")
		: Node<timestamp_type>(capacity), juce::Thread(threadName), intervalMicros_(interval_micros), isRunning_(false) {}
	
	// ***** Destructor *****
	
	~TimerNode() {
		stop();
	}

	// ***** Timing Methods *****
	//
	// These functions start and stop the timer without deleting the data it has generated.
	
	void start(timestamp_type where = 0);
	void stop();
	
	// Allow viewing of the interval as a timestamp type.  Allow viewing or setting it as an integer number
	// of microseconds.  Don't set it directly as a timestamp_type: if timestamp_type is floating point, it
	// gives a misleading impression of the behavior of the timer when the interval doesn't round to an even
	// number of microseconds.
	
	timestamp_diff_type interval() { return microseconds_to_timestamp(intervalMicros_); }
	unsigned long long& interval_micros() { return intervalMicros_; }
	
	// The loop runs in its own thread and feeds new ticks to the data source at regular intervals.  Give it
	// the interval length in microseconds and the timestamp of the first tick.
	// static void staticRunLoop(Timer* timer, timestamp_type starting_timestamp) { timer->runLoop(starting_timestamp); }
	void run();

private:	
	//TimerThread *thread_;
	unsigned long long intervalMicros_;
	bool isRunning_;
    timestamp_type startingTimestamp_;
};
