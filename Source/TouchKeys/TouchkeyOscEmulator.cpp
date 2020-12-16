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
 
  TouchkeyOscEmulator.cpp: emulates a TouchKeys source using OSC messages
*/

#include "TouchkeyOscEmulator.h"

// Main constructor
TouchkeyOscEmulator::TouchkeyOscEmulator(PianoKeyboard& keyboard, OscMessageSource& messageSource)
: keyboard_(keyboard), source_(messageSource), lowestMidiNote_(48)
{
    allTouchesOff(false);
    setOscController(&source_);
    addOscListener("/emulation0*");
}

// Main destructor
TouchkeyOscEmulator::~TouchkeyOscEmulator()
{
}

// Turn off all touches on the virtual TouchKeys keyboard
// send indicates whether to send messages for touches that are disabled
// (false silently resets state)
void TouchkeyOscEmulator::allTouchesOff(bool send) {
    for(int i = 0; i < 127; i++) {
        if(send && touchFrames_[i].count > 0) {
            if(keyboard_.key(i) != 0)
                keyboard_.key(i)->touchOff(keyboard_.schedulerCurrentTimestamp());
        }
        clearTouchData(i);
    }
}

// OSC handler method, called when a registered message is received
bool TouchkeyOscEmulator::oscHandlerMethod(const char *path, const char *types,
                                           int numValues, lo_arg **values, void *data) {
    try {
        // Parse the OSC path looking for particular emulation messages
        if(!strncmp(path, "/emulation0/note", 16) && strlen(path) > 16) {
            // Emulation0 messages are of this form:
            //   noteN/M
            //   noteN/M/z
            // Here, N specifies the number of the note (with 0 being the lowest on the controller)
            // and M specifies the touch number (starting at 1 for the first touch). The messages ending
            // in /z indicate a touch on-off event for that particular touch.
            std::string subpath(&path[16]);
            int separatorLoc = subpath.find_first_of('/');
            if(separatorLoc == std::string::npos || separatorLoc == subpath.length() - 1) {
                // Malformed input (no slash or it's the last character): ignore
                return false;
            }
            std::stringstream noteNumberSStream(subpath.substr(0, separatorLoc));
            
            int noteNumber = 0;
            noteNumberSStream >> noteNumber;
            
            if(noteNumber < 0)  // Unknown note number
                return false;
            // Now we have a note number from the OSC path
            // Figure out the touch number, starting after the separator
            subpath = subpath.substr(separatorLoc + 1);
            separatorLoc = subpath.find_first_of("/z");
            bool isZ = false;
            if(separatorLoc != std::string::npos) {
                // This is a z message; drop the last part
                isZ = true;
                subpath = subpath.substr(0, separatorLoc);
            }
            
            std::stringstream touchNumberSStream(subpath);
            int touchNumber = 0;
            touchNumberSStream >> touchNumber;
            
            // We only care about touch numbers 1-3, since we're emulating the capabilities
            // of the TouchKeys
            if(touchNumber < 1 || touchNumber > 3)
                return false;
            
            if(isZ) {
                // Z messages indicate touch on/off. We only respond specifically
                // to the off message: the on message is implicit in receiving XY data
                if(numValues >= 1) {
                    if(types[0] == 'i') {
                        if(values[0]->i == 0)
                            touchOffReceived(noteNumber, touchNumber);
                    }
                    else if(types[0] == 'f') {
                        if(values[0]->f == 0)
                            touchOffReceived(noteNumber, touchNumber);
                    }
                }
            }
            else {
                // Other messages contain XY data for the given touch, but with Y first as
                // the layout is turned sideways (landscape)
                if(numValues >= 2) {
                    if(types[0] == 'f' && types[1] == 'f')
                        touchReceived(noteNumber, touchNumber, values[1]->f, values[0]->f);
                }
            }
        }
    }
    catch(...) {
        return false;
    }
        
    return true;
}

