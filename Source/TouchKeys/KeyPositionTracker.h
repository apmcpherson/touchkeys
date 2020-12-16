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
 
  KeyPositionTracker.h: parses continuous key position and detects the
  state of the key.
*/

#pragma once

#include "../Utility/Node.h"
#include "../Utility/Accumulator.h"
#include "PianoTypes.h"
#include <set>

// Three states of idle detector
enum {
	kPositionTrackerStateUnknown = 0,
    kPositionTrackerStatePartialPressAwaitingMax,
    kPositionTrackerStatePartialPressFoundMax,
    kPositionTrackerStatePressInProgress,
    kPositionTrackerStateDown,
    kPositionTrackerStateReleaseInProgress,
    kPositionTrackerStateReleaseFinished
};

// Constants for key state detection
const key_position kPositionTrackerPressPosition = scale_key_position(0.75);
const key_position kPositionTrackerPressHysteresis = scale_key_position(0.05);
const key_position kPositionTrackerMinMaxSpacingThreshold = scale_key_position(0.02);
const key_position kPositionTrackerFirstMaxThreshold = scale_key_position(0.075);
const key_position kPositionTrackerReleaseFinishPosition = scale_key_position(0.2);

// How far back to search at the beginning to find the real start or release of a key press
const int kPositionTrackerSamplesToSearchForStartLocation = 50;
const int kPositionTrackerSamplesToSearchBeyondStartLocation = 20;
const int kPositionTrackerSamplesToSearchForReleaseLocation = 100;
const int kPositionTrackerSamplesToAverageForStartVelocity = 3;
const key_velocity kPositionTrackerStartVelocityThreshold = scale_key_velocity(0.5);
const key_velocity kPositionTrackerStartVelocitySpikeThreshold = scale_key_velocity(2.5);
const key_velocity kPositionTrackerReleaseVelocityThreshold = scale_key_velocity(-0.2);

// Constants for feature calculations. The first one is the approximate location of the escapement
// (empirically measured on one piano, so only approximate), used for velocity calculations
const key_position kPositionTrackerDefaultPositionForPressVelocityCalculation = scale_key_position(0.65);
const key_position kPositionTrackerDefaultPositionForReleaseVelocityCalculation = scale_key_position(0.5);
const key_position kPositionTrackerPositionThresholdForPercussivenessCalculation = scale_key_position(0.4);
const int kPositionTrackerSamplesNeededForPressVelocityAfterEscapement = 1;
const int kPositionTrackerSamplesNeededForReleaseVelocityAfterEscapement = 1;

// KeyPositionTrackerNotification
//
// This class contains information on the notifications sent and stored by
// KeyPositionTracker. Includes state changes and feature available.

class KeyPositionTrackerNotification {
public:
    enum {
        kNotificationTypeStateChange = 1,
        kNotificationTypeFeatureAvailableVelocity,
        kNotificationTypeFeatureAvailableReleaseVelocity,
        kNotificationTypeFeatureAvailablePercussiveness,
        kNotificationTypeNewMinimum,
        kNotificationTypeNewMaximum
    };
    
    enum {
        kFeaturesNone = 0,
        kFeaturePressVelocity = 0x0001,
        kFeatureReleaseVelocity = 0x0002,
        kFeaturePercussiveness = 0x0004
    };
    
    int type;
    int state;
    int features;
};

// KeyPositionTracker
//
// This class implements a state machine for a key that is currently active (not idle),
// using continuous key position data to determine the location and parameters of key
// presses and other events. It includes management of partial press patterns and detection
// of percussiveness as well as velocity features.
//
// This class is triggered by new data points in the key position buffer. Its output is
// a series of state changes which indicate what the key is doing.

class KeyPositionTracker : public Node<KeyPositionTrackerNotification> {
public:
    typedef Node<key_position>::size_type key_buffer_index;
    //typedef void (*KeyActionFunction)(KeyPositionTracker *object, void *userData);
    
    // Simple class to hold index/position/timestamp triads
    class Event {
    public:
        Event() : index(0), position(missing_value<key_position>::missing()),
        timestamp(missing_value<timestamp_type>::missing()) {}
        
        Event(key_buffer_index i, key_position p, timestamp_type t)
        : index(i), position(p), timestamp(t) {}
        
