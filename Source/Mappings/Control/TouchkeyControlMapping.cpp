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

  TouchkeyControlMapping.cpp: per-note mapping for the TouchKeys control
  mapping, which converts an arbitrary touch parameter into a MIDI or
  OSC control message.
*/

#include "TouchkeyControlMapping.h"
#include "../MappingScheduler.h"

#undef DEBUG_CONTROL_MAPPING

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
TouchkeyControlMapping::TouchkeyControlMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                                   Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: TouchkeyBaseMapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
controlIsEngaged_(false),
inputMin_(0.0), inputMax_(1.0), outputMin_(0.0), outputMax_(1.0), outputDefault_(0.0),
inputParameter_(kInputParameterYPosition), inputType_(kTypeAbsolute),
threshold_(0.0), ignoresTwoFingers_(kDefaultIgnoresTwoFingers),
ignoresThreeFingers_(kDefaultIgnoresThreeFingers), direction_(kDefaultDirection),
touchOnsetValue_(missing_value<float>::missing()),
midiOnsetValue_(missing_value<float>::missing()),
lastValue_(missing_value<float>::missing()),
lastTimestamp_(missing_value<timestamp_type>::missing()), lastProcessedIndex_(0),
controlEngageLocation_(missing_value<float>::missing()),
controlScalerPositive_(missing_value<float>::missing()),
controlScalerNegative_(missing_value<float>::missing()),
lastControlValue_(outputDefault_),
rawValues_(kDefaultFilterBufferLength)
{
    resetDetectionState();
}

TouchkeyControlMapping::~TouchkeyControlMapping() {
#if 0
#ifndef NEW_MAPPING_SCHEDULER
    try {
        disengage();
    }
    catch(...) {
        std::cerr << "~TouchkeyControlMapping(): exception during disengage()\n";
    }
#endif
#endif
}

// Turn on mapping of data.
/*void TouchkeyControlMapping::engage() {
    Mapping::engage();
    
    // Register for OSC callbacks on MIDI note on/off
    addOscListener("/midi/noteon");
	addOscListener("/midi/noteoff");
}

// Turn off mapping of data. Remove our callback from the scheduler
void TouchkeyControlMapping::disengage(bool shouldDelete) {
    // Remove OSC listeners first
    removeOscListener("/midi/noteon");
	removeOscListener("/midi/noteoff");
    
    // Don't send any separate message here, leave it where it was
    
    Mapping::disengage(shouldDelete);
    
    if(noteIsOn_) {
        // TODO
    }
    noteIsOn_ = false;
}*/

// Reset state back to defaults
void TouchkeyControlMapping::reset() {
    TouchkeyBaseMapping::reset();
    sendControlMessage(outputDefault_);
    resetDetectionState();
    //noteIsOn_ = false;
}

// Resend all current parameters
void TouchkeyControlMapping::resend() {
    sendControlMessage(lastControlValue_, true);
}

// Name for this control, used in the OSC path
/*void TouchkeyControlMapping::setName(const std::string& name) {
    controlName_ = name;
}*/

// Parameters for the controller handling
// Input parameter to use for this control mapping and whether it is absolute or relative
void TouchkeyControlMapping::setInputParameter(int parameter, int type) {
    if(inputParameter_ >= 0 && inputParameter_ < kInputParameterMaxValue)
        inputParameter_ = parameter;
    if(type >= 0 && type < kTypeMaxValue)
        inputType_ = type;
}

// Input/output range for this parameter
void TouchkeyControlMapping::setRange(float inputMin, float inputMax, float outputMin, float outputMax, float outputDefault) {
    inputMin_ = inputMin;
    inputMax_ = inputMax;
    outputMin_ = outputMin;
    outputMax_ = outputMax;
    outputDefault_ = outputDefault;
}

// Threshold which must be exceeded for the control to engage (for relative position), or 0 if not used
void TouchkeyControlMapping::setThreshold(float threshold) {
    threshold_ = threshold;
}

void TouchkeyControlMapping::setIgnoresMultipleFingers(bool ignoresTwo, bool ignoresThree) {
    ignoresTwoFingers_ = ignoresTwo;
    ignoresThreeFingers_ = ignoresThree;
}

void TouchkeyControlMapping::setDirection(int direction) {
    if(direction >= 0 && direction < kDirectionMaxValue)
        direction_ = direction;
}

