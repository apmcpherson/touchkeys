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
 
  TouchkeyOscEmulator.h: emulates a TouchKeys source using OSC messages
*/

#pragma once

#include "PianoKeyboard.h"
#include <cstdlib>
#include <cstring>
#include <sstream>

class TouchkeyOscEmulator : public OscHandler {
public:
    // *** Constructor ***
    TouchkeyOscEmulator(PianoKeyboard& keyboard, OscMessageSource& messageSource);
    
    // *** Destructor ***
    ~TouchkeyOscEmulator();
    
    // *** TouchKeys emulation methods ***
    int lowestMidiNote() { return lowestMidiNote_; }
	void setLowestMidiNote(int note) { lowestMidiNote_ = note; }
    
    void allTouchesOff(bool send = true);
    
    // *** OSC handler method ***
	bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);

    // *** Data processing methods ***
    // New touch data point received
    void touchReceived(int key, int touch, float x, float y);
    
    // Touch removed
    void touchOffReceived(int key, int touch);
    
private:
    void clearTouchData(int i);                     // Clear touch data for a particular key
    void removeTouchFromFrame(int note, int index); // Remove a particular touch from the frame
    void addTouchToFrame(int note, int touchId, float x, float y);       // Add a touch with a particular ID
    void updateTouchInFrame(int note, int index, float x, float y);      // Update the touch at the given index
    
private:
	PianoKeyboard& keyboard_;           // Main keyboard controller
    OscMessageSource& source_;          // Source of OSC messages
    
    int lowestMidiNote_;                // Lowest MIDI note
    
    KeyTouchFrame touchFrames_[127];    // Current state of each virtual TouchKey
    int touchIdAssignments_[127][3];    // Assignments between OSC touch IDs and TouchKeys touch IDs
};
