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

  TouchkeyKeyDivisionMapping.cpp: per-note mapping for the split-key mapping
  which triggers different actions or pitches depending on where the key
  was struck.
*/

#include "TouchkeyKeyDivisionMapping.h"
#include "TouchkeyKeyDivisionMappingFactory.h"

#undef DEBUG_KEY_DIVISION_MAPPING

const int TouchkeyKeyDivisionMapping::kDefaultNumberOfSegments = 2;
const timestamp_diff_type TouchkeyKeyDivisionMapping::kDefaultDetectionTimeout = milliseconds_to_timestamp(25.0);
const int TouchkeyKeyDivisionMapping::kDefaultDetectionParameter = kDetectionParameterYPositionAndNumberOfTouches;
const int TouchkeyKeyDivisionMapping::kDefaultRetriggerNumFrames = 2;

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
TouchkeyKeyDivisionMapping::TouchkeyKeyDivisionMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                                     Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: TouchkeyBaseMapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
numberOfSegments_(kDefaultNumberOfSegments), candidateSegment_(-1), detectedSegment_(-1), defaultSegment_(0),
detectionParameter_(kDefaultDetectionParameter), retriggerable_(true), retriggerNumFrames_(kDefaultRetriggerNumFrames),
retriggerKeepsVelocity_(true),
midiNoteOnTimestamp_(missing_value<timestamp_type>::missing()), timeout_(kDefaultDetectionTimeout),
lastNumActiveTouches_(-1)
{
}

// Reset state back to defaults
void TouchkeyKeyDivisionMapping::reset() {
    TouchkeyBaseMapping::reset();
    
    candidateSegment_ = detectedSegment_ = -1;
    midiNoteOnTimestamp_ = missing_value<timestamp_type>::missing();
}

// Resend all current parameters
void TouchkeyKeyDivisionMapping::resend() {
    if(detectedSegment_ >= 0)
        sendSegmentMessage(detectedSegment_, true);
}

// Set the pitch bend values (in semitones) for each segment. These
// values are in relation to the pitch of this note
void TouchkeyKeyDivisionMapping::setSegmentPitchBends(const float *bendsInSemitones, int numBends) {
    // Clear old values and refill the vector
    segmentBends_.clear();
    for(int i = 0; i < numBends; i++)
        segmentBends_.push_back(bendsInSemitones[i]);
}