// OSC handler method. Called from PianoKeyboard when MIDI data comes in.
/*bool TouchkeyControlMapping::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
    if(!strcmp(path, "/midi/noteon") && !noteIsOn_ && numValues >= 1) {
        if(types[0] == 'i' && values[0]->i == noteNumber_) {
            // MIDI note has gone on. Set the starting location to be most recent
            // location. It's possible there has been no touch data before this,
            // in which case lastX and lastY will hold missing values.
            midiOnsetValue_ = lastValue_;
            if(!missing_value<float>::isMissing(midiOnsetValue_)) {
                if(inputType_ == kTypeNoteOnsetRelative) {
                    // Already have touch data. Clear the buffer here.
                    // Clear buffer and start with default value for this point
                    clearBuffers();
                    
#ifdef DEBUG_CONTROL_MAPPING
                    std::cout << "MIDI on: starting at (" << midiOnsetValue_ << ")\n";
#endif
                }
            }
            else {
#ifdef DEBUG_CONTROL_MAPPING
                std::cout << "MIDI on but no touch\n";
#endif
            }
            
            noteIsOn_ = true;
            return false;
        }
    }
    else if(!strcmp(path, "/midi/noteoff") && noteIsOn_ && numValues >= 1) {
        if(types[0] == 'i' && values[0]->i == noteNumber_) {
            // MIDI note goes off
            noteIsOn_ = false;
            if(controlIsEngaged_) {
                // TODO: should anything happen here?
            }
#ifdef DEBUG_CONTROL_MAPPING
            std::cout << "MIDI off\n";
#endif
            return false;
        }
    }
    
    return false;
}*/

