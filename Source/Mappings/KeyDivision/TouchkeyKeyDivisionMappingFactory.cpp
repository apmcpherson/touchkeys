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

  TouchkeyKeyDivisionMappingFactory.cpp: factory for the split-key mapping
  which triggers different actions or pitches depending on where the key
  was struck.
*/

#include "TouchkeyKeyDivisionMappingFactory.h"

/* Yarman-24c microtonal tuning */
const float TouchkeyKeyDivisionMappingFactory::kTuningsYarman24c[24] = {
    0, (1124.744 - 1200.0), 83.059, 143.623, 203.9, 191.771, 292.413, 348.343,
    383.54, 362.503, 498.04, 415.305, 581.382, 634.184, 695.885, 648.682,
    788.736, 853.063, 905.87, 887.656, 996.1, 1043.623, 1085.49, 1071.942,
};

/* As arranged:
 *
 *   B|  Db/  Dd  Eb/  Ed  E|  Gb/  Gd  Ab/  Ad  Bb/  Bd
 *   C   C#   D   D#   E   F   F#   G   G#   A   A#   B
 */

TouchkeyKeyDivisionMappingFactory::TouchkeyKeyDivisionMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment)
: TouchkeyBaseMappingFactory<TouchkeyKeyDivisionMapping>(keyboard, segment),
  tuningPreset_(-1), tunings_(0),
  numSegmentsPerKey_(TouchkeyKeyDivisionMapping::kDefaultNumberOfSegments),
  timeout_(TouchkeyKeyDivisionMapping::kDefaultDetectionTimeout),
  detectionParameter_(TouchkeyKeyDivisionMapping::kDefaultDetectionParameter),
  retriggerable_(false),
  retriggerNumFrames_(TouchkeyKeyDivisionMapping::kDefaultRetriggerNumFrames),
  retriggerKeepsVelocity_(true),
  referenceNote_(0), globalOffsetCents_(0)
{
    //setName("/touchkeys/segmentpitch");
    setBendParameters();
    setTuningPreset(kTuningPreset24TET);
    
    KeyboardDisplay *display = keyboard_.gui();
    if(display != nullptr ) {
        display->addKeyDivision(this, segment.noteRange().first, segment.noteRange().second, numSegmentsPerKey_);
    }
}

TouchkeyKeyDivisionMappingFactory::~TouchkeyKeyDivisionMappingFactory() {
    // Remove the divisions from the keys, if this mapping has added them
    KeyboardDisplay *display = keyboard_.gui();
    if(display != nullptr )
        display->removeKeyDivision(this);
    if(tunings_ != nullptr) {
        delete tunings_;
        tunings_ = nullptr;
    }
}

void TouchkeyKeyDivisionMappingFactory::setName(const std::string& name) {
    TouchkeyBaseMappingFactory<TouchkeyKeyDivisionMapping>::setName(name);
    setBendParameters();
}

