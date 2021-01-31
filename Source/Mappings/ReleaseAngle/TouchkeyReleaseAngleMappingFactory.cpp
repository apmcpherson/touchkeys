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

  TouchkeyReleaseAngleMappingFactory.cpp: factory for the release angle
  mapping, which measures the speed of finger motion along the key at
  the time of MIDI note off.
*/

#include "TouchkeyReleaseAngleMappingFactory.h"
#include "TouchkeyReleaseAngleMappingExtendedEditor.h"

// Class constants

const std::string TouchkeyReleaseAngleMappingFactory::kConfigurationNames[] = {
    "Sample Modeling Trombone",
    "Sample Modeling Trumpet"
};

TouchkeyReleaseAngleMappingFactory::TouchkeyReleaseAngleMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment)
: TouchkeyBaseMappingFactory<TouchkeyReleaseAngleMapping>(keyboard, segment),
currentConfiguration_(-1),
upEnabled_(true), downEnabled_(true),
upMinimumAngle_(TouchkeyReleaseAngleMapping::kDefaultUpMinimumAngle),
downMinimumAngle_(TouchkeyReleaseAngleMapping::kDefaultDownMinimumAngle)
{
    // Get default values from the first configuration
    setCurrentConfiguration(0);
}

// Set a particular configuration; make sure the values here match the
// defined names and indices above
void TouchkeyReleaseAngleMappingFactory::setCurrentConfiguration(int index) {
    if(index == 0) {
        // Sample Modeling Trombone
        currentConfiguration_ = 0;
        upEnabled_ = downEnabled_ = true;
        upMinimumAngle_ = 1.0;
        downMinimumAngle_ = 1.5;
        windowSizeMilliseconds_ = 100.0;
        
        clearNotes();
        
        upNotes_[0] = 36;
        upVelocities_[0] = 64;
        upNotes_[1] = 31;
        upVelocities_[1] = 96;
        
        downNotes_[0] = 36;
        downVelocities_[0] = 64;
        downNotes_[1] = 33;
        downVelocities_[1] = 80;
    }
    else if(index == 1) {
        // Sample Modeling Trumpet
        currentConfiguration_ = 1;
        upEnabled_ = downEnabled_ = true;
        upMinimumAngle_ = 1.0;
        downMinimumAngle_ = 1.5;
        windowSizeMilliseconds_ = 100.0;
        
        clearNotes();

        upNotes_[0] = 48;
        upVelocities_[0] = 64;
        upNotes_[1] = 42;
        upVelocities_[1] = 96;
        
        downNotes_[0] = 48;
        downVelocities_[0] = 64;
        downNotes_[1] = 46;
        downVelocities_[1] = 96;
    }
}

// Parameters for release angle algorithm
int TouchkeyReleaseAngleMappingFactory::getUpNote(int sequence) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return 0;
    return upNotes_[sequence];
}

int TouchkeyReleaseAngleMappingFactory::getUpVelocity(int sequence) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return 0;
    return upVelocities_[sequence];
}

int TouchkeyReleaseAngleMappingFactory::getDownNote(int sequence) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return 0;
    return downNotes_[sequence];
}

int TouchkeyReleaseAngleMappingFactory::getDownVelocity(int sequence) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return 0;
    return downVelocities_[sequence];
}

void TouchkeyReleaseAngleMappingFactory::setWindowSize(float windowSize) {
    if(windowSizeMilliseconds_ != windowSize)
        currentConfiguration_ = -1;
    windowSizeMilliseconds_ = windowSize;
}

void TouchkeyReleaseAngleMappingFactory::setUpMessagesEnabled(bool enable) {
    if(upEnabled_ != enable)
        currentConfiguration_ = -1;
    upEnabled_ = enable;
}

void TouchkeyReleaseAngleMappingFactory::setDownMessagesEnabled(bool enable) {
    if(downEnabled_ != enable)
        currentConfiguration_ = -1;
    downEnabled_ = enable;
}

void TouchkeyReleaseAngleMappingFactory::setUpMinimumAngle(float minAngle) {
    if(upMinimumAngle_ != minAngle)
        currentConfiguration_ = -1;
    upMinimumAngle_ = fabsf(minAngle);
}

void TouchkeyReleaseAngleMappingFactory::setUpNote(int sequence, int note) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return;
    if(note < 0 || note > 127)
        upNotes_[sequence] = 0;
    else {
        if(upNotes_[sequence] != note)
            currentConfiguration_ = -1;
        upNotes_[sequence] = note;
    }
}

void TouchkeyReleaseAngleMappingFactory::setUpVelocity(int sequence, int velocity) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return;
    if(velocity < 0 || velocity > 127)
        upVelocities_[sequence] = 0;
    else {
        if(upVelocities_[sequence] != velocity)
            currentConfiguration_ = -1;
        upVelocities_[sequence] = velocity;
    }
}

void TouchkeyReleaseAngleMappingFactory::setDownMinimumAngle(float minAngle) {
    if(downMinimumAngle_ != minAngle)
        currentConfiguration_ = -1;
    downMinimumAngle_ = fabsf(minAngle);
}

