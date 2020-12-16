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
 
  PianoKey.h: handles the operation of a single key on the keyboard,
  including fusing touch and MIDI data. Also has hooks for continuous
  key angle.
*/

#pragma once

#include "../Utility/Node.h"
#include "PianoTypes.h"
#include "KeyIdleDetector.h"
#include "KeyPositionTracker.h"
#include "KeyTouchFrame.h"
#include "../Utility/Scheduler.h"
#include "../Utility/IIRFilter.h"
#include <set>
#include <map>
#include <list>

const unsigned int kPianoKeyStateBufferLength = 20;	// How many previous states to save
const unsigned int kPianoKeyIdleBufferLength = 10;  // How many idle/active transitions to save
const unsigned int kPianoKeyPositionTrackerBufferLength = 30; // How many state histories to save
const key_position kPianoKeyDefaultIdleActivityThreshold = scale_key_position(.020);
const key_position kPianoKeyDefaultIdlePositionThreshold = scale_key_position(.05);
const int kPianoKeyDefaultIdleCounter = 20;
const timestamp_diff_type kPianoKeyDefaultTouchTimeoutInterval = microseconds_to_timestamp(0); // was 20000
const timestamp_diff_type kPianoKeyGuiUpdateInterval = microseconds_to_timestamp(15000); // How frequently to update the position display

// Possible key states
enum {
	kKeyStateToBeInitialized = -1,
	kKeyStateUnknown = 0,
	kKeyStateDisabled,
	kKeyStateIdle,
	kKeyStateActive
};

// Possible touch events
enum {
	kTouchEventIdle = 0,
	kTouchEventAdd,
	kTouchEventRemove
};

typedef int key_state;

class PianoKeyboard;
class MidiKeyboardSegment;

/*
 * PianoKey
 *
 * This class holds the buffer and state information for a single piano key,
 * with methods to manage its status.
 */

class PianoKey : public TriggerDestination {
private:
	// ***** Internal Classes *****
	
	// Data on key touch events: what, when and where
	struct KeyTouchEvent {
		int type;
		timestamp_type timestamp;
		KeyTouchFrame frame;
	};
	
public:
	// ***** Constructors *****
	
	PianoKey(PianoKeyboard& keyboard, int noteNumber, int bufferLength);
	//PianoKey(PianoKey const& obj);
    
    // ***** Destructor *****
    
    ~PianoKey();
    		
	// ***** Access Methods *****
	
	Node<key_position>& buffer() { return positionBuffer_; }
	
	// ***** Control Methods *****
	//
	// Force changes in the key state (e.g. to resolve stuck notes)
	
	// Enable or disable a key from generating events
	void disable();
	void enable();
	
	// Force the key to the Idle state, provided it is enabled
	void forceIdle();
	
	// Clear any previous state, go back to initial state
	void reset();
	
	void insertSample(key_position pos, timestamp_type ts);
	
	// ***** Trigger Methods *****
	//
	// This will be called by positionBuffer_ on each new sample.  Examine each sample to see
	// whether the key is idle or not.
	
	void triggerReceived(TriggerSource* who, timestamp_type timestamp);
	
	// ***** MIDI Methods *****
	//
	// If MIDI input is used to control this note, the controller should call these functions
	
	void midiNoteOn(MidiKeyboardSegment *who, int velocity, int channel, timestamp_type timestamp);
	void midiNoteOff(MidiKeyboardSegment *who, timestamp_type timestamp);
	void midiAftertouch(MidiKeyboardSegment *who, int value, timestamp_type timestamp);
	
	bool midiNoteIsOn() { return midiNoteIsOn_; }
	int midiVelocity() { return midiVelocity_; }
	int midiChannel() { return midiChannel_; }
	void changeMidiChannel(int newChannel) { midiChannel_ = newChannel; }
    int midiOutputPort() { return midiOutputPort_; }
    void changeMidiOutputPort(int newPort) { midiOutputPort_ = newPort; }
	
