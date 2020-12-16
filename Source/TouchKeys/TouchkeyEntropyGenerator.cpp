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
 
  TouchkeyEntropyGenerator.cpp: generate random TouchKeys data for testing
*/

#include "TouchkeyEntropyGenerator.h"

TouchkeyEntropyGenerator::TouchkeyEntropyGenerator(PianoKeyboard& keyboard)
: juce::Thread("TouchkeyEntropyGenerator"), keyboard_(keyboard),
  waitableEvent_(true), isRunning_(false),
  keyboardRangeLow_(36), keyboardRangeHigh_(60),
  dataInterval_(milliseconds_to_timestamp(5.0))
{
    srand(time(NULL));
}

// Start the thread handling the scheduling.
void TouchkeyEntropyGenerator::start() {
	if(isRunning_)
		return;
    // Initialize the touch data before starting
    for(int i = 0; i <= 127; i++) {
        clearTouchData(i);
        nextOnOffFrameCount_[i] = abs(rand()) % 8192;
    }
    isRunning_ = true;
    startThread();
}

// Stop the scheduler thread if it is currently running.
void TouchkeyEntropyGenerator::stop() {
	if(!isRunning_)
		return;
    
    // Tell the thread to quit and signal the event it waits on
    signalThreadShouldExit();
    stopThread(-1);
    
	isRunning_ = false;
}

// Run the entropy generator in its own thread
void TouchkeyEntropyGenerator::run() {
    timestamp_type lastDataTime = keyboard_.schedulerCurrentTimestamp();
    timestamp_type currentTime;
    
    while(!threadShouldExit()) {
        // Generate random data at regular intervals
        currentTime = keyboard_.schedulerCurrentTimestamp();
        while(currentTime - lastDataTime < dataInterval_ && !threadShouldExit()) {
            waitableEvent_.wait(1);
            currentTime = keyboard_.schedulerCurrentTimestamp();
        }
        
        for(int i = keyboardRangeLow_; i <= keyboardRangeHigh_; i++) {
            if(touchFrames_[i].count != 0) {
                // This key is on. Check if it should go off; otherwise generate new data
                if(--nextOnOffFrameCount_[i] <= 0) {
                    clearTouchData(i);
                    if(keyboard_.key(i) != 0)
                        keyboard_.key(i)->touchOff(currentTime);
                    nextOnOffFrameCount_[i] = abs(rand()) % 8192;
                }
                else {
                    generateRandomFrame(i);
                    if(keyboard_.key(i) != 0) {
                        KeyTouchFrame copyFrame(touchFrames_[i]);
                        keyboard_.key(i)->touchInsertFrame(copyFrame, currentTime);
                    }
                }
            }
            else {
                // This key is off. Check if it should go on
                if(--nextOnOffFrameCount_[i] <= 0) {
                    generateRandomFrame(i);
                    if(keyboard_.key(i) != 0) {
                        KeyTouchFrame copyFrame(touchFrames_[i]);
                        keyboard_.key(i)->touchInsertFrame(copyFrame, currentTime);
                    }
                    nextOnOffFrameCount_[i] = abs(rand()) % 8192;
                }
            }
        }
    }
    
    // Tell all currently enabled notes to turn off
    for(int i = 0; i < 128; i++) {
        if(touchFrames_[i].count > 0 && keyboard_.key(i) != 0)
            keyboard_.key(i)->touchOff(currentTime);
    }
}

// Clear touch data for a particular key
void TouchkeyEntropyGenerator::clearTouchData(int i) {
    touchFrames_[i].count = 0;
    touchFrames_[i].locH = -1.0;
    touchFrames_[i].nextId = 0;
    for(int j = 0; j < 3; j++) {
        touchFrames_[i].ids[j] = -1;
        touchFrames_[i].locs[j] = -1.0;
        touchFrames_[i].sizes[j] = 0;
    }
    int key = i % 12;
    if(key == 1 || key == 3 || key == 6 || key == 8 || key == 10)
        touchFrames_[i].white = false;
    else
        touchFrames_[i].white = true;
}

// Generate a random frame of touch data on this key
void TouchkeyEntropyGenerator::generateRandomFrame(int key) {
    touchFrames_[key].count = 1;
    touchFrames_[key].locH = (float)abs(rand()) / (float)RAND_MAX;
    touchFrames_[key].locs[0] = (float)abs(rand()) / (float)RAND_MAX;
    touchFrames_[key].sizes[0] = (float)abs(rand()) / (float)RAND_MAX;
}