// New touch data point received
void TouchkeyOscEmulator::touchReceived(int key, int touch, float x, float y) {
    // std::cout << "Key " << key << " touch " << touch << ": (" << x << ", " << y << ")\n";

    int noteNumber = lowestMidiNote_ + key;
    
    // Sanity checks
    if(noteNumber < 0 || noteNumber > 127)
        return;
    if(touch < 1 || touch > 3)
        return;
    if(y < 0 || y > 1.0 || x > 1.0) // Okay for x < 0
        return;
    
    // Find TouchKeys ID associated with this OSC touch ID
    int touchId = touchIdAssignments_[noteNumber][touch - 1];
    bool updatedExistingTouch = false;
    
    if(touchId >= 0) {
        for(int i = 0; i < 3; i++) {
            if(touchFrames_[noteNumber].ids[i] == touchId) {
                // Found continuing touch
                // std::cout << "matched touch " << touch << " to ID " << touchId << std::endl;
                updateTouchInFrame(noteNumber, i, x, y);
                updatedExistingTouch = true;
            }
        }
    }
    
    if(!updatedExistingTouch) {
        // Didn't find an ID for this touch: add it to the frame
        // provided there aren't 3 existing touches (shouldn't happen)
        if(touchFrames_[noteNumber].count < 3) {
            // std::cout << "assigning touch " << touch << " to ID " << touchFrames_[noteNumber].nextId << std::endl;
            touchIdAssignments_[noteNumber][touch - 1] = touchFrames_[noteNumber].nextId++;
            addTouchToFrame(noteNumber, touchIdAssignments_[noteNumber][touch - 1], x, y);
        }
    }
    
    if(keyboard_.key(noteNumber) != 0) {
        // Pass the frame to the keyboard by copy since PianoKey does its own ID number tracking.
        // The important thing is that the Y values are always ordered. If ID tracking later changes
        // in PianoKey it's of no consequence here as long as we retain the ability to track OSC
        // touch IDs.
        KeyTouchFrame copyFrame(touchFrames_[noteNumber]);
        keyboard_.key(noteNumber)->touchInsertFrame(copyFrame, keyboard_.schedulerCurrentTimestamp());
    }
}

// Touch removed
void TouchkeyOscEmulator::touchOffReceived(int key, int touch) {
    int noteNumber = lowestMidiNote_ + key;
    
    // Sanity checks
    if(noteNumber < 0 || noteNumber > 127)
        return;
    if(touch < 1 || touch > 3)
        return;
    
    // Find TouchKeys ID associated with this OSC touch ID and
    // meanwhile disassociate this touch with any future touch ID
    int touchId = touchIdAssignments_[noteNumber][touch - 1];
    touchIdAssignments_[noteNumber][touch - 1] = -1;
    
    if(touchId < 0) // No known touch with this ID
        return;
    
    for(int i = 0; i < 3; i++) {
        if(touchFrames_[noteNumber].ids[i] == touchId) {
            // Found the touch: remove it
            removeTouchFromFrame(noteNumber, i);
            
            // Anything left?
            if(touchFrames_[noteNumber].count == 0) {
                clearTouchData(noteNumber);
                if(keyboard_.key(noteNumber) != 0)
                    keyboard_.key(noteNumber)->touchOff(keyboard_.schedulerCurrentTimestamp());
            }
            else if(keyboard_.key(noteNumber) != 0) {
                KeyTouchFrame copyFrame(touchFrames_[noteNumber]);
                keyboard_.key(noteNumber)->touchInsertFrame(copyFrame, keyboard_.schedulerCurrentTimestamp());
            }
            break;
        }
    }
}

// Clear touch data for a particular key
void TouchkeyOscEmulator::clearTouchData(int i) {
    touchFrames_[i].count = 0;
    touchFrames_[i].locH = -1.0;
    touchFrames_[i].nextId = 0;
    for(int j = 0; j < 3; j++) {
        touchFrames_[i].ids[j] = -1;
        touchFrames_[i].locs[j] = -1.0;
        touchFrames_[i].sizes[j] = 0;
        touchIdAssignments_[i][j] = -1;
    }
    int key = i % 12;
    if(key == 1 || key == 3 || key == 6 || key == 8 || key == 10)
        touchFrames_[i].white = false;
    else
        touchFrames_[i].white = true;
}

