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

  TouchkeyControlMappingFactory.cpp: factory for the TouchKeys control
  mapping, which converts an arbitrary touch parameter into a MIDI or
  OSC control message.
*/

#include "TouchkeyControlMappingFactory.h"
#include "TouchkeyControlMappingShortEditor.h"
#include "TouchkeyControlMappingExtendedEditor.h"

TouchkeyControlMappingFactory::TouchkeyControlMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment) :
TouchkeyBaseMappingFactory<TouchkeyControlMapping>(keyboard, segment),
inputParameter_(TouchkeyControlMapping::kInputParameterYPosition),
inputType_(TouchkeyControlMapping::kTypeAbsolute),
outputRangeMin_(kDefaultOutputRangeMin), outputRangeMax_(kDefaultOutputRangeMax),
outputDefault_(kDefaultOutputDefault), threshold_(0.0),
ignoresTwoFingers_(TouchkeyControlMapping::kDefaultIgnoresTwoFingers),
ignoresThreeFingers_(TouchkeyControlMapping::kDefaultIgnoresThreeFingers),
direction_(TouchkeyControlMapping::kDefaultDirection)
{
    setController(kDefaultController);
}

// ***** Destructor *****

TouchkeyControlMappingFactory::~TouchkeyControlMappingFactory() {

}

// ***** Accessors / Modifiers *****

int TouchkeyControlMappingFactory::getDirection() {
    // Get the direction of motion. This is always positive for
    if(inputType_ == TouchkeyControlMapping::kTypeAbsolute)
        return TouchkeyControlMapping::kDirectionPositive;
    return direction_;
}

void TouchkeyControlMappingFactory::setInputParameter(int inputParameter) {
    if(inputParameter >= 1 && inputParameter < TouchkeyControlMapping::kInputParameterMaxValue)
        inputParameter_ = inputParameter;
}

void TouchkeyControlMappingFactory::setInputType(int inputType) {
    if(inputType >= 1 && inputType < TouchkeyControlMapping::kTypeMaxValue)
        inputType_ = inputType;
}