        Event(const Event& obj)
        : index(obj.index), position(obj.position), timestamp(obj.timestamp) {}
        
        Event& operator=(const Event& obj) {
            index = obj.index;
            position = obj.position;
            timestamp = obj.timestamp;
            return *this;
        }
        
        key_buffer_index index;
        key_position position;
        timestamp_type timestamp;
    };
    
    // Collection of features related to whether a key is percussively played or not
    struct PercussivenessFeatures {
        float percussiveness;                   // Calculated single feature based on everything below
        Event velocitySpikeMaximum;             // Maximum and minimum points of the initial
        Event velocitySpikeMinimum;             // velocity spike on a percussive press
        timestamp_type timeFromStartToSpike;    // How long it took to reach the velocity spike
        key_velocity areaPrecedingSpike;        // Total sum of velocity values from start to max
        key_velocity areaFollowingSpike;        // Total sum of velocity values from max to min
    };
    
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	KeyPositionTracker(capacity_type capacity, Node<key_position>& keyBuffer);
	
	// Copy constructor
	//KeyPositionTracker(KeyPositionTracker const& obj);
	
	// ***** State Access *****
	
    // Whether this object is currently tracking states
    bool engaged() {
        return engaged_;
    }
    
	// Return the current state (unknown if nothing is in the buffer)
	int currentState() {
        return currentState_;
    }
    
    // Information about important recent points
    Event currentMax() {
        return Event(currentMaxIndex_, currentMaxPosition_, currentMaxTimestamp_);
    }
    Event currentMin() {
        return Event(currentMinIndex_, currentMinPosition_, currentMinTimestamp_);
    }
    Event pressStart() {
        return Event(startIndex_, startPosition_, startTimestamp_);
    }
    Event pressFinish() {
        return Event(pressIndex_, pressPosition_, pressTimestamp_);
    }
    Event releaseStart() {
        return Event(releaseBeginIndex_, releaseBeginPosition_, releaseBeginTimestamp_);
    }
    Event releaseFinish() {
        return Event(releaseEndIndex_, releaseEndPosition_, releaseEndTimestamp_);
    }
    
    // ***** Key Press Features *****
    
    // Velocity for onset and release. The values without an argument use the stored
    // current escapement point (which is also used for notification of availability).
    std::pair<timestamp_type, key_velocity> pressVelocity();
    std::pair<timestamp_type, key_velocity> releaseVelocity();
    
    std::pair<timestamp_type, key_velocity> pressVelocity(key_position escapementPosition);
    std::pair<timestamp_type, key_velocity> releaseVelocity(key_position returnPosition);
    
    // Set the threshold where we look for press velocity calculations. It
    // can be anything up to the press position threshold on the upward side
    // and anything down to the final release position on the downward side.
    void setPressVelocityEscapementPosition(key_position pos) {
        if(pos > kPositionTrackerPressPosition + kPositionTrackerPressHysteresis)
            pressVelocityEscapementPosition_ = kPositionTrackerPressPosition + kPositionTrackerPressHysteresis;
        else
            pressVelocityEscapementPosition_ = pos;
    }
    void setReleaseVelocityEscapementPosition(key_position pos) {
        if(pos < kPositionTrackerReleaseFinishPosition)
            releaseVelocityEscapementPosition_ = kPositionTrackerReleaseFinishPosition;
        else
            releaseVelocityEscapementPosition_ = pos;
    }
    
    
    // Percussiveness (struck vs. pressed keys)
    PercussivenessFeatures pressPercussiveness();
    
	// ***** Modifiers *****
    
    // Register for updates from the key positon buffer
    void engage();
    
    // Unregister for updates from the key position buffer
    void disengage();
	
    // Reset the state back initial values
	void reset();
	
	// ***** Evaluator *****
	
    // This method receives triggers whenever a new sample enters the buffer. It updates
    // the state depending on the profile of the key position.
	void triggerReceived(TriggerSource* who, timestamp_type timestamp);
	
private:
    // ***** Internal Helper Methods *****
    
    // Change the current state
    void changeState(int newState, timestamp_type timestamp);
    
