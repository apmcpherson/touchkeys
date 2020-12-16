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

  TouchkeyOnsetAngleMappingFactory.h: factory for the onset angle mapping,
  which measures the speed of finger motion along the key surface at the
  time of MIDI note onset.
*/

#pragma once

#include "../TouchkeyBaseMappingFactory.h"
#include "TouchkeyOnsetAngleMapping.h"

class TouchkeyOnsetAngleMappingFactory : public TouchkeyBaseMappingFactory<TouchkeyOnsetAngleMapping> {
private:
    static const timestamp_diff_type kDefaultMaxLookbackTime;
    
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyOnsetAngleMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment);
	
    // ***** Destructor *****
    
    ~TouchkeyOnsetAngleMappingFactory() {}
    
    // ***** Accessors *****
    
    virtual const std::string factoryTypeName() { return "Onset\nAngle"; }
    
    // ****** Preset Save/Load ******
    std::unique_ptr< juce::XmlElement > getPreset();
    bool loadPreset(juce::XmlElement const* preset);
    
    // ***** State Updaters *****
    
    // Override the MIDI note on method to process the onset angle
    void midiNoteOn(int noteNumber, bool touchIsOn, bool keyMotionActive,
                     Node<KeyTouchFrame>* touchBuffer,
                     Node<key_position>* positionBuffer,
                     KeyPositionTracker* positionTracker);
    
};