// Trigger method. This receives updates from the TouchKey data or from state changes in
// the continuous key position (KeyPositionTracker). It will potentially change the scheduled
// behavior of future mapping calls, but the actual OSC messages should be transmitted in a different
// thread.
void TouchkeyControlMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(who == nullptr)
        return;
    
    if(who == touchBuffer_) {
        if(!touchBuffer_->empty()) {
            // New touch data is available. Find the distance from the onset location.
            KeyTouchFrame frame = touchBuffer_->latest();
            lastTimestamp_ = timestamp;
            
            if(frame.count == 0) {
                // No touches. Last values are "missing", and we're not tracking any
                // particular touch ID
                lastValue_ = missing_value<float>::missing();
                idsOfCurrentTouches_[0] = idsOfCurrentTouches_[1] = idsOfCurrentTouches_[2] = -1;
                
#ifdef DEBUG_CONTROL_MAPPING
                std::cout << "Touch off\n";
#endif
            }
            else {
                //ScopedLock sl(rawValueAccessMutex_);
                
                // At least one touch. Check if we are already tracking an ID and, if so,
                // use its coordinates. Otherwise grab the lowest current ID.
                lastValue_ = getValue(frame);
                
                // Check that the value actually exists
                if(!missing_value<float>::isMissing(lastValue_)) {
                    // If we have no onset value, this is it
                    if(missing_value<float>::isMissing(touchOnsetValue_)) {
                        touchOnsetValue_ = lastValue_;
                        if(inputType_ == kTypeFirstTouchRelative) {
                            clearBuffers();
#ifdef DEBUG_CONTROL_MAPPING
                            std::cout << "Starting at " << lastValue_ << std::endl;
#endif
                        }
                    }
                    
                    // If MIDI note is on and we don't previously have a value, this is it
                    if(noteIsOn_ && missing_value<float>::isMissing(midiOnsetValue_)) {
                        midiOnsetValue_ = lastValue_;
                        if(inputType_ == kTypeNoteOnsetRelative) {
                            clearBuffers();
#ifdef DEBUG_CONTROL_MAPPING
                            std::cout << "Starting at " << lastValue_ << std::endl;
#endif
                        }
                    }
                    
                    if(noteIsOn_) {
                        // Insert the latest sample into the buffer depending on how the data should be processed
                        if(inputType_ == kTypeAbsolute) {
                            rawValues_.insert(lastValue_, timestamp);
                        }
                        else if(inputType_ == kTypeFirstTouchRelative) {
                            rawValues_.insert(lastValue_ - touchOnsetValue_, timestamp);
                        }
                        else if(inputType_ == kTypeNoteOnsetRelative) {
                            rawValues_.insert(lastValue_ - midiOnsetValue_, timestamp);
                        }
                        
                        // Move the current scheduled event up to the present time.
                        // FIXME: this may be more inefficient than just doing everything in the current thread!
#ifdef NEW_MAPPING_SCHEDULER
                        keyboard_.mappingScheduler().scheduleNow(this);
#else
                        keyboard_.unscheduleEvent(this);
                        keyboard_.scheduleEvent(this, mappingAction_, keyboard_.schedulerCurrentTimestamp());
#endif
                    }
                }
            }
        }
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type TouchkeyControlMapping::performMapping() {
    //ScopedLock sl(rawValueAccessMutex_);
    
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    bool newSamplePresent = false;
    
    // Go through the filtered distance samples that are remaining to process.
    if(lastProcessedIndex_ < rawValues_.beginIndex() + 1) {
        // Fell off the beginning of the position buffer. Skip to the samples we have
        // (shouldn't happen except in cases of exceptional system load, and not too
        // consequential if it does happen).
        lastProcessedIndex_ = rawValues_.beginIndex() + 1;
    }
    
    while(lastProcessedIndex_ < rawValues_.endIndex()) {
        float value = rawValues_[lastProcessedIndex_];
        //timestamp_type timestamp = rawValues_.timestampAt(lastProcessedIndex_);
        newSamplePresent = true;
        
        if(inputType_ == kTypeAbsolute) {
            controlIsEngaged_ = true;
        }
        else if(!controlIsEngaged_) {
            // Compare value against threshold to see if the control should engage
            if(fabsf(value) > threshold_) {
                float startingValue;
                
                controlIsEngaged_ = true;
                controlEngageLocation_ = (value > 0 ? threshold_ : -threshold_);
                
#ifdef DEBUG_CONTROL_MAPPING
                std::cout << "engaging control at distance " << controlEngageLocation_ << std::endl;
#endif
                
                if(inputType_ == kTypeFirstTouchRelative)
                    startingValue = touchOnsetValue_;
                else
                    startingValue = midiOnsetValue_;
                
                // This is how much range we would have had without the threshold
                float distanceToPositiveEdgeWithoutThreshold = 1.0 - startingValue;
                float distanceToNegativeEdgeWithoutThreshold = 0.0 + startingValue;
                
                // This is how much range we actually have with the threshold
                float actualDistanceToPositiveEdge = 1.0 - (startingValue + controlEngageLocation_);
                float actualDistanceToNegativeEdge = 0.0 + startingValue + controlEngageLocation_;
                
                // Make it so moving toward edge of key gets as far as it would have without
                // the distance lost by the threshold
                if(actualDistanceToPositiveEdge > 0.0)
                    controlScalerPositive_ = (outputMax_ - outputDefault_) * distanceToPositiveEdgeWithoutThreshold / actualDistanceToPositiveEdge;
                else
                    controlScalerPositive_ = (outputMax_ - outputDefault_); // Sanity check
                if(actualDistanceToNegativeEdge > 0.0)
                    controlScalerNegative_ = (outputDefault_ - outputMin_) * distanceToNegativeEdgeWithoutThreshold / actualDistanceToNegativeEdge;
                else
                    controlScalerNegative_ = (outputDefault_ - outputMin_); // Sanity check
            }
        }
        
        lastProcessedIndex_++;
    }
    
    if(controlIsEngaged_) {
        // Having processed every sample individually for any detection/filtering, now send
        // the most recent output as an OSC message
        if(newSamplePresent) {
            float latestValue = rawValues_.latest();

            // In cases of relative values, the place the control engages will actually be where it crosses
            // the threshold, not the onset location itself. Need to update the value accordingly.
            if(inputType_ == kTypeFirstTouchRelative ||
               inputType_ == kTypeNoteOnsetRelative) {
                if(latestValue > 0) {
                    latestValue -= threshold_;
                    if(latestValue < 0)
                        latestValue = 0;
                }
                else if(latestValue < 0) {
                    latestValue += threshold_;
                    if(latestValue > 0)
                        latestValue = 0;
                }
                
                if(direction_ == kDirectionNegative)
                    latestValue = -latestValue;
                else if((direction_ == kDirectionBoth) && latestValue < 0)
                    latestValue = -latestValue;
            }
            
            sendControlMessage(latestValue);
            lastControlValue_ = latestValue;
        }
    }
    
    // Register for the next update by returning its timestamp
    nextScheduledTimestamp_ = 0; //currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

// MIDI note-on message received
void TouchkeyControlMapping::midiNoteOnReceived(int channel, int velocity) {
    // MIDI note has gone on. Set the starting location to be most recent
    // location. It's possible there has been no touch data before this,
    // in which case lastX and lastY will hold missing values.
    midiOnsetValue_ = lastValue_;
    if(!missing_value<float>::isMissing(midiOnsetValue_)) {
        if(inputType_ == kTypeNoteOnsetRelative) {
            // Already have touch data. Clear the buffer here.
            // Clear buffer and start with default value for this point
            clearBuffers();
            
#ifdef DEBUG_CONTROL_MAPPING
            std::cout << "MIDI on: starting at (" << midiOnsetValue_ << ")\n";
#endif
        }
    }
    else {
#ifdef DEBUG_CONTROL_MAPPING
        std::cout << "MIDI on but no touch\n";
#endif
    }
}

// MIDI note-off message received
void TouchkeyControlMapping::midiNoteOffReceived(int channel) {
    if(controlIsEngaged_) {
        // TODO: should anything happen here?
    }
}

// Reset variables involved in detecting a pitch bend gesture
void TouchkeyControlMapping::resetDetectionState() {
    controlIsEngaged_ = false;
    controlEngageLocation_ = missing_value<float>::missing();
    idsOfCurrentTouches_[0] = idsOfCurrentTouches_[1] = idsOfCurrentTouches_[2] = -1;
}

// Clear the buffers that hold distance measurements
void TouchkeyControlMapping::clearBuffers() {
    rawValues_.clear();
    rawValues_.insert(0.0, lastTimestamp_);
    lastProcessedIndex_ = 0;
}

// Return the current parameter value depending on which one we are listening to
float TouchkeyControlMapping::getValue(const KeyTouchFrame& frame) {
    if(inputParameter_ == kInputParameterXPosition)
        return frame.locH;
    /*else if(inputParameter_ == kInputParameter2FingerMean ||
            inputParameter_ == kInputParameter2FingerDistance) {
        if(frame.count < 2)
            return missing_value<float>::missing();
        if(frame.count == 3 && ignoresThreeFingers_)
            return missing_value<float>::missing();
        
        bool foundCurrentTouch = false;
        float currentValue;
        
        // Look for the touches we were tracking last frame
        if(idsOfCurrentTouches_[0] >= 0) {
            for(int i = 0; i < frame.count; i++) {
                if(frame.ids[i] == idsOfCurrentTouches_[0]) {
                    if(inputParameter_ == kInputParameterYPosition)
                        currentValue = frame.locs[i];
                    else // kInputParameterTouchSize
                        currentValue = frame.sizes[i];
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
            
            idsOfCurrentTouches_[0] = lowestRemainingId;
            if(inputParameter_ == kInputParameterYPosition)
                currentValue = frame.locs[lowestIndex];
            else if(inputParameter_ == kInputParameterTouchSize)
                currentValue = frame.sizes[lowestIndex];
            else // Shouldn't happen
                currentValue = missing_value<float>::missing();
            
#ifdef DEBUG_CONTROL_MAPPING
            std::cout << "Previous touch stopped; now ID " << idsOfCurrentTouches_[0] << " at (" << currentValue << ")\n";
#endif
        }
        
    }*/
    else {
        if(frame.count == 0)
            return missing_value<float>::missing();
        if((inputParameter_ == kInputParameter2FingerMean ||
           inputParameter_ == kInputParameter2FingerDistance) &&
           frame.count < 2)
            return missing_value<float>::missing();
        if(frame.count == 2 && ignoresTwoFingers_)
            return missing_value<float>::missing();
        if(frame.count == 3 && ignoresThreeFingers_)
            return missing_value<float>::missing();
        /*
        // The other values are dependent on individual touches
        bool foundCurrentTouch = false;
        float currentValue;
        
        // Look for the touch we were tracking last frame
        if(idsOfCurrentTouches_[0] >= 0) {
            for(int i = 0; i < frame.count; i++) {
                if(frame.ids[i] == idsOfCurrentTouches_[0]) {
                    if(inputParameter_ == kInputParameterYPosition)
                        currentValue = frame.locs[i];
                    else // kInputParameterTouchSize
                        currentValue = frame.sizes[i];
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
            
            idsOfCurrentTouches_[0] = lowestRemainingId;
            if(inputParameter_ == kInputParameterYPosition)
                currentValue = frame.locs[lowestIndex];
            else if(inputParameter_ == kInputParameterTouchSize)
                currentValue = frame.sizes[lowestIndex];
            else // Shouldn't happen
                currentValue = missing_value<float>::missing();
            
#ifdef DEBUG_CONTROL_MAPPING
            std::cout << "Previous touch stopped; now ID " << idsOfCurrentTouches_[0] << " at (" << currentValue << ")\n";
#endif
        }*/
        
        float currentValue = 0;
        
        int idWithinFrame0 = locateTouchId(frame, 0);
        if(idWithinFrame0 < 0) { 
            // Touch ID not found, start a new value
            idsOfCurrentTouches_[0] = lowestUnassignedTouch(frame, &idWithinFrame0);
#ifdef DEBUG_CONTROL_MAPPING
            std::cout << "Previous touch stopped (0); now ID " << idsOfCurrentTouches_[0] << '\n';
#endif
            if(idsOfCurrentTouches_[0] < 0) {
                std::cout << "BUG: didn't find any unassigned touch!\n";
            }
        }
        
        if(inputParameter_ == kInputParameterYPosition)
            currentValue = frame.locs[idWithinFrame0];
        else if(inputParameter_ == kInputParameterTouchSize) // kInputParameterTouchSize
            currentValue = frame.sizes[idWithinFrame0];
        else if(inputParameter_ == kInputParameter2FingerMean ||
           inputParameter_ == kInputParameter2FingerDistance) {
            int idWithinFrame1 = locateTouchId(frame, 1);
            if(idWithinFrame1 < 0) {
                // Touch ID not found, start a new value
                idsOfCurrentTouches_[1] = lowestUnassignedTouch(frame, &idWithinFrame1);
#ifdef DEBUG_CONTROL_MAPPING
                std::cout << "Previous touch stopped (1); now ID " << idsOfCurrentTouches_[1] << '\n';
#endif
                if(idsOfCurrentTouches_[1] < 0) {
                    std::cout << "BUG: didn't find any unassigned touch for second finger!\n";
                }
            }
            
            if(inputParameter_ == kInputParameter2FingerMean)
                currentValue = (frame.locs[idWithinFrame0] + frame.locs[idWithinFrame1]) * 0.5;
            else
                currentValue = fabsf(frame.locs[idWithinFrame1] - frame.locs[idWithinFrame0]);
        }
        
        return currentValue;
    }
}

// Look for a touch index in the frame matching the given value of idsOfCurrentTouches[index]
// Returns -1 if not found
int TouchkeyControlMapping::locateTouchId(KeyTouchFrame const& frame, int index) {
    if(idsOfCurrentTouches_[index] < 0)
        return -1;
    
    for(int i = 0; i < frame.count; i++) {
        if(frame.ids[i] == idsOfCurrentTouches_[index]) {
            return i;
        }
    }
    
    return -1;
}

// Locates the lowest touch ID that is not assigned to a current touch
// Returns -1 if no unassigned touches were found
int TouchkeyControlMapping::lowestUnassignedTouch(KeyTouchFrame const& frame, int *indexWithinFrame) {
    int lowestRemainingId = INT_MAX;
    int lowestIndex = -1;
    
    for(int i = 0; i < frame.count; i++) {
        if(frame.ids[i] < lowestRemainingId) {
            bool alreadyAssigned = false;
            for(int j = 0; j < 3; j++) {
                if(idsOfCurrentTouches_[j] == frame.ids[i])
                    alreadyAssigned = true;
            }
            
            if(!alreadyAssigned) {
                lowestRemainingId = frame.ids[i];
                lowestIndex = i;

            }
        }
    }
    
    if(indexWithinFrame != 0)
        *indexWithinFrame = lowestIndex;
    return lowestRemainingId;
}

// Send the pitch bend message of a given number of a semitones. Send by OSC,
// which can be mapped to MIDI CC externally
void TouchkeyControlMapping::sendControlMessage(float value, bool force) {
    if(force || !suspended_) {
#ifdef DEBUG_CONTROL_MAPPING
        std::cout << "TouchkeyControlMapping: sending " << value << " for note " << noteNumber_ << std::endl;
#endif
        keyboard_.sendMessage(controlName_.c_str(), "if", noteNumber_, value, LO_ARGS_END);
    }
}