// This method receives data from the touch buffer or possibly the continuous key angle (not used here)
void TouchkeyKeyDivisionMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(who == touchBuffer_) {
        // If we get here, a new touch frame has been received and there is no segment detected
        // yet. We should come up with a candidate segment. If the MIDI note is on, activate this
        // segment right away. Otherwise, save it for later so when the MIDI note begins, we have
        // it ready to go.
        if(!touchBuffer_->empty()) {
            const KeyTouchFrame& frame = touchBuffer_->latest();
            
            if(detectedSegment_ < 0) {
                int candidateBasedOnYPosition = -1, candidateBasedOnNumberOfTouches = -1;
                
                // Find the first touch. TODO: eventually look for the largest touch
                float yPosition = frame.locs[0];
                
                // Calculate two possible segments based on touch location and based on
                // number of touches.
                candidateBasedOnYPosition = segmentForLocation(yPosition);
                candidateBasedOnNumberOfTouches = segmentForNumTouches(frame.count);
                
                if(detectionParameter_ == kDetectionParameterYPosition)
                    candidateSegment_ = candidateBasedOnYPosition;
                else if(detectionParameter_ == kDetectionParameterNumberOfTouches)
                    candidateSegment_ = candidateBasedOnNumberOfTouches;
                else if(detectionParameter_ == kDetectionParameterYPositionAndNumberOfTouches) {
                    // Choose the maximum segment specified by the other two methods
                    candidateSegment_ = candidateBasedOnNumberOfTouches > candidateBasedOnYPosition ? candidateBasedOnNumberOfTouches : candidateBasedOnYPosition;
                }
                else // Shouldn't happen
                    candidateSegment_ = -1;
                
                if(noteIsOn_) {
                    detectedSegment_ = candidateSegment_;
#ifdef DEBUG_KEY_DIVISION_MAPPING
                    std::cout << "TouchkeyKeyDivisionMapping::triggerReceived(): detectedSegment_ = " << detectedSegment_ << '\n';
#endif
                    sendSegmentMessage(detectedSegment_);
                }
            }
            else if(retriggerable_ &&
                    (lastNumActiveTouches_ == 1 &&
                    frame.count >= 2) && noteIsOn_) {
                // Here, there was one touch active before and now there are two. Look for the
                // location of the most recently added touch, and determine whether it matches a
                // segment different from the one we're in. If so, retrigger the MIDI note
                // with a different pitch bend
                
                int newCandidate = segmentForLocation(locationOfNewestTouch(frame));
                
#ifdef DEBUG_KEY_DIVIOSION_MAPPING
                std::cout << "TouchkeyKeyDivisionMapping: touch added with candidate segment " << newCandidate << " (current is " << detectedSegment_ << ")\n";
#endif
                if(newCandidate != detectedSegment_) {
                    // Set up a new segment to retrigger and tell the scheduler to insert the mapping
                    detectedSegment_ = newCandidate;
                    
                    // Find the keyboard segment, which gives us the output port
                    int outputPort = static_cast<TouchkeyKeyDivisionMappingFactory*>(factory_)->segment().outputPort();
                    
                    // Send MIDI note-on on the same channel as previously
                    int ch = keyboard_.key(noteNumber_)->midiChannel();
                    int vel = 64;
                    if(retriggerKeepsVelocity_)
                        vel = keyboard_.key(noteNumber_)->midiVelocity();
                    keyboard_.midiOutputController()->sendNoteOn(outputPort, ch, noteNumber_, vel);
                    sendSegmentMessage(detectedSegment_);
                }
            }
            
            // Save the number of active touches for next time
            lastNumActiveTouches_ = frame.count;
        }
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type TouchkeyKeyDivisionMapping::performMapping() {
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    
    if(detectedSegment_ >= 0) {
        // Found segment; no need to keep sending mapping callbacks
        nextScheduledTimestamp_ = 0;
        return 0;
    }
    
    if(currentTimestamp - midiNoteOnTimestamp_ > timeout_) {
        // Timeout occurred. Activate default segment
#ifdef DEBUG_KEY_DIVISION_MAPPING
        std::cout << "TouchkeyKeyDivisionMapping: timeout\n";
#endif
        detectedSegment_ = defaultSegment_;
        sendSegmentMessage(detectedSegment_);
        nextScheduledTimestamp_ = 0;
        return 0;
    }
    
    // Register for the next update by returning its timestamp
    nextScheduledTimestamp_ = currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

// MIDI note-on received. If we have a candidate segment, activate it as the actual segment
void TouchkeyKeyDivisionMapping::midiNoteOnReceived(int channel, int velocity) {
    midiNoteOnTimestamp_ = keyboard_.schedulerCurrentTimestamp();
    
    if(detectedSegment_ < 0) {
#ifdef DEBUG_KEY_DIVISION_MAPPING
        std::cout << "TouchkeyKeyDivisionMapping::midiNoteOnReceived(): candidateSegment_ = " << candidateSegment_ << '\n';
#endif
        detectedSegment_ = candidateSegment_;
        if(detectedSegment_ >= 0) {
            sendSegmentMessage(detectedSegment_);
        }
    }
}

// MIDI note-off received. Reset back to the detecting state so we can assign the next note to a segment
void TouchkeyKeyDivisionMapping::midiNoteOffReceived(int channel) {
    detectedSegment_ = candidateSegment_ = -1;
}

void TouchkeyKeyDivisionMapping::sendSegmentMessage(int segment, bool force) {
    if(force || !suspended_) {
        keyboard_.sendMessage("/touchkeys/keysegment", "ii", noteNumber_, segment, LO_ARGS_END);
        if(segment < segmentBends_.size() && segment >= 0) {
#ifdef DEBUG_KEY_DIVISION_MAPPING
            std::cout << "TouchkeyKeyDivisionMapping::sendSegmentMessage(): pitch bend = " << segmentBends_[segment] << '\n';
#endif
            sendPitchBendMessage(segmentBends_[segment], force);
        }
        else {
#ifdef DEBUG_KEY_DIVISION_MAPPING
            std::cout << "TouchkeyKeyDivisionMapping::sendSegmentMessage(): no bend for segment " << segment << '\n';
#endif
        }
    }
    else {
#ifdef DEBUG_KEY_DIVISION_MAPPING
        std::cout << "TouchkeyKeyDivisionMapping::sendSegmentMessage(): suspended, not sending segment " << segment << '\n';
#endif
    }
}

// Send the pitch bend message of a given number of a semitones. Send by OSC,
// which can be mapped to MIDI CC externally
void TouchkeyKeyDivisionMapping::sendPitchBendMessage(float pitchBendSemitones, bool force) {
    if(force || !suspended_)
        keyboard_.sendMessage(controlName_.c_str(), "if", noteNumber_, pitchBendSemitones, LO_ARGS_END);
}

// Find the segment corresponding to a (Y) touch location
int TouchkeyKeyDivisionMapping::segmentForLocation(float location) {
    // Divide the key into evenly-spaced regions, and identify and candidate segment.
    // Since the location can go up to 1.0, make sure the top value doesn't overflow
    // the number of segments
    int segment = floorf(location * (float)numberOfSegments_);
    if(segment >= numberOfSegments_)
        segment = numberOfSegments_ - 1;
    return segment;
}

// Find the segment corresponding to a number of touches
int TouchkeyKeyDivisionMapping::segmentForNumTouches(int numTouches) {
    // Check the number of touches, which could divide the note into as many
    // as three segments.
    if(numTouches <= 0)
        return -1;
    
    int segment = numTouches - 1;
    if(segment >= numberOfSegments_)
        segment = numberOfSegments_ - 1;
    return segment;
}

// Return the location of the most recently added touch (indicated by the highest ID)
float TouchkeyKeyDivisionMapping::locationOfNewestTouch(KeyTouchFrame const& frame) {
    if(frame.count == 0)
        return -1.0;
    
    // Go through the active touches and find the one with the highest id
    int highestId = -1;
    float location = -1.0;
    for(int i = 0; i < frame.count; i++) {
        if(frame.ids[i] > highestId) {
            highestId = frame.ids[i];
            location = frame.locs[i];
        }
    }
    
    return location;
}