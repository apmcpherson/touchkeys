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

  MRPMapping.cpp: mapping class for magnetic resonator piano using continuous
  key position.
*/

#include "MRPMapping.h"

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
MRPMapping::MRPMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                     Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: Mapping(keyboard, factory, noteNumber, touchBuffer, positionBuffer, positionTracker),
  noteIsOn_(false), lastIntensity_(missing_value<float>::missing()),
  lastBrightness_(missing_value<float>::missing()), lastPitch_(missing_value<float>::missing()),
  lastHarmonic_(missing_value<float>::missing()),
  shouldLookForPitchBends_(true), rawVelocity_(kMRPMappingVelocityBufferLength),
  filteredVelocity_(kMRPMappingVelocityBufferLength, rawVelocity_), lastCalculatedVelocityIndex_(0),
  vibratoActive_(false), vibratoVelocityPeakCount_(0), vibratoLastPeakTimestamp_(missing_value<timestamp_type>::missing())
{
    setAftertouchSensitivity(1.0);
    
    // Initialize the filter coefficients for filtered key velocity (used for vibrato detection)
    std::vector<double> bCoeffs, aCoeffs;
    designSecondOrderLowpass(bCoeffs, aCoeffs, 15.0, 0.707, 1000.0);
    std::vector<float> bCf(bCoeffs.begin(), bCoeffs.end()), aCf(aCoeffs.begin(), aCoeffs.end());
    filteredVelocity_.setCoefficients(bCf, aCf);
}

// Copy constructor
/*MRPMapping::MRPMapping(MRPMapping const& obj)
: Mapping(obj), lastIntensity_(obj.lastIntensity_), lastBrightness_(obj.lastBrightness_),
aftertouchScaler_(obj.aftertouchScaler_), noteIsOn_(obj.noteIsOn_), lastPitch_(obj.lastPitch_),
lastHarmonic_(obj.lastHarmonic_),
shouldLookForPitchBends_(obj.shouldLookForPitchBends_), activePitchBends_(obj.activePitchBends_),
rawVelocity_(obj.rawVelocity_), filteredVelocity_(obj.filteredVelocity_),
lastCalculatedVelocityIndex_(obj.lastCalculatedVelocityIndex_), vibratoActive_(obj.vibratoActive_),
vibratoVelocityPeakCount_(obj.vibratoVelocityPeakCount_), vibratoLastPeakTimestamp_(obj.vibratoLastPeakTimestamp_) {

}*/

MRPMapping::~MRPMapping() {
    //std::cerr << "~MRPMapping(): " << this << std::endl;
    
    try {
        disengage();
    }
    catch(...) {
        std::cerr << "~MRPMapping(): exception during disengage()\n";
    }
    
    //std::cerr << "~MRPMapping(): done\n";
}

// Turn off mapping of data. Remove our callback from the scheduler
void MRPMapping::disengage() {
    Mapping::disengage();
    if(noteIsOn_) {
        int newNoteNumber = noteNumber_;
        //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
        keyboard_.sendMessage("/mrp/midi",
                              "iii", (int)(kMIDINoteOnMessage + kDefaultMIDIChannel), (int)newNoteNumber, (int)0, LO_ARGS_END);
       // if(!touchBuffer_->empty())
       //     keyboard_.testLog_ << touchBuffer_->latestTimestamp() << " /mrp/midi iii " << (kMIDINoteOnMessage + kDefaultMIDIChannel) << " " << newNoteNumber << " " << 0 << '\n';
        
        // Reset qualities
        lastPitch_ = lastHarmonic_ = lastBrightness_ = lastIntensity_ = missing_value<float>::missing();
    }
    noteIsOn_ = false;
    shouldLookForPitchBends_ = true;
}

// Reset state back to defaults
void MRPMapping::reset() {
    Mapping::reset();
    noteIsOn_ = false;
    shouldLookForPitchBends_ = true;
}

// Set the aftertouch sensitivity on continuous key position
// 0 means no aftertouch, 1 means default sensitivity, upward
// from there
void MRPMapping::setAftertouchSensitivity(float sensitivity) {
    if(sensitivity <= 0)
        aftertouchScaler_ = 0;
    else
        aftertouchScaler_ = kDefaultAftertouchScaler * sensitivity;
}

