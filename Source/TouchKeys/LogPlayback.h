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
 
  LogPlayback.h: basic functions for playing back a recorded TouchKeys log.
*/

#pragma once

#include "MidiInputController.h"
#include "KeyTouchFrame.h"
#include "PianoKeyboard.h"
#include "../Utility/Scheduler.h"
#include <boost/bind.hpp>
#include <iostream>
#include <fstream>
#include <vector>

class LogPlayback {
public:
    // ***** Constructor and Destructor *****
    LogPlayback(PianoKeyboard& keyboard, MidiInputController& midi);
    
    ~LogPlayback();
    
    // ***** Log File Playback *****
    // File management
    bool openLogFiles(std::string const& touchPath, std::string const& midiPath);
    void closeLogFiles();
    
    // Start, stop, pause, resume
    void startPlayback(timestamp_type startingTimestamp = 0);
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    
    // Seek to location and/or change the rate
    void seekPlayback(timestamp_type newTimestamp);
    void changePlaybackRate(float rate);
    
    // Scheduler action functions
    timestamp_type nextTouchEvent();
    timestamp_type nextMidiEvent();
   
private:
    bool readNextTouchFrame();
    bool readNextMidiFrame();
    
	PianoKeyboard& keyboard_;	    // Main keyboard controller
    MidiInputController& midiInputController_; // MIDI controller
    Scheduler playbackScheduler_;   // Scheduler thread to send messages
    Scheduler::action touchAction_; // Scheduler action for playing next touch
    Scheduler::action midiAction_;  // Scheduler action for playing next MIDI event
    
    std::ifstream touchLog_;           // Log file for key touches
    std::ifstream midiLog_;            // Log file for MIDI data
    
    bool open_;                   // Whether files are open
    bool playing_;                // Whether playback is active
    bool paused_;                 // Whether playback is active, but paused
    bool usingTouch_, usingMidi_; // Whether touch and MIDI files are present
    float playbackRate_;          // Current playback rate (default 1.0)
    
    KeyTouchFrame nextTouch_;     // Next touch frame to play
    int nextTouchMidiNote_;       // MIDI note for next touch frame to play
    timestamp_type nextTouchTimestamp_; // When the next touch happens
    std::vector<unsigned char> nextMidi_;    // Next MIDI event to play
    timestamp_type nextMidiTimestamp_;  // When the next MIDI event happens
    timestamp_type lastMidiTimestamp_;  // When the last MIDI event happened
    timestamp_type pauseTimestamp_;     // When the playback was paused
    timestamp_diff_type timestampOffset_;   // Difference between file and current playback time
};
