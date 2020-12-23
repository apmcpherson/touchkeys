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

  MRPMapping.h: mapping class for magnetic resonator piano using continuous
  key position.
*/

#pragma once

#include "Mapping.h"
#include "../Utility/IIRFilter.h"

// How many velocity samples to save in the buffer. Make sure this is
// enough to cover the frequency of updates.
const int kMRPMappingVelocityBufferLength = 30;

// This class handles the mapping from key position and, optionally,
// touch information to OSC messages which control the magnetic resonator
// piano. One copy of the object is created for each active note, and
// all objects use the PianoKeyboard Scheduler facility to request timed
// updates.

class MRPMapping : public Mapping {
private:

    // Useful constants for mapping MRP messages
    static constexpr int kMIDINoteOnMessage = 0x90;
    static constexpr int kDefaultMIDIChannel = 15;
    static constexpr float kDefaultAftertouchScaler = 100.0;

    // Parameters for vibrato detection and mapping
    static constexpr key_velocity kVibratoVelocityThreshold = scale_key_velocity( 2.0 );
    static constexpr timestamp_diff_type kVibratoMinimumPeakSpacing = microseconds_to_timestamp( 60000 );
    static constexpr timestamp_diff_type kVibratoTimeout = microseconds_to_timestamp( 500000 );
    static constexpr int kVibratoMinimumOscillations = 4;
    static constexpr float kVibratoRateScaler = 0.005;

    struct PitchBend {
        int note;                               // Note number of the bending key
        bool isControllingBend;                 // True if the note in this structure
                                                // is the one controlling bend (false if it's us)
        bool isFinished;                        // True if the bend should finish after this cycle
        Node<key_position>* positionBuffer;     // Key position for bending key
        KeyPositionTracker* positionTracker;    // Key states for bending key
    };
    
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	MRPMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
              Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);
	
	// Copy constructor
	//MRPMapping(MRPMapping const& obj);
    
    // ***** Destructor *****
    
    ~MRPMapping();
	
    // ***** Modifiers *****
    
    // Disable mappings from being sent
    void disengage();
	
    // Reset the state back initial values
	void reset();
    
    // Set the aftertouch sensitivity on continuous key position
    // 0 means no aftertouch, 1 means default sensitivity, upward
    // from there
    void setAftertouchSensitivity(float sensitivity);
    
    // Engage a pitch bend from a different key, based on its position and state
    void enablePitchBend(int toNote, Node<key_position>* toPositionBuffer,
                         KeyPositionTracker *toPositionTracker);
    
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
    
    // Bring velocity calculations up to date
    key_velocity updateVelocityMeasurements();
    
    // Find the timestamp of the first transition into a PartialPress state
    timestamp_type findTimestampOfPartialPress();
    
	// ***** Member Variables *****
    
    bool noteIsOn_;                             // Whether the MIDI note is active or not
    float aftertouchScaler_;                    // Scaler which affects aftertouch sensitivity
    float lastIntensity_, lastBrightness_;      // Cached values for mapping qualities
    float lastPitch_, lastHarmonic_;
    
    bool shouldLookForPitchBends_;              // Whether to search for adjacent keys to start a pitch bend
    std::vector<PitchBend> activePitchBends_;   // Which keys are involved in a pitch bend
    
    Node<key_velocity> rawVelocity_;            // History of key velocity measurements
    IIRFilterNode<key_velocity> filteredVelocity_;  // Filtered key velocity information
    Node<key_position>::size_type lastCalculatedVelocityIndex_; // Keep track of how many velocity samples we've calculated
    
    bool vibratoActive_;                        // Whether a vibrato gesture is currently detected
    int vibratoVelocityPeakCount_;              // Counter for tracking velocity oscillations
    timestamp_type vibratoLastPeakTimestamp_;   // When the last velocity peak took place
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MRPMapping)
};

