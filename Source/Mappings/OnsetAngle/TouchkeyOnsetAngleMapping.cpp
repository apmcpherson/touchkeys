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

  TouchkeyOnsetAngleMapping.cpp: per-note mapping for the onset angle mapping,
  which measures the speed of finger motion along the key surface at the
  time of MIDI note onset.
*/

#include "TouchkeyOnsetAngleMapping.h"

#define DEBUG_NOTE_ONSET_MAPPING

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
TouchkeyOnsetAngleMapping::TouchkeyOnsetAngleMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                                         Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: TouchkeyBaseMapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
pastSamples_(kDefaultFilterBufferLength), maxLookbackTime_(kDefaultMaxLookbackTime),
startingPitchBendSemitones_(0), lastPitchBendSemitones_(0),
rampBeginTime_(missing_value<timestamp_type>::missing()), rampLength_(0)
{
}

// Reset state back to defaults
void TouchkeyOnsetAngleMapping::reset() {
    juce::ScopedLock sl(sampleBufferMutex_);
    
    TouchkeyBaseMapping::reset();
    pastSamples_.clear();
}

// Resend all current parameters
void TouchkeyOnsetAngleMapping::resend() {
    // Message is only sent at release; resend may not apply here.
}

// This method receives data from the touch buffer or possibly the continuous key angle (not used here)
void TouchkeyOnsetAngleMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(who == touchBuffer_) {
        juce::ScopedLock sl(sampleBufferMutex_);
        
        // Save the latest frame, even if it is an empty touch (we need to know what happened even
        // after the touch ends since the MIDI off may come later)
        if(!touchBuffer_->empty())
            pastSamples_.insert(touchBuffer_->latest(), touchBuffer_->latestTimestamp());
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type TouchkeyOnsetAngleMapping::performMapping() {
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    
    if(rampLength_ != 0 && currentTimestamp <= rampBeginTime_ + rampLength_) {
        float rampValue = 1.0 - (float)(currentTimestamp - rampBeginTime_)/(float)rampLength_;
        
        lastPitchBendSemitones_ = startingPitchBendSemitones_ * rampValue;
#ifdef DEBUG_NOTE_ONSET_MAPPING
        std::cout << "onset pitch = " << lastPitchBendSemitones_ << '\n';
#endif
        sendPitchBendMessage(lastPitchBendSemitones_);
    }
    else if(lastPitchBendSemitones_ != 0) {
        lastPitchBendSemitones_ = 0;
#ifdef DEBUG_NOTE_ONSET_MAPPING
        std::cout << "onset pitch = " << lastPitchBendSemitones_ << '\n';
#endif
        sendPitchBendMessage(lastPitchBendSemitones_);
    }
        
    // Register for the next update by returning its timestamp
    nextScheduledTimestamp_ = currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

void TouchkeyOnsetAngleMapping::processOnset(timestamp_type timestamp) {

    sampleBufferMutex_.enter();
    
    // Look backwards from the current timestamp to find the velocity
    float calculatedVelocity = missing_value<float>::missing();
    bool touchWasOn = false;
    int sampleCount = 0;
    
#ifdef DEBUG_NOTE_ONSET_MAPPING
    std::cout << "processOnset begin = " << pastSamples_.beginIndex() << " end = " << pastSamples_.endIndex() << "\n";
#endif
    
    if(!pastSamples_.empty()) {
        Node<KeyTouchFrame>::size_type index = pastSamples_.endIndex() - 1;
        Node<KeyTouchFrame>::size_type mostRecentTouchPresentIndex = pastSamples_.endIndex() - 1;
        while(index >= pastSamples_.beginIndex()) {
#ifdef DEBUG_NOTE_ONSET_MAPPING
            std::cout << "examining sample " << index << " with " << pastSamples_[index].count << " touches and time diff " << timestamp - pastSamples_.timestampAt(index) << "\n";
#endif
            if(timestamp - pastSamples_.timestampAt(index) >= maxLookbackTime_)
                break;
            if(pastSamples_[index].count == 0) {
                if(touchWasOn) {
                    // We found a break in the touch; stop here. But don't stop
                    // if the first frames we consider have no touches.
                    if(index < pastSamples_.endIndex() - 1)
                        index++;
                    break;
                }
            }
            else if(!touchWasOn) {
                mostRecentTouchPresentIndex = index;
                touchWasOn = true;
            }
            if(sampleCount++ >= kDefaultMaxLookbackSamples)
                break;
            
            // Can't decrement past 0 in an unsigned type
            if(index == 0)
                break;
            index--;
        }
        
        // If we fell off the beginning of the buffer, back up.
        if(index < pastSamples_.beginIndex())
            index =  pastSamples_.beginIndex();
        
        // Need at least two points for this calculation to work
        timestamp_type endingTimestamp = pastSamples_.timestampAt(mostRecentTouchPresentIndex);
        timestamp_type startingTimestamp = pastSamples_.timestampAt(index);
        if(endingTimestamp - startingTimestamp > 0) {
            float endingPosition = pastSamples_[mostRecentTouchPresentIndex].locs[0];
            float startingPosition = pastSamples_[index].locs[0];
            calculatedVelocity = (endingPosition - startingPosition) / (endingTimestamp - startingTimestamp);
        }
        else { // DEBUG
#ifdef DEBUG_NOTE_ONSET_MAPPING
            std::cout << "Found 0 timestamp difference on key onset (indices " << index << " and " << pastSamples_.endIndex() - 1 << "\n";
#endif
        }
    }
    else {
#ifdef DEBUG_NOTE_ONSET_MAPPING
        std::cout << "Found empty touch buffer on key onset\n";
#endif
    }
    
    sampleBufferMutex_.exit();
    
    if(!missing_value<float>::isMissing(calculatedVelocity)) {
#ifdef DEBUG_NOTE_ONSET_MAPPING
        std::cout << "Found onset velocity " << calculatedVelocity << " on note " << noteNumber_ << std::endl;
#endif
        if(calculatedVelocity > 6.0)
            calculatedVelocity = 6.0;
        
        if(calculatedVelocity > 1.5) {
            startingPitchBendSemitones_ = -1.0 * (calculatedVelocity / 5.0);
            rampLength_ = milliseconds_to_timestamp((50.0 + calculatedVelocity*25.0));
            rampBeginTime_ = keyboard_.schedulerCurrentTimestamp();
        }
        else
            rampLength_ = 0;
        
        sendOnsetAngleMessage(calculatedVelocity);
    }
}

void TouchkeyOnsetAngleMapping::sendOnsetAngleMessage(float onsetAngle, bool force) {
    if(force || !suspended_) {
        keyboard_.sendMessage("/touchkeys/onsetangle", "if", noteNumber_, onsetAngle, LO_ARGS_END);
    }
}

// Send the pitch bend message of a given number of a semitones. Send by OSC,
// which can be mapped to MIDI CC externally
void TouchkeyOnsetAngleMapping::sendPitchBendMessage(float pitchBendSemitones, bool force) {
    if(force || !suspended_)
        keyboard_.sendMessage("/touchkeys/scoop", "if", noteNumber_, pitchBendSemitones, LO_ARGS_END);
}
