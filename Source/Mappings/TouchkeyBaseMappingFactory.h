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

  TouchkeyBaseMappingFactory.h: base factory class specifically for
  TouchKeys mappings. It provides a collection of useful methods for
  creating and destroying individual mappings on touch/MIDI onset and
  release, as well as parameter adjustment code and OSC to MIDI conversion.
  This is a template class that must be created with a specific Mapping
  subclass.
*/

#pragma once

#include "MappingFactory.h"
#include "MappingScheduler.h"
#include "../TouchKeys/OscMidiConverter.h"
#include "../TouchKeys/MidiOutputController.h"
#include "../TouchKeys/MidiKeyboardSegment.h"
#include <sstream>

#undef DEBUG_TOUCHKEY_BASE_MAPPING_FACTORY

// Base class for mapping factories that meet the following criteria:
// * MIDI and TouchKeys data (no continuous angle)
// * Mappings begin when either or touch or MIDI starts and end when both finish
// * Each mapping object affects a single note

template <class MappingType>
class TouchkeyBaseMappingFactory : public MappingFactory {
    
public:
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    TouchkeyBaseMappingFactory(PianoKeyboard &keyboard, MidiKeyboardSegment& segment) :
      MappingFactory(keyboard), keyboardSegment_(segment), midiConverter_(nullptr),
      controlName_(""), shortControlName_(""),
      inputRangeMin_(0.0), inputRangeMax_(1.0), inputRangeCenter_(0.0),
      outOfRangeBehavior_(OscMidiConverter::kOutOfRangeClip),
      use14BitControl_(false),
      midiControllerNumber_(-1), bypassed_(false), activeNotes_(0x0FFF) {}
    
    // ***** Destructor *****
    
    virtual ~TouchkeyBaseMappingFactory()  {
        removeAllMappings();
        if(midiConverter_ != nullptr && controlName_ != "")
            midiConverter_->removeControl(controlName_.c_str());
        if(midiControllerNumber_ >= 0) {
            keyboardSegment_.releaseOscMidiConverter(midiControllerNumber_);
        }
    }
    
    // ***** Accessors / Modifiers *****
   
    // Return the keyboard segment associated with this factory
    MidiKeyboardSegment& segment() { return keyboardSegment_; }
    
    // Look up a mapping with the given note number
    virtual MappingType* mapping(int noteNumber) {
        juce::ScopedLock sl(mappingsMutex_);
        if(mappings_.count(noteNumber) == 0)
            return nullptr;
        return mappings_[noteNumber];
    }
    
    // Return a list of all active notes
    virtual std::vector<int> activeMappings()  {
        juce::ScopedLock sl(mappingsMutex_);
        std::vector<int> keys;
        typename std::map<int, MappingType*>::iterator it = mappings_.begin();
        while(it != mappings_.end()) {
            int nextKey = (it++)->first;
            keys.push_back(nextKey);
        }
        return keys;
    }

    // Remove all active mappings
    virtual void removeAllMappings() {
        juce::ScopedLock sl(mappingsMutex_);
        typename std::map<int, MappingType*>::iterator it = mappings_.begin();
        
        while(it != mappings_.end()) {
            // Delete everybody in the container
            MappingType *mapping = it->second;
#ifdef NEW_MAPPING_SCHEDULER
            mapping->disengage(true);
            //keyboard_.mappingScheduler().unscheduleAndDelete(mapping);
#else
            mapping->disengage();
            delete mapping;
#endif
            it++;
        }
        
        // Now clear the container
        mappings_.clear();
    }
    
    // Callback from mapping to say it's finished
    virtual void mappingFinished(int noteNumber) {
        juce::ScopedLock sl(mappingsMutex_);
        removeMapping(noteNumber);
    }
    
    // Suspend messages from a particular note
    virtual void suspendMapping(int noteNumber) {
        juce::ScopedLock sl(mappingsMutex_);
        if(mappings_.count(noteNumber) == 0)
            return;
        mappings_[noteNumber]->suspend();
    }
    
    // Suspend messages from all notes
    virtual void suspendAllMappings() {
        juce::ScopedLock sl(mappingsMutex_);
        typename std::map<int, MappingType*>::iterator it = mappings_.begin();
        
        while(it != mappings_.end()) {
            //std::cout << "suspending mapping on note " << it->first << std::endl;
            it->second->suspend();
            it++;
        }
    }
    