// This is called by another MRPMapping when it finds a pitch bend starting.
// Add the sending note to our list of bends, with the sending note marked
// as controlling the bend
void MRPMapping::enablePitchBend(int toNote, Node<key_position>* toPositionBuffer,
                                KeyPositionTracker *toPositionTracker) {
    if(toPositionBuffer == 0 || toPositionTracker == 0)
        return;
    
    std::cout << "enablePitchBend(): this note = " << noteNumber_ << " note = " << toNote << " posBuf = " << toPositionBuffer << " posTrack = " << toPositionTracker << "\n";
    PitchBend newBend = {toNote, true, false, toPositionBuffer, toPositionTracker};
    activePitchBends_.push_back(newBend);
}

// Trigger method. This receives updates from the TouchKey data or from state changes in
// the continuous key position (KeyPositionTracker). It will potentially change the scheduled
// behavior of future mapping calls, but the actual OSC messages should be transmitted in a different
// thread.
void MRPMapping::triggerReceived(TriggerSource* who, timestamp_type timestamp) {
    if(who == nullptr)
        return;
    if(who == positionTracker_) {
        // The state of the key (based on continuous position) just changed.
        // Might want to alter our mapping strategy.
    }
    else if(who == touchBuffer_) {
        // TouchKey data is available
    }
}

