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

  TouchkeyPitchBendMapping.cpp: per-note mapping for the pitch-bend mapping,
  which handles changing pitch based on relative finger motion.
*/

#include "TouchkeyPitchBendMapping.h"

#undef DEBUG_PITCHBEND_MAPPING

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
TouchkeyPitchBendMapping::TouchkeyPitchBendMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                               Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: TouchkeyBaseMapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
bendIsEngaged_(false), snapIsEngaged_(false),
thresholdSemitones_(kDefaultBendThresholdSemitones), thresholdKeyLength_(kDefaultBendThresholdKeyLength),
snapZoneSemitones_(kDefaultSnapZoneSemitones),
bendMode_(kDefaultPitchBendMode), fixedModeMinEnableDistance_(kDefaultFixedModeEnableDistance),
fixedModeBufferDistance_(kDefaultFixedModeBufferDistance),
ignoresTwoFingers_(kDefaultIgnoresTwoFingers), ignoresThreeFingers_(kDefaultIgnoresThreeFingers),
onsetLocationX_(missing_value<float>::missing()),
onsetLocationY_(missing_value<float>::missing()),
lastX_(missing_value<float>::missing()), lastY_(missing_value<float>::missing()),
idOfCurrentTouch_(-1), lastTimestamp_(missing_value<timestamp_type>::missing()), 
lastProcessedIndex_(0), bendScalerPositive_(missing_value<float>::missing()),
bendScalerNegative_(missing_value<float>::missing()),
currentSnapDestinationSemitones_(missing_value<float>::missing()),
bendRangeSemitones_(kDefaultBendRangeSemitones), lastPitchBendSemitones_(0),
rawDistance_(kDefaultFilterBufferLength)
{
    resetDetectionState();
    updateCombinedThreshold();
}

TouchkeyPitchBendMapping::~TouchkeyPitchBendMapping() {

}

// Reset state back to defaults
void TouchkeyPitchBendMapping::reset() {
    TouchkeyBaseMapping::reset();
    sendPitchBendMessage(0.0);
    resetDetectionState();
}


// Resend all current parameters
void TouchkeyPitchBendMapping::resend() {
    sendPitchBendMessage(lastPitchBendSemitones_, true);
}

// Set the range of vibrato
void TouchkeyPitchBendMapping::setRange(float rangeSemitones) {
    bendRangeSemitones_ = rangeSemitones;
}

// Set the vibrato detection thresholds
void TouchkeyPitchBendMapping::setThresholds(float thresholdSemitones, float thresholdKeyLength) {
    thresholdSemitones_ = thresholdSemitones;
    thresholdKeyLength_ = thresholdKeyLength;
    updateCombinedThreshold();
}

// Set the mode to bend a fixed amount up and down the key, regardless of where
// the touch starts. minimumDistanceToEnable sets a floor below which the bend isn't
// possible (for starting very close to an edge) and bufferAtEnd sets the amount
// of key length beyond which no further bend takes place.
void TouchkeyPitchBendMapping::setFixedEndpoints(float minimumDistanceToEnable, float bufferAtEnd) {
    bendMode_ = kPitchBendModeFixedEndpoints;
    fixedModeMinEnableDistance_ = minimumDistanceToEnable;
    fixedModeBufferDistance_ = bufferAtEnd;
}

// Set the mode to bend an amount proportional to distance, which means
// that the total range of bend will depend on where the finger started.
void TouchkeyPitchBendMapping::setVariableEndpoints() {
    bendMode_ = kPitchBendModeVariableEndpoints;
}

void TouchkeyPitchBendMapping::setIgnoresMultipleFingers(bool ignoresTwo, bool ignoresThree) {
    ignoresTwoFingers_ = ignoresTwo;
    ignoresThreeFingers_ = ignoresThree;
}