    // Resume messages from a particular note
    virtual void resumeMapping(int noteNumber, bool resend) {
        juce::ScopedLock sl(mappingsMutex_);
        if(mappings_.count(noteNumber) == 0)
            return;
        //std::cout << "resuming mapping on note " << noteNumber << std::endl;
        mappings_[noteNumber]->resume(resend);
    }
    
    // Resume messages on all notes
    virtual void resumeAllMappings(bool resend) {
        juce::ScopedLock sl(mappingsMutex_);
        typename std::map<int, MappingType*>::iterator it = mappings_.begin();
        
        while(it != mappings_.end()) {
            it->second->resume(resend);
            it++;
        }
    }
    
    // Whether this mapping is bypassed
    virtual int bypassed() {
        return bypassed_ ? kBypassOn : kBypassOff;
    }
    
    // Set whether the mapping is bypassed or not
    virtual void setBypassed(bool bypass) {
        bypassed_ = bypass;
    }
    
    // ***** Class-Specific Methods *****
    
    virtual void setMidiParameters(int controller, float inputMinValue, float inputMaxValue, float inputCenterValue,
                           int outputDefaultValue = -1, int outputMinValue = -1, int outputMaxValue = -1,
                           int outputCenterValue = -1, bool use14BitControl = false,
                           int outOfRangeBehavior = OscMidiConverter::kOutOfRangeClip) {
        if(controller < 0)
            return;
        
        inputRangeMin_ = inputMinValue;
        inputRangeMax_ = inputMaxValue;
        inputRangeCenter_ = inputCenterValue;
        outOfRangeBehavior_ = outOfRangeBehavior;
        use14BitControl_ = use14BitControl;
        
        // Remove listener on previous name (if any)
        //midiConverter_.removeAllControls();
        if(midiControllerNumber_ >= 0 && controller != midiControllerNumber_) {
            keyboardSegment_.releaseOscMidiConverter(midiControllerNumber_);
            midiConverter_ = keyboardSegment_.acquireOscMidiConverter(controller);
        }
        else if(midiControllerNumber_ < 0 || midiConverter_ == nullptr) {
            midiConverter_ = keyboardSegment_.acquireOscMidiConverter(controller);
        }        
        midiControllerNumber_ = controller;

        midiConverter_->setMidiMessageType(outputDefaultValue, outputMinValue, outputMaxValue, outputCenterValue, use14BitControl);

        // Add listener for new name
        if(controlName_ != "")
            midiConverter_->addControl(controlName_.c_str(), 1, inputRangeMin_, inputRangeMax_, inputRangeCenter_, outOfRangeBehavior_);
    }
    
    virtual std::string const getName() { return controlName_; }
    virtual std::string const getShortName() { return shortControlName_; }
    
    virtual void setName(const std::string& name) {
        if(name == "")
            return;
        shortControlName_ = name;
        
        std::stringstream ss;
        
        // Remove listener on previous name (if any)
        if(midiConverter_ != nullptr && controlName_ != "")
            midiConverter_->removeControl(controlName_.c_str());
        
        ss << "/touchkeys/mapping/segment" << (int)keyboardSegment_.outputPort() << "/" << name;
        controlName_ = ss.str();

        // Add listener for new name
        if(midiConverter_ != nullptr)
            midiConverter_->addControl(controlName_.c_str(), 1, inputRangeMin_, inputRangeMax_, inputRangeCenter_, outOfRangeBehavior_);
    }
    
    // Set which keys should have this mapping enable
    virtual void setActiveNotes(unsigned int notes) {
        activeNotes_ = notes;
    }
    
    // ****** Preset Save/Load ******
    
    // These generate XML settings files and reload settings from them
    
    virtual std::unique_ptr< juce::XmlElement > getPreset() {
        juce::PropertySet properties;
        storeCommonProperties(properties);
        
        auto presetElement = properties.createXml("MappingFactory");
        presetElement->setAttribute("type", "Unknown");
        return presetElement;
    }
    
    virtual bool loadPreset(juce::XmlElement const* preset) {
        if(preset == nullptr)
            return false;
        
        juce::PropertySet properties;
        properties.restoreFromXml(*preset);
        
        if(!loadCommonProperties(properties))
            return false;
        return true;
    }
    
    // ***** State Updaters *****
    
    // These are called by PianoKey whenever certain events occur that might
    // merit the start and stop of a mapping. What is done with them depends on
    // the particular factory subclass.
    
