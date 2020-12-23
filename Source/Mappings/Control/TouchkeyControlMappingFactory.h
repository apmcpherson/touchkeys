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

  TouchkeyControlMappingFactory.h: factory for the TouchKeys control
  mapping, which converts an arbitrary touch parameter into a MIDI or
  OSC control message.
*/

#pragma once

#include <iostream>

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyControlMapping.h"

// Factory class to produce Touchkey control messages for generic MIDI/OSC controllers

class TouchkeyControlMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyControlMapping> {
private:
    static constexpr int kDefaultController = 1;
    static constexpr float kDefaultOutputRangeMin = 0.0;
    static constexpr float kDefaultOutputRangeMax = 127.0;
    static constexpr float kDefaultOutputDefault = 0.0;
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyControlMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyControlMappingFactory();
    
    // ***** Accessors / Modifiers *****
    
    virtual const std::string factoryTypeName() { return "Control"; }

    // ***** Class-Specific Methods *****

    int getController() { return midiControllerNumber_; }
    int getInputParameter() { return inputParameter_; }
    int getInputType() { return inputType_; }
    float getRangeInputMin() { return inputRangeMin_; }
    float getRangeInputMax() { return inputRangeMax_; }
    float getRangeInputCenter() { return inputRangeCenter_; }
    float getRangeOutputMin() { return outputRangeMin_; }
    float getRangeOutputMax() { return outputRangeMax_; }
    float getRangeOutputDefault() { return outputDefault_; }
    float getThreshold() { return threshold_; }
    bool getIgnoresTwoFingers() { return ignoresTwoFingers_; }
    bool getIgnoresThreeFingers() { return ignoresThreeFingers_; }
    int getDirection();
    int getOutOfRangeBehavior() { return outOfRangeBehavior_; }
    bool getUses14BitControl() { return use14BitControl_; }
    
    void setInputParameter(int inputParameter);
    void setInputType(int inputType);
    void setController(int controller);
    //void setRange(float inputMin, float inputMax, float inputCenter, float outputMin, float outputMax, float outputDefault);
    void setRangeInputMin(float inputMin);
    void setRangeInputMax(float inputMax);
    void setRangeInputCenter(float inputCenter);
    void setRangeOutputMin(float outputMin);
    void setRangeOutputMax(float outputMax);
    void setRangeOutputDefault(float outputDefault);
    void setThreshold(float threshold);
    void setIgnoresTwoFingers(bool ignoresTwo);
    void setIgnoresThreeFingers(bool ignoresThree);
    void setDirection(int direction);
    void setOutOfRangeBehavior(int behavior);
    void setUses14BitControl(bool use);
    
#ifndef TOUCHKEYS_NO_GUI
    // ***** GUI Support *****
    bool hasBasicEditor() { return true; }
    std::unique_ptr< MappingEditorComponent > createBasicEditor();
    bool hasExtendedEditor() { return true; }
    std::unique_ptr< MappingEditorComponent > createExtendedEditor();
#endif
    
    // ****** OSC Control Support ******
    OscMessage* oscControlMethod(const char *path, const char *types,
                                 int numValues, lo_arg **values, void *data);
    
    // ****** Preset Save/Load ******
    std::unique_ptr< juce::XmlElement > getPreset();
    bool loadPreset(juce::XmlElement const* preset);
    
private:
    // ***** Private Methods *****
    void initializeMappingParameters(int noteNumber, TouchkeyControlMapping *mapping);

    int inputParameter_;                                // Type of input data
    int inputType_;                                     // Whether data is absolute or relative
    float outputRangeMin_, outputRangeMax_;             // Output ranges
    float outputDefault_;                               // Default values
    float threshold_;                                   // Detection threshold for relative motion
    bool ignoresTwoFingers_;                            // Whether this mapping suspends messages when two
    bool ignoresThreeFingers_;                          // or three fingers are present
    int direction_;                                     // Whether the mapping uses the absolute value in negative cases
};
