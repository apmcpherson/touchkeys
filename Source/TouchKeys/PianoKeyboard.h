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
 
  PianoKeyboard.h: main class that keeps track of each key (and pedal)
  on the keyboard, while also providing hooks for mapping and scheduling
  of events. One shared instance of this class is used widely throughout
  the program.
*/

#pragma once

#include "Osc.h"
#include "PianoKey.h"
#include "PianoPedal.h"
#include "../Display/KeyboardDisplay.h"
#include "../Display/KeyPositionGraphDisplay.h"
#include "../Utility/Scheduler.h"

#define NUM_KEYS 88
#define NUM_PEDALS 3

enum {							// Index of each pedal in the buffers
	kPedalDamper = 0,
	kPedalSostenuto = 1,
	kPedalUnaCorda = 2,
	kNumPedals
};

const int kDefaultKeyHistoryLength = 8192;
const int kDefaultPedalHistoryLength = 1024;

class TouchkeyDevice;
class Mapping;
class MidiOutputController;
class MappingFactory;
class MidiKeyboardSegment;
class MappingScheduler;

/*
 * PianoKeyboard
 *
 * Base class that implements all the functionality needed to measure and process
 * real-time piano key motion.  This class is abstract in that it doesn't define a particular
 * source for the key motion data.  The data source depends on the hardware used: PianoBar,
 * PnoScan II, CEUS, etc.
 *
 * PianoKeyboard is a source of OSC messages (generated by various key actions).  Hence,
 * objects can register with it to implement custom behavior for different actions.
 */

class PianoKeyboard : public OscMessageSource {
public:
	std::ofstream testLog_;
    
	// ***** Constructors *****
	
	PianoKeyboard();
	
	// ***** Destructor *****
	
	~PianoKeyboard();
	
	// ***** Control Methods *****
	//
	// These methods start and stop the device and all associated processing.  The implementation
	// is specific to the hardware used.  Methods return true on success.
	
	//virtual bool start() = 0;
	//virtual bool stop() = 0;
	
	bool isInitialized() { return isInitialized_; }
	bool isRunning() { return isRunning_; }
	
	void reset();
	
	std::pair<int, int> keyboardGUIRange() { return std::pair<int, int>(lowestMidiNote_, highestMidiNote_); }
	void setKeyboardGUIRange(int lowest, int highest);
	
	int numberOfPedals() { return numberOfPedals_; }
	void setNumberOfPedals(int number);
	
	// ***** Communication Links and Methods *****
	
    // Set/query the output controller
	MidiOutputController* midiOutputController() { return midiOutputController_; }
	void setMidiOutputController(MidiOutputController* ct) { midiOutputController_ = ct; }
    
	// Set reference to GUI displays
	KeyboardDisplay* gui() { return gui_; }
	void setGUI(KeyboardDisplay* gui);
    KeyPositionGraphDisplay *graphGUI() { return graphGui_; }
    void setGraphGUI(KeyPositionGraphDisplay* newGui) { graphGui_ = newGui; }
	
	// OSC transmitter handles the mechanics of sending messages to one or more targets
	void setOscTransmitter(OscTransmitter* trans) { oscTransmitter_ = trans; }
    
    // TouchkeyDevice handles communication with the touch-sensor/piano-scanner hardware
    void setTouchkeyDevice(TouchkeyDevice* device) { touchkeyDevice_ = device; }
	
	// Send a named message by OSC (and potentially by MIDI or other means if suitable listeners
	// are enabled)
	void sendMessage(const char * path, const char * type, ...);
	
	// ***** Scheduling Methods *****
	
	// Add or remove events from the scheduler queue
	void scheduleEvent(void *who, Scheduler::action func, timestamp_type timestamp) {
		futureEventScheduler_.schedule(who, func, timestamp);
	}
    void unscheduleEvent(void *who) {
		futureEventScheduler_.unschedule(who);
	}
	void unscheduleEvent(void *who, timestamp_type timestamp) {
		futureEventScheduler_.unschedule(who, timestamp);
	}
	
	// Return the current timestamp associated with the scheduler
	timestamp_type schedulerCurrentTimestamp() { return futureEventScheduler_.currentTimestamp(); }
	
	// ***** Individual Key/Pedal Methods *****
	
	// Access to individual keys and pedals
	PianoKey* key(int note) {
		//if(note < lowestMidiNote_ || note > highestMidiNote_)
		//	return nullptr;
		//return keys_[note - lowestMidiNote_];
        if(note < 0 || note > 127)
            return nullptr;
        return keys_[note];
	}
	PianoPedal* pedal(int pedal) {
		if(pedal < 0 || pedal >= numberOfPedals_)
			return nullptr;
		return pedals_[pedal];
	}
	
	// Keys and pedals are enabled by default.  If one has been disabled, reenable it so it reads data
	// and triggers notes, as normal.
	void enableKey(int key);
	void enablePedal(int pedal);
	
	// Disable a key or pedal from causing any activity to occur.
	void disableKey(int key);
	void disablePedal(int pedal);
	