// Mapping method. This actually does the real work of sending OSC data in response to the
// latest information from the touch sensors or continuous key angle
timestamp_type MRPMapping::performMapping() {
    if(!engaged_)
        return 0;
    
    timestamp_type currentTimestamp = keyboard_.schedulerCurrentTimestamp();
    float intensity = 0;
    float brightness = 0;
    float pitch = 0;
    float harmonic = 0;

    // Calculate the output features as a function of input sensor data
    if(positionBuffer_ == nullptr) {
        // No buffer -> all 0
    }
    else if(positionBuffer_->empty()) {
        // No samples -> all 0
    }
    else {
        // TODO: IIR filter on the position data before mapping it
        key_position latestPosition = positionBuffer_->latest();
        int trackerState = kPositionTrackerStateUnknown;
        if(positionTracker_ != 0)
            trackerState = positionTracker_->currentState();
        
        // Get the latest velocity measurements
        key_velocity latestVelocity = updateVelocityMeasurements();
        
        // Every time we enter a state of PartialPress, check whether this key
        // is part of a multi-key pitch bend gesture with another key that's already
        // down. Only do this once, though, since keys that go down after we enter
        // PartialPress state are not part of such a gesture.
        if(shouldLookForPitchBends_) {
            if(trackerState == kPositionTrackerStatePartialPressAwaitingMax ||
               trackerState == kPositionTrackerStatePartialPressFoundMax) {
                // Look for a pitch bend gesture by searching for neighboring
                // keys which are in the Down state and reached that state before
                // this one reached PartialPress state.
                for(int neighborNote = noteNumber_ - 2; neighborNote < noteNumber_; neighborNote++) {
                    // If one of the lower keys is in the Down state, then this note should bend it up
                    MRPMapping *neighborMapper = dynamic_cast<MRPMapping*>(keyboard_.mapping(neighborNote));
                    if(neighborMapper == nullptr)
                        continue;
                    if(neighborMapper->positionTracker_ != 0) {
                        int neighborState = neighborMapper->positionTracker_->currentState();
                        if(neighborState == kPositionTrackerStateDown) {
                            // Here we've found a neighboring note in the Down state. But did it precede our transition?
                            timestamp_type timeOfDownTransition = neighborMapper->positionTracker_->latestTimestamp();
                            timestamp_type timeOfOurPartialActivation = findTimestampOfPartialPress();
                            
                            std::cout << "Found key " << neighborNote << " in Down state\n";
                            
                            if(!missing_value<timestamp_type>::isMissing(timeOfOurPartialActivation)) {
                                if(timeOfOurPartialActivation > timeOfDownTransition) {
                                    // The neighbor note went down before us; pitch bend should engage
                                    std::cout << "Found pitch bend: " << noteNumber_ << " to " << neighborNote << '\n';
                                    
                                    // Insert the details for the neighboring note into our buffer. The bend
                                    // is controlled by our own key, and the target is the neighbor note.
                                    PitchBend newBend = {neighborNote, false, false, neighborMapper->positionBuffer_,
                                        neighborMapper->positionTracker_};
                                    activePitchBends_.push_back(newBend);
                                
                                    // Tell the other note to bend its pitch based on our position
                                    neighborMapper->enablePitchBend(noteNumber_, positionBuffer_, positionTracker_);
                                }
                            }
                        }
                    }
                }
                for(int neighborNote = noteNumber_ + 1; neighborNote < noteNumber_ + 3; neighborNote++) {
                    // If one of the upper keys is in the Down state, then this note should bend it down
                    MRPMapping *neighborMapper = dynamic_cast<MRPMapping*>(keyboard_.mapping(neighborNote));
                    if(neighborMapper == nullptr)
                        continue;
                    if(neighborMapper->positionTracker_ != 0) {
                        int neighborState = neighborMapper->positionTracker_->currentState();
                        if(neighborState == kPositionTrackerStateDown) {
                            // Here we've found a neighboring note in the Down state. But did it precede our transition?
                            timestamp_type timeOfDownTransition = neighborMapper->positionTracker_->latestTimestamp();
                            timestamp_type timeOfOurPartialActivation = findTimestampOfPartialPress();
                            
                            std::cout << "Found key " << neighborNote << " in Down state\n";
                            
                            if(!missing_value<timestamp_type>::isMissing(timeOfOurPartialActivation)) {
                                if(timeOfOurPartialActivation > timeOfDownTransition) {
                                    // The neighbor note went down before us; pitch bend should engage
                                    std::cout << "Found pitch bend: " << noteNumber_ << " to " << neighborNote << '\n';
                                    
                                    // Insert the details for the neighboring note into our buffer. The bend
                                    // is controlled by our own key, and the target is the neighbor note.
                                    PitchBend newBend = {neighborNote, false, false, neighborMapper->positionBuffer_,
                                        neighborMapper->positionTracker_};
                                    activePitchBends_.push_back(newBend);
                                    
                                    // Tell the other note to bend its pitch based on our position
                                    neighborMapper->enablePitchBend(noteNumber_, positionBuffer_, positionTracker_);
                                }
                            }
                        }
                    }
                }
                
                shouldLookForPitchBends_ = false;
            }
        }
        
        if(trackerState == kPositionTrackerStatePartialPressAwaitingMax ||
           trackerState == kPositionTrackerStatePartialPressFoundMax) {
            // Look for active vibrato gestures which are defined as oscillating
            // motion in the key velocity. They could conceivably occur at a variety
            // of raw key positions, as long as the key is not yet down
            
            if(missing_value<timestamp_type>::isMissing(vibratoLastPeakTimestamp_))
                vibratoLastPeakTimestamp_ = currentTimestamp;
            
            if(vibratoVelocityPeakCount_ % 2 == 0) {
                if(latestVelocity > kVibratoVelocityThreshold && currentTimestamp - vibratoLastPeakTimestamp_ > kVibratoMinimumPeakSpacing) {
                    std::cout << "Vibrato count = " << vibratoVelocityPeakCount_ << std::endl;
                    vibratoVelocityPeakCount_++;
                    vibratoLastPeakTimestamp_ = currentTimestamp;
                }
            }
            else {
                if(latestVelocity < -kVibratoVelocityThreshold && currentTimestamp - vibratoLastPeakTimestamp_ > kVibratoMinimumPeakSpacing) {
                    std::cout << "Vibrato count = " << vibratoVelocityPeakCount_ << std::endl;
                    vibratoVelocityPeakCount_++;
                    vibratoLastPeakTimestamp_ = currentTimestamp;
                }
            }
            
            if(vibratoVelocityPeakCount_ >= kVibratoMinimumOscillations) {
                vibratoActive_ = true;
            }
            
            
            if(vibratoActive_) {
                // Update the harmonic parameter, which increases linearly with the absolute
                // value of velocity. The value will accumulate over the course of a vibrato
                // gesture and retain its value when the vibrato finishes. It reverts to minimum
                // when the note finishes.
                if(missing_value<float>::isMissing(lastHarmonic_))
                    lastHarmonic_ = 0.0;
                harmonic = lastHarmonic_ + fabsf(latestVelocity) * kVibratoRateScaler;
                std::cout << "harmonic = " << harmonic << std::endl;
                
                // Check whether the current vibrato has timed out
                if(currentTimestamp - vibratoLastPeakTimestamp_ > kVibratoTimeout) {
                    std::cout << "Vibrato timed out\n";
                    vibratoActive_ = false;
                    vibratoVelocityPeakCount_ = 0;
                    vibratoLastPeakTimestamp_ = currentTimestamp;
                }
            }
        }
        else {
            // Vibrato can't be active in these states
            //std::cout << "Vibrato finished from state change\n";
            vibratoActive_ = false;
            vibratoVelocityPeakCount_ = 0;
            vibratoLastPeakTimestamp_ = currentTimestamp;
        }
        
        if(trackerState != kPositionTrackerStateReleaseFinished) {
            // For all active states except post-release, calculate
            // Intensity and Brightness parameters based on key position
            
            if(latestPosition > 1.0) {
                intensity = 1.0;
                brightness = (latestPosition - 1.0) * aftertouchScaler_;
            }
            else if(latestPosition < 0.0) {
                intensity = 0.0;
                brightness = 0.0;
            }
            else {
                intensity = latestPosition;
                brightness = 0.0;
            }
            
            if(!activePitchBends_.empty()) {
                // Look for active multi-key pitch bend gestures
                pitch = 0.0;
                
                for(auto it = activePitchBends_.begin(); it != activePitchBends_.end(); it++) {
                    PitchBend& bend(*it);

                    if(bend.isControllingBend) {
                        // First find out of the bending key is still in a PartialPress state
                        // If not, remove it and move on
                        if((bend.positionTracker->currentState() != kPositionTrackerStatePartialPressAwaitingMax &&
                           bend.positionTracker->currentState() != kPositionTrackerStatePartialPressFoundMax)
                           || !bend.positionTracker->engaged()) {
                            std::cout << "Removing bend from note " << bend.note << '\n';
                            bend.isFinished = true;
                            continue;
                        }
                        
                        // This is the case where the other note is controlling our pitch
                        if(bend.positionBuffer->empty()) {
                            continue;
                        }
                        
                        float noteDifference = (float)(bend.note - noteNumber_);
                        key_position latestBenderPosition = bend.positionBuffer->latest();
                        
                        // Key position at 0 = 0 pitch bend; key position at max = most pitch bend
                        float bendAmount = key_position_to_float(latestBenderPosition - kPianoKeyDefaultIdlePositionThreshold*2) /
                                                key_position_to_float(1.0 - kPianoKeyDefaultIdlePositionThreshold*2);
                        if(bendAmount < 0)
                            bendAmount = 0;
                        pitch += noteDifference * bendAmount;
                    }
                    else {
                        // This is the case where we're controlling the other note's pitch. Our own
                        // pitch is the inverse of what we're sending to the neighboring note.
                        // Compared to the above case, we know a few things since we're using our own
                        // position: the buffer isn't empty and the tracker is engaged.
                        
                        if(trackerState != kPositionTrackerStatePartialPressAwaitingMax &&
                           trackerState != kPositionTrackerStatePartialPressFoundMax) {
                            std::cout << "Removing our bend on note " << bend.note << '\n';
                            bend.isFinished = true;
                            continue;
                        }
  
                        float noteDifference = (float)(bend.note - noteNumber_);
                        
                        // Key position at 0 = 0 pitch bend; key position at max = most pitch bend
                        float bendAmount = key_position_to_float(latestPosition - kPianoKeyDefaultIdlePositionThreshold*2) /
                                            key_position_to_float(1.0 - kPianoKeyDefaultIdlePositionThreshold*2);
                        if(bendAmount < 0)
                            bendAmount = 0;
                        pitch += noteDifference * (1.0f - bendAmount);
                    }
                }
                
                // Now reiterate to remove any of them that have finished
                auto it = activePitchBends_.begin();
                
                while(it != activePitchBends_.end()) {
                    if(it->isFinished) {
                        // Go back to beginning and look again after erasing each one
                        // This isn't very efficient but there will never be more than 4 elements anyway
                        activePitchBends_.erase(it);
                        it = activePitchBends_.begin();
                    }
                    else
                        it++;
                }
                
                std::cout << "pitch = " << pitch << std::endl;
            }
            else
                pitch = 0.0;
        }
        else {
            intensity = 0.0;
            brightness = 0.0;
            if(noteIsOn_) {
                        int newNoteNumber = noteNumber_;
                //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
                keyboard_.sendMessage("/mrp/midi",
                                      "iii", (int)(kMIDINoteOnMessage + kDefaultMIDIChannel), (int)newNoteNumber, (int)0, LO_ARGS_END);
                //keyboard_.testLog_ << currentTimestamp << " /mrp/midi iii " << (kMIDINoteOnMessage + kDefaultMIDIChannel) << " " << newNoteNumber << " " << 0 << '\n';
            }
            noteIsOn_ = false;
            shouldLookForPitchBends_ = true;
        }
    }
    
    // TODO: TouchKeys mapping
    
    // Send OSC message with these parameters unless they are unchanged from before
    if(!noteIsOn_ && intensity > 0.0) {
                int newNoteNumber = noteNumber_;
        //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
        keyboard_.sendMessage("/mrp/midi",
                              "iii", (int)(kMIDINoteOnMessage + kDefaultMIDIChannel), (int)newNoteNumber, (int)127, LO_ARGS_END);
        //keyboard_.testLog_ << currentTimestamp << " /mrp/midi iii " << (kMIDINoteOnMessage + kDefaultMIDIChannel) << " " << newNoteNumber << " " << 127 << '\n';
        noteIsOn_ = true;
    }
    
    // Set key LED color according to key parameters
    // Partial press --> green of varying intensity
    // Aftertouch (brightness) --> green moving to red depending on brightness parameter
    // Pitch bend --> note bends toward blue as pitch value departs from center
    // Harmonic glissando --> cycle through hues with whitish tint (lower saturation)
    if(intensity != lastIntensity_ || brightness != lastBrightness_ || pitch != lastPitch_ || harmonic != lastHarmonic_) {
        if(harmonic != 0.0) {
            float hue = fmodf(harmonic, 1.0);
            keyboard_.setKeyLEDColorHSV(noteNumber_, hue, 0.25, 0.5);
        }
        else if(intensity >= 1.0) {
            if(pitch != 0.0)
                keyboard_.setKeyLEDColorHSV(noteNumber_, 0.33 + 0.33 * fabsf(pitch) - (brightness * 0.2), 1.0, intensity);
            else
                keyboard_.setKeyLEDColorHSV(noteNumber_, 0.33 - (brightness * 0.2), 1.0, 1.0);
        }
        else {
            if(pitch != 0.0)
                keyboard_.setKeyLEDColorHSV(noteNumber_, 0.33 + 0.33 * fabsf(pitch), 1.0, intensity);
            else
                keyboard_.setKeyLEDColorHSV(noteNumber_, 0.33, 1.0, intensity);
        }
    }
        
    if(intensity != lastIntensity_) {
                int newNoteNumber = noteNumber_;
        //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
        keyboard_.sendMessage("/mrp/quality/intensity",
                              "iif", (int)kDefaultMIDIChannel, (int)newNoteNumber, (float)intensity, LO_ARGS_END);
        //keyboard_.testLog_ << currentTimestamp << " /mrp/quality/intensity iif " << kDefaultMIDIChannel << " " << newNoteNumber << " " << intensity << '\n';
    }
    if(brightness != lastBrightness_) {
                int newNoteNumber = noteNumber_;
        //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
        keyboard_.sendMessage("/mrp/quality/brightness",
                              "iif", (int)kDefaultMIDIChannel, (int)newNoteNumber, (float)brightness, LO_ARGS_END);
        //keyboard_.testLog_ << currentTimestamp << " /mrp/quality/brightness iif " << kDefaultMIDIChannel << " " << newNoteNumber << " " << brightness << '\n';
    }
    if(pitch != lastPitch_) {
                int newNoteNumber = noteNumber_;
        //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
        keyboard_.sendMessage("/mrp/quality/pitch",
                              "iif", (int)kDefaultMIDIChannel, (int)newNoteNumber, (float)pitch, LO_ARGS_END);
        //keyboard_.testLog_ << currentTimestamp << " /mrp/quality/pitch iif " << kDefaultMIDIChannel << " " << newNoteNumber << " " << pitch << '\n';
    }
    if(harmonic != lastHarmonic_) {
        int newNoteNumber = noteNumber_;
        //int newNoteNumber = ((noteNumber_ - 21) * 25)%88 + 21;
        keyboard_.sendMessage("/mrp/quality/harmonic",
                              "iif", (int)kDefaultMIDIChannel, (int)newNoteNumber, (float)harmonic, LO_ARGS_END);
        //keyboard_.testLog_ << currentTimestamp << " /mrp/quality/harmonic iif " << kDefaultMIDIChannel << " " << newNoteNumber << " " << harmonic << '\n';
    }
    
    lastIntensity_ = intensity;
    lastBrightness_ = brightness;
    lastPitch_ = pitch;
    lastHarmonic_ = harmonic;
    
    // Register for the next update by returning its timestamp
    nextScheduledTimestamp_ = currentTimestamp + updateInterval_;
    return nextScheduledTimestamp_;
}

