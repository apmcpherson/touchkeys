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

  TimestampSynchronizer.cpp: handles aligning timestamps between multiple
  asynchronous sources, while reducing the jitter that occurs when using
  system clock time for every received sample.
*/

#include "TimestampSynchronizer.h"

// Constructor
TimestampSynchronizer::TimestampSynchronizer()
: history_(kTimestampSynchronizerHistoryLength), nominalSampleInterval_(0), currentSampleInterval_(0),
frameModulus_(0), startingClockTimeMilliseconds_(0), startingTimestamp_(0), 
bufferLengthCounter_(0)
{
}

// Clear the accumulated timestamp history and reset the current
// value to its nominal "expected" value.  Also (re-)establish
// the relationship between system clock time and output timestamp.
// If multiple streams are to be synchronized, they should be
// initialized with the same values

void TimestampSynchronizer::initialize(double clockTimeMilliseconds,
									   timestamp_type startingTimestamp) {
	history_.clear();
	currentSampleInterval_ = nominalSampleInterval_;
	startingClockTimeMilliseconds_ = clockTimeMilliseconds;
	startingTimestamp_ = startingTimestamp;
	
	//std::cout << "initialize(): startingTimestamp = " << startingTimestamp_ << ", interval = " << nominalSampleInterval_ << '\n';
}

// Given a frame number, calculate a current timestamp
timestamp_type TimestampSynchronizer::synchronizedTimestamp(int rawFrameNumber) {
	// Calculate the current system clock-related timestamp
	timestamp_type clockTime = startingTimestamp_ + milliseconds_to_timestamp( juce::Time::getMillisecondCounterHiRes() - startingClockTimeMilliseconds_);
	timestamp_type frameTime;

	// Retrieve the timestamp of the previous frame
	// Need at least 2 samples in the buffer for the calculations that follow
	if(history_.empty()) {
		frameTime = clockTime;
	}
	else if(history_.size() < 2) {
		// One sample in buffer: make sure the new sample is new before
		// storing it in the buffer.
		
		int lastFrame = history_.latest().first;
		
		frameTime = clockTime;
		
		if(lastFrame == rawFrameNumber) // Don't reprocess identical frames
			return frameTime;		
	}
	else {
		int totalHistoryFrames;		
		int lastFrame = history_.latest().first;
		frameTime = history_.latest().second;
		
		if(lastFrame == rawFrameNumber) // Don't reprocess identical frames
			return frameTime;
			
		if(frameModulus_ == 0) {
			// No modulus, just compare the raw frame number to the last frame number
			frameTime += currentSampleInterval_ * (timestamp_type)(rawFrameNumber - lastFrame);
			
			totalHistoryFrames = (history_.latest().first - history_.earliest().first);
			if(totalHistoryFrames <= 0) {
				//std::cout << "Warning: TimestampSynchronizer history buffer has a difference of " << totalHistoryFrames << " frames.\n";
				//std::cout << "Size = " << history_.size() << " first = " << history_.earliest().first << " last = " << history_.latest().first << '\n';
				totalHistoryFrames = 1;
			}
		}
		else {
			// Use mod arithmetic to handle wraparounds in the frame number
			frameTime += currentSampleInterval_ * (timestamp_type)((rawFrameNumber + frameModulus_ - lastFrame) % frameModulus_);
			
			totalHistoryFrames = (history_.latest().first - history_.earliest().first + frameModulus_) % frameModulus_;
			if(totalHistoryFrames <= 0) {
				//std::cout << "Warning: TimestampSynchronizer history buffer has a difference of " << totalHistoryFrames << " frames.\n";
				//std::cout << "Size = " << history_.size() << " first = " << history_.earliest().first << " last = " << history_.latest().first << '\n';

				totalHistoryFrames = 1;
			}			
		}
		
		// Recalculate the nominal sample interval by examining the difference in times
		// between first and last frames in the buffer.
		
		currentSampleInterval_ = (history_.latestTimestamp() - history_.earliestTimestamp()) / (timestamp_diff_type)totalHistoryFrames;
		
		// The frame time was just incremented by the current sample period.  Check whether
		// this puts the frame time ahead of the clock time.  Don't allow the frame time to get
		// ahead of the system clock (this will also push future frame timestamps back).
		
		if(frameTime > clockTime) {
			//std::cout << "CLIP " << 100.0 * (frameTime - clockTime) / currentSampleInterval_ << "%: frame=" << frameTime << " to clock=" << clockTime << '\n';
			frameTime = clockTime;			
		}
		
		bufferLengthCounter_++;
		
		if(bufferLengthCounter_ >= kTimestampSynchronizerHistoryLength) {
			//timestamp_diff_type currentLatency = clockTime - frameTime;
			timestamp_diff_type maxLatency = 0, minLatency = 1000000.0;
			
			Node< std::pair<int, timestamp_type> >::iterator it;

			for( it = history_.begin(); it != history_.end(); ++it) {
				timestamp_diff_type l = (it.timestamp() - it->second);
				if(l > maxLatency)
					maxLatency = l;
				if(l < minLatency)
					minLatency = l;
			}
			
			//std::cout << "frame " << rawFrameNumber << ": rate = " << currentSampleInterval_ << " clock = " << clockTime << " frame = " << frameTime << " latency = " 
			//	<< currentLatency << " max = " << maxLatency << " min = " << minLatency << '\n';
			
			//timestamp_diff_type targetMinLatency = (maxLatency - minLatency) * 2.0 / sqrt(kTimestampSynchronizerHistoryLength);
			
			/*if(minLatency > targetMinLatency) {
				std::cout << "ADDING " << 50.0 * (minLatency - targetMinLatency) / (currentSampleInterval_) << "%: (target " << targetMinLatency << ")\n";
				frameTime += (minLatency - targetMinLatency) / 2.0;
			}*/
			//frameTime += minLatency / 4.0;
			
			bufferLengthCounter_ = 0;
		}
	}
	
	// Insert the new frame time and clock times into the buffer
	history_.insert( std::pair<int, timestamp_type>(rawFrameNumber, frameTime), clockTime);

	// The timestamp we return is associated with the frame, not the clock (which is potentially much
	// higher jitter)
	return frameTime;
}