void TouchkeyReleaseAngleMappingFactory::setDownNote(int sequence, int note) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return;
    if(note < 0 || note > 127)
        downNotes_[sequence] = 0;
    else {
        if(downNotes_[sequence] != note)
            currentConfiguration_ = -1;
        downNotes_[sequence] = note;
    }
}

void TouchkeyReleaseAngleMappingFactory::setDownVelocity(int sequence, int velocity) {
    if(sequence < 0 || sequence >= RELEASE_ANGLE_MAX_SEQUENCE_LENGTH)
        return;
    if(velocity < 0 || velocity > 127)
        downVelocities_[sequence] = 0;
    else {
        if(downVelocities_[sequence] != velocity)
            currentConfiguration_ = -1;
        downVelocities_[sequence] = velocity;
    }
}

#ifndef TOUCHKEYS_NO_GUI
// ***** GUI Support *****

std::unique_ptr< MappingEditorComponent > TouchkeyReleaseAngleMappingFactory::createExtendedEditor() {
    return std::make_unique< TouchkeyReleaseAngleMappingExtendedEditor >(*this);
}
#endif


// ****** Preset Save/Load ******
std::unique_ptr< juce::XmlElement > TouchkeyReleaseAngleMappingFactory::getPreset() {
    juce::PropertySet properties;
    
    storeCommonProperties(properties);
    
    properties.setValue("currentConfiguration", currentConfiguration_);
    properties.setValue("upEnabled", upEnabled_);
    properties.setValue("downEnabled", downEnabled_);
    properties.setValue("upMinimumAngle", upMinimumAngle_);
    properties.setValue("downMinimumAngle", downMinimumAngle_);
    properties.setValue("windowSizeMilliseconds", windowSizeMilliseconds_);

    // TODO: set arrays of notes and velocities
    
    auto preset = properties.createXml("MappingFactory");
    preset->setAttribute("type", "ReleaseAngle");
    
    return preset;
}

bool TouchkeyReleaseAngleMappingFactory::loadPreset(juce::XmlElement const* preset) {
    if(preset == nullptr)
        return false;
    
    juce::PropertySet properties;
    properties.restoreFromXml(*preset);
    
    if(!loadCommonProperties(properties))
        return false;
    
    // First check if there's a default configuration in use
    // We can get all other parameters from that regardless of the
    // remaining contents
    if(properties.containsKey("currentConfiguration")) {
        int config = properties.getIntValue("currentConfiguration");
        if(config >= 0 && config < kNumConfigurations)
            setCurrentConfiguration(config);
    }
    else {
        if(!properties.containsKey("upEnabled") ||
           !properties.containsKey("downEnabled") ||
           !properties.containsKey("upMinimumAngle") ||
           !properties.containsKey("downMinimumAngle") ||
           !properties.containsKey("windowSizeMilliseconds"))
            return false;
        
        currentConfiguration_ = -1;
        upEnabled_ = properties.getBoolValue("upEnabled");
        downEnabled_ = properties.getBoolValue("downEnabled");
        upMinimumAngle_ = properties.getDoubleValue("upMinimumAngle");
        downMinimumAngle_ = properties.getDoubleValue("downMinimumAngle");
        windowSizeMilliseconds_ = properties.getDoubleValue("windowSizeMilliseconds");

        // TODO: load arrays of notes and velocities
    }
    
    return true;
}

// MIDI note ended: see whether the mapping was suspended and if not, execute the angle calculation
/*void TouchkeyReleaseAngleMappingFactory::midiNoteOff(int noteNumber, bool touchIsOn, bool keyMotionActive,
                                                     Node<KeyTouchFrame>* touchBuffer,
                                                     Node<key_position>* positionBuffer,
                                                     KeyPositionTracker* positionTracker) {
    if(mappings_.count(noteNumber) != 0) {
        mappings_[noteNumber]->processRelease(keyboard_.schedulerCurrentTimestamp());
    }
    
    // Call base class method
    TouchkeyBaseMappingFactory<TouchkeyReleaseAngleMapping>::midiNoteOff(noteNumber, touchIsOn, keyMotionActive, touchBuffer, positionBuffer, positionTracker);
}*/

void TouchkeyReleaseAngleMappingFactory::initializeMappingParameters(int noteNumber,
                                                                     TouchkeyReleaseAngleMapping *mapping) {
    mapping->setWindowSize(windowSizeMilliseconds_);
    mapping->setUpMessagesEnabled(upEnabled_);
    mapping->setDownMessagesEnabled(downEnabled_);
    mapping->setUpMinimumAngle(upMinimumAngle_);
    mapping->setDownMinimumAngle(downMinimumAngle_);
    
    for(int i = 0; i < RELEASE_ANGLE_MAX_SEQUENCE_LENGTH; i++) {
        mapping->setUpNote(i, upNotes_[i]);
        mapping->setUpVelocity(i, upVelocities_[i]);
        mapping->setDownNote(i, downNotes_[i]);
        mapping->setDownVelocity(i, downVelocities_[i]);
    }
}

// Reset notes and velocities to defaults
void TouchkeyReleaseAngleMappingFactory::clearNotes() {
    for(int i = 0; i < RELEASE_ANGLE_MAX_SEQUENCE_LENGTH; i++)
        upNotes_[i] = downNotes_[i] = upVelocities_[i] = downVelocities_[i] = 0;
}
