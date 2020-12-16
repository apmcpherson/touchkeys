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

  TouchkeyMultiFingerTriggerMapping.cpp: per-note mapping for the multiple-
  finger trigger mapping, which performs actions when two or more fingers
  are added or removed from the key.
*/

#include "TouchkeyMultiFingerTriggerMapping.h"
#include "TouchkeyMultiFingerTriggerMappingFactory.h"

// Class constants
const int TouchkeyMultiFingerTriggerMapping::kDefaultFilterBufferLength = 30;
const int TouchkeyMultiFingerTriggerMapping::kDefaultNumTouchesForTrigger = 2;
const int TouchkeyMultiFingerTriggerMapping::kDefaultNumFramesForTrigger = 2;
const int TouchkeyMultiFingerTriggerMapping::kDefaultNumConsecutiveTapsForTrigger = 1;
const timestamp_diff_type TouchkeyMultiFingerTriggerMapping::kDefaultMaxTapSpacing = milliseconds_to_timestamp(300.0);
const int TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOnAction = TouchkeyMultiFingerTriggerMapping::kActionNoteOn;
const int TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOffAction = TouchkeyMultiFingerTriggerMapping::kActionNone;
const int TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOnNoteNum = -1;
const int TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOffNoteNum = -1;
const int TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOnNoteVel =  -1;
const int TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOffNoteVel = -1;

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
TouchkeyMultiFingerTriggerMapping::TouchkeyMultiFingerTriggerMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                                         Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: TouchkeyBaseMapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
numTouchesForTrigger_(kDefaultNumTouchesForTrigger), numFramesForTrigger_(kDefaultNumFramesForTrigger),
numConsecutiveTapsForTrigger_(kDefaultNumConsecutiveTapsForTrigger), maxTapSpacing_(kDefaultMaxTapSpacing),
needsMidiNoteOn_(true), triggerOnAction_(kDefaultTriggerOnAction), triggerOffAction_(kDefaultTriggerOffAction),
triggerOnNoteNum_(kDefaultTriggerOnNoteNum), triggerOffNoteNum_(kDefaultTriggerOffNoteNum),
triggerOnNoteVel_(kDefaultTriggerOnNoteVel), triggerOffNoteVel_(kDefaultTriggerOffNoteVel),
pastSamples_(kDefaultFilterBufferLength)
{
    reset();
}

// Turn off mapping of data.
void TouchkeyMultiFingerTriggerMapping::disengage(bool shouldDelete) {
    // Send note off messages for anything currently on
    int port = static_cast<TouchkeyMultiFingerTriggerMappingFactory*>(factory_)->segment().outputPort();
    
    for( auto it = otherNotesOn_.begin(); it != otherNotesOn_.end(); ++it) {
        int ch = it->first;
        int note = it->second;
        
        keyboard_.midiOutputController()->sendNoteOn(port, ch, note, 0);
    }
    
    otherNotesOn_.clear();
    TouchkeyBaseMapping::disengage(shouldDelete);
}

// Reset state back to defaults
void TouchkeyMultiFingerTriggerMapping::reset() {
    juce::ScopedLock sl(sampleBufferMutex_);
    
    TouchkeyBaseMapping::reset();
    pastSamples_.clear();
    
    lastNumActiveTouches_ = 0;
    lastActiveTouchLocations_[0] = lastActiveTouchLocations_[1] = lastActiveTouchLocations_[2] = 0;
    framesCount_ = 0;
    tapsCount_ = 0;
    hasGeneratedTap_ = false;
    lastTapStartTimestamp_ = missing_value<timestamp_type>::missing();
    hasTriggered_ = false;
}

// Resend all current parameters
void TouchkeyMultiFingerTriggerMapping::resend() {
    // Message is only sent at release; resend may not apply here.
}

void TouchkeyMultiFingerTriggerMapping::setTouchesForTrigger(int touches) {
    numTouchesForTrigger_ = touches;
}

void TouchkeyMultiFingerTriggerMapping::setFramesForTrigger(int frames) {
    numFramesForTrigger_ = frames;
}

void TouchkeyMultiFingerTriggerMapping::setConsecutiveTapsForTrigger(int taps) {
    numConsecutiveTapsForTrigger_ = taps;
}

