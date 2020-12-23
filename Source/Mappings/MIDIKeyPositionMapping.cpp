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

  MIDIKeyPositionMapping.cpp: handles generating MIDI data out of continuous
  key position.
*/

#include "MIDIKeyPositionMapping.h"
#include "../TouchKeys/MidiOutputController.h"

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
MIDIKeyPositionMapping::MIDIKeyPositionMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                       Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: Mapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker), noteIsOn_(false),
  midiChannel_(kDefaultMIDIChannel), lastAftertouchValue_(0), midiPercussivenessChannel_(-1)
{
    setAftertouchSensitivity(1.0);
}

// Copy constructor
MIDIKeyPositionMapping::MIDIKeyPositionMapping(MIDIKeyPositionMapping const& obj)
: Mapping(obj), noteIsOn_(obj.noteIsOn_), aftertouchScaler_(obj.aftertouchScaler_), 
  midiChannel_(obj.midiChannel_), lastAftertouchValue_(obj.lastAftertouchValue_),
  midiPercussivenessChannel_(obj.midiPercussivenessChannel_)
{
    
}

MIDIKeyPositionMapping::~MIDIKeyPositionMapping() {
    try {
        disengage();
    }
    catch(...) {
        std::cerr << "~MIDIKeyPositionMapping(): exception during disengage()\n";
    }
}

// Turn off mapping of data. Remove our callback from the scheduler
void MIDIKeyPositionMapping::disengage() {
    Mapping::disengage();
    if(noteIsOn_) {
        generateMidiNoteOff();
    }
    noteIsOn_ = false;
}

// Reset state back to defaults
void MIDIKeyPositionMapping::reset() {
    Mapping::reset();
    noteIsOn_ = false;
}

// Set the aftertouch sensitivity on continuous key position
// 0 means no aftertouch, 1 means default sensitivity, upward
// from there
void MIDIKeyPositionMapping::setAftertouchSensitivity(float sensitivity) {
    if(sensitivity <= 0)
        aftertouchScaler_ = 0;
    else
        aftertouchScaler_ = kDefaultAftertouchScaler * sensitivity;
}