    // Touch becomes active on a key where it wasn't previously
    virtual void touchBegan(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker)  {
        juce::ScopedLock sl(mappingsMutex_);
        // Add a new mapping if one doesn't exist already
        if(mappings_.count(noteNumber) == 0) {
#ifdef DEBUG_TOUCHKEY_BASE_MAPPING_FACTORY
            std::cout << "Note " << noteNumber << ": adding mapping (touch)\n";
#endif
            int moduloNoteNumber = noteNumber % 12;
            if((activeNotes_ & (1 << moduloNoteNumber)) && !bypassed_)
                addMapping(noteNumber, touchBuffer, positionBuffer, positionTracker);
        }
    }
    
    // Touch ends on a key where it wasn't previously
    virtual void touchEnded(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker) {
        juce::ScopedLock sl(mappingsMutex_);
        // If a mapping exists but the MIDI note is off, remove the mapping
        if(mappings_.count(noteNumber) != 0 && !midiNoteIsOn) {
#ifdef DEBUG_TOUCHKEY_BASE_MAPPING_FACTORY
            std::cout << "Note " << noteNumber << ": removing mapping (touch)\n";
#endif
            if(mappings_[noteNumber]->requestFinish())
                removeMapping(noteNumber);
        }
    }
    
    // MIDI note on for a key
    virtual void midiNoteOn(int noteNumber, bool touchIsOn, bool keyMotionActive,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker)  {
        juce::ScopedLock sl(mappingsMutex_);
        // Add a new mapping if one doesn't exist already
        if(mappings_.count(noteNumber) == 0) {
#ifdef DEBUG_TOUCHKEY_BASE_MAPPING_FACTORY
            std::cout << "Note " << noteNumber << ": adding mapping (MIDI)\n";
#endif
            int moduloNoteNumber = noteNumber % 12;
            if((activeNotes_ & (1 << moduloNoteNumber)) && !bypassed_)
                addMapping(noteNumber, touchBuffer, positionBuffer, positionTracker);
        }
    }

    // MIDI note off for a key
    virtual void midiNoteOff(int noteNumber, bool touchIsOn, bool keyMotionActive,
                     Node<KeyTouchFrame>* touchBuffer,
                     Node<key_position>* positionBuffer,
                     KeyPositionTracker* positionTracker)  {
        juce::ScopedLock sl(mappingsMutex_);
        // If a mapping exists but the touch is off, remove the mapping
        if(mappings_.count(noteNumber) != 0 && !touchIsOn) {
#ifdef DEBUG_TOUCHKEY_BASE_MAPPING_FACTORY
            std::cout << "Note " << noteNumber << ": removing mapping (MIDI)\n";
#endif
            if(mappings_[noteNumber]->requestFinish())
                removeMapping(noteNumber);
        }
    }
    
    // Subclasses of this one won't care about these two methods:
    
    // Key goes active from continuous key position
    virtual void keyMotionActive(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                         Node<KeyTouchFrame>* touchBuffer,
                         Node<key_position>* positionBuffer,
                         KeyPositionTracker* positionTracker) {}
    // Key goes idle from continuous key position
    virtual void keyMotionIdle(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                       Node<KeyTouchFrame>* touchBuffer,
                       Node<key_position>* positionBuffer,
                       KeyPositionTracker* positionTracker) {}
    
    // But we do use this one to send out default values:
    virtual void noteWillBegin(int noteNumber, int midiChannel, int midiVelocity) {
        if(midiConverter_ == nullptr)
            return;
        midiConverter_->clearLastValues(midiChannel, true);
    }
    
    // ****** OSC Control ******
    // As an alternative to GUI control, the mapping factories can receive OSC messages
    // from the keyboard segment to which they are attached.
    virtual OscMessage* oscControlMethod(const char *path, const char *types,
                                  int numValues, lo_arg **values, void *data) {
        if(!strcmp(path, "/set-bypass")) {
            // Enable/disable suspend mapping
            if(numValues > 0) {
                if(types[0] == 'i') {
                    if(values[0]->i != 0)
                        setBypassed(true);
                    else
                        setBypassed(false);
                    return OscTransmitter::createSuccessMessage();
                }
            }
        }
        else if(!strcmp(path, "/set-active-notes")) {
            // Set which notes it applies to
            // Bitmask: lower 12 bits of the number for pitch classes 0-11
            if(numValues > 0) {
                if(types[0] == 'i') {
                    setActiveNotes((values[0]->i) & 0x0FFF);
                    
                    return OscTransmitter::createSuccessMessage();
                }
            }
        }
        
        return nullptr;
    }

    
protected:
    // ***** Protected Methods *****
    
