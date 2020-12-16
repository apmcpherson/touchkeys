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

  TouchkeyMultiFingerTriggerMappingFactory.cpp: factory for the multiple-
  finger trigger mapping, which performs actions when two or more fingers
  are added or removed from the key.
*/

#include "TouchkeyMultiFingerTriggerMappingFactory.h"

TouchkeyMultiFingerTriggerMappingFactory::TouchkeyMultiFingerTriggerMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment)
: TouchkeyBaseMappingFactory<TouchkeyMultiFingerTriggerMapping>(keyboard, segment),
numTouchesForTrigger_(TouchkeyMultiFingerTriggerMapping::kDefaultNumTouchesForTrigger),
numFramesForTrigger_(TouchkeyMultiFingerTriggerMapping::kDefaultNumFramesForTrigger),
numConsecutiveTapsForTrigger_(TouchkeyMultiFingerTriggerMapping::kDefaultNumConsecutiveTapsForTrigger),
maxTapSpacing_(TouchkeyMultiFingerTriggerMapping::kDefaultMaxTapSpacing),
needsMidiNoteOn_(true),
triggerOnAction_(TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOnAction),
triggerOffAction_(TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOffAction),
triggerOnNoteNum_(TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOnNoteNum),
triggerOffNoteNum_(TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOffNoteNum),
triggerOnNoteVel_(TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOnNoteVel),
triggerOffNoteVel_(TouchkeyMultiFingerTriggerMapping::kDefaultTriggerOffNoteVel)
{
    
}

void TouchkeyMultiFingerTriggerMappingFactory::setTouchesForTrigger(int touches) {
    if(touches < 1)
        touches = 1;
    if(touches > 3)
        touches = 3;
    numTouchesForTrigger_ = touches;
}

void TouchkeyMultiFingerTriggerMappingFactory::setFramesForTrigger(int frames) {
    if(frames < 1)
        frames = 1;
    numFramesForTrigger_ = frames;
}

void TouchkeyMultiFingerTriggerMappingFactory::setConsecutiveTapsForTrigger(int taps) {
    if(taps < 1)
        taps = 1;
    numConsecutiveTapsForTrigger_ = taps;
}

void TouchkeyMultiFingerTriggerMappingFactory::setMaxTimeBetweenTapsForTrigger(timestamp_diff_type timeDiff) {
    if(timeDiff < 0)
        timeDiff = 0;
    maxTapSpacing_ = timeDiff;
}

void TouchkeyMultiFingerTriggerMappingFactory::setNeedsMidiNoteOn(bool needsMidi) {
    needsMidiNoteOn_ = needsMidi;
}

void TouchkeyMultiFingerTriggerMappingFactory::setTriggerOnAction(int action) {
    if(action > 0 && action < TouchkeyMultiFingerTriggerMapping::kActionMax)
        triggerOnAction_ = action;
}

void TouchkeyMultiFingerTriggerMappingFactory::setTriggerOffAction(int action) {
    if(action > 0 && action < TouchkeyMultiFingerTriggerMapping::kActionMax)
        triggerOffAction_ = action;
}

void TouchkeyMultiFingerTriggerMappingFactory::setTriggerOnNoteNumber(int note) {
    triggerOnNoteNum_ = note;
}

void TouchkeyMultiFingerTriggerMappingFactory::setTriggerOffNoteNumber(int note) {
    triggerOffNoteNum_ = note;
}

void TouchkeyMultiFingerTriggerMappingFactory::setTriggerOnNoteVelocity(int velocity) {
    if(velocity > 127)
        velocity = 127;
    triggerOnNoteVel_ = velocity;
}

void TouchkeyMultiFingerTriggerMappingFactory::setTriggerOffNoteVelocity(int velocity) {
    if(velocity > 127)
        velocity = 127;
    triggerOffNoteVel_ = velocity;
}

#ifndef TOUCHKEYS_NO_GUI
// ***** GUI Support *****
std::unique_ptr< MappingEditorComponent > TouchkeyMultiFingerTriggerMappingFactory::createBasicEditor() {
    return std::make_unique< TouchkeyMultiFingerTriggerMappingShortEditor >(*this);
}
#endif

// ****** OSC Control Support ******
OscMessage* TouchkeyMultiFingerTriggerMappingFactory::oscControlMethod(const char *path, const char *types,
                                                              int numValues, lo_arg **values, void *data) {
    // TODO
    
    // If no match, check the base class
    return TouchkeyBaseMappingFactory<TouchkeyMultiFingerTriggerMapping>::oscControlMethod(path, types, numValues, values, data);
}

