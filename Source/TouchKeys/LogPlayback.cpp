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
 
  LogPlayback.cpp: basic functions for playing back a recorded TouchKeys log.
*/

#include "LogPlayback.h"

LogPlayback::LogPlayback(PianoKeyboard& keyboard, MidiInputController& midi)
: keyboard_(keyboard), midiInputController_(midi), open_(false), playing_(false), paused_(false),
  usingTouch_(false), usingMidi_(false), playbackRate_(1.0),
  nextTouchMidiNote_(0), nextTouchTimestamp_(0), nextMidiTimestamp_(0),
  lastMidiTimestamp_(0), timestampOffset_(0)
{
    // Create a statically bound call to the performMapping() method that
    // we use each time we schedule a new mapping
    touchAction_ = boost::bind(&LogPlayback::nextTouchEvent, this);
    midiAction_ = boost::bind(&LogPlayback::nextMidiEvent, this);
}

LogPlayback::~LogPlayback()
{
    if(open_)
        closeLogFiles();
}

// File management. Open a touch and/or MIDI file. Returns true on success.
// Pass a blank string to either one of the paths to not use that form of data capture
bool LogPlayback::openLogFiles( std::string const& touchPath, std::string const& midiPath) {
    touchLog_.open (touchPath.c_str(), std::ios::in | std::ios::binary);
    midiLog_.open (midiPath.c_str(), std::ios::in | std::ios::binary);
    
    usingTouch_ = touchLog_.is_open();
    usingMidi_ = midiLog_.is_open();
    
    // Check for bad file paths
    if(!usingTouch_ && touchPath != "")
        return false;
    if(!usingMidi_ && midiPath != "")
        return false;
    if(!usingTouch_ && !usingMidi_)
        return false;
    
    // Set defaults
    open_ = true;
    playing_ = paused_ = false;
    playbackRate_ = 1.0;
    return true;
}

// Close the current files
void LogPlayback::closeLogFiles() {
    if(!open_)
        return;
    if(playing_)
        stopPlayback();
    touchLog_.close();
    midiLog_.close();
    open_ = playing_ = paused_ = false;
}

// Start, stop, pause, resume
void LogPlayback::startPlayback(timestamp_type startingTimestamp) {
    if(!open_)
        return;
    
    // Start the playback scheduler thread
    playbackScheduler_.start(0);
    
    timestamp_type firstTouchTimestamp, firstMidiTimestamp;
    
    // Register actions on the scheduler thread
    if(usingTouch_) {
        readNextTouchFrame();
        firstTouchTimestamp = nextTouchTimestamp_;
        timestampOffset_ = playbackScheduler_.currentTimestamp() - firstTouchTimestamp;
    }
    if(usingMidi_) {
        readNextMidiFrame();
        firstMidiTimestamp = nextMidiTimestamp_;
        lastMidiTimestamp_ = nextMidiTimestamp_; // First timestamp difference is 0
        
        // Timestamp offset is to first MIDI event, unless there's an earlier touch event
        if(!(usingTouch_ && (firstTouchTimestamp < firstMidiTimestamp)))
            timestampOffset_ = playbackScheduler_.currentTimestamp() - firstMidiTimestamp;
    }
    
    std::cout << "Touch " << firstTouchTimestamp << " MIDI " << firstMidiTimestamp << " offset " << timestampOffset_ << '\n';
    
    playing_ = true;
    paused_ = false;
    
    if(usingTouch_)
        playbackScheduler_.schedule(this, touchAction_, playbackScheduler_.currentTimestamp());        
    if(usingMidi_)
        playbackScheduler_.schedule(this, midiAction_, playbackScheduler_.currentTimestamp());
}

void LogPlayback::stopPlayback() {
    playing_ = paused_ = false;
    
    // Stop the playback scheduler thread
    playbackScheduler_.stop();
    playbackScheduler_.unschedule(this);
}

// Pause a currently playing file. Save the pause time so the offset
// can be recalculated when it resumes
void LogPlayback::pausePlayback() {
    if(open_ && playing_) {
        playbackScheduler_.unschedule(this);
        
        // TODO: consider thread safety: what happens if this comes during one of the scheduled calls?
        
        paused_ = true;
        pauseTimestamp_ = playbackScheduler_.currentTimestamp();
    }
}

// Resume playback after a pause
void LogPlayback::resumePlayback() {
    if(paused_) {
        paused_ = false;
        timestamp_type resumeTimestamp = playbackScheduler_.currentTimestamp();
        
        // Update the timestamp offset
        timestampOffset_ += resumeTimestamp - pauseTimestamp_;
        
        // Reschedule calls
        if(usingTouch_)
            playbackScheduler_.schedule(this, touchAction_, nextTouchTimestamp_ + timestampOffset_);
        if(usingMidi_)
            playbackScheduler_.schedule(this, touchAction_, nextMidiTimestamp_ + timestampOffset_);
    }
}

