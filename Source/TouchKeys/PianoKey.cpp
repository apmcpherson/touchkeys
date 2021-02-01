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
 
  PianoKey.cpp: handles the operation of a single key on the keyboard,
  including fusing touch and MIDI data. Also has hooks for continuous
  key angle.
*/

#include "PianoKey.h"
#include "PianoKeyboard.h"
#include "../Mappings/MappingFactory.h"

#undef TOUCHKEYS_LEGACY_OSC

// Default constructor
PianoKey::PianoKey(PianoKeyboard& keyboard, int noteNumber, int bufferLength) 
: TriggerDestination(), keyboard_(keyboard), noteNumber_(noteNumber),
  midiNoteIsOn_(false), midiChannel_(-1), midiOutputPort_(0), midiVelocity_(0),
  midiAftertouch_(bufferLength), midiOnTimestamp_(0), midiOffTimestamp_(0),
  positionBuffer_(bufferLength),
  idleDetector_(kPianoKeyIdleBufferLength, positionBuffer_, kPianoKeyDefaultIdlePositionThreshold,
              kPianoKeyDefaultIdleActivityThreshold, kPianoKeyDefaultIdleCounter),
  positionTracker_(kPianoKeyPositionTrackerBufferLength, positionBuffer_),
  stateBuffer_(kPianoKeyStateBufferLength), state_(kKeyStateToBeInitialized),
  touchSensorsArePresent_(true), touchIsActive_(false), 
  touchBuffer_(bufferLength), touchIsWaiting_(false), touchWaitingSource_(0),
  touchWaitingTimestamp_(0),
  touchTimeoutInterval_(kPianoKeyDefaultTouchTimeoutInterval)
{    
	enable();
	registerForTrigger(&idleDetector_);
}

// Destructor
PianoKey::~PianoKey() {
    // Remove any mappings we've created
    //keyboard_.removeMapping(noteNumber_);
}


// Disable the key from sending events.  Do this by removing anything that
// listens to its status.
void PianoKey::disable() {
	juce::ScopedLock sl(stateMutex_);
    
	if(state_ == kKeyStateDisabled) {
		return;
	}
	// No longer run the idle comparator.  This ensures that no further state
	// changes take place, and that the idle coefficient is not calculated.
	//idleComparator_.clearTriggers();
	
	terminateActivity();
	changeState(kKeyStateDisabled);	
}

// Start listening for key activity.  This will allow the state to transition to
// idle, and then to active as appropriate.
void PianoKey::enable() {
	juce::ScopedLock sl(stateMutex_);
    
	if(state_ != kKeyStateDisabled) {
		return;
	}
	changeState(kKeyStateUnknown);	
}

// Reset the key to its default state
void PianoKey::reset() {
	juce::ScopedLock sl(stateMutex_);
	
	terminateActivity();		// Stop any current activity
	positionBuffer_.clear();	// Clear all history
	stateBuffer_.clear();
	idleDetector_.clear();
	changeState(kKeyStateUnknown);	// Reinitialize with unknown state
}

// Insert a new sample in the key buffer
void PianoKey::insertSample(key_position pos, timestamp_type ts) {
    positionBuffer_.insert(pos, ts);
    
    if((timestamp_diff_type)ts - (timestamp_diff_type)timeOfLastGuiUpdate_ > kPianoKeyGuiUpdateInterval) {
        timeOfLastGuiUpdate_ = ts;
        if(keyboard_.gui() != nullptr) {
            keyboard_.gui()->setAnalogValueForKey(noteNumber_, pos);
        }
    }
    
    /*if((timestamp_diff_type)ts - (timestamp_diff_type)timeOfLastDebugPrint_ > 1.0) {
        timeOfLastDebugPrint_ = ts;
        key_position kmin = missing_value<key_position>::missing(), kmax = missing_value<key_position>::missing();
        key_position mean = 0;
        int count = 0;
        Node<key_position>::iterator it = positionBuffer_.begin();
        while(it != positionBuffer_.end()) {
            if(missing_value<key_position>::isMissing(*it))
               continue;
            if(missing_value<key_position>::isMissing(kmin) || *it < kmin)
                kmin = *it;
            if(missing_value<key_position>::isMissing(kmax) || *it > kmax)
                kmax = *it;
            mean += *it;
            it++;
            count++;
        }
        mean /= (key_position)count;
        
        key_position var = 0;
        it = positionBuffer_.begin();
        while(it != positionBuffer_.end()) {
            if(missing_value<key_position>::isMissing(*it))
                continue;
            var += (*it - mean)*(*it - mean);
            it++;
        }
        var /= (key_position)count;
        
        std::cout << "Key " << noteNumber_ << " mean " << mean << " var " << var << std::'\n';
    }*/
}

