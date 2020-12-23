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

  TouchkeyVibratoMappingFactory.h: factory for the vibrato mapping class,
  which creates vibrato through side-to-side motion of the finger on the
  key surface.
*/

#pragma once

#include <iostream>

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyVibratoMapping.h"

// Factory class to produce Touchkey vibrato (pitch-bend) mappings
// This class keeps track of all the active mappings and responds
// whenever touches or notes begin or end

class TouchkeyVibratoMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyVibratoMapping> {
private:
    static constexpr int kDefaultVibratoControl = MidiKeyboardSegment::kControlPitchWheel;

public:
    
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyVibratoMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyVibratoMappingFactory();
    
    // ***** Accessors / Modifiers *****
    
    virtual const std::string factoryTypeName() { return "Vibrato"; }

    void setName(const std::string& name);
    
    // ***** Vibrato-Specific Methods *****
    
    int getVibratoControl() { return vibratoControl_; }
    float getVibratoRange() { return vibratoRange_; }
    float getVibratoPrescaler() { return vibratoPrescaler_; }
    float getVibratoThreshold() { return vibratoOnsetThresholdX_; }
    float getVibratoRatio() { return vibratoOnsetRatioX_; }
    timestamp_diff_type getVibratoTimeout() { return vibratoTimeout_; }
    
    void setVibratoControl(int vibratoControl);
    void setVibratoRange(float range, bool updateCurrent = false);
    void setVibratoPrescaler(float prescaler, bool updateCurrent = false);
    void setVibratoThreshold(float threshold, bool updateCurrent = false);
    void setVibratoThresholds(float thresholdX, float thresholdY, float ratioX, float ratioY, bool updateCurrent = false);
    void setVibratoTimeout(timestamp_diff_type timeout, bool updateCurrent = false);
    
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
    void initializeMappingParameters(int noteNumber, TouchkeyVibratoMapping *mapping);
    
    void configurePitchWheelVibrato();
    void configureControlChangeVibrato();
    
    int vibratoControl_;                                // Controller to use with vibrato
    float vibratoRange_;                                // Range that the vibrato should use, in semitones or CC values
    float vibratoPrescaler_;                            // Prescaler value to use before nonlinear vibrato mapping
    float vibratoTimeout_;                              // Timeout for vibrato detection
    float vibratoOnsetThresholdX_;                      // Thresholds for detection
    float vibratoOnsetThresholdY_;
    float vibratoOnsetRatioX_;
    float vibratoOnsetRatioY_;
};
