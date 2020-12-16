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

  TimestampSynchronizer.h: handles aligning timestamps between multiple
  asynchronous sources, while reducing the jitter that occurs when using
  system clock time for every received sample.
*/

#pragma once

#include "Node.h"

const int kTimestampSynchronizerHistoryLength = 100;

/* TimestampSynchronizer
 *
 * We often want to deal with multiple independent data streams, roughly
 * synchronized in time to one another but operating on separate clocks.
 * Each data stream can be assumed to have a regular, constant frame interval
 * but the exact duration of the interval might not be known and might drift
 * with respect to the system clock.
 *
 * In this class, the self-reported frame number is compared to the current
 * system time.  In any multitasking OS, the system time when we receive a
 * frame may jitter around, but in the long-term average, we want system clock
 * and frame clock to stay in sync.  Thus we use the low-pass-filtered difference
 * between system clock and frame clock to adjust the reported frame rate, keeping
 * the two locked together.
 */


 
class TimestampSynchronizer {
public:
	// Constructor
	TimestampSynchronizer();
	
	// Clear accumulated timestamps and reinitialize a relationship between clock
	// time and output timestamp.
	void initialize(double clockTimeMilliseconds, timestamp_type startingTimestamp);
	
	// Return or set the expected interval between frames
	timestamp_type nominalSampleInterval() { return nominalSampleInterval_; }
	void setNominalSampleInterval(timestamp_type interval) { 
		nominalSampleInterval_ = interval; 
		currentSampleInterval_ = interval;
	}
	
	// Return the current calculated interval between frames
	timestamp_type currentSampleInterval() { return currentSampleInterval_; }
	
	// Return or set the frame modulus (at what number the frame counter wraps
	// around to 0, since it can't increase forever).
	int frameModulus() { return frameModulus_; }
	void setFrameModulus(int modulus) { frameModulus_ = modulus; }
	
	// Process a new timestamp value and return the value synchronized to the
	// system clock
	timestamp_type synchronizedTimestamp(int rawFrameNumber);

private:
	// History buffer of clock time vs. frame time.  The Node has a data type
	// (frame number and frame timestamp, respectively) and a timestamp (clock timestamp);
	// in other words, two different times are held within the buffer as well as the frame numbers.
	
	Node< std::pair<int, timestamp_type> > history_;
	
	// Expected and currently calculated frame intervals
	
	timestamp_type nominalSampleInterval_;
	timestamp_type currentSampleInterval_;

	// Modulus of frame number, i.e. the number at which the frame counter
	// wraps around back to 0.
	int frameModulus_;
	
	// The time we start from (clock and output timestamp)
	
	double startingClockTimeMilliseconds_;
	timestamp_type startingTimestamp_;
	
	int bufferLengthCounter_;
};
