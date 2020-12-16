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

  TouchkeyVibratoMappingFactory.cpp: factory for the vibrato mapping class,
  which creates vibrato through side-to-side motion of the finger on the
  key surface.
*/

#include "TouchkeyVibratoMappingFactory.h"
#include "TouchkeyVibratoMappingShortEditor.h"

// Class constants
const int TouchkeyVibratoMappingFactory::kDefaultVibratoControl = MidiKeyboardSegment::kControlPitchWheel;

// Default constructor, containing a reference to the PianoKeyboard class.

TouchkeyVibratoMappingFactory::TouchkeyVibratoMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment) :
TouchkeyBaseMappingFactory<TouchkeyVibratoMapping>(keyboard, segment),
vibratoControl_(kDefaultVibratoControl),
vibratoRange_(TouchkeyVibratoMapping::kDefaultVibratoRangeSemitones),
vibratoPrescaler_(TouchkeyVibratoMapping::kDefaultVibratoPrescaler),
vibratoTimeout_(TouchkeyVibratoMapping::kDefaultVibratoTimeout),
vibratoOnsetThresholdX_(TouchkeyVibratoMapping::kDefaultVibratoThresholdX),
vibratoOnsetThresholdY_(TouchkeyVibratoMapping::kDefaultVibratoThresholdY),
vibratoOnsetRatioX_(TouchkeyVibratoMapping::kDefaultVibratoRatioX),
vibratoOnsetRatioY_(TouchkeyVibratoMapping::kDefaultVibratoRatioY)
{    
    // Set up the MIDI converter to use pitch wheel
    configurePitchWheelVibrato();
}

// ***** Destructor *****

TouchkeyVibratoMappingFactory::~TouchkeyVibratoMappingFactory() {

}

// ***** Accessors / Modifiers *****

void TouchkeyVibratoMappingFactory::setName(const std::string& name) {
    TouchkeyBaseMappingFactory<TouchkeyVibratoMapping>::setName(name);
    setVibratoControl(vibratoControl_);
}

// ***** Vibrato Methods *****

void TouchkeyVibratoMappingFactory::setVibratoControl(int vibratoControl) {
    if(vibratoControl < 0 || vibratoControl >= MidiKeyboardSegment::kControlMax)
        return;
    
    // Update the variable which affects future mappings
    vibratoControl_ = vibratoControl;
    
    if(vibratoControl_ == MidiKeyboardSegment::kControlPitchWheel)
        configurePitchWheelVibrato();
    else
        configureControlChangeVibrato();
}

void TouchkeyVibratoMappingFactory::setVibratoRange(float range, bool updateCurrent) {
    /*if(updateCurrent) {
        // Send new range to all active mappings
        // TODO: mutex protect
        auto it = mappings_.begin();
        while(it != mappings_.end()) {
            // Tell this mapping to update its range
            TouchkeyVibratoMapping *mapping = it->second.first;
            mapping->setRange(rangeSemitones);
            it++;
        }
    }*/
    
    // Update the variable which affects future mappings
    vibratoRange_ = range;
    if(vibratoRange_ < 0.01)
        vibratoRange_ = 0.01;
    if(vibratoRange_ > 127.0)
        vibratoRange_ = 127.0;
}

void TouchkeyVibratoMappingFactory::setVibratoPrescaler(float prescaler, bool updateCurrent) {
    /*if(updateCurrent) {
        // Send new range to all active mappings
        // TODO: mutex protect
        auto it = mappings_.begin();
        while(it != mappings_.end()) {
            // Tell this mapping to update its range
            TouchkeyVibratoMapping *mapping = it->second.first;
            mapping->setPrescaler(prescaler);
            it++;
        }
    }*/
    
    // Update the variable which affects future mappings
    vibratoPrescaler_ = prescaler;
}

void TouchkeyVibratoMappingFactory::setVibratoThreshold(float threshold, bool updateCurrent) {
    vibratoOnsetThresholdX_ = threshold;
    if(vibratoOnsetThresholdX_ < 0)
        vibratoOnsetThresholdX_ = 0;
    if(vibratoOnsetThresholdX_ > 1.0)
        vibratoOnsetThresholdX_ = 1.0;
}

void TouchkeyVibratoMappingFactory::setVibratoThresholds(float thresholdX, float thresholdY, float ratioX, float ratioY, bool updateCurrent) {
    /*if(updateCurrent) {
        // Send new range to all active mappings
        // TODO: mutex protect
        auto it = mappings_.begin();
        while(it != mappings_.end()) {
            // Tell this mapping to update its range
            TouchkeyVibratoMapping *mapping = it->second.first;
            mapping->setThresholds(thresholdX, thresholdY, ratioX, ratioY);
            it++;
        }
    }*/
    
    // Update the variables which affect future mappings
    vibratoOnsetThresholdX_ = thresholdX;
    vibratoOnsetThresholdY_ = thresholdY;
    vibratoOnsetRatioX_ = ratioX;
    vibratoOnsetRatioY_ = ratioY;
}

void TouchkeyVibratoMappingFactory::setVibratoTimeout(timestamp_diff_type timeout, bool updateCurrent) {
    /*if(updateCurrent) {
        // Send new range to all active mappings
        // TODO: mutex protect
        auto it = mappings_.begin();
        while(it != mappings_.end()) {
            // Tell this mapping to update its range
            TouchkeyVibratoMapping *mapping = it->second.first;
            mapping->setTimeout(timeout);
            it++;
        }
    }*/
    
    // Update the variable which affects future mappings
    vibratoTimeout_ = timeout;
}

