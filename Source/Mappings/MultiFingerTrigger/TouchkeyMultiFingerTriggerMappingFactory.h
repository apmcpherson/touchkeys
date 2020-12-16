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

  TouchkeyMultiFingerTriggerMappingFactory.h: factory for the multiple-
  finger trigger mapping, which performs actions when two or more fingers
  are added or removed from the key.
*/
#pragma once

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyMultiFingerTriggerMapping.h"
#include "TouchkeyMultiFingerTriggerMappingShortEditor.h"

class TouchkeyMultiFingerTriggerMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyMultiFingerTriggerMapping> {
private:
  
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyMultiFingerTriggerMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyMultiFingerTriggerMappingFactory() {}
    
    // ***** Accessors / Modifiers *****
    
    virtual const std::string factoryTypeName() { return "Multi-Finger\nTrigger"; }
    
    // ***** Class-Specific Methods *****
    
    // Parameters for multi-finger trigger
    int getTouchesForTrigger() { return numTouchesForTrigger_; }
    int getFramesForTrigger() { return numFramesForTrigger_; }
    int getConsecutiveTapsForTrigger() { return numConsecutiveTapsForTrigger_; }
    timestamp_diff_type getMaxTimeBetweenTapsForTrigger() { return maxTapSpacing_; }
    bool getNeedsMidiNoteOn() { return needsMidiNoteOn_; }
    int getTriggerOnAction() { return triggerOnAction_; }
    int getTriggerOffAction()  { return triggerOffAction_; }
    int getTriggerOnNoteNumber() { return triggerOnNoteNum_; }
    int getTriggerOffNoteNumber()  { return triggerOffNoteNum_; }
    int getTriggerOnNoteVelocity()  { return triggerOnNoteVel_; }
    int getTriggerOffNoteVelocity() { return triggerOffNoteVel_; }
    
    void setTouchesForTrigger(int touches);
    void setFramesForTrigger(int frames);
    void setConsecutiveTapsForTrigger(int taps);
    void setMaxTimeBetweenTapsForTrigger(timestamp_diff_type timeDiff);
    void setNeedsMidiNoteOn(bool needsMidi);
    void setTriggerOnAction(int action);
    void setTriggerOffAction(int action);
    void setTriggerOnNoteNumber(int note);
    void setTriggerOffNoteNumber(int note);
    void setTriggerOnNoteVelocity(int velocity);
    void setTriggerOffNoteVelocity(int velocity);
    
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
    void initializeMappingParameters(int noteNumber, TouchkeyMultiFingerTriggerMapping *mapping);

    // Parameters
    int numTouchesForTrigger_;                  // How many touches are needed for a trigger
    int numFramesForTrigger_;                   // How many consecutive frames with these touches are needed to trigger
    int numConsecutiveTapsForTrigger_;          // How many taps with this number of touches are needed to trigger
    timestamp_diff_type maxTapSpacing_;         // How far apart the taps can come and be considered a multi-tap gesture
    bool needsMidiNoteOn_;                      // Whether the MIDI note has to be on for this gesture to trigger
    int triggerOnAction_, triggerOffAction_;    // Actions to take on trigger on/off
    int triggerOnNoteNum_, triggerOffNoteNum_;  // Which notes to send if a note is being sent
    int triggerOnNoteVel_, triggerOffNoteVel_;  // Velocity to send if a note is being sent
};
