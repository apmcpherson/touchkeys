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

  TouchkeyPitchBendMappingFactory.h: factory for the pitch-bend mapping,
  which handles changing pitch based on relative finger motion.
*/

#pragma once

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyPitchBendMapping.h"
#include <iostream>

// Factory class to produce Touchkey pitch bend messages
// This class keeps track of all the active mappings and responds
// whenever touches or notes begin or end

class TouchkeyPitchBendMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyPitchBendMapping> {
    
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyPitchBendMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyPitchBendMappingFactory();
    
    // ***** Accessors / Modifiers *****
    
    virtual const std::string factoryTypeName() { return "Pitch\nBend"; }
    
    void setName(const std::string& name);
    
    // ***** Bend-Specific Methods *****
    
    float getBendRange() { return bendRangeSemitones_; }
    float getBendThresholdSemitones() { return bendThresholdSemitones_; }
    float getBendThresholdKeyLength() { return bendThresholdKeyLength_; }
    int getBendMode() { return bendMode_; }
    
    void setBendRange(float rangeSemitones, bool updateCurrent = false);
    void setBendThresholdSemitones(float thresholdSemitones);
    void setBendThresholdKeyLength(float thresholdKeyLength);
    void setBendThresholds(float thresholdSemitones, float thresholdKeyLength, bool updateCurrent = false);
    void setBendFixedEndpoints(float minimumDistanceToEnable, float bufferAtEnd);
    void setBendVariableEndpoints();
    void setBendIgnoresMultipleFingers(bool ignoresTwo, bool ignoresThree);
    
#ifndef TOUCHKEYS_NO_GUI
    // ***** GUI Support *****
    bool hasBasicEditor() { return true; }
    std::unique_ptr< MappingEditorComponent > createBasicEditor();
    bool hasExtendedEditor() { return false; }
    std::unique_ptr< MappingEditorComponent > createExtendedEditor() { return nullptr; }
#endif
    
    // ****** OSC Control Support ******
    OscMessage* oscControlMethod(const char *path, const char *types,
                                 int numValues, lo_arg **values, void *data);

    // ****** Preset Save/Load ******
    std::unique_ptr< juce::XmlElement > getPreset();
    bool loadPreset(juce::XmlElement const* preset);
    
private:
    // ***** Private Methods *****
    void initializeMappingParameters(int noteNumber, TouchkeyPitchBendMapping *mapping);
    void setBendParameters();
    
    float bendRangeSemitones_;                          // Range of the pitch bend component
    float bendThresholdSemitones_;                      // Threshold for engaging pitch bend
    float bendThresholdKeyLength_;                      // Threshold in key length for engaging pitch bend
    int bendMode_;                                      // What mode the bend works in (fixed, variable, etc.)
    float fixedModeMinEnableDistance_;                  // Minimum distance to engage in fixed mode
    float fixedModeBufferDistance_;                     // Extra distance at end beyond which no bend happens
    bool bendIgnoresTwoFingers_;                        // Whether the pitch bends ignore two
    bool bendIgnoresThreeFingers_;                      // or three fingers on the key at once
};