// ****** Preset Save/Load ******
std::unique_ptr< juce::XmlElement > TouchkeyMultiFingerTriggerMappingFactory::getPreset() {
    juce::PropertySet properties;
    
    storeCommonProperties(properties);
    
    properties.setValue("numTouchesForTrigger", numTouchesForTrigger_);
    properties.setValue("numFramesForTrigger", numFramesForTrigger_);
    properties.setValue("numConsecutiveTapsForTrigger", numConsecutiveTapsForTrigger_);
    properties.setValue("maxTapSpacing", maxTapSpacing_);
    properties.setValue("needsMidiNoteOn", needsMidiNoteOn_);
    properties.setValue("triggerOnAction", triggerOnAction_);
    properties.setValue("triggerOffAction", triggerOffAction_);
    properties.setValue("triggerOnNoteNum", triggerOnNoteNum_);
    properties.setValue("triggerOffNoteNum", triggerOffNoteNum_);
    properties.setValue("triggerOnNoteVel", triggerOnNoteVel_);
    properties.setValue("triggerOffNoteVel", triggerOffNoteVel_);
    
    auto preset = properties.createXml("MappingFactory");
    preset->setAttribute("type", "MultiFingerTrigger");
    
    return preset;
}

bool TouchkeyMultiFingerTriggerMappingFactory::loadPreset(juce::XmlElement const* preset) {
    if(preset == nullptr)
        return false;
    
    juce::PropertySet properties;
    properties.restoreFromXml(*preset);
    
    if(!loadCommonProperties(properties))
        return false;
    
    // Load specific properties
    if(properties.containsKey("numTouchesForTrigger"))
        numTouchesForTrigger_ = properties.getIntValue("numTouchesForTrigger");
    if(properties.containsKey("numFramesForTrigger"))
        numFramesForTrigger_ = properties.getIntValue("numFramesForTrigger");
    if(properties.containsKey("numConsecutiveTapsForTrigger"))
        numConsecutiveTapsForTrigger_ = properties.getIntValue("numConsecutiveTapsForTrigger");
    if(properties.containsKey("maxTapSpacing"))
        maxTapSpacing_ = properties.getDoubleValue("maxTapSpacing");
    if(properties.containsKey("needsMidiNoteOn"))
        needsMidiNoteOn_ = properties.getBoolValue("needsMidiNoteOn");
    if(properties.containsKey("triggerOnAction"))
        triggerOnAction_ = properties.getBoolValue("triggerOnAction");
    if(properties.containsKey("triggerOffAction"))
        triggerOffAction_ = properties.getBoolValue("triggerOffAction");
    if(properties.containsKey("triggerOnNoteNum"))
        triggerOnNoteNum_ = properties.getBoolValue("triggerOnNoteNum");
    if(properties.containsKey("triggerOffNoteNum"))
        triggerOffNoteNum_ = properties.getBoolValue("triggerOffNoteNum");
    if(properties.containsKey("triggerOnNoteVel"))
        triggerOnNoteVel_ = properties.getBoolValue("triggerOnNoteVel");
    if(properties.containsKey("triggerOffNoteVel"))
        triggerOffNoteVel_ = properties.getBoolValue("triggerOffNoteVel");
    
    return true;
}

// ***** Private Methods *****

// Set the initial parameters for a new mapping
void TouchkeyMultiFingerTriggerMappingFactory::initializeMappingParameters(int noteNumber, TouchkeyMultiFingerTriggerMapping *mapping) {
    mapping->setTouchesForTrigger(numTouchesForTrigger_);
    mapping->setFramesForTrigger(numFramesForTrigger_);
    mapping->setConsecutiveTapsForTrigger(numConsecutiveTapsForTrigger_);
    mapping->setMaxTimeBetweenTapsForTrigger(maxTapSpacing_);
    mapping->setNeedsMidiNoteOn(needsMidiNoteOn_);
    mapping->setTriggerOnAction(triggerOnAction_);
    mapping->setTriggerOffAction(triggerOffAction_);
    mapping->setTriggerOnNoteNumber(triggerOnNoteNum_);
    mapping->setTriggerOffNoteNumber(triggerOffNoteNum_);
    mapping->setTriggerOnNoteVelocity(triggerOnNoteVel_);
    mapping->setTriggerOffNoteVelocity(triggerOffNoteVel_);
}