void TouchkeyOscEmulator::removeTouchFromFrame(int note, int index) {
    // Remove the touch and collapse the other touches around it down
    // so they stay in order.
    int lastTouchIndex = touchFrames_[note].count - 1;
    if(lastTouchIndex < 0)
        return;
    
    for(int i = index; i < lastTouchIndex; i++) {
        touchFrames_[note].ids[i] = touchFrames_[note].ids[i+1];
        touchFrames_[note].locs[i] = touchFrames_[note].locs[i+1];
        touchFrames_[note].sizes[i] = touchFrames_[note].sizes[i+1];
    }
    
    touchFrames_[note].ids[lastTouchIndex] = -1;
    touchFrames_[note].locs[lastTouchIndex] = -1.0;
    touchFrames_[note].sizes[lastTouchIndex] = 0.0;
    touchFrames_[note].count--;
}

// Add a touch with the indicated ID to the frame. Its position in the KeyTouchFrame
// order may depend on its y value
void TouchkeyOscEmulator::addTouchToFrame(int note, int touchId, float x, float y) {
    if(touchFrames_[note].count >= 3) // Already full?
        return;
    
    // Touches are ordered by y position. Look for where this fits in the sequence
    // and insert it there.
    int insertLoc;
    for(insertLoc = 0; insertLoc < touchFrames_[note].count; insertLoc++) {
        if(touchFrames_[note].locs[insertLoc] > y)
            break;
    }
    
    // Move the other touches back to make space
    for(int i = touchFrames_[note].count; i > insertLoc; i--) {
        touchFrames_[note].ids[i] = touchFrames_[note].ids[i-1];
        touchFrames_[note].locs[i] = touchFrames_[note].locs[i-1];
        touchFrames_[note].sizes[i] = touchFrames_[note].sizes[i-1];
    }
    
    // Add the new touch in the vacant spot
    touchFrames_[note].ids[insertLoc] = touchId;
    touchFrames_[note].locs[insertLoc] = y;
    touchFrames_[note].sizes[insertLoc] = 1.0; // Size is not supported over OSC emulation
    touchFrames_[note].count++;
    
    // Lowest touch controls the horizontal position
    if(insertLoc == 0) {
        // Emulate the partial X sensing of the white TouchKeys
        if(touchFrames_[note].locs[0] > kWhiteFrontBackCutoff && touchFrames_[note].white)
            touchFrames_[note].locH  = -1.0;
        else
            touchFrames_[note].locH = x;
    }
}

// Update the touch at the given index to the new value. Depending on the values, it
// may be necessary to reorder the touches to keep them in order of increasing Y value.
void TouchkeyOscEmulator::updateTouchInFrame(int note, int index, float x, float y) {
    // Is it still in proper order?
    bool ordered = true;
    if(index > 0) {
        if(touchFrames_[note].locs[index-1] > y)
            ordered = false;
    }
    if(index < touchFrames_[note].count - 1) {
        if(touchFrames_[note].locs[index + 1] < y)
            ordered = false;
    }
    
    // If out of order, the simplest strategy is to remove the touch and re-add it which
    // will keep everything in order. Otherwise just update the information
    if(ordered) {
        touchFrames_[note].locs[index] = y;
        if(index == 0) {
            if(y > kWhiteFrontBackCutoff && touchFrames_[note].white)
                touchFrames_[note].locH = -1;
            else
                touchFrames_[note].locH = x;
        }
    }
    else {
        int currentId = touchFrames_[note].ids[index];
        removeTouchFromFrame(note, index);
        addTouchToFrame(note, currentId, x, y);
    }
}