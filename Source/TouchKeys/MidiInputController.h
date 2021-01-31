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
 
  MidiInputController.h: handles incoming MIDI data and manages input
  ports. Detailed processing is broken down by keyboard segment; see
  MidiKeyboardSegment.h/cpp for more.
*/

#pragma once

#include "PianoKeyboard.h"
#include "MidiKeyboardSegment.h"

class MidiOutputController;

// MIDI standard messages

enum {
	kMidiMessageNoteOff = 0x80,
	kMidiMessageNoteOn = 0x90,
	kMidiMessageAftertouchPoly = 0xA0,
	kMidiMessageControlChange = 0xB0,
	kMidiMessageProgramChange = 0xC0,
	kMidiMessageAftertouchChannel = 0xD0,
	kMidiMessagePitchWheel = 0xE0,
	kMidiMessageSysex = 0xF0,
	kMidiMessageSysexEnd = 0xF7,
	kMidiMessageActiveSense = 0xFE,
	kMidiMessageReset = 0xFF
};

enum {
	kMidiControlAllSoundOff = 120,
	kMidiControlAllControllersOff = 121,
	kMidiControlLocalControl = 122,
	kMidiControlAllNotesOff = 123
};

class MidiInputController : public juce::MidiInputCallback {
public:
	// Constructor
	MidiInputController(PianoKeyboard& keyboard);
	
	// Query available devices
	std::vector< std::pair<int, std::string> > availableMidiDevices();
	
	// Add/Remove MIDI input ports;
	// Enable methods return true on success (at least one port enabled) 
	bool enablePort(int portNumber, bool isPrimary);
	bool enableAllPorts(int primaryPortNumber);
	void disablePort(int portNumber);
    void disablePrimaryPort();
	void disableAllPorts(bool auxiliaryOnly);
    int primaryActivePort();
    std::vector<int> auxiliaryActivePorts();
    
    // Get the name of a particular port index
    juce::String deviceName(int portNumber);
    int indexOfDeviceNamed(juce::String const& name);

	// Set/query the output controller
	MidiOutputController* midiOutputController() { return midiOutputController_; }
	void setMidiOutputController(MidiOutputController* ct);
	
	// All Notes Off: can be sent by MIDI or controlled programmatically
	void allNotesOff();
    
    // Return the number of keyboard segments, and a specific segment
    int numSegments() {
        juce::ScopedLock sl(segmentsMutex_);
        return segments_.size();
    }
    MidiKeyboardSegment* segment(int num) {
        juce::ScopedLock sl(segmentsMutex_);
        if(num < 0 || num >= segments_.size())
            return nullptr;
        return segments_[num];
    }
    // Return a unique signature which tells us when the MIDI segments have changed,
    // allowing any listeners to re-query all the segments.
    int segmentUniqueIdentifier() {
        return segmentUniqueIdentifier_;
    }

    // Add a new keyboard segment. Returns a pointer to the newly created segment
    MidiKeyboardSegment* addSegment(int outputPortNumber, int noteMin = 0, int noteMax = 127, int channelMask = 0xFFFF);
    
    // Remove a segment by index or by object
    bool removeSegment(int index);
    bool removeSegment(MidiKeyboardSegment* segment);
    void removeAllSegments();
    
    // Preset save/load for keyboard segments
    juce::XmlElement* getSegmentPreset();
    bool loadSegmentPreset(juce::XmlElement const* preset);
    
    // OSC handling for keyboard segments
    OscMessage* oscControlMessageForSegment(int segment, const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
    // Juce MIDI callbacks
    void handleIncomingMidiMessage( juce::MidiInput* source, const juce::MidiMessage& message) override;
    void handlePartialSysexMessage( juce::MidiInput* source,
                                   const juce::uint8* messageData,
                                   int numBytesSoFar,
                                   double timestamp) {}
	
	// OSC method: used to get touch callback data from the keyboard
	// bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
    // for logging
    void createLogFile(std::string midiLog_filename, std::string path);
    void closeLogFile();
    void startLogging();
    void stopLogging();
    
    bool logFileCreated;
    bool loggingActive;

	// Destructor
	~MidiInputController();
	
private:
	// ***** Member Variables *****
	
	PianoKeyboard& keyboard_;						// Reference to main keyboard data
    MidiOutputController *midiOutputController_;	// Destination for MIDI output
    
    std::map<int, std::unique_ptr< juce::MidiInput > > activePorts_;  // Sources of MIDI data
    int primaryActivePort_;                         // Which source is primary
    
    std::vector<MidiKeyboardSegment*> segments_;    // Segments of the keyboard
    juce::CriticalSection segmentsMutex_;                 // Mutex protecting the segments list
    int segmentUniqueIdentifier_;                   // Identifier of when segment structure has changed
    
    // for logging
    std::ofstream midiLog;
};