// Helper function that brings the velocity buffer up to date with the latest
// samples. Velocity is not updated on every new position sample since it's not
// efficient to run that many triggers all the time. Instead, it's brought up to
// date on an as-needed basis during performMapping().
key_velocity MRPMapping::updateVelocityMeasurements() {
    positionBuffer_->lock_mutex();
    
    // Need at least 2 samples to calculate velocity (first difference)
    if(positionBuffer_->size() < 2) {
        positionBuffer_->unlock_mutex();
        return missing_value<key_velocity>::missing();
    }
    
    if(lastCalculatedVelocityIndex_ < positionBuffer_->beginIndex() + 1) {
        // Fell off the beginning of the position buffer. Reset calculations.
        filteredVelocity_.clear();
        rawVelocity_.clear();
        lastCalculatedVelocityIndex_ = positionBuffer_->beginIndex() + 1;
    }
    
    while(lastCalculatedVelocityIndex_ < positionBuffer_->endIndex()) {
        // Calculate the velocity and add to buffer
        key_position diffPosition = (*positionBuffer_)[lastCalculatedVelocityIndex_] - (*positionBuffer_)[lastCalculatedVelocityIndex_ - 1];
        timestamp_diff_type diffTimestamp = positionBuffer_->timestampAt(lastCalculatedVelocityIndex_) - positionBuffer_->timestampAt(lastCalculatedVelocityIndex_ - 1);
        key_velocity vel;
        
        if(diffTimestamp != 0)
            vel = calculate_key_velocity(diffPosition, diffTimestamp);
        else
            vel = 0; // Bad measurement: replace with 0 so as not to mess up IIR calculations
        
        // Add the raw velocity to the buffer
        rawVelocity_.insert(vel, positionBuffer_->timestampAt(lastCalculatedVelocityIndex_));
        lastCalculatedVelocityIndex_++;
    }
    
    positionBuffer_->unlock_mutex();
    
    // Bring the filtered velocity up to date
    key_velocity filteredVel = filteredVelocity_.calculate();
    //std::cout << "Key " << noteNumber_ << " velocity " << filteredVel << std::endl;
    return filteredVel;
}

