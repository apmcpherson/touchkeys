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

  TouchkeyKeyDivisionMappingFactory.h: factory for the split-key mapping
  which triggers different actions or pitches depending on where the key
  was struck.
*/

#pragma once

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyKeyDivisionMapping.h"
#include "TouchkeyKeyDivisionMappingShortEditor.h"
#include "../../Display/KeyboardDisplay.h"

class TouchkeyKeyDivisionMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyKeyDivisionMapping> {
private:
    static const float kTuningsYarman24c[];
    static const int kMaxSegmentsPerKey;
    
public:
    enum {
        kTuningPreset19TET = 0,
        kTuningPreset24TET,
        kTuningPreset31TET,
        kTuningPreset36TET,
        kTuningPresetYarman24c,
        kTuningPresetMaxValue
    };
    
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyKeyDivisionMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyKeyDivisionMappingFactory();

    // ***** Accessors / Modifiers *****
    
    virtual const std::string factoryTypeName() { return "Split\nKeys"; }

    void setName(const std::string& name);
    
    // ***** Split-Key Specific Methods *****
    
    int getNumberOfSegments() { return numSegmentsPerKey_; }
    timestamp_diff_type getTimeout() { return timeout_; }
    int getDetectionParameter() { return detectionParameter_; }
    bool getRetriggerable() { return retriggerable_; }
    int getRetriggerNumFrames() { return retriggerNumFrames_; }
    bool getRetriggerKeepsVelocity() { return retriggerKeepsVelocity_; }
    int getReferenceNote() { return referenceNote_; }
    float getGlobalOffset() { return globalOffsetCents_; }
    int getTuningPreset() { return tuningPreset_; }
    
    void setNumberOfSegments(int segments) {
        if(segments > 0 && segments <= kMaxSegmentsPerKey)
            numSegmentsPerKey_ = segments;
    }
    
    // Set the detection timeout value (how long from MIDI note on to touch)
    void setTimeout(timestamp_diff_type timeout) {
        timeout_ = timeout;
    }
    
    // Set the detection parameter for choosing a segment
    void setDetectionParameter(int detectionParameter) {
        if(detectionParameter >= 1 && detectionParameter < TouchkeyKeyDivisionMapping::kDetectionParameterMaxValue)
            detectionParameter_ = detectionParameter;
    }
    
    // Set whether placing a second finger in the other segment triggers a
    // new note with that segment. Two forms of this...
    void setRetriggerable(bool retrigger, int numFrames, bool keepOriginalVelocity) {
        retriggerable_ = retrigger;
        retriggerNumFrames_ = numFrames;
        retriggerKeepsVelocity_ = keepOriginalVelocity;
    }
    
    void setRetriggerable(bool retrigger) {
        retriggerable_ = retrigger;
    }
    
    // Set the note that acts as the reference point in a microtonal scale
    void setReferenceNote(int note) {
        if(note >= 0)
            referenceNote_ = note % 12;
    }
    
    void setGlobalOffset(float offsetCents) {
        globalOffsetCents_ = offsetCents;
    }
    
    void setTuningPreset(int preset);
    
#ifndef TOUCHKEYS_NO_GUI
    // ***** GUI Support *****
    bool hasBasicEditor() { return true; }
    std::unique_ptr< MappingEditorComponent > createBasicEditor();
    bool hasExtendedEditor() { return false; }
    std::unique_ptr< MappingEditorComponent > createExtendedEditor() { return nullptr; }
#endif
    
    // ****** Preset Save/Load ******
    std::unique_ptr< juce::XmlElement > getPreset();
    bool loadPreset(juce::XmlElement const* preset);
    
private:
    // ***** Private Methods *****
    void initializeMappingParameters(int noteNumber, TouchkeyKeyDivisionMapping *mapping);
    void setBendParameters();
    
    int tuningPreset_;                                  // Number of the preset tuning, if active
    float *tunings_;                                    // Array of tuning values, set by preset
    int numSegmentsPerKey_;                             // How many segments per key
    timestamp_diff_type timeout_;                       // How long before timeout activates default segment
    int detectionParameter_;                            // Which parameter separates it into segments
    bool retriggerable_;                                // Whether a second touch can retrigger this note
    int retriggerNumFrames_;                            // How many frames a new touch must be present to retrigger
    bool retriggerKeepsVelocity_;                       // Whether a retriggered note keeps the original velocity or a default
    int referenceNote_;                                 // Which note acts as the reference point
    float globalOffsetCents_;                           // Offset of every note in cents
    
    juce::CriticalSection tuningMutex_;                       // Mutex to avoid triggers during tuning changes
};