void TouchkeyControlMappingFactory::setController(int controller) {
    if(controller < 1 ||
       controller >= MidiKeyboardSegment::kControlMax)
        return;
    
    // Before changing the controller, check if we were going to or from the pitch wheel.
    // If so, we should scale the value to or from a 14-bit value
    if(midiControllerNumber_ == MidiKeyboardSegment::kControlPitchWheel &&
       controller != MidiKeyboardSegment::kControlPitchWheel) {
        outputRangeMax_ = outputRangeMax_ / 128.0;
        if(outputRangeMax_ > 127.0)
            outputRangeMax_ = 127.0;
        outputRangeMin_ = outputRangeMin_ / 128.0;
        if(outputRangeMin_ > 127.0)
            outputRangeMin_ = 127.0;
        outputDefault_ = outputDefault_ / 128.0;
        if(outputDefault_ > 127.0)
            outputDefault_ = 127.0;
    }
    else if(midiControllerNumber_ != MidiKeyboardSegment::kControlPitchWheel &&
            controller == MidiKeyboardSegment::kControlPitchWheel) {
        if(outputRangeMax_ == 127.0)
            outputRangeMax_ = 16383.0;
        else
            outputRangeMax_ = outputRangeMax_ * 128.0;
        if(outputRangeMin_ == 127.0)
            outputRangeMin_ = 16383.0;
        else
            outputRangeMin_ = outputRangeMin_ * 128.0;
        if(outputDefault_ == 127.0)
            outputDefault_ = 16383.0;
        else
            outputDefault_ = outputDefault_ * 128.0;
    }

    setMidiParameters(controller, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
    
    // Listen to incoming controls from the keyboard too, if this is enabled
    // in MidiKeyboardSegment
    if(midiConverter_ != 0) {
        midiConverter_->listenToIncomingControl(midiControllerNumber_);
    }
}

void TouchkeyControlMappingFactory::setRangeInputMin(float inputMin) {
    if(inputMin < -1.0)
        inputRangeMin_ = -1.0;
    else if(inputMin > 1.0)
        inputRangeMin_ = 1.0;
    else
        inputRangeMin_ = inputMin;
    
    // Update control
    //if(midiConverter_ == 0)
    //    return;
    //midiConverter_->setControlMinValue(controlName_.c_str(), inputRangeMin_);
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setRangeInputMax(float inputMax) {
    if(inputMax < -1.0)
        inputRangeMax_ = -1.0;
    else if(inputMax > 1.0)
        inputRangeMax_ = 1.0;
    else
        inputRangeMax_ = inputMax;
    
    // Update control
    //if(midiConverter_ == 0)
    //    return;
    //midiConverter_->setControlMaxValue(controlName_.c_str(), inputRangeMax_);
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setRangeInputCenter(float inputCenter) {
    if(inputCenter < -1.0)
        inputRangeCenter_ = -1.0;
    else if(inputCenter > 1.0)
        inputRangeCenter_ = 1.0;
    else
        inputRangeCenter_ = inputCenter;
    
    // Update control
    //if(midiConverter_ == 0)
    //    return;
    //midiConverter_->setControlCenterValue(controlName_.c_str(), inputRangeCenter_);
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setRangeOutputMin(float outputMin) {
    outputRangeMin_ = outputMin;
    
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setRangeOutputMax(float outputMax) {
    outputRangeMax_ = outputMax;
    
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setRangeOutputDefault(float outputDefault) {
    outputDefault_ = outputDefault;
    
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setThreshold(float threshold) {
    threshold_ = threshold;
}

void TouchkeyControlMappingFactory::setIgnoresTwoFingers(bool ignoresTwo) {
    ignoresTwoFingers_ = ignoresTwo;
}

void TouchkeyControlMappingFactory::setIgnoresThreeFingers(bool ignoresThree) {
    ignoresThreeFingers_ = ignoresThree;
}

void TouchkeyControlMappingFactory::setDirection(int direction) {
    direction_ = direction;
}

void TouchkeyControlMappingFactory::setOutOfRangeBehavior(int behavior) {
    outOfRangeBehavior_ = behavior;
    
    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

void TouchkeyControlMappingFactory::setUses14BitControl(bool use) {
    use14BitControl_ = use;

    setMidiParameters(midiControllerNumber_, inputRangeMin_, inputRangeMax_, inputRangeCenter_,
                      outputDefault_, outputRangeMin_, outputRangeMax_,
                      -1, use14BitControl_, outOfRangeBehavior_);
}

#ifndef TOUCHKEYS_NO_GUI
// ***** GUI Support *****
std::unique_ptr< MappingEditorComponent > TouchkeyControlMappingFactory::createBasicEditor() {
    return std::make_unique< TouchkeyControlMappingShortEditor >(*this);
}

std::unique_ptr< MappingEditorComponent > TouchkeyControlMappingFactory::createExtendedEditor() {
    return std::make_unique< TouchkeyControlMappingExtendedEditor >(*this);
}
#endif

// ****** OSC Control Support ******
OscMessage* TouchkeyControlMappingFactory::oscControlMethod(const char *path, const char *types,
                                                            int numValues, lo_arg **values, void *data) {
    if(!strcmp(path, "/set-input-parameter")) {
        // Change the input parameter for the control mapping
        if(numValues > 0) {
            if(types[0] == 'i') {
                setInputParameter(values[0]->i);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-input-type")) {
        // Change the input type (absolute/relative)
        if(numValues > 0) {
            if(types[0] == 'i') {
                setInputType(values[0]->i);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-input-range-min")) {
        // Change the input range
        if(numValues > 0) {
            if(types[0] == 'f') {
                setRangeInputMin(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-input-range-max")) {
        // Change the input range
        if(numValues > 0) {
            if(types[0] == 'f') {
                setRangeInputMax(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-input-range-center")) {
        // Change the input range
        if(numValues > 0) {
            if(types[0] == 'f') {
                setRangeInputCenter(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-output-range-min")) {
        // Change the output range
        if(numValues > 0) {
            if(types[0] == 'f') {
                setRangeOutputMin(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-output-range-max")) {
        // Change the output range
        if(numValues > 0) {
            if(types[0] == 'f') {
                setRangeOutputMax(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-output-default")) {
        // Change the output range
        if(numValues > 0) {
            if(types[0] == 'f') {
                setRangeOutputDefault(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-out-of-range-behavior")) {
        // Change how out-of-range inputs are handled
        if(numValues > 0) {
            if(types[0] == 'i') {
                setOutOfRangeBehavior(values[0]->i);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-midi-controller")) {
        // Set the MIDI output CC, including pitchwheel etc. and 14-bit options
        if(numValues > 0) {
            if(types[0] == 'i') {
                if(numValues >= 2)
                    if(types[1] == 'i')
                        setUses14BitControl(values[1]->i != 0);
                
                setController(values[0]->i);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-threshold")) {
        // Set the threshold for relative activations
        if(numValues > 0) {
            if(types[0] == 'f') {
                setThreshold(values[0]->f);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-ignores-multiple-fingers")) {
        // Change whether two or three finger touches are ignored
        if(numValues >= 2) {
            if(types[0] == 'i' && types[1] == 'i') {
                setIgnoresTwoFingers(values[0]->i);
                setIgnoresThreeFingers(values[1]->i);
                return OscTransmitter::createSuccessMessage();
            }
        }
    }
    else if(!strcmp(path, "/set-direction")) {
        // Set the direction of the mapping (normal/reverse/absolute val)
        if(numValues > 0) {
            if(types[0] == 'i') {
                setDirection(values[0]->i);
                return OscTransmitter::createSuccessMessage();
            }
            else if(types[0] == 's') {
                const char *str = &values[0]->s;
                
                if(!strncmp(str, "norm", 4)) {
                    setDirection(TouchkeyControlMapping::kDirectionPositive);
                    return OscTransmitter::createSuccessMessage();
                }
                else if(!strncmp(str, "rev", 3)) {
                    setDirection(TouchkeyControlMapping::kDirectionNegative);
                    return OscTransmitter::createSuccessMessage();
                }
                if(!strncmp(str, "always", 6) || !strncmp(str, "both", 4)) {
                    setDirection(TouchkeyControlMapping::kDirectionBoth);
                    return OscTransmitter::createSuccessMessage();
                }
                else
                    return OscTransmitter::createFailureMessage();
            }
        }
    }
    
    // If no match, check the base class
    return TouchkeyBaseMappingFactory<TouchkeyControlMapping>::oscControlMethod(path, types, numValues, values, data);
}


// ****** Preset Save/Load ******
std::unique_ptr< juce::XmlElement > TouchkeyControlMappingFactory::getPreset() {
    juce::PropertySet properties;
    
    storeCommonProperties(properties);
    properties.setValue("inputParameter", inputParameter_);
    properties.setValue("inputType", inputType_);
    properties.setValue("outputRangeMin", outputRangeMin_);
    properties.setValue("outputRangeMax", outputRangeMax_);
    properties.setValue("outputDefault", outputDefault_);
    properties.setValue("threshold", threshold_);
    properties.setValue("ignoresTwoFingers", ignoresTwoFingers_);
    properties.setValue("ignoresThreeFingers", ignoresThreeFingers_);
    properties.setValue("direction", direction_);
    properties.setValue("use14Bit", use14BitControl_);
    
    auto preset = properties.createXml("MappingFactory");
    preset->setAttribute("type", "Control");
    
    return preset;
}

bool TouchkeyControlMappingFactory::loadPreset(juce::XmlElement const* preset) {
    if(preset == 0)
        return false;
    
    juce::PropertySet properties;
    properties.restoreFromXml(*preset);
    
    if(!loadCommonProperties(properties))
        return false;
    if(!properties.containsKey("inputParameter") ||
       !properties.containsKey("inputType") ||
       !properties.containsKey("outputRangeMin") ||
       !properties.containsKey("outputRangeMax") ||
       !properties.containsKey("outputDefault") ||
       !properties.containsKey("threshold") ||
       !properties.containsKey("ignoresTwoFingers") ||
       !properties.containsKey("ignoresThreeFingers") ||
       !properties.containsKey("direction"))
        return false;
    
    inputParameter_ = properties.getIntValue("inputParameter");
    inputType_ = properties.getIntValue("inputType");
    outputRangeMin_ = properties.getDoubleValue("outputRangeMin");
    outputRangeMax_ = properties.getDoubleValue("outputRangeMax");
    outputDefault_ = properties.getDoubleValue("outputDefault");
    threshold_ = properties.getDoubleValue("threshold");
    ignoresTwoFingers_ = properties.getBoolValue("ignoresTwoFingers");
    ignoresThreeFingers_ = properties.getBoolValue("ignoresThreeFingers");
    direction_ = properties.getIntValue("direction");
    
    // These values added to later versions of the presets so check
    // whether they actually exist or not
    if(properties.containsKey("use14Bit"))
        use14BitControl_ = properties.getBoolValue("use14Bit");
    
    // Update MIDI information; this doesn't actually change the controller
    // (which is already set) but it adds a listener and updates the ranges
    setController(midiControllerNumber_);
    
    return true;
}

// ***** Private Methods *****

void TouchkeyControlMappingFactory::initializeMappingParameters(int noteNumber, TouchkeyControlMapping *mapping) {
    // Set parameters
    mapping->setInputParameter(inputParameter_, inputType_);
    mapping->setRange(inputRangeMin_, inputRangeMax_, outputRangeMin_, outputRangeMax_, outputDefault_);
    mapping->setThreshold(threshold_);
    mapping->setIgnoresMultipleFingers(ignoresTwoFingers_, ignoresThreeFingers_);
    mapping->setDirection(direction_);
}