// Helper function that locates the timestamp at which this key entered the
// PartialPress (i.e. first non-idle) state. Returns missing value if the
// state can't be located.
timestamp_type MRPMapping::findTimestampOfPartialPress() {
    if(positionTracker_ == nullptr)
        return missing_value<timestamp_type>::missing();
    if(positionTracker_->empty())
        return missing_value<timestamp_type>::missing();
    //Node<int>::reverse_iterator it = positionTracker_->rbegin();
    Node<int>::size_type index = positionTracker_->endIndex() - 1;
    bool foundPartialPressState = false;
    timestamp_type earliestPartialPressTimestamp;

    // Search backwards from present
    while(index >= positionTracker_->beginIndex()/*it != positionTracker_->rend()*/) {
        if((*positionTracker_)[index].state == kPositionTrackerStatePartialPressAwaitingMax ||
           (*positionTracker_)[index].state == kPositionTrackerStatePartialPressFoundMax) {
            std::cout << "index " << index << " state " << (*positionTracker_)[index].state << '\n';
            foundPartialPressState = true;
            earliestPartialPressTimestamp = positionTracker_->timestampAt(index);
        }
        else {
            // This state is not a PartialPress state. Two cases: either
            // we haven't yet encountered a partial press or we have found
            // a state before the partial press, in which case the previous
            // state we found was the first.
                        std::cout << "index " << index << " state " << (*positionTracker_)[index].state << '\n';
            if(foundPartialPressState) {
                return earliestPartialPressTimestamp;
            }
        }
        
        // Step backwards one sample, but stop if we hit the beginning index
        if(index == 0)
            break;
        index--;
    }
    
    if(foundPartialPressState)
        return earliestPartialPressTimestamp;
    
    // Didn't find anything if we get here
    return missing_value<timestamp_type>::missing();
}