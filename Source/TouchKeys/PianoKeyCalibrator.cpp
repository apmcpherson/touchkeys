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
 
  PianoKeyCalibrator.cpp: handles calibration of continuous key position data
*/

#include "PianoKeyCalibrator.h"

// Constructor
PianoKeyCalibrator::PianoKeyCalibrator(bool pressValueGoesDown, key_position* warpTable)
: status_(kPianoKeyNotCalibrated), prevStatus_(kPianoKeyNotCalibrated),
  pressValueGoesDown_(pressValueGoesDown), history_(0), warpTable_(warpTable) {}

// Destructor
PianoKeyCalibrator::~PianoKeyCalibrator() {
    if(history_ != nullptr)
        delete history_;
    
	// warpTable_ is passed in externally-- don't delete it
}

// Produce the calibrated value for a raw sample
key_position PianoKeyCalibrator::evaluate(int rawValue) {
	key_position calibratedValue, calibratedValueDenominator;

    juce::ScopedLock sl(calibrationMutex_);
	
	switch(status_) {
		case kPianoKeyCalibrated:
			if(missing_value<int>::isMissing(quiescent_) ||
			   missing_value<int>::isMissing(press_)) {
				return missing_value<key_position>::missing();
			}
			
			// Do the calculation either in integer or floating-point arithmetic
			calibratedValueDenominator = (key_position)(press_ - quiescent_);
			
			// Prevent divide-by-0 errors
			if(calibratedValueDenominator == 0)
				calibratedValue = missing_value<key_position>::missing();
			else {
                // Scale the value and clip it to a sensible range (for badly calibrated sensors)
				calibratedValue = (scale_key_position((rawValue - quiescent_))) / calibratedValueDenominator;
                if(calibratedValue < -0.5)
                    calibratedValue = -0.5;
                if(calibratedValue > 1.2)
                    calibratedValue = 1.2;
            }
			
			if(warpTable_ != nullptr) {
				// TODO: warping
			}
			return calibratedValue;
		case kPianoKeyInCalibration:
			historyMutex_.enter();

			// Add the sample to the calibration buffer, and wait until we have enough samples to do anything
			history_->push_back(rawValue);
			if(history_->size() < kPianoKeyCalibrationPressLength) {
				historyMutex_.exit();
				return missing_value<key_position>::missing();
			}
            
			if(pressValueGoesDown_) {      // Pressed keys have a lower value than quiescent keys
				int currentAverage = averagePosition(kPianoKeyCalibrationPressLength);
                
				// Look for minimum overall value
				if(currentAverage < newPress_ || missing_value<int>::isMissing(newPress_)) {
					newPress_ = currentAverage;
                }
			}
			else {                          // Pressed keys have a higher value than quiescent keys
				int currentAverage = averagePosition(kPianoKeyCalibrationPressLength);
				
				// Look for maximum overall value
				if(currentAverage > newPress_ || missing_value<int>::isMissing(newPress_)) {
					newPress_ = currentAverage;
                }
			}
			
			// Don't return a value while calibrating
			historyMutex_.exit();
			return missing_value<key_position>::missing();
		case kPianoKeyNotCalibrated:	// Don't do anything
		default:
			return missing_value<key_position>::missing();
	}
}

// Begin the calibrating process.
void PianoKeyCalibrator::calibrationStart() {
	if(status_ == kPianoKeyInCalibration)	// Throw away the old results if we're already in progress
		calibrationAbort();					// This will clear the slate
	
    historyMutex_.enter();
    if(history_ != nullptr)
        delete history_;
    history_ = new boost::circular_buffer<int>(kPianoKeyCalibrationBufferSize);
    historyMutex_.exit();
    
	calibrationMutex_.enter();
    newPress_ = quiescent_ = missing_value<int>::missing();
	changeStatus(kPianoKeyInCalibration);
	calibrationMutex_.exit();
}

// Finish calibrating and accept the new results. Returns true if
// calibration was successful; false if one or more values were missing
// or if insufficient range is available.