	// ***** Touch Methods *****
	//
	// If touchkeys are used, the controller uses these functions to provide data
	// touchInsertFrame() implies touch is active if not already.
	
	void touchInsertFrame(KeyTouchFrame& newFrame, timestamp_type timestamp);
	void touchOff(timestamp_type timestamp);
	bool touchIsActive() { return touchIsActive_; }
    void setTouchSensorsPresent(bool present) { touchSensorsArePresent_ = present; }
	
	// This function is called on a timer when the key receives MIDI data before touch data
	// and wants to wait to integrate the two.  If the touch data doesn't materialize, this function
	// is called by the scheduler.
	timestamp_type touchTimedOut();
	
private:
	// ***** MIDI Methods (private) *****
	
	// This method does the real work of midiNoteOn(), and might be called from it directly
	// or after a delay.
	void midiNoteOnHelper(MidiKeyboardSegment *who);
	
	// ***** Touch Methods (private) *****
	
	std::pair<float, std::list<int> > touchMatchClosestPoints(const float* oldPoints, const float *newPoints, float count,
														 int oldIndex, std::set<int>& availableNewPoints, float currentTotalDistance); 
	void touchAdd(const KeyTouchFrame& frame, int index, timestamp_type timestamp);
	void touchRemove(const KeyTouchFrame& frame, int idRemoved, int remainingCount, timestamp_type timestamp);
	void touchMultiFingerGestures(const KeyTouchFrame& lastFrame, const KeyTouchFrame& newFrame, timestamp_type timestamp);
	
	// ***** State Machine Methods *****
	//
	// This class maintains a current state that determines its response to the incoming
	// key position data.
	
	void changeState(key_state newState);
	void changeState(key_state newState, timestamp_type timestamp);	
	
	void terminateActivity();
	
	// ***** Member Variables *****
	
	// Reference back to the keyboard which centralizes control
	PianoKeyboard& keyboard_;
	
	// Identity of the key (MIDI note number)
	int noteNumber_;
	
	// --- Data related to MIDI ---
	
	bool midiNoteIsOn_;					// Whether this note is currently active from MIDI
	int midiChannel_;					// MIDI channel currently associated with this note
    int midiOutputPort_;                // Which port MIDI output for this note goes to
	int midiVelocity_;					// Velocity of last MIDI onset
	Node<int> midiAftertouch_;			// Aftertouch history on this note, if any
	
	// Timestamps for the most recent MIDI note on and note off events
	timestamp_type midiOnTimestamp_, midiOffTimestamp_;
	
	// --- Data related to continuous key position ---

	Node<key_position> positionBuffer_;     // Buffer that holds the key positions
	KeyIdleDetector idleDetector_;          // Detector for whether the key is still or moving
    KeyPositionTracker positionTracker_;    // Object to track the various active states of the key
    timestamp_type timeOfLastGuiUpdate_;    // How long it's been since the last key position GUI call
    timestamp_type timeOfLastDebugPrint_;   // TESTING
    
	Node<key_state> stateBuffer_;		// State history
	key_state state_;					// Current state of the key (see enum above)
	juce::CriticalSection stateMutex_;		// Use this to synchronize changes of state
    
    //IIRFilterNode<key_position> testFilter_;    // Filter the raw key position data, for testing
	
	// --- Data related to surface touches ---

    bool touchSensorsArePresent_;                   // Whether touch sensitivity exists on this key
	bool touchIsActive_;							// Whether the user is currently touching the key
	Node<KeyTouchFrame> touchBuffer_;				// Buffer that holds touchkey frames
	std::multimap<int, KeyTouchEvent> touchEvents_;	// Mapping from touch number to event
	bool touchIsWaiting_;							// Whether we're waiting for a touch to occur
    MidiKeyboardSegment *touchWaitingSource_;  // Who we're waiting from a touch for
	timestamp_type touchWaitingTimestamp_;			// When the timeout will occur
	timestamp_diff_type touchTimeoutInterval_;		// How long to wait for a touch before timing out
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKey)
};
