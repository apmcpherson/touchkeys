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

  TouchkeyVibratoMapping.h: per-note mapping for the vibrato mapping class,
  which creates vibrato through side-to-side motion of the finger on the
  key surface.
*/

#pragma once

#include "../../TouchKeys/KeyTouchFrame.h"
#include "../../TouchKeys/KeyPositionTracker.h"
#include "../../TouchKeys/PianoKeyboard.h"
#include "../TouchkeyBaseMapping.h"
#include "../../Utility/IIRFilter.h"
#include <boost/bind.hpp>
#include <map>
#include <vector>
#include <climits>
#include <cmath>

// This class handles the detection and mapping of vibrato gestures
// based on Touchkey data. It outputs MIDI or OSC messages that
// can be used to affect the pitch of the active note.

class TouchkeyVibratoMapping : public TouchkeyBaseMapping {
    friend class TouchkeyVibratoMappingFactory;

private:
    // Useful constants for mapping MRP messages
    /*constexpr static const int kDefaultMIDIChannel = 0;
    constexpr static const int kDefaultFilterBufferLength = 30;
    
    constexpr static const float kDefaultVibratoThresholdX = 0.05;
    constexpr static const float kDefaultVibratoRatioX = 0.3;
    constexpr static const float kDefaultVibratoThresholdY = 0.02;
    constexpr static const float kDefaultVibratoRatioY = 0.8;
    constexpr static const timestamp_diff_type kDefaultVibratoTimeout = microseconds_to_timestamp(400000); // 0.4s
    constexpr static const float kDefaultVibratoPrescaler = 2.0;
    constexpr static const float kDefaultVibratoRangeSemitones = 1.25;
    
    constexpr static const timestamp_diff_type kZeroCrossingMinimumTime = microseconds_to_timestamp(50000); // 50ms
    constexpr static const timestamp_diff_type kMinimumOnsetTime = microseconds_to_timestamp(30000); // 30ms
    constexpr static const timestamp_diff_type kMaximumOnsetTime = microseconds_to_timestamp(300000); // 300ms
    constexpr static const timestamp_diff_type kMinimumReleaseTime = microseconds_to_timestamp(30000); // 30ms
    constexpr static const timestamp_diff_type kMaximumReleaseTime = microseconds_to_timestamp(300000); // 300ms*/
    
    static const int kDefaultMIDIChannel;
    static const int kDefaultFilterBufferLength;
    
    static const float kDefaultVibratoThresholdX;
    static const float kDefaultVibratoRatioX;
    static const float kDefaultVibratoThresholdY;
    static const float kDefaultVibratoRatioY;
    static const timestamp_diff_type kDefaultVibratoTimeout;
    static const float kDefaultVibratoPrescaler;
    static const float kDefaultVibratoRangeSemitones;
    
    static const timestamp_diff_type kZeroCrossingMinimumTime;
    static const timestamp_diff_type kMinimumOnsetTime;
    static const timestamp_diff_type kMaximumOnsetTime;
    static const timestamp_diff_type kMinimumReleaseTime;
    static const timestamp_diff_type kMaximumReleaseTime;
    
    static const float kWhiteKeySingleAxisThreshold;
    
    enum {
        kStateInactive = 0,
        kStateSwitchingOn,
        kStateActive,
        kStateSwitchingOff
    };
    
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	TouchkeyVibratoMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
               Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);

    // ***** Destructor *****
    
    ~TouchkeyVibratoMapping();
	
    // ***** Modifiers *****
    
    // Disable mappings from being sent
    void disengage(bool shouldDelete = false);
	
    // Reset the state back initial values
	void reset();
    
    // Resend the current state of all parameters
    void resend();

    // Parameters for vibrato algorithm
    //void setType(int vibratoType);
    void setRange(float rangeSemitones);
    void setPrescaler(float prescaler);
    void setThresholds(float thresholdX, float thresholdY, float ratioX, float ratioY);
    void setTimeout(timestamp_diff_type timeout);
    
	// ***** Evaluators *****
    
    // This method receives triggers whenever events occur in the touch data or the
    // continuous key position (state changes only). It alters the behavior and scheduling
    // of the mapping but does not itself send OSC messages
	void triggerReceived(TriggerSource* who, timestamp_type timestamp);
	
    // This method handles the OSC message transmission. It should be run in the Scheduler
    // thread provided by PianoKeyboard.
    timestamp_type performMapping();
    
private:
    // ***** Private Methods *****
    void midiNoteOnReceived(int channel, int velocity);
    void midiNoteOffReceived(int channel);
    
    void changeStateSwitchingOn(timestamp_type timestamp);
    void changeStateActive(timestamp_type timestamp);
    void changeStateSwitchingOff(timestamp_type timestamp);
    void changeStateInactive(timestamp_type timestamp);

    void resetDetectionState();
    void clearBuffers();
    
    bool keyIsWhite();
    
    void sendVibratoMessage(float pitchBendSemitones, bool force = false);
    
	// ***** Member Variables *****
    
    int vibratoState_;                          // Whether a vibrato gesture is currently detected
    
    timestamp_type rampBeginTime_;              // If in a switching state, when does the transition begin?
    float rampScaleValue_;                      // If in a switching state, what is the end point of the ramp?
    timestamp_diff_type rampLength_;            // If in a switching state, how long is the transition?
    float lastCalculatedRampValue_;             // Value of the ramp that was last calculated
    
    float onsetThresholdX_, onsetThresholdY_;   // Thresholds for detecting vibrato (first extremum)
    float onsetRatioX_, onsetRatioY_;           // Thresholds for detection vibrato (second extremum)
    timestamp_diff_type onsetTimeout_;          // Timeout between first and second extrema
    
    float onsetLocationX_, onsetLocationY_;     // Where the touch began at MIDI note on
    float lastX_, lastY_;                       // Where the touch was at the last frame we received
    int idOfCurrentTouch_;                      // Which touch ID we're currently following
    timestamp_type lastTimestamp_;              // When the last data point arrived
    Node<float>::size_type lastProcessedIndex_; // Index of the last filtered position sample we've handled
    
    timestamp_type lastZeroCrossingTimestamp_;  // Timestamp of the last zero crossing
    timestamp_diff_type lastZeroCrossingInterval_;   // Interval between the last two zero-crossings of filtered distance
    bool lastSampleWasPositive_;                // Whether the last sample was > 0
    
    bool foundFirstExtremum_;                   // Whether the first extremum has occurred
    float firstExtremumX_, firstExtremumY_;     // Where the first extremum occurred
    timestamp_type firstExtremumTimestamp_;     // Where the first extremum occurred
    timestamp_type lastExtremumTimestamp_;      // When the most recent extremum occurred
    
    float vibratoPrescaler_;                    // Parameter controlling prescaler before nonlinear scaling
    float vibratoRangeSemitones_;               // Amount of pitch bend in one direction at maximum
    
    float lastPitchBendSemitones_;              // The last pitch bend value we sent out
    
    Node<float> rawDistance_;                   // Distance from onset location
    IIRFilterNode<float> filteredDistance_;     // Bandpass filtered finger motion
    juce::CriticalSection distanceAccessMutex_;       // Mutex that protects the access buffer from changes
};

#pragma once