bool PianoKeyCalibrator::calibrationFinish() {
    bool updatedCalibration = false;
    int oldQuiescent = quiescent_;
    
	if(status_ != kPianoKeyInCalibration)
		return false;
    
    juce::ScopedLock sl(calibrationMutex_);
    
    // Check that we were successfully able to update the quiescent value
    // (should always be the case but this is a sanity check)
	bool updatedQuiescent = internalUpdateQuiescent();
    
    if(updatedQuiescent && abs(newPress_ - quiescent_) >= kPianoKeyCalibrationMinimumRange) {
        press_ = newPress_;
        changeStatus(kPianoKeyCalibrated);
        updatedCalibration = true;
    }
    else {
        quiescent_ = oldQuiescent;
        
        if(prevStatus_ == kPianoKeyCalibrated) {	// There may or may not have been valid data in press_ and quiescent_ before, depending on whether
            changeStatus(kPianoKeyCalibrated);      // they were previously calibrated.
        }
        else {
            changeStatus(kPianoKeyNotCalibrated);
        }
    }
    
    cleanup();
    return updatedCalibration;
}

// Finish calibrating without saving results
void PianoKeyCalibrator::calibrationAbort() {
    juce::ScopedLock sl(calibrationMutex_);
	cleanup();
	if(prevStatus_ == kPianoKeyCalibrated) {	// There may or may not have been valid data in press_ and quiescent_ before, depending on whether
		changeStatus(kPianoKeyCalibrated);	// they were previously calibrated.
	}
	else {
		changeStatus(kPianoKeyNotCalibrated);
	}
}

// Clear the existing calibration, reverting to an uncalibrated state
void PianoKeyCalibrator::calibrationClear() {
	if(status_ == kPianoKeyInCalibration)
		calibrationAbort();
    juce::ScopedLock sl(calibrationMutex_);
	status_ = prevStatus_ = kPianoKeyNotCalibrated;
}

// Generate new quiescent values without changing the press values
void PianoKeyCalibrator::calibrationUpdateQuiescent() {
	calibrationStart();
	juce::Thread::sleep(250);			// Wait 0.25 seconds for data to collect
	internalUpdateQuiescent();
	calibrationAbort();
}

// Load calibration data from an XML string
void PianoKeyCalibrator::loadFromXml(const juce::XmlElement& baseElement) {
	// Abort any calibration in progress and reset to default values
	if(status_ == kPianoKeyInCalibration)
		calibrationAbort();
	calibrationClear();
	
	juce::XmlElement *calibrationElement = baseElement.getChildByName("Calibration");
	
	if(calibrationElement != nullptr) {
        if(calibrationElement->hasAttribute("quiescent") &&
           calibrationElement->hasAttribute("press")) {
            quiescent_ = calibrationElement->getIntAttribute("quiescent");
            press_ = calibrationElement->getIntAttribute("press");
            changeStatus(kPianoKeyCalibrated);
        }
	}
}

// Saves calibration data within the provided XML Element.  Child elements
// will be added for each sequence.  Returns true if valid data was saved.
bool PianoKeyCalibrator::saveToXml(juce::XmlElement& baseElement) {
	if(status_ != kPianoKeyCalibrated)
		return false;

    juce::XmlElement *newElement = baseElement.createNewChildElement("Calibration");
    
    if(newElement == nullptr)
        return false;
    
    newElement->setAttribute("quiescent", quiescent_);
    newElement->setAttribute("press", press_);

	return true;
}

// ***** Internal Methods *****

// Internal method to clean up after a calibration session.
void PianoKeyCalibrator::cleanup() {
    juce::ScopedLock sl(historyMutex_);
    if(history_ != nullptr)
        delete history_;
    history_ = 0;
    newPress_ = missing_value<int>::missing();
}

// This internal method actually calculates the new quiescent values.  Used by calibrationUpdateQuiescent()
// and calibrationFinish(). Returns true if successful.
bool PianoKeyCalibrator::internalUpdateQuiescent() {
    juce::ScopedLock sl(historyMutex_);
    if(history_ == nullptr) {
        return false;
    }
    if(history_->size() < kPianoKeyCalibrationPressLength) {
        return false;
    }
    quiescent_  = averagePosition(kPianoKeyCalibrationBufferSize);
    return true;
}

// Get the average position of several samples in the buffer. 
int PianoKeyCalibrator::averagePosition(int length) {
	boost::circular_buffer<int>::reverse_iterator rit = history_->rbegin();
	int count = 0, sum = 0;
	
	while(rit != history_->rend() && count < length) {
		sum += *rit++;
		count++;
	}
	
	if(count == 0) {
		return missing_value<int>::missing();
    }
    
    return (int)(sum / count);
}