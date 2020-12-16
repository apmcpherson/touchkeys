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
 
  TouchkeyEntropyGenerator.h: generate random TouchKeys data for testing
*/

#pragma once

#include "PianoKeyboard.h"
#include <cstdlib>

class TouchkeyEntropyGenerator : public juce::Thread {
public:
    // *** Constructor ***
    TouchkeyEntropyGenerator(PianoKeyboard& keyboard);
    
    // *** Destructor ***
    ~TouchkeyEntropyGenerator() {
        stop();
    }
    
    // *** Control methods ***
    // Start running the generator
    void start();
    
    // Stop running the generator
    void stop();
    
    // Check if the generator is running
    bool isRunning() { return isRunning_; }
    
    // *** Data generation ***
    
    // Set the range of keys for which touch data will be generated
    void setKeyboardRange(int lowest, int highest) {
        keyboardRangeLow_ = lowest;
        keyboardRangeHigh_ = highest;
    }
    
    // Set the interval at which data should be generated
    void setDataInterval(timestamp_diff_type interval) {
        dataInterval_ = interval;
    }
    
    // *** Juce Thread method ***
    void run();
    
private:
    void clearTouchData(int i);         // Clear touch data for a particular key
    
    void generateRandomFrame(int key);  // Generate a random touch frame on this key
    
private:
	PianoKeyboard& keyboard_;           // Main keyboard controller
    
    juce::WaitableEvent waitableEvent_;       // For thread timing
    bool isRunning_;
    int keyboardRangeLow_;              // Range of keys to generate data for
    int keyboardRangeHigh_;
    timestamp_diff_type dataInterval_;  // Interval between frames of data
    
    KeyTouchFrame touchFrames_[127];    // Current state of each virtual TouchKey
    int nextOnOffFrameCount_[127];      // How long until the next note goes on or off
};
