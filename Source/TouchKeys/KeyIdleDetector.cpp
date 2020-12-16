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
 
  KeyIdleDetector.cpp: uses continuous key position to detect whether a key
  is idle or active; active keys will have more detailed tracking applied
  to their position, so detecting idle keys saves processing.
*/

#include "KeyIdleDetector.h"

// Default constructor
KeyIdleDetector::KeyIdleDetector(capacity_type capacity, Node<key_position>& keyBuffer, key_position positionThreshold, 
								 key_position activityThreshold, int counterThreshold)
: Node<int>(capacity), keyBuffer_(keyBuffer), accumulator_(kKeyIdleNumSamples+1, keyBuffer),
  keyIdleThreshold_(kDefaultKeyIdleThreshold), activityThreshold_(activityThreshold), positionThreshold_(positionThreshold),
  numberOfFramesWithoutActivity_(0), noActivityCounterThreshold_(counterThreshold),
  idleState_(kIdleDetectorUnknown)
{
	// Register to receive messages from the accumulator each time it gets a new sample
	  //std::cout << "Registering IdleDetector\n";
	  
	  registerForTrigger(&accumulator_);
	  
	//  std::cout << "IdleDetector: this_source = " << (TriggerSource*)this << " this_dest = " << (TriggerDestination*)this << " accumulator = " << &accumulator_ << std::endl;
}

// Copy constructor
/*KeyIdleDetector::KeyIdleDetector(KeyIdleDetector const& obj)
  : Node<int>(obj), keyBuffer_(obj.keyBuffer_), accumulator_(obj.accumulator_), idleState_(obj.idleState_), 
    activityThreshold_(obj.activityThreshold_), positionThreshold_(obj.positionThreshold_),
    numberOfFramesWithoutActivity_(obj.numberOfFramesWithoutActivity_),
    keyIdleThreshold_(obj.keyIdleThreshold_), noActivityCounterThreshold_(obj.noActivityCounterThreshold_) {
	registerForTrigger(&accumulator_);
}*/

// Clear current state and reset to unknown idle state.
void KeyIdleDetector::clear() {
	Node<int>::clear();
	idleState_ = kIdleDetectorUnknown;
	numberOfFramesWithoutActivity_ = 0;
}

// Evaluator function.  Find the maximum deviation from average of the key motion.

void KeyIdleDetector::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
	//std::cout << "KeyIdleDetector::triggerReceived\n";

	if(who != &accumulator_)
		return;

    key_position currentKeyPosition = keyBuffer_.latest();
    std::pair<int, key_position> currentAccumulator = accumulator_.latest();
    
    // Check that we have enough samples
    if(currentAccumulator.first < kKeyIdleNumSamples)
        return;
    
    // Behavior depends on whether we were idle or not before (or in unknown state)
    if(idleState_ == kIdleDetectorIdle) {
        // If idle right now, don't do anything if the key position is below a threshold
        if(currentKeyPosition < keyIdleThreshold_)
            return;

        // If average is below a second, slightly higher threshold, stay idle
        key_position averageValue = currentAccumulator.second / (key_position)currentAccumulator.first;
        if(averageValue < keyIdleThreshold_ * 2)
            return;
        
        // Go active, notifying any listeners
        idleState_ = kIdleDetectorActive;
        insert(kIdleDetectorActive, timestamp);
    }
    else { // Active or unknown
        // Rule out any cases that would immediately take the key active
        key_position averageValue = currentAccumulator.second / (key_position)currentAccumulator.first;
        if(averageValue >= keyIdleThreshold_ * 2) {
            numberOfFramesWithoutActivity_ = 0;
            return;
        }
        
#if 0
        key_position maxDeviation = 0;
        size_type endIndex = keyBuffer_.endIndex();
        // Find and return the maximum deviation from the average
        for(int i = endIndex - kKeyIdleNumSamples; i < endIndex; i++) {
            key_position diff = key_abs(keyBuffer_[i] - averageValue);
            if(diff > maxDeviation)
                maxDeviation = diff;
        }
#endif
        key_position averageDeviation = 0;
        size_type endIndex = keyBuffer_.endIndex();
        // Find and return the average deviation from mean
        for(int i = endIndex - kKeyIdleNumSamples; i < endIndex; i++) {
            averageDeviation += key_abs(keyBuffer_[i] - averageValue);
        }
        averageDeviation /= kKeyIdleNumSamples;
        
        //std::cout << "averageDeviation = " << averageDeviation << " counter = " << numberOfFramesWithoutActivity_ << std::endl;
        
        if(averageDeviation < activityThreshold_) {
            // Key registers as "flat".  Check if it has stayed that way for long enough, and with a position close enough
            // to resting position, to change the state back to Idle.
            
            numberOfFramesWithoutActivity_++;
            if(numberOfFramesWithoutActivity_ >= noActivityCounterThreshold_) {
                idleState_ = kIdleDetectorIdle;
                insert(kIdleDetectorIdle, timestamp);
            }
        }
        else
            numberOfFramesWithoutActivity_ = 0;
    }

#if 0 /* Old idle detection */
	
	//std::cout << "KeyIdleDetector::triggerReceived2\n";
	
	std::pair<int, key_position> current = accumulator_.latest();
	
	if(current.first < kKeyIdleNumSamples)
		return;

	// Find the average value
	key_position averageValue = current.second / (key_position)current.first;
	key_position maxDeviation = 0;
	
	size_type endIndex = keyBuffer_.endIndex();
	// Find and return the maximum deviation from the average
	for(int i = endIndex - kKeyIdleNumSamples; i < endIndex; i++) {
		key_position diff = key_abs(keyBuffer_[i] - averageValue);
		if(diff > maxDeviation)
			maxDeviation = diff;
	}
	
	// TODO: If we get here, good enough to go to initial activity.  But need to search back to determine start point.
	// Also need to update current start values (see kblisten code).
	
	// Insert a new sample (and hence send a trigger) whenever the maximum deviation crosses the threshold.
	
	if((maxDeviation >= activityThreshold_ || keyBuffer_.latest() >= positionThreshold_) && idleState_ != kIdleDetectorActive) {
		idleState_ = kIdleDetectorActive;
		numberOfFramesWithoutActivity_ = 0;
		insert(kIdleDetectorActive, timestamp);
		//std::cout << "deviation = " << maxDeviation << " average = " << averageValue << std::endl;
	}
	else if(maxDeviation < activityThreshold_ && idleState_ != kIdleDetectorIdle) {
		// Key registers as "flat".  Check if it has stayed that way for long enough, and with a position close enough
		// to resting position, to change the state back to Idle.
		
		numberOfFramesWithoutActivity_++;
		if(numberOfFramesWithoutActivity_ >= noActivityCounterThreshold_ && keyBuffer_.latest() < positionThreshold_) {
			idleState_ = kIdleDetectorIdle;
			insert(kIdleDetectorIdle, timestamp);
			//std::cout << "deviation = " << maxDeviation << " average = " << averageValue << std::endl;
			/*Accumulator<key_position,kKeyIdleNumSamples>::iterator it;
			
			for(it = accumulator_.begin(); it != accumulator_.end(); it++) {
				std::cout << it->first << " " << it->second << std::endl;
			}*/
		}
	}
#endif
}