// If a key is active, force it to become idle, stopping any processes that it has created
void PianoKey::forceIdle() {
	juce::ScopedLock sl(stateMutex_);
    
	if(state_ == kKeyStateDisabled || state_ == kKeyStateIdle) {
		return;
	}
	terminateActivity();
	changeState(kKeyStateIdle);
}

// Handle triggers sent when specific conditions are met (called by various objects)

void PianoKey::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
	juce::ScopedLock sl(stateMutex_);
	
	if(who == &idleDetector_) {
		//std::cout << "Key " << noteNumber_ << ": IdleDetector says: " << idleDetector_.latest() << std::endl;
		
		if(idleDetector_.latest() == kIdleDetectorIdle) {
            std::cout << "Key " << noteNumber_ << " --> Idle\n";
            
            keyboard_.tellAllMappingFactoriesKeyMotionIdle(noteNumber_, midiNoteIsOn_, touchIsActive_,
                                                           &touchBuffer_, &positionBuffer_, &positionTracker_);
            
            // Remove any mapping present on this key
            //keyboard_.removeMapping(noteNumber_);
            
            positionTracker_.disengage();
            unregisterForTrigger(&positionTracker_);
			terminateActivity();
			changeState(kKeyStateIdle);
            keyboard_.setKeyLEDColorRGB(noteNumber_, 0, 0, 0);
		}
		else if(idleDetector_.latest() == kIdleDetectorActive && state_ != kKeyStateUnknown) {
            std::cout << "Key " << noteNumber_ << " --> Active\n";
            
            keyboard_.tellAllMappingFactoriesKeyMotionActive(noteNumber_, midiNoteIsOn_, touchIsActive_,
                                                             &touchBuffer_, &positionBuffer_, &positionTracker_);
            
			// Only allow transition to active from a known previous state
			// TODO: set up min/max listener
			// TODO: may want to change the parameters on the idleDetector
			changeState(kKeyStateActive);
            //keyboard_.setKeyLEDColorRGB(noteNumber_, 1.0, 0.0, 0);
            
            // Engage the position tracker that handles specific measurement of key states
            registerForTrigger(&positionTracker_);
            positionTracker_.reset();
            positionTracker_.engage();
            
            // Allocate a new mapping that converts key position gestures to sound
            // control messages. TODO: how do we handle this with the TouchKey data too?
            //MRPMapping *mapping = new MRPMapping(keyboard_, noteNumber_, &touchBuffer_,
            //                                   &positionBuffer_, &positionTracker_);
            //MIDIKeyPositionMapping *mapping = new MIDIKeyPositionMapping(keyboard_, noteNumber_, &touchBuffer_,
            //                                                             &positionBuffer_, &positionTracker_);
            //keyboard_.addMapping(noteNumber_, mapping);
            //mapping->setPercussivenessMIDIChannel(1);
            //mapping->engage();
		}
	}
    else if(who == &positionTracker_ && !positionTracker_.empty()) {
        KeyPositionTrackerNotification notification = positionTracker_.latest();
        
        if(notification.type == KeyPositionTrackerNotification::kNotificationTypeStateChange) {
            int positionTrackerState = notification.state;
            
            KeyPositionTracker::Event recentEvent;
            std::pair<timestamp_type, key_velocity> velocityInfo;
            std::cout << "Key " << noteNumber_ << " --> State " << positionTrackerState << '\n';
            
            switch(positionTrackerState) {
                case kPositionTrackerStatePartialPressAwaitingMax:
                    //keyboard_.setKeyLEDColorRGB(noteNumber_, 1.0, 0.0, 0);
                    recentEvent = positionTracker_.pressStart();
                    std::cout << "  start = (" << recentEvent.index << ", " << recentEvent.position << ", " << recentEvent.timestamp << ")\n";
                    break;
                case kPositionTrackerStatePartialPressFoundMax:
                    //keyboard_.setKeyLEDColorRGB(noteNumber_, 1.0, 0.6, 0);
                    recentEvent = positionTracker_.currentMax();
                    std::cout << "  max = (" << recentEvent.index << ", " << recentEvent.position << ", " << recentEvent.timestamp << ")\n";
                    break;
                case kPositionTrackerStatePressInProgress:                    
                    //keyboard_.setKeyLEDColorRGB(noteNumber_, 0.8, 0.8, 0);
                    velocityInfo = positionTracker_.pressVelocity();
                    std::cout << "  escapement time = " << velocityInfo.first << " velocity = " << velocityInfo.second << '\n';
                    break;
                case kPositionTrackerStateDown:
                    //keyboard_.setKeyLEDColorRGB(noteNumber_, 0, 1.0, 0);
                    recentEvent = positionTracker_.pressStart();
                    std::cout << "  start = (" << recentEvent.index << ", " << recentEvent.position << ", " << recentEvent.timestamp << ")\n";
                    recentEvent = positionTracker_.pressFinish();
                    std::cout << "  finish = (" << recentEvent.index << ", " << recentEvent.position << ", " << recentEvent.timestamp << ")\n";
                    velocityInfo = positionTracker_.pressVelocity();
                    std::cout << "  escapement time = " << velocityInfo.first << " velocity = " << velocityInfo.second << '\n';
                    
                    if(keyboard_.graphGUI() != nullptr) {
                        keyboard_.graphGUI()->setKeyPressStart(positionTracker_.pressStart().position, positionTracker_.pressStart().timestamp);
                        keyboard_.graphGUI()->setKeyPressFinish(positionTracker_.pressFinish().position, positionTracker_.pressFinish().timestamp);
                        keyboard_.graphGUI()->copyKeyDataFromBuffer(positionBuffer_, positionTracker_.pressStart().index - 10,
                                                                    positionBuffer_.endIndex());
                    }
                    break;
                case kPositionTrackerStateReleaseInProgress:
                    //keyboard_.setKeyLEDColorRGB(noteNumber_, 0, 0, 1.0);
                    recentEvent = positionTracker_.releaseStart();
                    std::cout << "  start = (" << recentEvent.index << ", " << recentEvent.position << ", " << recentEvent.timestamp << ")\n";
                    if(keyboard_.graphGUI() != nullptr) {
                        keyboard_.graphGUI()->setKeyReleaseStart(positionTracker_.releaseStart().position, positionTracker_.releaseStart().timestamp);
                        keyboard_.graphGUI()->copyKeyDataFromBuffer(positionBuffer_, positionTracker_.pressStart().index - 10,
                                                                    positionBuffer_.endIndex());
                        //keyboard_.graphGUI()->copyKeyDataFromBuffer(testFilter_, testFilter_.indexNearestTo(positionBuffer_.timestampAt(positionTracker_.pressStart().index - 10)), testFilter_.endIndex());
                    }
                    break;
                case kPositionTrackerStateReleaseFinished:
                    //keyboard_.setKeyLEDColorRGB(noteNumber_, 0.5, 0, 1.0);
                    recentEvent = positionTracker_.releaseFinish();
                    std::cout << "  finish = (" << recentEvent.index << ", " << recentEvent.position << ", " << recentEvent.timestamp << ")\n";
                    if(keyboard_.graphGUI() != nullptr) {
                        keyboard_.graphGUI()->setKeyReleaseStart(positionTracker_.releaseStart().position, positionTracker_.releaseStart().timestamp);
                        keyboard_.graphGUI()->setKeyReleaseFinish(positionTracker_.releaseFinish().position, positionTracker_.releaseFinish().timestamp);
                        keyboard_.graphGUI()->copyKeyDataFromBuffer(positionBuffer_, positionTracker_.pressStart().index - 10,
                                                                    positionBuffer_.endIndex());
                        //keyboard_.graphGUI()->copyKeyDataFromBuffer(testFilter_, testFilter_.indexNearestTo(positionBuffer_.timestampAt(positionTracker_.pressStart().index - 10)), testFilter_.endIndex());
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

// Update the current state

void PianoKey::changeState(key_state newState) {
	if(!positionBuffer_.empty())
		changeState(newState, positionBuffer_.latestTimestamp());
	else
		changeState(newState, 0);
}

void PianoKey::changeState(key_state newState, timestamp_type timestamp) {
	stateBuffer_.insert(newState, timestamp);
	state_ = newState;
}

// Stop any activity that's currently taking place on account of the key motion

void PianoKey::terminateActivity() {
	
}

#pragma mark MIDI Methods
// ***** MIDI Methods *****

// Note On message from associated MIDI keyboard: record channel we should use
// for the duration of this note, as well as the note's velocity
void PianoKey::midiNoteOn(MidiKeyboardSegment *who, int velocity, int channel, timestamp_type timestamp) {
	midiNoteIsOn_ = true;
	midiChannel_ = channel;
	midiVelocity_ = velocity;
	midiOnTimestamp_ = timestamp;

    if(keyboard_.mappingFactory(who) != nullptr)
        keyboard_.mappingFactory(who)->midiNoteOn(noteNumber_, touchIsActive_, (idleDetector_.idleState() == kIdleDetectorActive),
                                               &touchBuffer_, &positionBuffer_, &positionTracker_);

    // Start the note right away unless we either need to wait for a touch or a delay has been enforced
	if((touchIsActive_ && !touchBuffer_.empty()) || !touchSensorsArePresent_ || (touchTimeoutInterval_ == 0)) {
        midiNoteOnHelper(who);
	}
	else {
		// If touch isn't yet active, we might delay the note onset for a short
		// time to wait for it.
		
		// Cases:
		//   (1) Touch was active --> send above messages and tell the MidiController to go ahead
		//      (a) Channel Selection mode: OSC messages will be used to choose a channel; messages
		//          that translate to control changes need to be archived and resent once the channel is known
		//      (b) Polyphonic and other modes: OSC messages used to generate control changes; channel
		//          is already known
		//   (2) Touch not yet active --> schedule a note on for a specified time interval in the future
		//      (a) Touch event arrives on this key before that --> send OSC and call the MidiController,
		//          removing the scheduled future event
		//      (b) No touch event arrives --> future event triggers without touch info, call MidiController
		//          and tell it to use defaults
        touchWaitingSource_ = who;
		touchIsWaiting_ = true;
		touchWaitingTimestamp_ = keyboard_.schedulerCurrentTimestamp() + touchTimeoutInterval_;
		keyboard_.scheduleEvent(this, 
								boost::bind(&PianoKey::touchTimedOut, this),
								touchWaitingTimestamp_);
	}
}

// This private method does the real work of midiNoteOn().  It's separated because under certain
// circumstances, the helper function is called in a delayed manner, after a touch has been received.
void PianoKey::midiNoteOnHelper(MidiKeyboardSegment *who) {
	touchIsWaiting_ = false;
    touchWaitingSource_ = 0;

	if(!touchBuffer_.empty()) {
		const KeyTouchFrame& frame(touchBuffer_.latest());
		int indexOfFirstTouch = 0;
		
		// Find which touch happened first so we can report its location
		for(int i = 0; i < frame.count; i++) {
			if(frame.ids[i] < frame.ids[indexOfFirstTouch])
				indexOfFirstTouch = i;
		}
		
		// Send a message reporting the touch location of the first touch and the
		// current number of touches.  The target (either MidiInputController or external)
		// may use this to change its behavior independently of later changes in touch.
		
		keyboard_.sendMessage("/touchkeys/preonset", "iiiiiiffiffifff",
							  noteNumber_, midiChannel_, midiVelocity_,	// MIDI data
							  frame.count, indexOfFirstTouch,	// General information: how many touches, which was first?
							  frame.ids[0], frame.locs[0], frame.sizes[0], // Specific touch information
							  frame.ids[1], frame.locs[1], frame.sizes[1],
							  frame.ids[2], frame.locs[2], frame.sizes[2],
							  frame.locH, LO_ARGS_END);

		// ----
		// The above function will trigger the callback in MidiInputController, if it is enabled.
		// Therefore, the calls below will take place after MidiInputController has handled its callback.
		// ----

#ifdef TOUCHKEYS_LEGACY_OSC
		// Send move and resize gestures for each active touch
		for(int i = 0; i < frame.count; i++) {
			keyboard_.sendMessage("/touchkeys/move", "iiff", noteNumber_, frame.ids[i],
								  frame.locs[i], frame.horizontal(i), LO_ARGS_END);
			keyboard_.sendMessage("/touchkeys/resize", "iif", noteNumber_, frame.ids[i],
								  frame.sizes[i], LO_ARGS_END);										
		}
		
		// If more than one touch is present, resend any pinch and slide gestures
		// before we start.
		if(frame.count == 2) {
			float newCentroid = (frame.locs[0] + frame.locs[1]) / 2.0;
			float newWidth = frame.locs[1] - frame.locs[0];	
			
			keyboard_.sendMessage("/touchkeys/twofinger/pinch", "iiif",
								  noteNumber_, frame.ids[0], frame.ids[1], newWidth, LO_ARGS_END);
			keyboard_.sendMessage("/touchkeys/twofinger/slide", "iiif",
								  noteNumber_, frame.ids[0], frame.ids[1], newCentroid, LO_ARGS_END);			
		}
		else if(frame.count == 3) {
			float newCentroid = (frame.locs[0] + frame.locs[1] + frame.locs[2]) / 3.0;
			float newWidth = frame.locs[2] - frame.locs[0];
			
			keyboard_.sendMessage("/touchkeys/threefinger/pinch", "iiiif",
								  noteNumber_, frame.ids[0], frame.ids[1], frame.ids[2], newWidth, LO_ARGS_END);
			keyboard_.sendMessage("/touchkeys/threefinger/slide", "iiiif",
								  noteNumber_, frame.ids[0], frame.ids[1], frame.ids[2], newCentroid, LO_ARGS_END);			
		}
#endif
	}
    
    // Before the note starts, inform the mapping factory in case there are default values to be sent out.
    if(keyboard_.mappingFactory(who) != nullptr)
        keyboard_.mappingFactory(who)->noteWillBegin(noteNumber_, midiChannel_, midiVelocity_);
	
	keyboard_.sendMessage("/midi/noteon", "iii", noteNumber_, midiChannel_, midiVelocity_, LO_ARGS_END);
    
    // Update GUI if it is available. TODO: fix the ordering problem for real!
	if(keyboard_.gui() != nullptr && midiNoteIsOn_) {
		keyboard_.gui()->setMidiActive(noteNumber_, true);
	}
}

// Note Off message from associated MIDI keyboard.  Clear all old MIDI state.
void PianoKey::midiNoteOff(MidiKeyboardSegment *who, timestamp_type timestamp) {
	midiNoteIsOn_ = false;
	midiOffTimestamp_ = timestamp;
    
    if(keyboard_.mappingFactory(who) != nullptr)
        keyboard_.mappingFactory(who)->midiNoteOff(noteNumber_, touchIsActive_, (idleDetector_.idleState() == kIdleDetectorActive),
                                               &touchBuffer_, &positionBuffer_, &positionTracker_);
    
	keyboard_.sendMessage("/midi/noteoff", "ii", noteNumber_, midiChannel_, LO_ARGS_END);
    
    midiVelocity_ = 0;
	midiChannel_ = -1;
	midiAftertouch_.clear();
    
    // Update GUI if it is available
	if(keyboard_.gui() != nullptr) {
		keyboard_.gui()->setMidiActive(noteNumber_, false);
	}
}

// Aftertouch (either channel or polyphonic) message from associated MIDI keyboard
void PianoKey::midiAftertouch(MidiKeyboardSegment *who, int value, timestamp_type timestamp) {
	if(!midiNoteIsOn_)
		return;
	midiAftertouch_.insert(value, timestamp);
	
	keyboard_.sendMessage("/midi/aftertouch-poly", "iii", noteNumber_, midiChannel_, value, LO_ARGS_END);
}

#pragma mark Touch Methods
// ***** Touch Methods *****

// Insert a new frame of touchkey data, making any necessary status changes
// (i.e. touch active, possibly changing number of active touches)

void PianoKey::touchInsertFrame(KeyTouchFrame& newFrame, timestamp_type timestamp) {
    if(!touchSensorsArePresent_)
        return;

	// First check if the key was previously inactive.  If so, send a message
	// that the touch has begun
	if(!touchIsActive_) {
		keyboard_.sendMessage("/touchkeys/on", "i", noteNumber_, LO_ARGS_END);
        keyboard_.tellAllMappingFactoriesTouchBegan(noteNumber_, midiNoteIsOn_, (idleDetector_.idleState() == kIdleDetectorActive),
                                                    &touchBuffer_, &positionBuffer_, &positionTracker_);
    }
	
	touchIsActive_ = true;
	
	// If previous touch frames are present on this key, check the preceding
	// frame to see if the state has changed in any important ways
	if(!touchBuffer_.empty()) {
		const KeyTouchFrame& lastFrame(touchBuffer_.latest());
		
		// Next ID is the touch ID that should be used for any new touches.  Scoop this
		// info from the last frame.
		
		newFrame.nextId = lastFrame.nextId;
		
		// Assign ID numbers to each touch.  This is easy if the number of touches
		// from the previous frame to this one stayed the same, somewhat more complex
		// if a touch was added or removed.
		
		if(newFrame.count > lastFrame.count) {
			// One or more points have been added.  Match the new points to the old ones to figure out
			// which points have been added, versus which moved from before.
			
			std::set<int> availableNewPoints;
			for(int i = 0; i < newFrame.count; i++)
				availableNewPoints.insert(i);
			
			std::list<int> ordering(touchMatchClosestPoints(lastFrame.locs, newFrame.locs, 3, 0, availableNewPoints, 0.0).second);
			
			// ordering tells us the index of the new point corresponding to each old index,
			// e.g. {2, 0, 1} --> old point 0 goes to new point 2, old point 1 goes to new point 0, ...
			
			// new points are still in ascending position order, so we use this matching to assign unique IDs
			// and send relevant "add" messages
			
			int counter = 0;
			for( auto it = ordering.begin(); it != ordering.end(); ++it) {
				newFrame.ids[*it] = lastFrame.ids[counter];
				
				if(newFrame.ids[*it] < 0) {
					// Matching to a negative ID means the touch is new
					
					newFrame.ids[*it] = newFrame.nextId++;
					touchAdd(newFrame, *it, timestamp);
				}
				else {
#ifdef TOUCHKEYS_LEGACY_OSC
					// Send "move" messages for the points that have moved
                    if(fabsf(newFrame.locs[*it] - lastFrame.locs[counter]) > 0 /*moveThreshold_*/)
						keyboard_.sendMessage("/touchkeys/move", "iiff", noteNumber_, newFrame.ids[*it],
													 newFrame.locs[*it], newFrame.horizontal(*it), LO_ARGS_END);
					if(fabsf(newFrame.sizes[*it] - lastFrame.sizes[counter]) > 0 /*resizeThreshold_*/)
						keyboard_.sendMessage("/touchkeys/resize", "iif", noteNumber_, newFrame.ids[*it],
													 newFrame.sizes[*it], LO_ARGS_END);
#endif
				}
				
				counter++;
			}			
		}
		else if(newFrame.count < lastFrame.count) {
			// One or more points have been removed.  Match the new points to the old ones to figure out
			// which points have been removed, versus which moved from before.
			
			std::set<int> availableNewPoints;
			for(int i = 0; i < 3; i++)		
				availableNewPoints.insert(i);
			
			std::list<int> ordering(touchMatchClosestPoints(lastFrame.locs, newFrame.locs, 3, 0, availableNewPoints, 0.0).second);
			
			// ordering tells us the index of the new point corresponding to each old index,
			// e.g. {2, 0, 1} --> old point 0 goes to new point 2, old point 1 goes to new point 0, ...
			
			// new points are still in ascending position order, so we use this matching to assign unique IDs
			// and send relevant "add" messages
			
			int counter = 0;
			for( auto it = ordering.begin(); it != ordering.end(); ++it) {
				if(*it < newFrame.count) {
					// Old index {counter} matches a valid new touch
					
					newFrame.ids[*it] = lastFrame.ids[counter];	// Match IDs for currently active touches
					
#ifdef TOUCHKEYS_LEGACY_OSC
					// Send "move" messages for the points that have moved
					if(fabsf(newFrame.locs[*it] - lastFrame.locs[counter]) > 0 /*moveThreshold_*/)
						keyboard_.sendMessage("/touchkeys/move", "iiff", noteNumber_, newFrame.ids[*it],
													 newFrame.locs[*it], newFrame.horizontal(*it), LO_ARGS_END);
					if(fabsf(newFrame.sizes[*it] - lastFrame.sizes[counter]) > 0 /*resizeThreshold_*/)
						keyboard_.sendMessage("/touchkeys/resize", "iif", noteNumber_, newFrame.ids[*it],
													 newFrame.sizes[*it], LO_ARGS_END);
#endif
				}
				else if(lastFrame.ids[counter] >= 0) {
					// Old index {counter} matches an invalid new index, meaning a touch has been removed.
					touchRemove(lastFrame, lastFrame.ids[counter], newFrame.count, timestamp);
				}
				
				counter++;
			}			
		}
		else {
			// Same number of touches as before.  Touches are always stored in increasing order,
			// so we just need to copy these over, maintaining the same ID numbers.
			
			for(int i = 0; i < newFrame.count; i++) {
				newFrame.ids[i] = lastFrame.ids[i];
				
#ifdef TOUCHKEYS_LEGACY_OSC
				// Send "move" messages for the points that have moved
				if(fabsf(newFrame.locs[i] - lastFrame.locs[i]) > 0 /*moveThreshold_*/)
					keyboard_.sendMessage("/touchkeys/move", "iiff", noteNumber_, newFrame.ids[i],
												 newFrame.locs[i], newFrame.horizontal(i), LO_ARGS_END);
				if(fabsf(newFrame.sizes[i] - lastFrame.sizes[i]) > 0 /*resizeThreshold_*/)
					keyboard_.sendMessage("/touchkeys/resize", "iif", noteNumber_, newFrame.ids[i],
												 newFrame.sizes[i], LO_ARGS_END);
#endif
			}
			
			// If the number of touches has stayed the same, look for multi-finger gestures (pinch and slide)
			if(newFrame.count > 1) {
				touchMultiFingerGestures(lastFrame, newFrame, timestamp);
			}
		}
	}
	else {
		// With no previous frame to compare to, assign IDs to each active touch sequentially
		
		newFrame.nextId = 0;
		for(int i = 0; i < newFrame.count; i++) {
			newFrame.ids[i] = newFrame.nextId++;
			touchAdd(newFrame, i, timestamp);
		}
	}
	
	// Add the new touch
	touchBuffer_.insert(newFrame, timestamp);
    
	if(touchIsWaiting_) {
		// If this flag was set, we were waiting for a touch to occur before taking further
		// action.  A timeout will have been scheduled, which we should clear.
		keyboard_.unscheduleEvent(this, touchWaitingTimestamp_);
		
		// Send the queued up MIDI/OSC events
		midiNoteOnHelper(touchWaitingSource_);
	}
	
	// Update GUI if it is available
	if(keyboard_.gui() != nullptr) {
		keyboard_.gui()->setTouchForKey(noteNumber_, newFrame);
	}
}

// This is called when all touch is removed from a key.  Clear out the previous state

void PianoKey::touchOff(timestamp_type timestamp) {
	if(!touchIsActive_ || !touchSensorsArePresent_)
		return;
    
    keyboard_.tellAllMappingFactoriesTouchEnded(noteNumber_, midiNoteIsOn_, (idleDetector_.idleState() == kIdleDetectorActive),
                                               &touchBuffer_, &positionBuffer_, &positionTracker_);
    
	touchEvents_.clear();
	
	// Create a new event that records the timestamp of the idle event
	// and the last frame before it occurred (but only if we have data
	// on at least one frame before idle occurred)
	if(!touchBuffer_.empty()) {
		KeyTouchEvent event = { kTouchEventIdle, timestamp, touchBuffer_.latest() };
		touchEvents_.insert(std::pair<int, KeyTouchEvent>(-1, event));
	}		
	
    // Insert a blank touch frame into the buffer so anyone listening knows the touch has gone off
    KeyTouchFrame emptyFrame;
    emptyFrame.count = 0;
    touchBuffer_.insert(emptyFrame, timestamp);
    
	// Send a message that the touch has ended
	touchIsActive_ = false;
	touchBuffer_.clear();
    keyboard_.sendMessage("/touchkeys/off", "i", noteNumber_, LO_ARGS_END);
	// Update GUI if it is available
	if(keyboard_.gui() != nullptr) {
		keyboard_.gui()->clearTouchForKey(noteNumber_);
	}
}

// This function is called when we time out waiting for a touch on the given note

timestamp_type PianoKey::touchTimedOut() {
	//std::cout << "Touch timed out on note " << noteNumber_ << '\n';
	
	// Do all the things we were planning to do once the touch was received.
	midiNoteOnHelper(touchWaitingSource_);
    
    return 0;
}

// Recursive function for matching old and new frames of touch locations, each with up to (count) points
//
// Example: old points 1-3, new points A-C
//   1A  *2A*  3A
//  *1B*  2B   3B
//   1C   2C  *3C*

std::pair<float, std::list<int> > PianoKey::touchMatchClosestPoints(const float* oldPoints, const float *newPoints, float count,
															   int oldIndex, std::set<int>& availableNewPoints, float currentTotalDistance) {
	if(availableNewPoints.size() == 0)	// Shouldn't happen but prevent an infinite loop
		throw new std::exception;
	
	// End case: only one possible point available
	if(availableNewPoints.size() == 1) {
		int newIndex = *(availableNewPoints.begin());
		
		std::list<int> singleOrder;
		singleOrder.push_front(newIndex);
		
		if(oldPoints[oldIndex] < 0.0 || newPoints[newIndex] < 0.0) {
			//if(verbose_ >= 4)
			//	std::cout << " -> [" << newIndex << "] (" << currentTotalDistance + 100.0 << ")\n";
			
			// Return the distance between the last old point and the only available new point
			return std::pair<float, std::list<int> > (currentTotalDistance + 100.0, singleOrder);			
		}
		else {
			//if(verbose_ >= 4)
			//	std::cout << " -> [" << newIndex << "] (" << currentTotalDistance + (oldPoints[oldIndex] - newPoints[newIndex])*(oldPoints[oldIndex] - newPoints[newIndex]) << ")\n";
			
			// Return the distance between the last old point and the only available new point
			return std::pair<float, std::list<int> > (currentTotalDistance + (oldPoints[oldIndex] - newPoints[newIndex])*(oldPoints[oldIndex] - newPoints[newIndex]), singleOrder);
		}
	}
	
	float minVal = std::numeric_limits<float>::infinity();
	std::set<int> newPointsCopy(availableNewPoints);

	std::list<int> order;
	
	// Go through all available new points
	for( auto it = availableNewPoints.begin(); it != availableNewPoints.end(); ++it) {
		// Temporarily remove (and test) one point and recursively call ourselves
		newPointsCopy.erase(*it);
		
		float dist;
		if(newPoints[*it] >= 0.0 && oldPoints[oldIndex] >= 0.0)
			dist = (oldPoints[oldIndex] - newPoints[*it])*(oldPoints[oldIndex] - newPoints[*it]);
		else
			dist = 100.0;
		
		std::pair<float, std::list<int> > rval = touchMatchClosestPoints(oldPoints, newPoints, count, oldIndex + 1, newPointsCopy,
														  currentTotalDistance + dist);
		
		//if(verbose_ >= 4)
		//	std::cout << "     from " << *it << " got " << rval.first << '\n';
		
		if(rval.first < minVal) {
			minVal = rval.first;
			order = rval.second;
			order.push_front(*it);
		}
		
		newPointsCopy.insert(*it);
	}
	
	/*if(verbose_ >= 4) {
		std::cout << " -> [";
		list<int>::iterator it2;
		
		for(it2 = order.begin(); it2 != order.end(); ++it2)
			std::cout << *it2 << ", ";
		std::cout << "] (" << minVal << ")\n";
	}*/
	
	return std::pair<float, std::list<int> >(minVal, order);
}

// A new touch was added from the last frame to this one

void PianoKey::touchAdd(const KeyTouchFrame& frame, int index, timestamp_type timestamp) {
	KeyTouchEvent event = { kTouchEventAdd, timestamp, frame };
	touchEvents_.insert(std::pair<int, KeyTouchEvent>(frame.ids[index], event));
#ifdef TOUCHKEYS_LEGACY_OSC
	keyboard_.sendMessage("/touchkeys/add", "iiifff", noteNumber_, frame.ids[index], frame.count,
						  frame.locs[index], frame.sizes[index], frame.horizontal(index),
						  LO_ARGS_END);
#endif
}

// A touch was removed from the last frame.  The frame in this case is the last frame containing
// the touch in question (so we can find its ending position later).

void PianoKey::touchRemove(const KeyTouchFrame& frame, int idRemoved, int remainingCount, timestamp_type timestamp) {
	KeyTouchEvent event = { kTouchEventRemove, timestamp, frame };
	touchEvents_.insert(std::pair<int, KeyTouchEvent>(idRemoved, event));
#ifdef TOUCHKEYS_LEGACY_OSC
	keyboard_.sendMessage("/touchkeys/remove", "iii", noteNumber_, idRemoved,
						  remainingCount, LO_ARGS_END);
#endif
}

// Process multi-finger gestures (pinch and slide) based on previous and current frames

void PianoKey::touchMultiFingerGestures(const KeyTouchFrame& lastFrame, const KeyTouchFrame& newFrame, timestamp_type timestamp) {
#ifdef TOUCHKEYS_LEGACY_OSC
	if(newFrame.count == 2 && lastFrame.count == 2) {
		float previousCentroid = (lastFrame.locs[0] + lastFrame.locs[1]) / 2.0;
		float newCentroid = (newFrame.locs[0] + newFrame.locs[1]) / 2.0;
		float previousWidth = lastFrame.locs[1] - lastFrame.locs[0];
		float newWidth = newFrame.locs[1] - newFrame.locs[0];
		
		if(fabsf(newWidth - previousWidth) >= 0 /*pinchThreshold_*/) {
			keyboard_.sendMessage("/touchkeys/twofinger/pinch", "iiif",
									noteNumber_, newFrame.ids[0], newFrame.ids[1], newWidth, LO_ARGS_END);
		}
		if(fabsf(newCentroid - previousCentroid) >= 0 /*slideThreshold_*/) {
			keyboard_.sendMessage("/touchkeys/twofinger/slide", "iiif",
									noteNumber_, newFrame.ids[0], newFrame.ids[1], newCentroid, LO_ARGS_END);
		}
	}
	else if(newFrame.count == 3 && lastFrame.count == 3) {
		float previousCentroid = (lastFrame.locs[0] + lastFrame.locs[1] + lastFrame.locs[2]) / 3.0;
		float newCentroid = (newFrame.locs[0] + newFrame.locs[1] + newFrame.locs[2]) / 3.0;
		float previousWidth = lastFrame.locs[2] - lastFrame.locs[0];
		float newWidth = newFrame.locs[2] - newFrame.locs[0];
		
		if(fabsf(newWidth - previousWidth) >= 0 /*pinchThreshold_*/) {
			keyboard_.sendMessage("/touchkeys/threefinger/pinch", "iiiif",
								  noteNumber_, newFrame.ids[0], newFrame.ids[1], newFrame.ids[2], newWidth, LO_ARGS_END);
		}
		if(fabsf(newCentroid - previousCentroid) >= 0 /*slideThreshold_*/) {
			keyboard_.sendMessage("/touchkeys/threefinger/slide", "iiiif",
								  noteNumber_, newFrame.ids[0], newFrame.ids[1], newFrame.ids[2], newCentroid, LO_ARGS_END);
		}
	}
#endif
}

/*
 * State Machine:
 *
 * Disabled
 *		--> Unknown: (by user)
 *			enable triggers on comparator
 * Unknown
 *		--> Idle: activity <= X, pos < Y
 * Idle
 *		--> Active: activity > X 
 *				start looking for maxes and mins
 *				watch for key down
 * Active
 *		--> Idle: activity <= X, pos < Y
 *				stop looking for maxes and mins
 *		--> Max: found maximum position > Z
 *				calculate features
 * Max:
 *		--> Max: found maximum position greater than before; time from start < T
 *				recalculate features
 *		--> Idle: activity <= X, pos < Y
 * Down:
 *		(just a special case of Max?)
 * Release:
 *		(means the user is no longer playing the key, ignore its motion)
 *		--> Idle: activity <= X, pos < Y
 *
 */