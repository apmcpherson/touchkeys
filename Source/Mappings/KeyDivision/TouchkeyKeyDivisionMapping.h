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

  TouchkeyKeyDivisionMapping.h: per-note mapping for the split-key mapping
  which triggers different actions or pitches depending on where the key
  was struck.
*/

#pragma once

#include "../TouchkeyBaseMapping.h"

// This class handles the division of a key into multiple
// subsections, for example to handle microtonal divisions

class TouchkeyKeyDivisionMapping : public TouchkeyBaseMapping {
    friend class TouchkeyKeyDivisionMappingFactory;
public:
    enum {
        kDetectionParameterYPosition = 1,
        kDetectionParameterNumberOfTouches,
        kDetectionParameterYPositionAndNumberOfTouches,
        kDetectionParameterMaxValue
    };
    
private:
    // Default values
    static const int kDefaultNumberOfSegments;
    static const timestamp_diff_type kDefaultDetectionTimeout;
    static const int kDefaultDetectionParameter;
    static const int kDefaultRetriggerNumFrames;
    
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	TouchkeyKeyDivisionMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                              Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);
	
    // ***** Modifiers *****
    
    // Reset the state back initial values
    void reset();
    
    // Resend the current state of all parameters
    void resend();
    
	// ***** Evaluators *****
    
    // This method receives triggers whenever events occur in the touch data or the
    // continuous key position (state changes only). It alters the behavior and scheduling
    // of the mapping but does not itself send OSC messages
	void triggerReceived(TriggerSource* who, timestamp_type timestamp);
	
    // This method handles the OSC message transmission. It should be run in the Scheduler
    // thread provided by PianoKeyboard.
    timestamp_type performMapping();
    
    // ***** Specific Methods *****
    
    // Set the number of segments
    void setNumberOfSegments(int segments) {
        if(segments > 0)
            numberOfSegments_ = segments;
    }
    
    // Set the default segment (upon timeout)
    void setDefaultSegment(int defaultSegment) {
        defaultSegment_ = defaultSegment;
    }
    
    // Set the pitch bend values associated with each key segment
    void setSegmentPitchBends(const float *bendsInSemitones, int numBends);
    
    // Set the detection timeout value (how long from MIDI note on to touch)
    void setTimeout(timestamp_diff_type timeout) {
        timeout_ = timeout;
    }
    
    // Set which parameter is used to detect segment
    void setDetectionParameter(int parameter) {
        detectionParameter_ = parameter;
    }
    
    // Set whether placing a second finger in the other segment triggers a
    // new note with that segment.
    void setRetriggerable(bool retrigger, int numFrames, bool keepOriginalVelocity) {
        retriggerable_ = retrigger;
        retriggerNumFrames_ = numFrames;
        retriggerKeepsVelocity_ = keepOriginalVelocity;
    }
    
private:
    // ***** Private Methods *****
    
    void midiNoteOnReceived(int channel, int velocity);
    void midiNoteOffReceived(int channel);
    
    void sendSegmentMessage(int segment, bool force = false);
    void sendPitchBendMessage(float pitchBendSemitones, bool force = false);
    
    int segmentForLocation(float location);
    int segmentForNumTouches(int numTouches);
    float locationOfNewestTouch(KeyTouchFrame const& frame);
    
	// ***** Member Variables *****
    
    int numberOfSegments_;          // How many segments to choose from total
    int candidateSegment_;          // Which segment we would be in if the press were now
    int detectedSegment_;           // Which segment of the key we detected
    int defaultSegment_;            // Which segment to choose by default (on timeout)
    int detectionParameter_;        // Which parameter is used to determine the segment
    bool retriggerable_;            // Whether a second touch can retrigger this note
    int retriggerNumFrames_;        // How many frames a new touch must be present to retrigger
    bool retriggerKeepsVelocity_;   // Whether a retriggered note keeps the original velocity or a default
    
    timestamp_type midiNoteOnTimestamp_; // When the MIDI note went on
    timestamp_diff_type timeout_;   // How long to wait for a touch event
    int lastNumActiveTouches_;                  // How many touches were active before
    
    std::vector<float> segmentBends_; // What the pitch bend values are for each segment
};