// Trigger method. This receives updates from the TouchKey data or from state changes in
// the continuous key position (KeyPositionTracker). It will potentially change the scheduled
// behavior of future mapping calls, but the actual OSC messages should be transmitted in a different
// thread.
void TouchkeyPitchBendMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(who == 0)
        return;
    
    if(who == touchBuffer_) {
        if(!touchBuffer_->empty()) {
            // New touch data is available. Find the distance from the onset location.
            KeyTouchFrame frame = touchBuffer_->latest();
            lastTimestamp_ = timestamp;
            
            if(frame.count == 0) {
                // No touches. Last values are "missing", and we're not tracking any
                // particular touch ID
                lastX_ = lastY_ = missing_value<float>::missing();
                idOfCurrentTouch_ = -1;
#ifdef DEBUG_PITCHBEND_MAPPING
                std::cout << "Touch off\n";
#endif
            }
            else if((frame.count == 2 && ignoresTwoFingers_)
                    || (frame.count == 3 && ignoresThreeFingers_)) {
                // Multiple touches that we have chosen to ignore. Do nothing for now...
            }
            else {
                // At least one touch. Check if we are already tracking an ID and, if so,
                // use its coordinates. Otherwise grab the lowest current ID.
                
                bool foundCurrentTouch = false;
                
                if(idOfCurrentTouch_ >= 0) {
                    for(int i = 0; i < frame.count; i++) {
                        if(frame.ids[i] == idOfCurrentTouch_) {
                            lastY_ = frame.locs[i];
                            if(frame.locH < 0)
                                lastX_ = missing_value<float>::missing();
                            else
                                lastX_ = frame.locH;
                            foundCurrentTouch = true;
                            break;
                        }
                    }
                }
                
                if(!foundCurrentTouch) {
                    // Assign a new touch to be tracked
                    int lowestRemainingId = INT_MAX;
                    int lowestIndex = 0;
                    
                    for(int i = 0; i < frame.count; i++) {
                        if(frame.ids[i] < lowestRemainingId) {
                            lowestRemainingId = frame.ids[i];
                            lowestIndex = i;
                        }
                    }
                    
                    if(!bendIsEngaged_)
                        onsetLocationX_ = onsetLocationY_ = missing_value<float>::missing();
                    idOfCurrentTouch_ = lowestRemainingId;
                    lastY_ = frame.locs[lowestIndex];
                    if(frame.locH < 0)
                        lastX_ = missing_value<float>::missing();
                    else
                        lastX_ = frame.locH;
#ifdef DEBUG_PITCHBEND_MAPPING
                    std::cout << "Previous touch stopped; now ID " << idOfCurrentTouch_ << " at (" << lastX_ << ", " << lastY_ << ")\n";
#endif
                }
                
                // Now we have an X and (maybe) a Y coordinate for the most recent touch.
                // Check whether we have an initial location (if the note is active).
                if(noteIsOn_) {
                    //ScopedLock sl(distanceAccessMutex_);
                    
                    if(missing_value<float>::isMissing(onsetLocationY_) ||
                       (!foundCurrentTouch && !bendIsEngaged_)) {
                        // Note is on but touch hasn't yet arrived --> this touch becomes
                        // our onset location. Alternatively, the current touch is a different
                        // ID from the previous one.
                        onsetLocationY_ = lastY_;
                        onsetLocationX_ = lastX_;
                        
                        // Clear buffer and start with 0 distance for this point
                        clearBuffers();
#ifdef DEBUG_PITCHBEND_MAPPING
                        std::cout << "Starting at (" << onsetLocationX_ << ", " << onsetLocationY_ << ")\n";
#endif
                    }
                    else {
                        float distance = 0.0;
                        
                        // Note is on and a start location exists. Calculate distance between
                        // start location and the current point.
                        
                        if(missing_value<float>::isMissing(onsetLocationX_) &&
                           !missing_value<float>::isMissing(lastX_)) {
                            // No X location indicated for onset but we have one now.
                            // Update the onset X location.
                            onsetLocationX_ = lastX_;
#ifdef DEBUG_PITCHBEND_MAPPING
                            std::cout << "Found first X location at " << onsetLocationX_ << std::endl;
#endif
                        }
                        
                        // Distance is based on Y location. TODO: do we need all the X location stuff??
                        distance = lastY_ - onsetLocationY_;
                        
                        // Insert raw distance into the buffer. The rest of the processing takes place
                        // in the dedicated thread so as not to slow down commmunication with the hardware.
                        rawDistance_.insert(distance, timestamp);
                        
                        // Move the current scheduled event up to the present time.
                        // FIXME: this may be more inefficient than just doing everything in the current thread!
#ifdef NEW_MAPPING_SCHEDULER
                        keyboard_.mappingScheduler().scheduleNow(this);
#else
                        keyboard_.unscheduleEvent(this);
                        keyboard_.scheduleEvent(this, mappingAction_, keyboard_.schedulerCurrentTimestamp());
#endif
                        
                        //std::cout << "Raw distance " << distance << " filtered " << filteredDistance_.latest() << std::endl;
                    }
                }
            }
        }
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type TouchkeyPitchBendMapping::performMapping() {
    //ScopedLock sl(distanceAccessMutex_);
    
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    bool newSamplePresent = false;
    float lastProcessedDistance = missing_value<float>::missing();
    
    // Go through the filtered distance samples that are remaining to process.
    if(lastProcessedIndex_ < rawDistance_.beginIndex() + 1) {
        // Fell off the beginning of the position buffer. Skip to the samples we have
        // (shouldn't happen except in cases of exceptional system load, and not too
        // consequential if it does happen).
        lastProcessedIndex_ = rawDistance_.beginIndex() + 1;
    }
    
    while(lastProcessedIndex_ < rawDistance_.endIndex()) {
        float distance = lastProcessedDistance = rawDistance_[lastProcessedIndex_];
        //timestamp_type timestamp = rawDistance_.timestampAt(lastProcessedIndex_);
        newSamplePresent = true;
        
        if(bendIsEngaged_) {
            /*
            // TODO: look for snapping
            // Raw distance is the distance from note onset. Adjusted distance takes into account
            // that the bend actually started on the cross of a threshold.
            float adjustedDistance = rawDistance_.latest() - bendEngageLocation_;
            float pitchBendSemitones = 0.0;
            
            // Calculate pitch bend based on most recent distance
            if(adjustedDistance > 0.0)
                pitchBendSemitones = adjustedDistance * bendScalerPositive_;
            else
                pitchBendSemitones = adjustedDistance * bendScalerNegative_;
            
            // Find the nearest semitone to the current value by rounding
            currentSnapDestinationSemitones_ = roundf(pitchBendSemitones);
            
            if(snapIsEngaged_) {
                // TODO: check velocity conditions; if above minimum velocity, disengage
            }
            else {
                if(fabsf(pitchBendSemitones - currentSnapDestinationSemitones_) < snapZoneSemitones_) {
                    // TODO: check velocity conditions; if below minimum velocity, engage
                    //engageSnapping();
                }
            }
            */
        }
        else {
            // Check if bend should engage, using two thresholds: one as fraction of
            // key length, one as distance in semitones
            if(fabsf(distance) > thresholdCombinedMax_) {
                bendIsEngaged_ = true;
#ifdef DEBUG_PITCHBEND_MAPPING
                std::cout << "engaging bend at distance " << distance << std::endl;
#endif
                // Set up dynamic scaling based on fixed distances to edge of key.
                // TODO: make this more flexible, to always nail the nearest semitone (optionally)
                
                // This is how far we would have had from the onset point to the edge of key.
                float distanceToPositiveEdgeWithoutThreshold = 1.0 - onsetLocationY_;
                float distanceToNegativeEdgeWithoutThreshold = onsetLocationY_;
                
                // This is how far we actually have to go to the edge of the key
                float actualDistanceToPositiveEdge = 1.0 - (onsetLocationY_ + thresholdCombinedMax_);
                float actualDistanceToNegativeEdge = onsetLocationY_ - thresholdCombinedMax_;
                
                // Make it so moving toward edge of key gets as far as it would have without
                // the distance lost by the threshold

                if(bendMode_ == kPitchBendModeVariableEndpoints) {
                    if(actualDistanceToPositiveEdge > 0.0)
                        bendScalerPositive_ = bendRangeSemitones_ * distanceToPositiveEdgeWithoutThreshold / actualDistanceToPositiveEdge;
                    else
                        bendScalerPositive_ = bendRangeSemitones_; // Sanity check
                    if(actualDistanceToNegativeEdge > 0.0)
                        bendScalerNegative_ = bendRangeSemitones_ * distanceToNegativeEdgeWithoutThreshold / actualDistanceToNegativeEdge;
                    else
                        bendScalerNegative_ = bendRangeSemitones_; // Sanity check
                }
                else if(bendMode_ == kPitchBendModeFixedEndpoints) {
                    // TODO: buffer distance at end
                    if(actualDistanceToPositiveEdge > fixedModeMinEnableDistance_)
                        bendScalerPositive_ = bendRangeSemitones_ / actualDistanceToPositiveEdge;
                    else
                        bendScalerPositive_ = 0.0;
                    if(actualDistanceToNegativeEdge > fixedModeMinEnableDistance_)
                        bendScalerNegative_ = bendRangeSemitones_ / actualDistanceToNegativeEdge;
                    else
                        bendScalerNegative_ = 0.0;
                }
                else // unknown mode
                    bendScalerPositive_ = bendScalerNegative_ = 0.0;
            }
        }
            
        lastProcessedIndex_++;
    }
    
    if(bendIsEngaged_ && !missing_value<float>::isMissing(lastProcessedDistance)) {
        // Having processed every sample individually for detection, send a pitch bend message based on the most
        // recent one (no sense in sending multiple pitch bend messages simultaneously).
        if(newSamplePresent) {
            // Raw distance is the distance from note onset. Adjusted distance takes into account
            // that the bend actually started on the cross of a threshold.
            float pitchBendSemitones;
            
            if(lastProcessedDistance > thresholdCombinedMax_)
                pitchBendSemitones = (lastProcessedDistance - thresholdCombinedMax_) * bendScalerPositive_;
            else if(lastProcessedDistance < -thresholdCombinedMax_)
                pitchBendSemitones = (lastProcessedDistance + thresholdCombinedMax_) * bendScalerNegative_;
            else
                pitchBendSemitones = 0.0;
    
            sendPitchBendMessage(pitchBendSemitones);
            lastPitchBendSemitones_ = pitchBendSemitones;
        }
        else if(snapIsEngaged_) {
            // We may have arrived here without a new touch, just based on timing. Even so, if pitch snapping
            // is engaged we need to continue to update the pitch
            
            // TODO: calculate the next filtered pitch based on snapping
        }
    }
    
    // Register for the next update by returning its timestamp
    nextScheduledTimestamp_ = currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

// MIDI note-on message received
void TouchkeyPitchBendMapping::midiNoteOnReceived(int channel, int velocity) {
    // MIDI note has gone on. Set the starting location to be most recent
    // location. It's possible there has been no touch data before this,
    // in which case lastX and lastY will hold missing values.
    onsetLocationX_ = lastX_;
    onsetLocationY_ = lastY_;
    bendIsEngaged_ = false;
    if(!missing_value<float>::isMissing(onsetLocationY_)) {
        // Already have touch data. Clear the buffer here.
        // Clear buffer and start with 0 distance for this point
        clearBuffers();
#ifdef DEBUG_PITCHBEND_MAPPING
        std::cout << "MIDI on: starting at (" << onsetLocationX_ << ", " << onsetLocationY_ << ")\n";
#endif
    }
    else {
#ifdef DEBUG_PITCHBEND_MAPPING
        std::cout << "MIDI on but no touch\n";
#endif
    }
}

// MIDI note-off message received
void TouchkeyPitchBendMapping::midiNoteOffReceived(int channel) {
    if(bendIsEngaged_) {
        // TODO: should anything happen here? No new samples processed anyway,
        // but we may want the snapping algorithm to still continue its work.
    }
}

// Reset variables involved in detecting a pitch bend gesture
void TouchkeyPitchBendMapping::resetDetectionState() {
    bendIsEngaged_ = false;
    snapIsEngaged_ = false;
}

// Clear the buffers that hold distance measurements
void TouchkeyPitchBendMapping::clearBuffers() {
    rawDistance_.clear();
    rawDistance_.insert(0.0, lastTimestamp_);
    lastProcessedIndex_ = 0;
}

// Engage the snapping algorithm to pull the pitch into the nearest semitone
void TouchkeyPitchBendMapping::engageSnapping() {
    snapIsEngaged_ = true;
}

// Disengage the snapping algorithm
void TouchkeyPitchBendMapping::disengageSnapping() {
    snapIsEngaged_ = false;
}

// Set the combined threshold based on the two independent parameters
// relating to semitones and key length
void TouchkeyPitchBendMapping::updateCombinedThreshold() {
    if(thresholdKeyLength_ > thresholdSemitones_ / bendRangeSemitones_)
        thresholdCombinedMax_ = thresholdKeyLength_;
    else
        thresholdCombinedMax_ = thresholdSemitones_ / bendRangeSemitones_;
}


// Send the pitch bend message of a given number of a semitones. Send by OSC,
// which can be mapped to MIDI CC externally
void TouchkeyPitchBendMapping::sendPitchBendMessage(float pitchBendSemitones, bool force) {
    if(force || !suspended_)
        keyboard_.sendMessage(controlName_.c_str(), "if", noteNumber_, pitchBendSemitones, LO_ARGS_END);
}