    // Insert a new feature notification
    void notifyFeature(int notificationType, timestamp_type timestamp);
    
    // Work backwards in the key position buffer to find the start/release of a press
    void findKeyPressStart(timestamp_type timestamp);
    void findKeyReleaseStart(timestamp_type timestamp);
    
    // Generic method to find the most recent crossing of a given point
    key_buffer_index findMostRecentKeyPositionCrossing(key_position threshold, bool greaterThan, int maxDistance);
    
    // Look for the crossing of the release velocity threshold to prepare to send the feature
    void prepareReleaseVelocityFeature(KeyPositionTracker::key_buffer_index mostRecentIndex, timestamp_type timestamp);
    
	// ***** Member Variables *****
	
	Node<key_position>& keyBuffer_;		// Raw key position data
    bool engaged_;                      // Whether we're actively listening to incoming updates
    int currentState_;                  // Our current state
    int currentlyAvailableFeatures_;    // Which features can be calculated for the current press
    
    // Position tracking information for significant points (minima and maxima)
    key_position startPosition_;                                // Position of where the key press started
    timestamp_type startTimestamp_;                             // Timestamp of where the key press started
    key_buffer_index startIndex_;                               // Index in the buffer where the start occurred
    key_position pressPosition_;                                // Position of where the key is fully pressed
    timestamp_type pressTimestamp_;                             // Timestamp of where the key is fully pressed
    key_buffer_index pressIndex_;                               // Index in the buffer where the press occurred
    key_position releaseBeginPosition_;                         // Position of where the key release began
    timestamp_type releaseBeginTimestamp_;                      // Timestamp of where the key release began
    key_buffer_index releaseBeginIndex_;                        // Index in the buffer of where the key release began
    key_position releaseEndPosition_;                           // Position of where the key release ended
    timestamp_type releaseEndTimestamp_;                        // Timestamp of where the key release ended
    key_buffer_index releaseEndIndex_;                          // Index in the buffer of where the key release ended
    key_position currentMinPosition_, currentMaxPosition_;      // Running min and max key position
    timestamp_type currentMinTimestamp_, currentMaxTimestamp_;  // Times for the above positions
    key_buffer_index currentMinIndex_, currentMaxIndex_;        // Indices in the buffer for the recent min/max
    key_position lastMinMaxPosition_;                           // Position of the last significant point
    
    // Persistent parameters relating to feature calculation
    key_position pressVelocityEscapementPosition_;              // Position at which onset velocity is calculated
    key_position releaseVelocityEscapementPosition_;            // Position at which release velocity is calculate
    key_buffer_index pressVelocityAvailableIndex_;              // When we can calculate press velocity
    key_buffer_index releaseVelocityAvailableIndex_;            // When we can calculate release velocity
    bool releaseVelocityWaitingForThresholdCross_;              // Set to true if we need to look for release escapement cross
    key_buffer_index percussivenessAvailableIndex_;             // When we can calculate percussiveness features
    
    /*
    typedef struct {
		int runningSum;						// sum of last N points (i.e. mean * N)
		int runningSumMaxLength;			// the value of N above
		int runningSumCurrentLength;		// How many values are actually part of the sum right now (transient condition)
		int startValuesSum;					// sum of the last N start values (to calculate returning quiescent position)
		int startValuesSumMaxLength;
		int startValuesSumCurrentLength;
		
		int maxVariation;					// The maximum deviation from mean of the last group of samples
		int flatCounter;					// how many successive samples have been "flat" (minimal change)
		int currentStartValue;				// values and positions of several key points for active keys
		int currentStartPosition;
		int currentMinValue;
		int currentMinPosition;
		int currentMaxValue;
		int currentMaxPosition;
		int lastKeyPointValue;				// the value of the last important point {start, max, min}
		
		deque<keyPointHistory> recentKeyPoints; // the minima and maxima since the key started
		bool sentPercussiveMidiOn;			// HACK: whether we've sent the percussive MIDI event
		
		int pressValue;						// the value at the maximum corresponding to the end of the key press motion
		int pressPosition;					// the location in the buffer of this event (note: not the timestamp)
		int releaseValue;					// the value the key held right before release
		int releasePosition;				// the location in the buffer of the release corner
	} keyParameters;
	*/
};