void TouchkeyMultiFingerTriggerMapping::setMaxTimeBetweenTapsForTrigger(timestamp_diff_type timeDiff) {
    maxTapSpacing_ = timeDiff;
}

void TouchkeyMultiFingerTriggerMapping::setNeedsMidiNoteOn(bool needsMidi) {
    needsMidiNoteOn_ = needsMidi;
}

void TouchkeyMultiFingerTriggerMapping::setTriggerOnAction(int action) {
    triggerOnAction_ = action;
}

void TouchkeyMultiFingerTriggerMapping::setTriggerOffAction(int action) {
    triggerOffAction_ = action;
}

void TouchkeyMultiFingerTriggerMapping::setTriggerOnNoteNumber(int note) {
    triggerOnNoteNum_ = note;
}

void TouchkeyMultiFingerTriggerMapping::setTriggerOffNoteNumber(int note) {
    triggerOffNoteNum_ = note;
}

void TouchkeyMultiFingerTriggerMapping::setTriggerOnNoteVelocity(int velocity) {
    triggerOnNoteVel_ = velocity;
}

void TouchkeyMultiFingerTriggerMapping::setTriggerOffNoteVelocity(int velocity) {
    triggerOffNoteVel_ = velocity;
}

// This method receives data from the touch buffer or possibly the continuous key angle (not used here)
void TouchkeyMultiFingerTriggerMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(needsMidiNoteOn_ && !noteIsOn_) {
        framesCount_ = 0;
        hasGeneratedTap_ = false;
        return;
    }
    
    if(who == touchBuffer_) {
        if(!touchBuffer_->empty()) {
            // Find the current number of touches
            KeyTouchFrame frame  = touchBuffer_->latest();
            int count = frame.count;
            
            if(count < numTouchesForTrigger_) {
                framesCount_ = 0;
                hasGeneratedTap_ = false;
                if(hasTriggered_) {
                    generateTriggerOff(timestamp);
                    hasTriggered_ = false;
                }
            }
            else if(count == numTouchesForTrigger_) {
                framesCount_++;
                if(framesCount_ >= numFramesForTrigger_ && !hasGeneratedTap_) {
                    // Enough frames have elapsed to consider this a tap
                    // Figure out if it is a multiple consecutive tap or the first
                    // of a set.
                    if(!missing_value<timestamp_diff_type>::isMissing(lastTapStartTimestamp_)) {
                        if(timestamp - lastTapStartTimestamp_ < maxTapSpacing_) {
                            tapsCount_++;
                        }
                        else
                            tapsCount_ = 1;
                    }
                    else
                        tapsCount_ = 1;
                    
                    // Check if the right number of taps has elapsed
                    if(tapsCount_ >= numConsecutiveTapsForTrigger_ && !hasTriggered_) {
                        hasTriggered_ = true;
                        
                        // Find the ID of the newest touch and compare its location
                        // to the immediately preceding touch(es) to find the distance
                        int newest = 0, oldest = 0, newestId = -1, oldestId = 1000000;
                        for(int i = 0; i < count; i++) {
                            if(frame.ids[i] > newestId) {
                                newest = i;
                                newestId = frame.ids[i];
                            }
                            if(frame.ids[i] < oldestId) {
                                oldest = i;
                                oldestId = frame.ids[i];
                            }
                        }

                        // Find the distance between the point before this tap and the
                        // point that was added to create the tap. If this is a 3-touch
                        // tap, find the distance between the farthest two points, with
                        // the direction determined by which end is older.
                        float distance = frame.locs[newest] - frame.locs[oldest];
                        if(count == 3) {
                            if(fabsf(frame.locs[2] - frame.locs[0]) > fabsf(distance)) {
                                if(frame.ids[2] > frame.ids[0])
                                    distance = frame.locs[2] - frame.locs[0];
                                else
                                    distance = frame.locs[0] - frame.locs[2];
                            }
                        }
                        
                        // Generate the trigger. If a multi-tap gesture, also indicate the timing
                        if(numConsecutiveTapsForTrigger_ <= 1)
                            generateTriggerOn(timestamp, 0, distance);
                        else
                            generateTriggerOn(timestamp, timestamp - lastTapStartTimestamp_, distance);
                    }
                    
                    hasGeneratedTap_ = true;
                    lastTapStartTimestamp_ = timestamp;
                }
            }
        
            // Save the count locations for next time
            lastNumActiveTouches_ = frame.count;
            for(int i = 0; i < count; i++) {
                lastActiveTouchLocations_[i] = frame.locs[i];
            }
        }
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type TouchkeyMultiFingerTriggerMapping::performMapping() {
    // Nothing to do here until note is released.
    // Register for the next update by returning its timestamp
    // TODO: do we even need this? Check Mapping::engage() and Mapping::disengage()
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    nextScheduledTimestamp_ = currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

void TouchkeyMultiFingerTriggerMapping::generateTriggerOn(timestamp_type timestamp, timestamp_diff_type timeBetweenTaps, float distanceBetweenPoints) {
    if(!suspended_) {
        if(triggerOnAction_ == kActionNoteOn ||
           triggerOnAction_ == kActionNoteOff) {
            // Send a MIDI note on message with given note number and velocity
            int port = static_cast<TouchkeyMultiFingerTriggerMappingFactory*>(factory_)->segment().outputPort();
            int ch = keyboard_.key(noteNumber_)->midiChannel();
            int vel = triggerOnNoteVel_;
            int note = triggerOnNoteNum_;
            if(note < 0)    // note < 0 means current note
                note = noteNumber_;
            if(note < 128) {
                if(triggerOnAction_ == kActionNoteOn) {
                    // Can't send notes above 127...
                    if(vel < 0)     // vel < 0 means same as current
                        vel = keyboard_.key(noteNumber_)->midiVelocity();
                    if(vel > 127)
                        vel = 127;
                    
                    // Register that this note has been turned on
                    if(note != noteNumber_)
                        otherNotesOn_.insert(std::pair<int,int>(ch, note));
                }
                else {
                    // Note off
                    vel = 0;
                    if(note != noteNumber_) {
                        // Unregister this note if we are turning it off
                        if(otherNotesOn_.count(std::pair<int,int>(ch, note)) > 0) {
                            otherNotesOn_.erase(std::pair<int,int>(ch, note));
                        }
                    }
                }
                
                keyboard_.midiOutputController()->sendNoteOn(port, ch, note, vel);
            }
        }
    }
}

void TouchkeyMultiFingerTriggerMapping::generateTriggerOff(timestamp_type timestamp) {
    if(!suspended_) {
        if(triggerOffAction_ == kActionNoteOn ||
           triggerOffAction_ == kActionNoteOff) {
            // Send a MIDI note on message with given note number and velocity
            int port = static_cast<TouchkeyMultiFingerTriggerMappingFactory*>(factory_)->segment().outputPort();
            int ch = keyboard_.key(noteNumber_)->midiChannel();
            int vel = triggerOffNoteVel_;
            int note = triggerOffNoteNum_;
            if(note < 0)    // note < 0 means current note
                note = noteNumber_;
            if(note < 128) {
                if(triggerOffAction_ == kActionNoteOn) {
                    // Can't send notes above 127...
                    if(vel < 0)     // vel < 0 means same as current
                        vel = keyboard_.key(noteNumber_)->midiVelocity();
                    if(vel > 127)
                        vel = 127;
                    
                    // Register that this note has been turned on
                    if(note != noteNumber_)
                        otherNotesOn_.insert(std::pair<int,int>(ch, note));
                }
                else {
                    // Note off
                    vel = 0;
                    if(note != noteNumber_) {
                        // Unregister this note if we are turning it off
                        if(otherNotesOn_.count(std::pair<int,int>(ch, note)) > 0) {
                            otherNotesOn_.erase(std::pair<int,int>(ch, note));
                        }
                    }
                }
                
                keyboard_.midiOutputController()->sendNoteOn(port, ch, note, vel);
            }
        }
    }
}

// MIDI note-off message received
void TouchkeyMultiFingerTriggerMapping::midiNoteOffReceived(int channel) {
    // int ch = keyboard_.key(noteNumber_)->midiChannel();
    // keyboard_.midiOutputController()->sendControlChange(0, ch, 73, 0);
}
