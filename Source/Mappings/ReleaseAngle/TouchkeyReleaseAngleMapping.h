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

  TouchkeyReleaseAngleMapping.h: per-note mapping for the release angle
  mapping, which measures the speed of finger motion along the key at
  the time of MIDI note off.
*/
#pragma once

#include "../TouchkeyBaseMapping.h"

#define RELEASE_ANGLE_MAX_SEQUENCE_LENGTH 16

// This class handles the detection of finger motion specifically at
// note release, which can be used to trigger specific release effects.

class TouchkeyReleaseAngleMapping : public TouchkeyBaseMapping {
    friend class TouchkeyReleaseAngleMappingFactory;
    
private:
    // Default values
    /*constexpr static const int kDefaultFilterBufferLength = 30;
    constexpr static const timestamp_diff_type kDefaultMaxLookbackTime = milliseconds_to_timestamp(100);*/
    
    static constexpr int kDefaultFilterBufferLength = 30;
    static constexpr timestamp_diff_type kDefaultMaxLookbackTime = milliseconds_to_timestamp( 100 );

    static constexpr float kDefaultUpMinimumAngle = 1.0;
    static constexpr float kDefaultDownMinimumAngle = 1.0;

public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	TouchkeyReleaseAngleMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                                Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);
	
	// Copy constructor
	//TouchkeyReleaseAngleMapping(TouchkeyReleaseAngleMapping const& obj);
	
    // ***** Modifiers *****
    
    // Reset the state back initial values
    void reset();
    
    // Resend the current state of all parameters
    void resend();
    
    // Parameters for release angle algorithm
    void setWindowSize(float windowSize);
    void setUpMessagesEnabled(bool enable);
    void setDownMessagesEnabled(bool enable);
    void setUpMinimumAngle(float minAngle);
    void setUpNote(int sequence, int note);
    void setUpVelocity(int sequence, int velocity);
    void setDownMinimumAngle(float minAngle);
    void setDownNote(int sequence, int note);
    void setDownVelocity(int sequence, int velocity);
    
	// ***** Evaluators *****
    
    // This method receives triggers whenever events occur in the touch data or the
    // continuous key position (state changes only). It alters the behavior and scheduling
    // of the mapping but does not itself send OSC messages
	void triggerReceived(TriggerSource* who, timestamp_type timestamp);
	
    // This method handles the OSC message transmission. It should be run in the Scheduler
    // thread provided by PianoKeyboard.
    timestamp_type performMapping();
    
    // Called when MIDI note release happens
    void midiNoteOffReceived(int channel);

    // ***** Specific Methods *****
    // Process the release by calculating the angle
    void processRelease(/*timestamp_type timestamp*/);
    
    timestamp_type releaseKeySwitch();
    
private:
    // ***** Private Methods *****
    
    void sendReleaseAngleMessage(float releaseAngle, bool force = false);
    
	// ***** Member Variables *****
    
    bool upEnabled_, downEnabled_;          // Whether messages are enabled for upward and downward releases
    float upMinimumAngle_;                  // Minimum release angle for trigger for up...
    float downMinimumAngle_;                // ...and down cases
    int upNotes_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];       // Notes and velocities to send on upward
    int upVelocities_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];  // and downward release
    int downNotes_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];
    int downVelocities_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];
    
    Node<KeyTouchFrame> pastSamples_;           // Locations of touch
    timestamp_diff_type maxLookbackTime_;       // How long to look backwards to find release velocity
    juce::CriticalSection sampleBufferMutex_;         // Mutex to protect threaded access to sample buffer
};
