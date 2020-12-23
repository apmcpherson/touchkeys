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

  MIDIKeyPositionMapping.h: handles generating MIDI data out of continuous
  key position.
*/

#pragma once

#include <map>
#include <boost/bind.hpp>
#include "../TouchKeys/KeyTouchFrame.h"
#include "../TouchKeys/KeyPositionTracker.h"
#include "../TouchKeys/PianoKeyboard.h"
#include "Mapping.h"

// This class handles the mapping from continuous key position to
// MIDI messages: note on, note off, aftertouch.

class MIDIKeyPositionMapping : public Mapping {
private:
    static constexpr int kDefaultMIDIChannel = 0;
    static constexpr float kDefaultAftertouchScaler = 127.0f / 0.03f;   // Default aftertouch sensitivity: MIDI 127 = 0.03
    static constexpr float kMinimumAftertouchPosition = 0.99f;         // Position at which aftertouch messages start
    static constexpr float kDefaultPercussivenessScaler = 1.0f / 300.0f; // Default scaler from percussiveness feature to MIDI
    static constexpr key_velocity kPianoKeyVelocityForMaxMIDI = scale_key_velocity( 40.0 );           // Press velocity for MIDI 127
    static constexpr key_velocity kPianoKeyReleaseVelocityForMaxMIDI = scale_key_velocity( -50.0 );   // Release velocity for MIDI 127

public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	MIDIKeyPositionMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
               Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);
	
	// Copy constructor
	MIDIKeyPositionMapping(MIDIKeyPositionMapping const& obj);
    
    // ***** Destructor *****
    
    ~MIDIKeyPositionMapping();
	
    // ***** Modifiers *****
    
    // Disable mappings from being sent
    void disengage();
	
    // Reset the state back initial values
	void reset();
    
    // Set the aftertouch sensitivity on continuous key position
    // 0 means no aftertouch, 1 means default sensitivity, upward
    // from there
    void setAftertouchSensitivity(float sensitivity);
    
    // Get or set the MIDI channel (0-15)
    int midiChannel() { return midiChannel_; }
    void setMIDIChannel(int ch) {
        if(ch >= 0 && ch < 16)
            midiChannel_ = ch;
    }
    
    // Get or set the MIDI channel for percussiveness messages
    int percussivenessMIDIChannel() { return midiPercussivenessChannel_; }
    void setPercussivenessMIDIChannel(int ch) {
        if(ch >= 0 && ch < 16)
            midiPercussivenessChannel_ = ch;
        else
            midiPercussivenessChannel_ = -1;
    }
    void disableMIDIPercussiveness() { midiPercussivenessChannel_ = -1; }
    
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

    void generateMidiNoteOn();
    void generateMidiNoteOff();
    void generateMidiPercussivenessNoteOn();
    
	// ***** Member Variables *****
    
    bool noteIsOn_;                             // Whether the MIDI note is active or not
    float aftertouchScaler_;                    // Scaler which affects aftertouch sensitivity
    int midiChannel_;                           // Channel on which to transmit MIDI messages
    int lastAftertouchValue_;                   // Value of the last aftertouch message
    int midiPercussivenessChannel_;             // Whether and where to transmit percussiveness messages
};