// Seek to a timestamp in the file
void LogPlayback::seekPlayback(timestamp_type newTimestamp) {
    // Advance through the file until we reach the indicated timestamp
    
    if(!playing_ || !open_)
        return;
    
    // Remove any future actions while we perform the seek
    playbackScheduler_.unschedule(this);
    //timestamp_diff_type offset = 0;
    timestamp_type firstUpcomingTimestamp = 0;
    
    if(usingTouch_) {
        //timestamp_type lastTimestamp = nextTouchTimestamp_;
        
        // TODO: this assumes the seek is moving forward
        while(nextTouchTimestamp_ <= newTimestamp) {
            if(!readNextTouchFrame()) { // EOF or error
                usingTouch_ = false;
                if(!usingMidi_)
                    playing_ = paused_ = false;
                break;
            }
        }
        
        // Now we have the first event scheduled after the seek location
        // Update timestamp offset to continue playback from here.

        //offset = nextTouchTimestamp_ - lastTimestamp;
        firstUpcomingTimestamp = nextTouchTimestamp_;
    }
    if(usingMidi_) {
        //timestamp_type lastTimestamp = nextMidiTimestamp_;
        
        // TODO: this assumes the seek is moving forward
        while(nextMidiTimestamp_ <= newTimestamp) {
            if(!readNextMidiFrame()) { // EOF or error
                usingMidi_ = false;
                if(!usingTouch_)
                    playing_ = paused_ = false;
                break;
            }
        }
        
        // Now we have the first event scheduled after the seek location
        // Update timestamp offset to continue playback from here.
        // Use whichever event came first
        
        //if(!(usingTouch_ && (nextMidiTimestamp_ - lastTimestamp) > offset))
        //    offset = (nextMidiTimestamp_ - lastTimestamp);
        if(!usingTouch_ || nextMidiTimestamp_ < nextTouchTimestamp_);
            firstUpcomingTimestamp = nextMidiTimestamp_;
    }
    
    // Update the timestamp offset
    timestampOffset_ = playbackScheduler_.currentTimestamp() - firstUpcomingTimestamp;
    
    if(usingTouch_)
        playbackScheduler_.schedule(this, touchAction_, nextTouchTimestamp_ + timestampOffset_);
    if(usingMidi_)
        playbackScheduler_.schedule(this, midiAction_, nextMidiTimestamp_ + timestampOffset_);
}

// Change the playback rate (1.0 being the standard speed)
void LogPlayback::changePlaybackRate(float rate) {
    playbackRate_ = rate;
}

// Events the scheduler calls when the right time elapses. Find the
// next touch or MIDI event and play it back
timestamp_type LogPlayback::nextTouchEvent() {
    if(!playing_ || !open_ || paused_)
        return 0;
    
    // TODO: handle playback rate
    
    // Play the most recent stored touch frame
    if(nextTouchMidiNote_ >= 0 && nextTouchMidiNote_ < 128) {
        // Use PianoKeyboard timestamps for the messages we send since our scheduler
        // may have a different idea of time.
        
        if(nextTouch_.count == 0) {
            if(keyboard_.key(nextTouchMidiNote_) != 0)
                keyboard_.key(nextTouchMidiNote_)->touchOff(keyboard_.schedulerCurrentTimestamp());
            /*
             // Send raw OSC message if enabled
             if(sendRawOscMessages_) {
             keyboard_.sendMessage("/touchkeys/raw-off", "iii",
             octave, key, frame,
             LO_ARGS_END );
             }
             */
        }
        else {
            if(keyboard_.key(nextTouchMidiNote_) != 0)
                keyboard_.key(nextTouchMidiNote_)->touchInsertFrame(nextTouch_,
                                                                keyboard_.schedulerCurrentTimestamp());
            /*if(sendRawOscMessages_) {
                keyboard_.sendMessage("/touchkeys/raw", "iiifffffff",
                                      octave, key, frame,
                                      sliderPosition[0],
                                      sliderSize[0],
                                      sliderPosition[1],
                                      sliderSize[1],
                                      sliderPosition[2],
                                      sliderSize[2],
                                      sliderPositionH,
                                      LO_ARGS_END );
            }*/
        }
    }
    
    bool newTouchFound = readNextTouchFrame();
    
    // Go through next touch frames and send them as long as the timestamp is not in the future
    while(newTouchFound && (nextTouchTimestamp_ + timestampOffset_) <= playbackScheduler_.currentTimestamp()) {
        if(nextTouchMidiNote_ >= 0 && nextTouchMidiNote_ < 128) {
            // Use PianoKeyboard timestamps for the messages we send since our scheduler
            // may have a different idea of time.
            
            if(keyboard_.key(nextTouchMidiNote_) != 0) {
                if(nextTouch_.count == 0)
                    keyboard_.key(nextTouchMidiNote_)->touchOff(keyboard_.schedulerCurrentTimestamp());
                else
                    keyboard_.key(nextTouchMidiNote_)->touchInsertFrame(nextTouch_,
                                                                        keyboard_.schedulerCurrentTimestamp());
            }
        }
        
        newTouchFound = readNextTouchFrame();
    }
    
    if(!newTouchFound) { // EOF or error
        usingTouch_ = false;
        if(!usingMidi_)
            playing_ = paused_ = false;
        return 0;
    }
    else // Return the timestamp of the next call
        return (nextTouchTimestamp_ + timestampOffset_);
}