	// Leave a key enabled, but terminate any activity it has initiated and return it to the idle state.
	// If the key is active because of a hardware problem, this may be a short-term solution at best, requiring
	// the key to be disabled until the problem can be properly resolved.
	void forceKeyIdle(int key);
    
    // Set the color of an RGB LED for the given key, if relevant hardware is present
    void setKeyLEDColorRGB(const int note, const float red, const float green, const float blue);
    void setKeyLEDColorHSV(const int note, const float hue, const float saturation, const float value);
    void setAllKeyLEDsOff();
    
    // ***** Mapping Methods *****
    // Mappings are identified by the MIDI note they affect and by
    // their owner object.
    void addMapping(int noteNumber, Mapping* mapping);    // Add a new mapping to the container
    void removeMapping(int noteNumber);                  // Remove a mapping from the container
    Mapping* mapping(int noteNumber);                    // Look up a mapping with the given note and owner
    std::vector<int> activeMappings();                   // Return a list of all active note mappings
    void clearMappings();                                // Remove all mappings
    
    // Managing the mapping factories
    MappingFactory *mappingFactory(MidiKeyboardSegment* segment) {
        juce::ScopedReadLock sl(mappingFactoriesMutex_);
        if(mappingFactories_.count(segment) == 0)
            return nullptr;
        return mappingFactories_[segment];
    }
    void setMappingFactory(MidiKeyboardSegment* segment, MappingFactory *factory) {
        juce::ScopedWriteLock sl(mappingFactoriesMutex_);
        mappingFactories_[segment] = factory;
    }
    void removeMappingFactory(MidiKeyboardSegment* segment) {
        juce::ScopedWriteLock sl(mappingFactoriesMutex_);
        if(mappingFactories_.count(segment) > 0)
            mappingFactories_.erase(segment);
    }
    
    // Passing data to all mapping factories; these methods are not specific to a particular
    // MIDI input segment so we need to check with each factory whether it wants this data.
    void tellAllMappingFactoriesTouchBegan(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                                           Node<KeyTouchFrame>* touchBuffer,
                                           Node<key_position>* positionBuffer,
                                           KeyPositionTracker* positionTracker);
    void tellAllMappingFactoriesTouchEnded(int noteNumber, bool midiNoteIsOn, bool keyMotionActive,
                                           Node<KeyTouchFrame>* touchBuffer,
                                           Node<key_position>* positionBuffer,
                                           KeyPositionTracker* positionTracker);
    void tellAllMappingFactoriesKeyMotionActive(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                                                Node<KeyTouchFrame>* touchBuffer,
                                                Node<key_position>* positionBuffer,
                                                KeyPositionTracker* positionTracker);
    void tellAllMappingFactoriesKeyMotionIdle(int noteNumber, bool midiNoteIsOn, bool touchIsOn,
                                              Node<KeyTouchFrame>* touchBuffer,
                                              Node<key_position>* positionBuffer,
                                              KeyPositionTracker* positionTracker);
    
    MappingScheduler& mappingScheduler() { return *mappingScheduler_; }
	
	// ***** Member Variables *****
public:
    // This mutex is grabbed by any thread which is supplying performance
    // data (MIDI or touch). By synchronizing access once centrally, we
    // can avoid many other lock scenarios in individual objects. The object
    // is declared public so it can be used in ScopedLocks.
    juce::CriticalSection performanceDataMutex_;
    
private:
	// Individual key and pedal data structures
	std::vector<PianoKey*> keys_;
	std::vector<PianoPedal*> pedals_;	
	
	// Reference to GUI display (if present)
	KeyboardDisplay* gui_;
    KeyPositionGraphDisplay *graphGui_;
    
    // Reference to the MIDI output controller
    MidiOutputController* midiOutputController_;
	
	// Reference to message transmitter class
	OscTransmitter* oscTransmitter_;
    
    // Reference to TouchKey hardware controller class
    TouchkeyDevice* touchkeyDevice_;
	
	// Keyboard range, expressed in MIDI note numbers
	int lowestMidiNote_, highestMidiNote_;
	int numberOfPedals_;
	
	bool isInitialized_;
	bool isRunning_;
	bool isCalibrated_;
	bool calibrationInProgress_;
	
	// Mapping objects associated with particular messages
	// When a sendMessage() command is received, it checks the message
	// against this set of possible listeners to see if it matches the
	// path.  If so, the handler function is called.
	std::multimap<std::string, OscHandler*> messageListeners_;
	
	// This object can be used to schedule events to be executed at future timestamps,
	// for example to handle timeouts.  This will often be called from within a particular
	// key, but we should maintain one central repository for these events.
	Scheduler futureEventScheduler_;
    
    // Data related to mappings for active notes
    std::map<int, Mapping*> mappings_;            // Mappings from key motion to sound
    
    // Collection of mapping factories organised by segment of the keyboard. Different
    // segments may have different mappings
    std::map<MidiKeyboardSegment*, MappingFactory*> mappingFactories_;
    juce::ReadWriteLock mappingFactoriesMutex_;
    
    // Scheduler specifically used for coordinating mappings
    MappingScheduler *mappingScheduler_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboard)
};