void TouchkeyKeyDivisionMappingFactory::setTuningPreset(int preset) {
    if(preset < 0 || preset >= kTuningPresetMaxValue)
        return;
    
    juce::ScopedLock sl(tuningMutex_);
    
    tuningPreset_ = preset;
    if(tunings_ != nullptr) {
        delete tunings_;
        tunings_ = nullptr;
    }
    
    if(tuningPreset_ == kTuningPreset19TET) {
        numSegmentsPerKey_ = 2;
        tunings_ = new float[24];
        for(int i = 0; i < 24; i++) {
            // Start with fraction of an octave, round to the nearest 19th of an octave
            float original = (float)i / 24.0;
            float rounded = floorf(original * 19.0 + 0.5);
            
            // Now convert the 19-tone index back to a fractional number of semitones
            tunings_[i] = rounded * 1200.0 / 19.0;
        }
    }
    else if(tuningPreset_ == kTuningPreset24TET) {
        numSegmentsPerKey_ = 2;
        tunings_ = new float[24];
        for(int i = 0; i < 24; i++) {
            tunings_[i] = (float)i * 50.0;
        }
    }
    else if(tuningPreset_ == kTuningPreset31TET) {
        numSegmentsPerKey_ = 3;
        tunings_ = new float[36];
        for(int i = 0; i < 36; i++) {
            // Start with fraction of an octave, round to the nearest 31st of an octave
            float original = (float)i / 36.0;
            float rounded = floorf(original * 31.0 + 0.5);
            
            // Now convert the 31-tone index back to a fractional number of semitones
            tunings_[i] = rounded * 1200.0 / 31.0;
        }
    }
    else if(tuningPreset_ == kTuningPreset36TET) {
        numSegmentsPerKey_ = 3;
        tunings_ = new float[36];
        for(int i = 0; i < 24; i++) {
            tunings_[i] = (float)i * 100.0 / 3.0;
        }
    }
    else if(tuningPreset_ == kTuningPresetYarman24c) {
        numSegmentsPerKey_ = 2;
        tunings_ = new float[24];
        for(int i = 0; i < 24; i++)
            tunings_[i] = kTuningsYarman24c[i];
    }
    
    KeyboardDisplay *display = keyboard_.gui();
    if(display != nullptr) {
        display->removeKeyDivision(this);
        display->addKeyDivision(this, keyboardSegment_.noteRange().first, keyboardSegment_.noteRange().second, numSegmentsPerKey_);
    }
}

#ifndef TOUCHKEYS_NO_GUI
// ***** GUI Support *****
std::unique_ptr< MappingEditorComponent > TouchkeyKeyDivisionMappingFactory::createBasicEditor() {
    return std::make_unique< TouchkeyKeyDivisionMappingShortEditor >(*this);
}
#endif

// ****** Preset Save/Load ******
std::unique_ptr< juce::XmlElement > TouchkeyKeyDivisionMappingFactory::getPreset() {
    juce::PropertySet properties;
    
    storeCommonProperties(properties);
    
    // No properties for now
    
    auto preset = properties.createXml("MappingFactory");
    preset->setAttribute("type", "KeyDivision");
    
    return preset;
}

bool TouchkeyKeyDivisionMappingFactory::loadPreset(juce::XmlElement const* preset) {
    if(preset == nullptr )
        return false;
    
    juce::PropertySet properties;
    properties.restoreFromXml(*preset);
    
    if(!loadCommonProperties(properties))
        return false;

    // Nothing specific to do for now
    
    return true;
}

// ***** Private Methods *****

// Set the initial parameters for a new mapping
void TouchkeyKeyDivisionMappingFactory::initializeMappingParameters(int noteNumber, TouchkeyKeyDivisionMapping *mapping) {
    juce::ScopedLock sl(tuningMutex_);
    
    // Convert absolute tunings into pitch bends in semitones
    float tunings[kMaxSegmentsPerKey];
    
    int index = (noteNumber + 12 - referenceNote_) % 12;
    float standardTuning = (float)index * 100.0;
    
    for(int i = 0; i < numSegmentsPerKey_; i++) {
        tunings[i] = (tunings_[index*numSegmentsPerKey_ + i] - standardTuning + globalOffsetCents_) * .01;
    }
    mapping->setSegmentPitchBends(tunings, numSegmentsPerKey_);
    mapping->setNumberOfSegments(numSegmentsPerKey_);
    mapping->setTimeout(timeout_);
    mapping->setDetectionParameter(detectionParameter_);
    mapping->setRetriggerable(retriggerable_, retriggerNumFrames_, retriggerKeepsVelocity_);
}

void TouchkeyKeyDivisionMappingFactory::setBendParameters() {
    // Range of 0 indicates special case of using global pitch wheel range
    setMidiParameters(MidiKeyboardSegment::kControlPitchWheel, 0.0, 0.0, 0.0);
    
    if(midiConverter_ != nullptr) {
        midiConverter_->listenToIncomingControl(MidiKeyboardSegment::kControlPitchWheel);
    }
}