timestamp_type LogPlayback::nextMidiEvent() {
    if(!playing_ || !open_ || paused_)
        return 0;
    
    // TODO: handle playback rate
    
    // Play the most recent stored touch frame
    if(nextMidi_.size() >= 3) {
        if((nextMidi_[0] & 0xF0) == 0xD0) // channel aftertouch has 2 bytes
                midiInputController_.handleIncomingMidiMessage(0, juce::MidiMessage(nextMidi_[0], nextMidi_[1]));
        else
            midiInputController_.handleIncomingMidiMessage(0, juce::MidiMessage(nextMidi_[0], nextMidi_[1], nextMidi_[2]));
    }
    //midiInputController_.rtMidiCallback(nextMidiTimestamp_ - lastMidiTimestamp_, &nextMidi_, 0);
    lastMidiTimestamp_ = nextMidiTimestamp_;
    
    bool newMidiEventFound = readNextMidiFrame();
    
    // Go through next touch frames and send them as long as the timestamp is not in the future
    while(newMidiEventFound && (nextMidiTimestamp_ + timestampOffset_) <= playbackScheduler_.currentTimestamp()) {
        if(nextMidi_.size() >= 3) {
            if((nextMidi_[0] & 0xF0) == 0xD0) // channel aftertouch has 2 bytes
                midiInputController_.handleIncomingMidiMessage(0, juce::MidiMessage(nextMidi_[0], nextMidi_[1]));
            else
                midiInputController_.handleIncomingMidiMessage(0, juce::MidiMessage(nextMidi_[0], nextMidi_[1], nextMidi_[2]));
        }
        //midiInputController_.rtMidiCallback(nextMidiTimestamp_ - lastMidiTimestamp_, &nextMidi_, 0);
        lastMidiTimestamp_ = nextMidiTimestamp_;
        
        readNextMidiFrame();
    }
    
    if(!newMidiEventFound) { // EOF or error
        usingMidi_ = false;
        if(!usingTouch_)
            playing_ = paused_ = false;
        return 0;
    }
    else // Return the timestamp of the next call
        return (nextMidiTimestamp_ + timestampOffset_);
}

// Retrieve the next key touch frame from the log file
// Return true if a touch was found, false if EOF or an error occurred
bool LogPlayback::readNextTouchFrame() {
    int frameCounter;
    
    try {
        touchLog_.read((char *)&nextTouchTimestamp_, sizeof(timestamp_type));
        touchLog_.read((char *)&frameCounter, sizeof(int));
        touchLog_.read((char *)&nextTouchMidiNote_, sizeof(int));
        touchLog_.read((char *)&nextTouch_, sizeof(KeyTouchFrame));
    }
    catch(...) {
        std::cout << "error reading touch\n";
        return false;
    }
    if(touchLog_.eof()) {
        std::cout << "Touch log playback finished\n";
        return false;
    }
    
    //std::cout << "read touch on key " << nextTouchMidiNote_ << " timestamp " << nextTouchTimestamp_ << '\n';
    
    // TODO: what about frameCounter
    
    return true;
}

// Retrieve the next MIDI frame from the log file
// Return true if an event was found, false if EOF or an error occurred
bool LogPlayback::readNextMidiFrame() {
    int midi0, midi1, midi2;
    
    try {
        midiLog_.read((char*)&nextMidiTimestamp_, sizeof (timestamp_type));
        midiLog_.read((char*)&midi0, sizeof (int));
        midiLog_.read((char*)&midi1, sizeof (int));
        midiLog_.read((char*)&midi2, sizeof (int));
    }
    catch(...) {
        std::cout << "error reading MIDI\n";
        return false;
    }
    
    if(midiLog_.eof()) {
        std::cout << "MIDI log playback finished\n";
        return false;
    }
    
    nextMidi_.clear();
    nextMidi_.push_back((unsigned char)midi0);
    nextMidi_.push_back((unsigned char)midi1);
    nextMidi_.push_back((unsigned char)midi2);
    
    //std::cout << "read MIDI data " << (int)midi0 << " " << (int)midi1 << " " << (int)midi2 << '\n';
    
    return true;
}