#ifndef TOUCHKEYS_NO_GUI
// ***** GUI Support *****
std::unique_ptr< MappingEditorComponent > TouchkeyVibratoMappingFactory::createBasicEditor() {
    return std::make_unique< TouchkeyVibratoMappingShortEditor >(*this);
}
#endif

// ****** OSC Control Support ******
OscMessage* TouchkeyVibratoMappingFactory::oscControlMethod(const char *path, const char *types,
                                                            int numValues, lo_arg **values, void *data) {
    if(!strcmp(path, "/set-vibrato-control")) {
        // Change the vibrato control
        if(numValues > 0) {
            if(types[0] == 'i') {
                setVibratoControl(values[0]->i);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-vibrato-range")) {
        // Change the vibrato range in semitones
        if(numValues > 0) {
            if(types[0] == 'f') {
                setVibratoRange(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-vibrato-prescaler")) {
        // Change the vibrato prescaler
        if(numValues > 0) {
            if(types[0] == 'f') {
                setVibratoPrescaler(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-vibrato-timeout")) {
        // Change the vibrato timeout
        if(numValues > 0) {
            if(types[0] == 'f') {
                setVibratoTimeout(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-vibrato-threshold")) {
        // Change the vibrato threshold
        if(numValues > 0) {
            if(types[0] == 'f') {
                setVibratoThreshold(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    
    // If no match, check the base class
    return TouchkeyBaseMappingFactory<TouchkeyVibratoMapping>::oscControlMethod(path, types, numValues, values, data);
}


// ****** Preset Save/Load ******
std::unique_ptr< juce::XmlElement > TouchkeyVibratoMappingFactory::getPreset() {
    juce::PropertySet properties;
    
    storeCommonProperties(properties);
    properties.setValue("vibratoControl", vibratoControl_);
    properties.setValue("vibratoRange", vibratoRange_);
    properties.setValue("vibratoPrescaler", vibratoPrescaler_);
    properties.setValue("vibratoTimeout", vibratoTimeout_);
    properties.setValue("vibratoOnsetThresholdX", vibratoOnsetThresholdX_);
    properties.setValue("vibratoOnsetThresholdY", vibratoOnsetThresholdY_);
    properties.setValue("vibratoOnsetRatioX", vibratoOnsetRatioX_);
    properties.setValue("vibratoOnsetRatioY", vibratoOnsetRatioY_);

    auto preset = properties.createXml("MappingFactory");
    preset->setAttribute("type", "Vibrato");
    
    return preset;
}

bool TouchkeyVibratoMappingFactory::loadPreset(juce::XmlElement const* preset) {
    if(preset == 0)
        return false;
    
    juce::PropertySet properties;
    properties.restoreFromXml(*preset);
    
    if(!loadCommonProperties(properties))
        return false;
    if(!properties.containsKey("vibratoControl") ||
       !properties.containsKey("vibratoRange") ||
       !properties.containsKey("vibratoPrescaler") ||
       !properties.containsKey("vibratoTimeout") ||
       !properties.containsKey("vibratoOnsetThresholdX") ||
       !properties.containsKey("vibratoOnsetThresholdY") ||
       !properties.containsKey("vibratoOnsetRatioX") ||
       !properties.containsKey("vibratoOnsetRatioY"))
        return false;
    
    vibratoControl_ = properties.getDoubleValue("vibratoControl");
    vibratoRange_ = properties.getDoubleValue("vibratoRange");
    vibratoPrescaler_ = properties.getDoubleValue("vibratoPrescaler");
    vibratoTimeout_ = properties.getDoubleValue("vibratoTimeout");
    vibratoOnsetThresholdX_ = properties.getDoubleValue("vibratoOnsetThresholdX");
    vibratoOnsetThresholdY_ = properties.getDoubleValue("vibratoOnsetThresholdY");
    vibratoOnsetRatioX_ = properties.getDoubleValue("vibratoOnsetRatioX");
    vibratoOnsetRatioY_ = properties.getDoubleValue("vibratoOnsetRatioY");
    
    // Update MIDI information; this doesn't actually change the controller
    // (which is already set) but it adds a listener and updates the ranges
    setVibratoControl(vibratoControl_);
    
    return true;
}


// ***** Private Methods *****

// Set the initial parameters for a new mapping
void TouchkeyVibratoMappingFactory::initializeMappingParameters(int noteNumber, TouchkeyVibratoMapping *mapping) {
    mapping->setRange(vibratoRange_);
    mapping->setPrescaler(vibratoPrescaler_);
    mapping->setThresholds(vibratoOnsetThresholdX_, vibratoOnsetThresholdY_, vibratoOnsetRatioX_, vibratoOnsetRatioY_);
    mapping->setTimeout(vibratoTimeout_);
}

// Configure the OSC-MIDI converter to handle pitchwheel vibrato
void TouchkeyVibratoMappingFactory::configurePitchWheelVibrato() {
    // Range of 0 indicates special case of using global pitch wheel range
    setMidiParameters(MidiKeyboardSegment::kControlPitchWheel, 0.0, 0.0, 0.0);
    
    if(midiConverter_ != 0) {
        midiConverter_->listenToIncomingControl(MidiKeyboardSegment::kControlPitchWheel);
    }
}

// Configure the OSC-MIDI converter to handle vibrato based on a CC
void TouchkeyVibratoMappingFactory::configureControlChangeVibrato() {
    setMidiParameters(vibratoControl_, 0.0, 127.0, 0.0, 0, 0, 127, 0, false, OscMidiConverter::kOutOfRangeExtrapolate);
    
    if(midiConverter_ != 0) {
        midiConverter_->listenToIncomingControl(vibratoControl_);
    }
}


