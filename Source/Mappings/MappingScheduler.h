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

  MappingScheduler.h: implements a thread in which mapping actions are
  performed. Each Mapping object implements a triggerReceived() method
  which is called by the hardware I/O thread. This method should do a
  minimal amount of work but pass the real work off to the performMapping()
  method which is called by the MappingScheduler thread. The scheduler
  also allows mapping calls to be performed in the absence of received data,
  for example to cause a parameter to ramp down over time if no touch data
  is received.
*/

#pragma once

#undef DEBUG_MAPPING_SCHEDULER_STATISTICS

#include "Mapping.h"
#include <JuceHeader.h>
#include <iostream>
#include <list>

/*
 * LockFreeQueue
 *
 * Placeholder implementation for a ring buffer. Will have issues with
 * instruction reordering, but should serve the purpose for testing for now.
 */

template <typename T>
struct LockFreeQueue
{
    LockFreeQueue()
    {
        list.push_back(T());
        iHead = list.begin();
        iTail = list.end();
    }
    
    void Produce(const T& t)
    {
        list.push_back(t);
        iTail = list.end();
        list.erase(list.begin(), iHead);
    }
    
    bool Consume(T& t)
    {
        typename TList::iterator iNext = iHead;
        ++iNext;
        if (iNext != iTail)
        {
            iHead = iNext;
            t = *iHead;
            return true;
        }
        return false;
    }
    
    T Consume()
    {
        T tmp;
        while (!Consume(tmp))
        {
            ;
        }
        return tmp;
    }
    
#ifdef DEBUG_MAPPING_SCHEDULER_STATISTICS
    unsigned long size()
    {
        return list.size();
    }
#endif
    
private:
    typedef std::list<T> TList;
    TList list;
    typename std::list<T>::iterator iHead, iTail;
};

/*
 * MappingScheduler
 *
 * This class manages the timing and execution of Mapping objects. It performs a function
 * similar to Scheduler but optimized specifically for running the performMapping() method
 * of objects inheriting from Mapping. It maintains facilities to run mappings either now
 * or in the future, including the ability to preempt future mapping calls with more immediate
 * ones.
 */

class MappingScheduler : public juce::Thread {
private:
    static constexpr timestamp_diff_type kAllowableAdvanceExecutionTime = milliseconds_to_timestamp( 1.0 );

    enum {
        kActionUnknown = 0,
        kActionRegister,
        kActionPerformMapping,
        kActionUnschedule,
        kActionUnregister,
        kActionUnregisterAndDelete
    };
    
    struct MappingAction {
    public:
        MappingAction() : who(0), counter(0), action(kActionUnknown) {}
        MappingAction(Mapping *x, unsigned long y, int z) :
          who(x), counter(y), action(z) {}
        
        Mapping *who;
        unsigned long counter;
        int action;
    };
    
public:
	// ***** Constructor *****
	//
	// Note: This class is not copy-constructable.
	
	MappingScheduler(PianoKeyboard& keyboard, juce::String threadName = "MappingScheduler");
	// ***** Destructor *****
	
	~MappingScheduler();
	
	// ***** Thread Methods *****
	//
	// These start and stop the thread that handles the scheduling of events.
	
	void start();
	void stop();
	
	bool isRunning() { return isRunning_; }
	
    // The main Juce::Thread run loop
	void run();
    
	// ***** Event Management Methods *****
	//
	// This interface provides the ability to schedule and unschedule events for
	// current or future times.
    
    void registerMapping(Mapping *who);
    
    void scheduleNow(Mapping *who);
    void scheduleLater(Mapping *who, timestamp_type timestamp);
    
    void unschedule(Mapping *who);
    
    void unregisterMapping(Mapping *who);
    void unregisterAndDelete(Mapping *who);
    
private:
    // ***** Private Methods *****
    void performAction(MappingAction const& mappingAction);
    
    // Reference to the main PianoKeyboard object which holds the master timestamp
    PianoKeyboard& keyboard_;
    
	// These variables keep track of the status of the separate thread running the events
    juce::CriticalSection actionsInsertionMutex_;
    juce::CriticalSection actionsLaterMutex_;
    
    juce::WaitableEvent waitableEvent_;
	bool isRunning_;
    
    // This counter keeps track of the sequence of insertions and executions
    // of mappings. It is incremented whenever the scheduler finishes the "now"
    // set of actions, and can be used to figure out whether an event has been
    // duplicated or preempted.
    unsigned long counter_;
    std::map<Mapping*, unsigned long> countersForMappings_;
    
    // These variables hold a ring buffer of actions to happen as soon as possible and a
    // lock-synchronized collection of events that happen at later timestamps
    LockFreeQueue<MappingAction> actionsNow_;
    std::multimap<timestamp_type, MappingAction> actionsLater_;
    
#ifdef DEBUG_MAPPING_SCHEDULER_STATISTICS
    timestamp_type lastDebugStatisticsTimestamp_;
    
    // Debugging method to indicate what is in the queue
    void printDebugStatistics();
#endif
};