// Trigger method. This receives updates from the TouchKey data or from state changes in
// the continuous key position (KeyPositionTracker). It will potentially change the scheduled
// behavior of future mapping calls, but the actual OSC messages should be transmitted in a different
// thread.
void MIDIKeyPositionMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(who == 0)
        return;
    if(who == positionTracker_) {
        if(!positionTracker_->empty()) {
            KeyPositionTrackerNotification notification = positionTracker_->latest();
            
            // New message from the key position tracker. Might be time to start or end MIDI note.
            if(notification.type == KeyPositionTrackerNotification::kNotificationTypeFeatureAvailableVelocity && !noteIsOn_) {
                std::cout << "Key " << noteNumber_ << " velocity available\n";
                generateMidiNoteOn();
                noteIsOn_ = true;
            }
            else if(notification.type == KeyPositionTrackerNotification::kNotificationTypeFeatureAvailableReleaseVelocity && noteIsOn_) {
                std::cout << "Key " << noteNumber_ << " release velocity available\n";
                generateMidiNoteOff();
                noteIsOn_ = false;
            }
            else if(notification.type == KeyPositionTrackerNotification::kNotificationTypeFeatureAvailablePercussiveness) {
                std::cout << "Key " << noteNumber_ << " percussiveness available\n";
                generateMidiPercussivenessNoteOn();
            }
        }
    }
    else if(who == touchBuffer_) {
        // TODO: New touch data is available from the keyboard
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type MIDIKeyPositionMapping::performMapping() {
    if(!engaged_)
        return 0;
    
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();

    // Calculate the output features as a function of input sensor data
    if(positionBuffer_ == 0) {
        // No buffer -> all 0
    }
    else if(positionBuffer_->empty()) {
        // No samples -> all 0
    }
    else if(noteIsOn_) {
        // Generate aftertouch messages based on key position, if the note is on and
        // if the position exceeds the aftertouch threshold. Note on and note off are
        // handled directly by the trigger thread.
        key_position latestPosition = positionBuffer_->latest();
        int aftertouchValue;
        
        if(latestPosition < kMinimumAftertouchPosition)
            aftertouchValue = 0;
        else {
            aftertouchValue = (int)((key_position_to_float(latestPosition) - kMinimumAftertouchPosition) * aftertouchScaler_);
            if(aftertouchValue < 0)
                aftertouchValue = 0;
            if(aftertouchValue > 127)
                aftertouchValue = 127;
        }
        
        if(aftertouchValue != lastAftertouchValue_) {
            if(keyboard_.midiOutputController() != 0) {
                keyboard_.midiOutputController()->sendAftertouchPoly(0, midiChannel_, noteNumber_, aftertouchValue);
            }
        }
        
        lastAftertouchValue_ = aftertouchValue;
    }

    // Register for the next update by returning its timestamp
    nextScheduledTimestamp_ = currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

// Generate a MIDI Note On from continuous key data
void MIDIKeyPositionMapping::generateMidiNoteOn() {
    if(positionTracker_ == 0)
        return;
    
    std::pair<timestamp_type, key_velocity> velocityInfo = positionTracker_->pressVelocity();
    
    // MIDI Velocity now available. Send a MIDI message if relevant.
    if(keyboard_.midiOutputController() != 0) {
        float midiVelocity = 0.5;
        if(!missing_value<key_velocity>::isMissing(velocityInfo.second))
            midiVelocity = (float)velocityInfo.second / (float)kPianoKeyVelocityForMaxMIDI;
        if(midiVelocity < 0.0)
            midiVelocity = 0.0;
        if(midiVelocity > 1.0)
            midiVelocity = 1.0;
        keyboard_.midiOutputController()->sendNoteOn(0, midiChannel_, noteNumber_, (unsigned char)(midiVelocity * 127.0));
    }
}

// Generate a MIDI Note Off from continuous key data
void MIDIKeyPositionMapping::generateMidiNoteOff() {
    if(positionTracker_ == 0)
        return;
    
    std::pair<timestamp_type, key_velocity> velocityInfo = positionTracker_->releaseVelocity();
    
    // MIDI release velocity now available. Send a MIDI message if relevant
    if(keyboard_.midiOutputController() != 0) {
        float midiReleaseVelocity = 0.5;
        if(!missing_value<key_velocity>::isMissing(velocityInfo.second))
            midiReleaseVelocity = (float)velocityInfo.second / (float)kPianoKeyReleaseVelocityForMaxMIDI;
        if(midiReleaseVelocity < 0.0)
            midiReleaseVelocity = 0.0;
        if(midiReleaseVelocity > 1.0)
            midiReleaseVelocity = 1.0;
        keyboard_.midiOutputController()->sendNoteOff(0, midiChannel_, noteNumber_, (unsigned char)(midiReleaseVelocity * 127.0));
        
        // Also turn off percussiveness note if enabled
        if(midiPercussivenessChannel_ >= 0)
            keyboard_.midiOutputController()->sendNoteOff(0, midiPercussivenessChannel_, noteNumber_, (unsigned char)(midiReleaseVelocity * 127.0));
    }
    
    lastAftertouchValue_ = 0;
}

void MIDIKeyPositionMapping::generateMidiPercussivenessNoteOn() {
    if(positionTracker_ == 0 || midiPercussivenessChannel_ < 0)
        return;
    
    KeyPositionTracker::PercussivenessFeatures features = positionTracker_->pressPercussiveness();
    std::cout << "found percussiveness value of " << features.percussiveness << std::endl;
    
    // MIDI Velocity now available. Send a MIDI message if relevant.
    if(keyboard_.midiOutputController() != 0) {
        float midiPercVelocity = 0.0;
        if(!missing_value<key_velocity>::isMissing(features.percussiveness))
            midiPercVelocity = features.percussiveness * kDefaultPercussivenessScaler;
        if(midiPercVelocity < 0.0)
            midiPercVelocity = 0.0;
        if(midiPercVelocity > 1.0)
            midiPercVelocity = 1.0;
        keyboard_.midiOutputController()->sendNoteOn(0, midiPercussivenessChannel_, noteNumber_, (unsigned char)(midiPercVelocity * 127.0));
    }
}