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

  TouchkeyOnsetAngleMappingFactory.cpp: factory for the onset angle mapping,
  which measures the speed of finger motion along the key surface at the
  time of MIDI note onset.
*/

#include "TouchkeyOnsetAngleMappingFactory.h"

TouchkeyOnsetAngleMappingFactory::TouchkeyOnsetAngleMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment)
: TouchkeyBaseMappingFactory<TouchkeyOnsetAngleMapping>(keyboard, segment) {
    //setName("/touchkeys/scoop");
    setMidiParameters(MidiKeyboardSegment::kControlPitchWheel, -2.0, 2.0, 0.0);
}

// ****** Preset Save/Load ******
std::unique_ptr< juce::XmlElement > TouchkeyOnsetAngleMappingFactory::getPreset() {
    juce::PropertySet properties;
    
    storeCommonProperties(properties);
    
    // No properties for now
    
    auto preset = properties.createXml("MappingFactory");
    preset->setAttribute("type", "OnsetAngle");
    
    return preset;
}

bool TouchkeyOnsetAngleMappingFactory::loadPreset(juce::XmlElement const* preset) {
    if(preset == 0)
        return false;
    
    juce::PropertySet properties;
    properties.restoreFromXml(*preset);
    
    if(!loadCommonProperties(properties))
        return false;
    
    // Nothing specific to do for now
    
    return true;
}

// MIDI note ended: see whether the mapping was suspended and if not, execute the angle calculation
void TouchkeyOnsetAngleMappingFactory::midiNoteOn(int noteNumber, bool touchIsOn, bool keyMotionActive,
                                                     Node<KeyTouchFrame>* touchBuffer,
                                                     Node<key_position>* positionBuffer,
                                                     KeyPositionTracker* positionTracker) {
    // Call base class method
    TouchkeyBaseMappingFactory<TouchkeyOnsetAngleMapping>::midiNoteOn(noteNumber, touchIsOn, keyMotionActive, touchBuffer, positionBuffer, positionTracker);

    if(mappings_.count(noteNumber) != 0) {
        mappings_[noteNumber]->processOnset(keyboard_.schedulerCurrentTimestamp());
    }
}