    // This method should be set by the subclass to initialize the parameters of
    // a new mapping.
    virtual void initializeMappingParameters(int noteNumber, MappingType *mapping) {}
    
    // This method adds the common mapping properties to the given PropertySet
    void storeCommonProperties( juce::PropertySet& properties) {
        properties.setValue("controlName", juce::String(controlName_));
        properties.setValue("inputRangeMin", inputRangeMin_);
        properties.setValue("inputRangeMax", inputRangeMax_);
        properties.setValue("inputRangeCenter", inputRangeCenter_);
        properties.setValue("outOfRangeBehavior", outOfRangeBehavior_);
        properties.setValue("midiControllerNumber", midiControllerNumber_);
        properties.setValue("bypassed", bypassed_);
        properties.setValue("activeNotes", (int)activeNotes_);
    }
    
    // This method loads the common mapping properties from the given PropertySet
    bool loadCommonProperties( juce::PropertySet const& properties) {
        if(!properties.containsKey("controlName") ||
           !properties.containsKey("inputRangeMin") ||
           !properties.containsKey("inputRangeMax") ||
           !properties.containsKey("inputRangeCenter") ||
           !properties.containsKey("outOfRangeBehavior") ||
           !properties.containsKey("midiControllerNumber") ||
           !properties.containsKey("bypassed") ||
           !properties.containsKey("activeNotes")) {
            return false;
        }
        
        // Setting the MIDI controller number needs to be done with
        // the setMidiParameters() method which will update midiControllerNumber_
        int tempMidiController = 1;
        
        controlName_ = properties.getValue("controlName").toUTF8();
        inputRangeMin_ = properties.getDoubleValue("inputRangeMin");
        inputRangeMax_ = properties.getDoubleValue("inputRangeMax");
        inputRangeCenter_ = properties.getDoubleValue("inputRangeCenter");
        outOfRangeBehavior_ = properties.getIntValue("outOfRangeBehavior");
        tempMidiController = properties.getIntValue("midiControllerNumber");
        bypassed_ = properties.getBoolValue("bypassed");
        activeNotes_ = properties.getIntValue("activeNotes");
        
        setMidiParameters(tempMidiController, inputRangeMin_, inputRangeMax_, inputRangeCenter_);
        
        return true;
    }
    
private:
    // ***** Private Methods *****
    
    // Add a new mapping
    void addMapping(int noteNumber,
                    Node<KeyTouchFrame>* touchBuffer,
                    Node<key_position>* positionBuffer,
                    KeyPositionTracker* positionTracker)  {
        // TODO: mutex
        removeMapping(noteNumber);  // Free any mapping that's already present on this note
        
        MappingType *mapping = new MappingType(keyboard_, this, noteNumber, touchBuffer,
                                               positionBuffer, positionTracker);

        // Set parameters
        mapping->setName(controlName_);
        initializeMappingParameters(noteNumber, mapping);
        
        // Save the mapping
        mappings_[noteNumber] = mapping;

        // Finally, engage the new mapping
        mapping->engage();
    }
    
    void removeMapping(int noteNumber)  {
        // TODO: mutex
        if(mappings_.count(noteNumber) == 0)
            return;
        MappingType* mapping = mappings_[noteNumber];
#ifdef NEW_MAPPING_SCHEDULER
        mapping->disengage(true);
        //keyboard_.mappingScheduler().unscheduleAndDelete(mapping);
#else
        mapping->disengage();
        delete mapping;
#endif
        mappings_.erase(noteNumber);
    }

protected:
    // State variables
    MidiKeyboardSegment& keyboardSegment_;         // Segment of the keyboard that this mapping addresses
    OscMidiConverter *midiConverter_;              // Object to convert OSC messages to MIDI
    std::map<int, MappingType*> mappings_;         // Collection of active mappings
    juce::CriticalSection mappingsMutex_;                // Mutex protecting mappings from changes
    
    std::string controlName_;                           // Name of the mapping in long..
    std::string shortControlName_;                      // ... and short forms
    float inputRangeMin_, inputRangeMax_;               // Input ranges
    float inputRangeCenter_;      
    int outOfRangeBehavior_;                            // What happens to out of range inputs
    bool use14BitControl_;                              // Whether to use a 14-bit control

    int midiControllerNumber_;                          // Which controller to use
    bool bypassed_;                                     // Whether the mapping has been bypassed by UI
    unsigned int activeNotes_;                          // Indication of which notes out of the 12 to use
};
