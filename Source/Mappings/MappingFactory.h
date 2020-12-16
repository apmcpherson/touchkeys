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

  MappingFactory.h: base class for creating mappings. A factory is a singular
  object, attached to a particular keyboard segment, which in turn allocates
  and destroys individual Mapping objects for each active note. The factory
  also is the usual point at which parameter changes are made.
*/

#pragma once

#include "Mapping.h"
#include "../GUI/MappingEditorComponent.h"

// This virtual base class defines a singular factory object from which individual
// instances of mapping objects can be created and destroyed. How the mappings are
// allocated and when is up to the factory, which also keeps track of which ones
// are active. The PianoKey class will call into any active factories when certain
// events occur: touch on/off, MIDI on/off, key idle/active.

class MappingFactory {
public:
    // States for bypass status
    enum {
        kBypassOff = 0,
        kBypassOn,
        kBypassMixed
    };
    
    // ***** Constructor *****
    
	// Default constructor, containing a reference to the PianoKeyboard class.
    MappingFactory(PianoKeyboard &keyboard) : keyboard_(keyboard) {}
	
    // ***** Destructor *****
    
    virtual ~MappingFactory() {}
    
    // ***** Accessors / Modifiers *****
    
    // Generic name for this type of factory
    virtual const std::string factoryTypeName() { return "Unknown\nMapping"; }
    
    // Specific name for this particular factory
    virtual std::string const getName() { return ""; }
    virtual std::string const getShortName() { return ""; }
    virtual void setName(const std::string& name) {}
    
    virtual Mapping* mapping(int noteNumber) = 0;      // Look up a mapping with the given note number
    virtual std::vector<int> activeMappings() = 0;     // Return a list of all active notes
    
    virtual void removeAllMappings() = 0;              // Remove all active mappings
    virtual void mappingFinished(int noteNumber) = 0;  // Callback from mapping to say it's done
    
    // Suspending mappings is a state managed internally by the TouchKeys
    // controllers, for example to turn off a mapping of an older note in monophonic
    // mode. By contrast, bypassing a mapping is intended to be manipulated from
    // an external UI.
    
    virtual void suspendMapping(int noteNumber) = 0;    // Suspend messages from a particular note
    virtual void suspendAllMappings() = 0;              // ... or all notes
    virtual void resumeMapping(int noteNumber, bool resend) = 0;  // Resume messages from a particular note
    virtual void resumeAllMappings(bool resend) = 0;              // ... or all notes
    
    virtual int bypassed() = 0;                     // Whether this mapping is bypassed
    virtual void setBypassed(bool bypass) = 0;      // Set whether the mapping is bypassed or not
    
    // ***** State Updaters *****
    
    // These are called by PianoKey whenever certain events occur that might
    // merit the start and stop of a mapping. What is done with them depends on
    // the particular factory subclass. The relevant buffers are passed in each
    // time so the factory or the mapping can make use of them
    
    // Touch becomes active on a key where it wasn't previously
    virtual void touchBegan(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                            Node<KeyTouchFrame>* touchBuffer,
                            Node<key_position>* positionBuffer,
                            KeyPositionTracker* positionTracker) = 0;
    // Touch ends on a key where it wasn't previously
    virtual void touchEnded(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                            Node<KeyTouchFrame>* touchBuffer,
                            Node<key_position>* positionBuffer,
                            KeyPositionTracker* positionTracker) = 0;
    // MIDI note on for a key
    virtual void midiNoteOn(int noteNumber, bool touchIsOn, bool keyMotionActive,
                            Node<KeyTouchFrame>* touchBuffer,
                            Node<key_position>* positionBuffer,
                            KeyPositionTracker* positionTracker) = 0;
    // MIDI note off for a key
    virtual void midiNoteOff(int noteNumber, bool touchIsOn, bool keyMotionActive,
                             Node<KeyTouchFrame>* touchBuffer,
                             Node<key_position>* positionBuffer,
                             KeyPositionTracker* positionTracker) = 0;
    // Key goes active from continuous key position
    virtual void keyMotionActive(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                                 Node<KeyTouchFrame>* touchBuffer,
                                 Node<key_position>* positionBuffer,
                                 KeyPositionTracker* positionTracker) = 0;
    // Key goes idle from continuous key position
    virtual void keyMotionIdle(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                               Node<KeyTouchFrame>* touchBuffer,
                               Node<key_position>* positionBuffer,
                               KeyPositionTracker* positionTracker) = 0;
    
    // Notification from key that a note is about to be sent out
    virtual void noteWillBegin(int noteNumber, int midiChannel, int midiVelocity) = 0;
    
#ifndef TOUCHKEYS_NO_GUI
    // ***** GUI Support *****
    // There are two types of editors for a mapping: one is a small editor that fits in the
    // list view for adjusting the most important parameters, the other goes in a window of
    // its own to adjust every parameter.
    
    virtual bool hasBasicEditor() { return false; }
    virtual std::unique_ptr< MappingEditorComponent > createBasicEditor() { return nullptr; }
    virtual bool hasExtendedEditor() { return false; }
    virtual std::unique_ptr< MappingEditorComponent > createExtendedEditor() { return nullptr; }
#endif
    
    // ****** OSC Control ******
    // As an alternative to GUI control, the mapping factories can receive OSC messages
    // from the keyboard segment to which they are attached.
    virtual OscMessage* oscControlMethod(const char *path, const char *types,
                                         int numValues, lo_arg **values, void *data) {
        // Nothing to do here in this virtual base class
        return 0;
    }
    
    // ****** Preset Save/Load ******
    // These methods generate XML settings files and reload values from them
    // The specific implementation is up to the subclass
    
    virtual std::unique_ptr< juce::XmlElement > getPreset() = 0;
    virtual bool loadPreset(juce::XmlElement const* preset) = 0;
    
protected:
	// ***** Member Variables *****
	
    PianoKeyboard& keyboard_;                   // Reference to the main keyboard controller
    
private:
    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingFactory)
};
