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
 
  PianoKeyCalibrator.h: handles calibration of continuous key position data
*/

#pragma once

#include "../Utility/Types.h"
#include "PianoKeyboard.h"
#include <boost/circular_buffer.hpp>

// Calibration status of the Piano Bar.  This compensates for variations in mechanical position, light level, etc.
enum {
	kPianoKeyNotCalibrated = 0,
	kPianoKeyCalibrated,
	kPianoKeyInCalibration
};

// Size of the calibration history we keep
const size_t kPianoKeyCalibrationBufferSize = 32; // 32
const size_t kPianoKeyCalibrationPressLength = 10; // 10

// Minimum amount of range between quiescent and press for a note to be calibrated
const int kPianoKeyCalibrationMinimumRange = 64;

/*
 * PianoKeyboardCalibrator
 *
 * This class defines a calibration from raw value to normalized value, generically for
 * any sensor which outputs a continuous value for key position. It allows the calibration 
 * to be learned and applied. It allows a direction to be set where pressed keys are either
 * greater or lower in value than unpressed keys, to accommodate different sensor topologies.
 */

class PianoKeyCalibrator {
public:
	// ***** Constructor *****
	
	PianoKeyCalibrator(bool pressValueGoesDown, key_position* warpTable);
	
	// ***** Destructor *****
	
	~PianoKeyCalibrator();
	
	// ***** Evaluator *****
	//
	// In normal (operational) mode, evaluate() returns the calibrated value for the raw input.
	// In other modes, it returns a "missing" value.  Specifically in calibration mode, it updates
	// the settings for calibration.
	
	key_position evaluate(int rawValue);
	
	// ***** Calibration Methods *****
	//
    // Return the current status
    int calibrationStatus() { return status_; }
    
	// Manage the calibration state
	
	void calibrationStart();
	bool calibrationFinish();   // Returns true on successful calibration
	void calibrationAbort();
	void calibrationClear();
	
	// Learn new quiescent values only.  This needs to be called from a thread other than the data source
	// (audio or serial callback) thread, since it waits for the buffer to fill up before calculating the values.
	
	void calibrationUpdateQuiescent();
	
	// ***** XML I/O Methods *****
	//
	// These methods load and save calibration data from an XML string.  The PianoKeyCalibrator object handles
	// the relevant file I/O.
	
	void loadFromXml(const juce::XmlElement& baseElement);
	bool saveToXml(juce::XmlElement& baseElement);
	
private:
	// ***** Helper Methods *****
	
	void changeStatus(int newStatus) {
		prevStatus_ = status_;
		status_ = newStatus;
	}
	
	// Update quiescent values
	bool internalUpdateQuiescent();
	
	// Average position over the history buffer, for finding minima and maxima
	int averagePosition(int length);
	
	// Clean up after a calibration; called by finish() and abort()
	void cleanup();
	
	// ***** Member Variables *****
	
	int status_, prevStatus_;		// Status of calibration (see enum above), and its previous value
	bool pressValueGoesDown_;		// If true, the pressed key value is expected to be lower than the quiescent
	int quiescent_;                 // Resting value for the sensor
	int press_;                     // Fully pressed value for the sensor
	int newPress_;                  // Value-in-training for press
	
	boost::circular_buffer<int>* history_;  // Buffer holds history of raw values for calibrating
	
	// Table of warping values to correct for sensor non-linearity
	key_position* warpTable_;
    
	juce::CriticalSection calibrationMutex_;	// This mutex protects access to the entire calibration structure
	juce::CriticalSection historyMutex_;		// This mutex is specifically tied to the history_ buffers
};
