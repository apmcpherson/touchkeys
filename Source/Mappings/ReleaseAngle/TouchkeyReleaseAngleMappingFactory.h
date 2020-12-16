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

  TouchkeyReleaseAngleMappingFactory.h: factory for the release angle
  mapping, which measures the speed of finger motion along the key at
  the time of MIDI note off.
*/

#pragma once

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyReleaseAngleMapping.h"

class TouchkeyReleaseAngleMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyReleaseAngleMapping> {
private:
    //constexpr static const timestamp_diff_type kDefaultMaxLookbackTime = milliseconds_to_timestamp(100);
    static const timestamp_diff_type kDefaultMaxLookbackTime;
    
    static const int kNumConfigurations;
    static const std::string kConfigurationNames[];
    
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyReleaseAngleMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyReleaseAngleMappingFactory() {}
    
    // ***** Accessors / Modifiers *****
    virtual const std::string factoryTypeName() { return "Release\nAngle"; }
    
    // ***** Specific Parameter Methods *****
    
    float getWindowSize() { return windowSizeMilliseconds_; }
    bool getUpMessagesEnabled() { return upEnabled_; }
    bool getDownMessageEnabled() { return downEnabled_; }
    float getUpMinimumAngle() { return upMinimumAngle_; }
    int getUpNote(int sequence);
    int getUpVelocity(int sequence);
    float getDownMinimumAngle() { return downMinimumAngle_; }
    int getDownNote(int sequence);
    int getDownVelocity(int sequence);
    
    void setWindowSize(float windowSize);
    void setUpMessagesEnabled(bool enable);
    void setDownMessagesEnabled(bool enable);
    void setUpMinimumAngle(float minAngle);
    void setUpNote(int sequence, int note);
    void setUpVelocity(int sequence, int velocity);
    void setDownMinimumAngle(float minAngle);
    void setDownNote(int sequence, int note);
    void setDownVelocity(int sequence, int velocity);
    
    // Methods for loading release-angle specific preset settings (different from the
    // global preset methdos which are for save/load of files)
    
    int getNumConfigurations() { return kNumConfigurations; }
    std::string getConfigurationName(int index) {
        if(index < 0 || index >= kNumConfigurations)
            return "";
        return kConfigurationNames[index];
    }
    
    // Returns current configuration or -1 if not a default setting
    int getCurrentConfiguration() {
        return currentConfiguration_;
    }
    void setCurrentConfiguration(int index);
    
#ifndef TOUCHKEYS_NO_GUI
    // ***** GUI Support *****
    bool hasBasicEditor() { return false; }
    std::unique_ptr< MappingEditorComponent > createBasicEditor() { return nullptr; }
    bool hasExtendedEditor() { return true; }
    std::unique_ptr< MappingEditorComponent > createExtendedEditor();
#endif
    
    // ****** Preset Save/Load ******
    std::unique_ptr< juce::XmlElement > getPreset();
    bool loadPreset(juce::XmlElement const* preset);
    
    // ***** State Updaters *****
    
    // Override the MIDI note off method to process the release angle
    /*void midiNoteOff(int noteNumber, bool touchIsOn, bool keyMotionActive,
                     Node<KeyTouchFrame>* touchBuffer,
                     Node<key_position>* positionBuffer,
                     KeyPositionTracker* positionTracker);*/
    
    //void midiNoteOffReceived(int channel);
    
private:
    // ***** Private Methods *****
    void initializeMappingParameters(int noteNumber, TouchkeyReleaseAngleMapping *mapping);
    void clearNotes();
    
    int currentConfiguration_;              // What configuration we're currently in
    float windowSizeMilliseconds_;          // How long before release to consider touch data
    bool upEnabled_, downEnabled_;          // Whether messages are enabled for upward and downward releases
    float upMinimumAngle_;                  // Minimum release angle for trigger for up...
    float downMinimumAngle_;                // ...and down cases
    int upNotes_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];       // Notes and velocities to send on upward
    int upVelocities_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];  // and downward release
    int downNotes_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];
    int downVelocities_[RELEASE_ANGLE_MAX_SEQUENCE_LENGTH];
};
