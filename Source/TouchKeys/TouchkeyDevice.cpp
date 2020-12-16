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
 
  TouchkeyDevice.cpp: handles communication with the TouchKeys hardware
*/

#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include "TouchkeyDevice.h"

const char* kKeyNames[13] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "c "};

// Constructor

TouchkeyDevice::TouchkeyDevice(PianoKeyboard& keyboard) 
: keyboard_(keyboard),
#ifdef _MSC_VER
serialHandle_(INVALID_HANDLE_VALUE),
#else
device_(-1),
#endif
ioThread_(boost::bind(&TouchkeyDevice::runLoop, this, _1), "TouchKeyDevice::ioThread"),
rawDataThread_(boost::bind(&TouchkeyDevice::rawDataRunLoop, this, _1), "TouchKeyDevice::rawDataThread"),
autoGathering_(false), shouldStop_(false), sendRawOscMessages_(false),
verbose_(0), numOctaves_(0), lowestMidiNote_(48), lowestKeyPresentMidiNote_(48),
updatedLowestMidiNote_(48), lowestNotePerOctave_(0),
deviceSoftwareVersion_(-1), deviceHardwareVersion_(-1),
expectedLengthWhite_(kTransmissionLengthWhiteNewHardware),
expectedLengthBlack_(kTransmissionLengthBlackNewHardware), deviceHasRGBLEDs_(false),
ledThread_(boost::bind(&TouchkeyDevice::ledUpdateLoop, this, _1), "TouchKeyDevice::ledThread"),
isCalibrated_(false), calibrationInProgress_(false),
keyCalibrators_(0), keyCalibratorsLength_(0), sensorDisplay_(0)
{
    // Tell the piano keyboard class how to call us back
    keyboard_.setTouchkeyDevice(this);
	
	// Initialize the frame -> timestamp synchronization.  Frame interval is nominally 1ms,
	// but this class helps us find the actual rate which might drift slightly, and it keeps
	// the time stamps of each data point in sync with other streams.
	timestampSynchronizer_.initialize( juce::Time::getMillisecondCounterHiRes(), keyboard_.schedulerCurrentTimestamp());
	timestampSynchronizer_.setNominalSampleInterval(.001);
	timestampSynchronizer_.setFrameModulus(65536);
    
    for(int i = 0; i < 4; i++)
        analogLastFrame_[i] = 0;
    
    logFileCreated_ = false;
    loggingActive_ = false;
}


// ------------------------------------------------------
// create a new MIDI log file, ready to have data written to it
void TouchkeyDevice::createLogFiles( std::string keyTouchLogFilename, std::string analogLogFilename, std::string path)
{
    if (path.compare("") != 0)
    {
        path = path + "/";
    }
    
    keyTouchLogFilename = path + keyTouchLogFilename;
    analogLogFilename = path + analogLogFilename;
    
    char *fileName = (char*)keyTouchLogFilename.c_str();
    
    // create output file for key touch
    keyTouchLog_.open (fileName, std::ios::out | std::ios::binary);
    keyTouchLog_.seekp(0);

    fileName = (char*)analogLogFilename.c_str();
    
    // create output file for analog data
    analogLog_.open (fileName, std::ios::out | std::ios::binary);
    analogLog_.seekp(0);
    
    // indicate that we have created a log file (so we can close it later)
    logFileCreated_ = true;
}

// ------------------------------------------------------
// close the log file
void TouchkeyDevice::closeLogFile()
{
    if (logFileCreated_)
    {
        keyTouchLog_.close();
        analogLog_.close();
        logFileCreated_ = false;
    }
    loggingActive_ = false;
}

// ------------------------------------------------------
// start logging midi data
void TouchkeyDevice::startLogging()
{
    loggingActive_ = true;
}

// ------------------------------------------------------
// stop logging midi data
void TouchkeyDevice::stopLogging()
{
    loggingActive_ = false;
}

// Open the touchkey device (a USB CDC device).  Returns true on success.

bool TouchkeyDevice::openDevice(const char * inputDevicePath) {
	// If the device is already open, close it
	if(isOpen())
		closeDevice();
	
	// Open the device
#ifdef _MSC_VER
	// Open the serial port
	serialHandle_ = CreateFile(inputDevicePath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(serialHandle_ == INVALID_HANDLE_VALUE) {
		juce::Logger::writeToLog("Unable to open serial port " + juce::String(inputDevicePath));
		return false;
	}

	// Set some serial parameters, though they don't actually affect the operation
	// of the port since it is all native USB
	DCB serialParams = { 0 };
	serialParams.DCBlength = sizeof(serialParams);

	if(!BuildCommDCBA("baud=1000000 data=8 parity=N stop=1 dtr=on rts=on", &serialParams)) {
		juce::Logger::writeToLog("Unable to create port settings\n");
		CloseHandle(serialHandle_);
		serialHandle_ = INVALID_HANDLE_VALUE;
		return false;
	}

	if(!SetCommState(serialHandle_, &serialParams)) {
		juce::Logger::writeToLog("Unable to set comm state\n");
		CloseHandle(serialHandle_);
		serialHandle_ = INVALID_HANDLE_VALUE;
		return false;
	}

	// Set timeouts
	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	if(!SetCommTimeouts(serialHandle_, &timeout)) {
		juce::Logger::writeToLog("Unable to set timeouts\n");
		CloseHandle(serialHandle_);
		serialHandle_ = INVALID_HANDLE_VALUE;
		return false;
	}
#else
	device_ = open(inputDevicePath, O_RDWR | O_NOCTTY | O_NDELAY);

	if(device_ < 0) {
		return false;
    }
#endif
	return true;
}

// Close the touchkey serial device
void TouchkeyDevice::closeDevice() {
	if(!isOpen())
		return;
	
	stopAutoGathering();
	keysPresent_.clear();

#ifdef _MSC_VER
	CloseHandle(serialHandle_);
	serialHandle_ = INVALID_HANDLE_VALUE;
#else
	close(device_);
    device_ = -1;
#endif
}

bool TouchkeyDevice::isOpen() {
#ifdef _MSC_VER
	return serialHandle_ != INVALID_HANDLE_VALUE;
#else
	return device_ >= 0; 
#endif
}

// Check if the device is present and ready to respond.  If status is not null, store the current
// controller status information.

bool TouchkeyDevice::checkIfDevicePresent(int millisecondsToWait) {
	//struct timeval startTime, currentTime;
    double startTime, currentTime;
	unsigned char ch;
	bool controlSeq = false, startingFrame = false;
    
	if(!isOpen())
		return false;
    deviceFlush(false);
    
	if(deviceWrite((char*)kCommandStatus, 5) < 0) {	// Write status command
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write status command.  errno = " << errno << '\n';
		return false;
	}	


	// Wait the specified amount of time for a response before giving up
    startTime = juce::Time::getMillisecondCounterHiRes();
    currentTime = startTime;

	while(currentTime - startTime < (double)millisecondsToWait) {
        long count = deviceRead((char *)&ch, 1);

		if(count < 0) {				// Check if an error occurred on read
			if(errno != EAGAIN) {
                if(verbose_ >= 1)
                    std::cout << "Unable to read from device (error " << errno << ").  Aborting.\n";
				return false;
			}
		}
		else if(count > 0) {		// Data received
			// Wait for a frame back that is of type status.  We don't even care what the 
			// status is at this point, just that we got something.
			
			if(controlSeq) {
				controlSeq = false;
				if(ch == kControlCharacterFrameBegin)
					startingFrame = true;
                else
                    startingFrame = false;
			}
			else {
				if(ch == ESCAPE_CHARACTER) {
					controlSeq = true;
                }
				else if(startingFrame) {
					if(ch == kFrameTypeStatus) {
						ControllerStatus status;
						unsigned char statusBuf[TOUCHKEY_MAX_FRAME_LENGTH];
						int statusBufLength = 0;
						bool frameError = false;
						
						// Gather and parse the status frame
						
						while(currentTime - startTime < millisecondsToWait) {
							count = deviceRead((char *)&ch, 1);
						
							if(count == 0)
								continue;
							if(count < 0) {
								if(errno != EAGAIN && verbose_ >= 1) {	// EAGAIN just means no data was available
                                    if(verbose_ >= 1)
                                        std::cout << "Unable to read from device (error " << errno << ").  Aborting.\n";
									return false;
								}
								
								continue;
							}
							
							if(controlSeq) {
								controlSeq = false;
								if(ch == kControlCharacterFrameEnd)	{		// frame finished?
									break;
                                }
								else if(ch == kControlCharacterFrameError)	// device telling us about an internal comm error
									frameError = true;
								else if(ch == ESCAPE_CHARACTER) {			// double-escape means a literal escape character
									statusBuf[statusBufLength++] = (unsigned char)ch;
									if(statusBufLength >= TOUCHKEY_MAX_FRAME_LENGTH) {
										frameError = true;
										break;
									}				
								}
								else if(ch == kControlCharacterNak && verbose_ >= 1) {
									std::cout << "Warning: received NAK\n";
								}			
							}
							else {
								if(ch == ESCAPE_CHARACTER)
									controlSeq = true;
								else {
									statusBuf[statusBufLength++] = (unsigned char)ch;
									if(statusBufLength >= TOUCHKEY_MAX_FRAME_LENGTH) {
										frameError = true;
										break;
									}
								}
							}
							
							currentTime = juce::Time::getMillisecondCounterHiRes();
						}
						
						if(frameError) {
                            if(verbose_ >= 1)
                                std::cout << "Warning: device present, but frame error received trying to get status.\n";
						}
						else if(processStatusFrame(statusBuf, statusBufLength, &status)) {
							// Clear keys present in preparation to read new list of keys
							keysPresent_.clear();
							
							numOctaves_ = status.octaves;
                            deviceSoftwareVersion_ = status.softwareVersionMajor;
                            deviceHardwareVersion_ = status.hardwareVersion;
                            deviceHasRGBLEDs_ = status.hasRGBLEDs;
                            lowestKeyPresentMidiNote_ = 127;
							
							if(verbose_ >= 1) {
								std::cout << '\n' << "Found Device: Hardware Version " << status.hardwareVersion;
								std::cout << " Software Version " << status.softwareVersionMajor << "." << status.softwareVersionMinor;
								std::cout << '\n' << "  " << status.octaves << " octaves connected" << '\n';
							}
                            
							for(int i = 0; i < status.octaves; i++) {
								bool foundKey = false;
								
								if(verbose_ >= 1) std::cout << "  Octave " << i << ": ";
								for(int j = 0; j < 13; j++) {
									if(status.connectedKeys[i] & (1<<j)) {
										if(verbose_ >= 1) std::cout << kKeyNames[j] << " ";
										keysPresent_.insert(octaveNoteToIndex(i, j));
										foundKey = true;
                                        if(octaveKeyToMidi(i, j) < lowestKeyPresentMidiNote_)
                                            lowestKeyPresentMidiNote_ = octaveKeyToMidi(i, j);
									}
									else {
										if(verbose_ >= 1) std::cout << "-  ";
									}

								}

								if(verbose_ >= 1) std::cout << '\n';
							}
                            
                            // Hardware version determines whether all keys have XY or not
                            if(status.hardwareVersion >= 2) {
                                expectedLengthWhite_ = kTransmissionLengthWhiteNewHardware;
                                expectedLengthBlack_ = kTransmissionLengthBlackNewHardware;
                                whiteMaxX_ = kWhiteMaxXValueNewHardware;
                                whiteMaxY_ = kWhiteMaxYValueNewHardware;
                                blackMaxX_ = kBlackMaxXValueNewHardware;
                                blackMaxY_ = kBlackMaxYValueNewHardware;
                            }
                            else {
                                expectedLengthWhite_ = kTransmissionLengthWhiteOldHardware;
                                expectedLengthBlack_ = kTransmissionLengthBlackOldHardware;
                                whiteMaxX_ = kWhiteMaxXValueOldHardware;
                                whiteMaxY_ = kWhiteMaxYValueOldHardware;
                                blackMaxX_ = 1.0; // irrelevant -- no X data
                                blackMaxY_ = kBlackMaxYValueOldHardware;
                            }
                            
                            // Software version indicates what information is available. On version
                            // 2 and greater, can indicate which is lowest sensor available. Might
                            // be different from lowest connected key.
                            if(status.softwareVersionMajor >= 2) {
                                lowestKeyPresentMidiNote_ = octaveKeyToMidi(0, status.lowestHardwareNote);
                                
                                if(status.softwareVersionMinor == 1) {
                                    // Version 2.1 uses the lowest MIDI note to handle keyboards which don't
                                    // begin and end at C, e.g. E-E or F-F keyboards.
                                    lowestNotePerOctave_ = status.lowestHardwareNote;
                                }
                                else {
                                    lowestNotePerOctave_ = 0;
                                }
                            }
                            else if(lowestKeyPresentMidiNote_ == 127) // No keys found and old device software
                                lowestKeyPresentMidiNote_ = lowestMidiNote_;
   
                            keyboard_.setKeyboardGUIRange(lowestKeyPresentMidiNote_, lowestMidiNote_ + 12*numOctaves_ + lowestNotePerOctave_);
                            calibrationInit(12*numOctaves_ + 1); // One more for the top C
						}
						else {
							if(verbose_ >= 1) std::cout << "Warning: device present, but received invalid status frame.\n";
                            deviceFlush(true);
							return false;					// Yes... found the device
						}

                        deviceFlush(true);
						return true;					// Yes... found the device
					}
				}
                
                startingFrame = false;
			}
		}
	
		currentTime = juce::Time::getMillisecondCounterHiRes();
	}
	
	return false;
}

// Start a run loop thread to receive centroid data.  Returns true
// on success.

bool TouchkeyDevice::startAutoGathering() {
    // Can only start if the device is open
	if(!isOpen())
		return false;
    // Already running?
	if(autoGathering_)
		return true;
	shouldStop_ = false;
    ledShouldStop_ = false;
	
	if(verbose_ >= 1)
		std::cout << "Starting auto centroid collection\n";
	
    // Start the data input and LED threads
    ioThread_.startThread();
    ledThread_.startThread();
	autoGathering_ = true;
    
    // Tell the device to start scanning for new data
	if(deviceWrite((char*)kCommandStartScanning, 5) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write startAutoGather command.  errno = " << errno << '\n';
	}

	keyboard_.sendMessage("/touchkeys/allnotesoff", "", LO_ARGS_END);
	if(keyboard_.gui() != 0) {
		// Update display: touch sensing enabled, which keys connected, no current touches
		keyboard_.gui()->setTouchSensingEnabled(true);
		for( auto it = keysPresent_.begin(); it != keysPresent_.end(); ++it) {
			keyboard_.gui()->setTouchSensorPresentForKey(octaveKeyToMidi(indexToOctave(*it), indexToNote(*it)), true);
		}
		keyboard_.gui()->clearAllTouches();
        keyboard_.gui()->clearAnalogData();
	}

	return true;
}

// Stop the run loop if applicable
void TouchkeyDevice::stopAutoGathering(bool writeStopCommandToDevice) {
    // Check if actually running
	if(!autoGathering_ || !isOpen())
		return;
    // Stop any calibration in progress
    calibrationAbort();	
    
    if(writeStopCommandToDevice) {
        // Tell device to stop scanning
        if(deviceWrite((char*)kCommandStopScanning, 5) < 0) {
            if(verbose_ >= 1)
                std::cout << "ERROR: unable to write stopAutoGather command.  errno = " << errno << '\n';
        }
    }
	
    // Setting this to true tells the run loop to exit what it's doing
	shouldStop_ = true;
    ledShouldStop_ = true;
	
	if(verbose_ >= 1)
		std::cout << "Stopping auto centroid collection\n";
	
    // Wait for run loop thread to finish. Set a timeout in case there's
    // some sort of device hangup
    if(ioThread_.getThreadId() != juce::Thread::getCurrentThreadId())
        if(ioThread_.isThreadRunning())
            ioThread_.stopThread(3000);
    if(ledThread_.getThreadId() != juce::Thread::getCurrentThreadId())
        if(ledThread_.isThreadRunning())
            ledThread_.stopThread(3000);
    if(rawDataThread_.getThreadId() != juce::Thread::getCurrentThreadId())
        if(rawDataThread_.isThreadRunning())
            rawDataThread_.stopThread(3000);
	
    // Stop any currently playing notes
	keyboard_.sendMessage("/touchkeys/allnotesoff", "", LO_ARGS_END);
	
	// Clear touch for all keys
	//std::pair<int, int> keyboardRange = keyboard_.keyboardRange();
	//for(int i = keyboardRange.first; i <= keyboardRange.second; i++)
    for(int i = 0; i <= 127; i++)
        if(keyboard_.key(i) != 0)
            keyboard_.key(i)->touchOff(lastTimestamp_);
								   
	if(keyboard_.gui() != 0) {
		// Update display: touch sensing disabled
		keyboard_.gui()->clearAllTouches();		
		keyboard_.gui()->setTouchSensingEnabled(false);
        keyboard_.gui()->clearAnalogData();
	}
	
	if(verbose_ >= 2)
		std::cout << "...done.\n";

	autoGathering_ = false;
}

// Begin raw data collection from a given single key

bool TouchkeyDevice::startRawDataCollection(int octave, int key, int mode, int scaler) {
	if(!isOpen())
		return false;
	
	stopAutoGathering();	// Stop the thread if it's running	
    
    // Account for the high C which is ID 12 on the top octave
    if(octave == numOctaves_ && key == 0) {
        octave--;
        key = 12;
    }
    
    rawDataCurrentOctave_ = octave;
    rawDataCurrentKey_ = key;
    rawDataCurrentMode_ = mode;
    rawDataCurrentScaler_ = scaler;
    rawDataShouldChangeMode_ = true;
    
	shouldStop_ = false;
    rawDataThread_.startThread();
    
	if(verbose_ >= 1)
		std::cout << "Starting raw data collection from octave " << octave << ", key " << key << '\n';
	
	autoGathering_ = true;
	
	return true;
}

void TouchkeyDevice::rawDataChangeKeyAndMode(int octave, int key, int mode, int scaler) {
    // Account for the high C which is ID 12 on the top octave
    if(octave == numOctaves_ && key == 0) {
        octave--;
        key = 12;
    }
    
    rawDataCurrentOctave_ = octave;
    rawDataCurrentKey_ = key;
    rawDataCurrentMode_ = mode;
    rawDataCurrentScaler_ = scaler;
    rawDataShouldChangeMode_ = true;
}

// Set the scan interval in milliseconds.  Returns true on success.

bool TouchkeyDevice::setScanInterval(int intervalMilliseconds) {
	if(!isOpen())
		return false;	
	if(intervalMilliseconds <= 0 || intervalMilliseconds > 255)
		return false;
	
	unsigned char command[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
		kFrameTypeScanRate, (unsigned char)(intervalMilliseconds & 0xFF), ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	// Send command
	if(deviceWrite((char*)command, 6) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write startRawDataCollection command.  errno = " << errno << '\n';
	}
	
	if(verbose_ >= 2)
		std::cout << "Setting scan interval to " << intervalMilliseconds << '\n';
	
	// Return value depends on ACK or NAK received
	return checkForAck(250);
}

// Key parameters.  Setting octave or key to -1 means all octaves or all keys, respectively.
// This controls the sensitivity of the capacitive touch sensing system on each key.
// It is a balance between achieving the best range of data and not saturating the sensors
// for the largest touches.
bool TouchkeyDevice::setKeySensitivity(int octave, int key, int value) {
	unsigned char chOctave, chKey, chVal;
	
	if(!isOpen())
		return false;
	if(octave > 255)
		return false;
	if(key > 12)
		return false;
	if(value > 255 || value < 0)
		return false;
	if(octave < 0)
		chOctave = 0xFF;
	else 
		chOctave = (unsigned char)(octave & 0xFF);
	if(key < 0)
		chKey = 0xFF;
	else
		chKey = (unsigned char)(key & 0xFF);
	chVal = (unsigned char)value;
	
	unsigned char command[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeSensitivity,
		chOctave, chKey, chVal, ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	// Send command
	if(deviceWrite((char*)command, 8) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setKeySensitivity command.  errno = " << errno << '\n';
	}

	if(verbose_ >= 2)
		std::cout << "Setting sensitivity to " << value << '\n';
	
	// Return value depends on ACK or NAK received
	return checkForAck(250);	
}

// Change how the calculated centroids are scaled to fit in a single byte. They
// will be right-shifted by the indicated number of bits before being transmitted.
bool TouchkeyDevice::setKeyCentroidScaler(int octave, int key, int value) {
	unsigned char chOctave, chKey, chVal;
	
	if(!isOpen())
		return false;	
	if(octave > 255)
		return false;
	if(key > 12)
		return false;
	if(value > 7 || value < 0)
		return false;
	if(octave < 0)
		chOctave = 0xFF;
	else 
		chOctave = (unsigned char)(octave & 0xFF);
	if(key < 0)
		chKey = 0xFF;
	else
		chKey = (unsigned char)(key & 0xFF);
	chVal = (unsigned char)value;
	
	unsigned char command[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeSizeScaler,
		chOctave, chKey, chVal, ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	// Send command
	if(deviceWrite((char*)command, 8) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setKeyCentroidScaler command.  errno = " << errno << '\n';
	}
    
	if(verbose_ >= 2)
		std::cout << "Setting size scaler to " << value << '\n';
	
	// Return value depends on ACK or NAK received
	return checkForAck(250);
}

// Set the minimum size of a centroid calculated on the key which is considered
// "real" and not noise.
bool TouchkeyDevice::setKeyMinimumCentroidSize(int octave, int key, int value) {
	unsigned char chOctave, chKey, chValHi, chValLo;
	
	if(!isOpen())
		return false;	
	if(octave > 255)
		return false;
	if(key > 12)
		return false;
	if(value > 0xFFFF || value < 0)
		return false;
	if(octave < 0)
		chOctave = 0xFF;
	else 
		chOctave = (unsigned char)(octave & 0xFF);
	if(key < 0)
		chKey = 0xFF;
	else
		chKey = (unsigned char)(key & 0xFF);
	chValHi = (unsigned char)((value >> 8) & 0xFF);
	chValLo = (unsigned char)(value & 0xFF);
	
	unsigned char command[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeMinimumSize,
		chOctave, chKey, chValHi, chValLo, ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	// Send command
	if(deviceWrite((char*)command, 9) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setKeyMinimumCentroidSize command.  errno = " << errno << '\n';
	}

	if(verbose_ >= 2)
		std::cout << "Setting minimum centroid size to " << value << '\n';
	
	// Return value depends on ACK or NAK received
	return checkForAck(250);	
}

// Set the noise threshold for individual sensor pads: the reading must exceed
// the background value by this amount to be considered an actual touch.
bool TouchkeyDevice::setKeyNoiseThreshold(int octave, int key, int value) {
	unsigned char chOctave, chKey, chVal;
	
	if(octave > 255)
		return false;
	if(key > 12)
		return false;
	if(value > 255 || value < 0)
		return false;
	if(octave < 0)
		chOctave = 0xFF;
	else 
		chOctave = (unsigned char)(octave & 0xFF);
	if(key < 0)
		chKey = 0xFF;
	else
		chKey = (unsigned char)(key & 0xFF);
	chVal = (unsigned char)value;
	
	unsigned char command[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeNoiseThreshold,
		chOctave, chKey, chVal, ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	// Send command
	if(deviceWrite((char*)command, 8) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setKeyNoiseThreshold command.  errno = " << errno << '\n';
	}

	if(verbose_ >= 2)
		std::cout << "Setting noise threshold to " << value << '\n';
	
	// Return value depends on ACK or NAK received
	return checkForAck(250);	
}

// Update the baseline sensor values on the given key
bool TouchkeyDevice::setKeyUpdateBaseline(int octave, int key) {
    unsigned char baselineCommand[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
        kFrameTypeSendI2CCommand, (unsigned char)octave, (unsigned char)key,
        2 /* xmit */, 0 /* response */, 0 /* command offset */, 6 /* baseline update */,
        ESCAPE_CHARACTER, kControlCharacterFrameEnd};
    
    // Send command
	if(deviceWrite((char*)baselineCommand, 11) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write baseline update command.  errno = " << errno << '\n';
	}

	if(verbose_ >= 2)
		std::cout << "Updating baseline on octave " << octave << " key " << key << '\n';
	
    checkForAck(100);
    
    unsigned char commandPrepareRead[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
        kFrameTypeSendI2CCommand, (unsigned char)octave, (unsigned char)key,
        1 /* xmit */, 0 /* response */, 6 /* data offset */,
        ESCAPE_CHARACTER, kControlCharacterFrameEnd};
    
	if(deviceWrite((char*)commandPrepareRead, 10) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write prepareRead command.  errno = " << errno << '\n';
	}

	// Return value depends on ACK or NAK received
	return checkForAck(100);
}

// Jump to the built-in bootloader of the TouchKeys device
void TouchkeyDevice::jumpToBootloader() {
    // The command includes a 4-byte magic number to avoid a corrupt packet accidentally triggering the jump
	unsigned char command[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeEnterSelfProgramMode,
		0xA1, 0xB2, 0xC3, 0xD4, ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	// Send command
	if(deviceWrite((char*)command, 9) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write jumpToBootloader command.  errno = " << errno << '\n';
	}
}

// Set the LED color for the given MIDI note (if RGB LEDs are present). This method
// does not directly communicate with the device, but it schedules an update to take
// place in the relevant thread.
void TouchkeyDevice::rgbledSetColor(const int midiNote, const float red, const float green, const float blue) {
    RGBLEDUpdate updateStructure;
    
    updateStructure.allLedsOff = false;
    updateStructure.midiNote = midiNote;
    
    // Convert 0-1 floating point range to 0-255
    updateStructure.red = (int)(red * 4095.0);
    updateStructure.green = (int)(green * 4095.0);
    updateStructure.blue = (int)(blue * 4095.0);
    
    ledUpdateQueue_.push_front(updateStructure);
}

// Same as rgbledSetColor() but uses HSV format color instead of RGB
void TouchkeyDevice::rgbledSetColorHSV(const int midiNote, const float hue, const float saturation, const float value) {
    float red = 0, green = 0, blue = 0;
    float chroma = value * saturation;
    
    // Hue will lie on one of 6 segments from 0 to 1; convert this from 0 to 6.
    float hueSegment = hue * 6.0;
    float x = chroma * (1.0 - fabsf(fmodf(hueSegment, 2.0) - 1.0));
    
    if(hueSegment < 1.0) {
        red = chroma;
        green = x;
        blue = 0;
    }
    else if(hueSegment < 2.0) {
        red = x;
        green = chroma;
        blue = 0;
    }
    else if(hueSegment < 3.0) {
        red = 0;
        green = chroma;
        blue = x;
    }
    else if(hueSegment < 4.0) {
        red = 0;
        green = x;
        blue = chroma;
    }
    else if(hueSegment < 5.0) {
        red = x;
        green = 0;
        blue = chroma;
    }
    else {
        red = chroma;
        green = 0;
        blue = x;
    }

    rgbledSetColor(midiNote, red, green, blue);
}

// Set all RGB LEDs off (if RGB LEDs are present). This method does not
// directly communicate with the device, but it schedules an update to take
// place in the relevant thread.
void TouchkeyDevice::rgbledAllOff() {
    RGBLEDUpdate updateStructure;
    
    updateStructure.allLedsOff = true;
    updateStructure.midiNote = 0;
    updateStructure.red = 0;
    updateStructure.green = 0;
    updateStructure.blue = 0;
    
    ledUpdateQueue_.push_front(updateStructure);
}

// Set the color of a given RGB LED (piano scanner boards only). LEDs are numbered from 0-24
// starting at left. Boards are numbered 0-3 starting at left.
bool TouchkeyDevice::internalRGBLEDSetColor(const int device, const int led, const int red, const int green, const int blue) {
	if(!isOpen())
		return false;
    if(!deviceHasRGBLEDs_)
        return false;
    if(device < 0 || device > 3)
        return false;
    if(led < 0 || led > 24)
        return false;
    if(red < 0 || red > 4095)
        return false;
    if(green < 0 || green > 4095)
        return false;
    if(blue < 0 || blue > 4095)
        return false;
    
    unsigned char command[17]; // 11 bytes + possibly 6 doubled characters
    
    // There's a chance that one of the bytes will come out to ESCAPE_CHARACTER (0xFE) depending
    // on LED color. We need to double up any bytes that come in that way.
    
    command[0] = ESCAPE_CHARACTER;
    command[1] = kControlCharacterFrameBegin;
    command[2] = kFrameTypeRGBLEDSetColors;
    
    int byte, location = 3;
    
    byte = (((unsigned char)device & 0xFF) << 6) | (unsigned char)led;
    command[location++] = byte;
    if(byte == ESCAPE_CHARACTER)
        command[location++] = byte;
    byte = (red >> 4) & 0xFF;
    command[location++] = byte;
    if(byte == ESCAPE_CHARACTER)
        command[location++] = byte;
    byte = ((red << 4) & 0xF0) | ((green >> 8) & 0x0F);
    command[location++] = byte;
    if(byte == ESCAPE_CHARACTER)
        command[location++] = byte;
    byte = (green & 0xFF);
    command[location++] = byte;
    if(byte == ESCAPE_CHARACTER)
        command[location++] = byte;
    byte = (blue >> 4) & 0xFF;
    command[location++] = byte;
    if(byte == ESCAPE_CHARACTER)
        command[location++] = byte;
    byte = (blue << 4) & 0xF0;
    command[location++] = byte;
    if(byte == ESCAPE_CHARACTER)
        command[location++] = byte;
    command[location++] = ESCAPE_CHARACTER;
    command[location++] = kControlCharacterFrameEnd;
    
	// Send command
	if(deviceWrite((char*)command, location) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setRGBLEDColor command.  errno = " << errno << '\n';
	}
	
	if(verbose_ >= 3)
		std::cout << "Setting RGB LED color for device " << device << ", led " << led << '\n';
        
	// Return value depends on ACK or NAK received
	return true; //checkForAck(20);
}

// Turn off all RGB LEDs on a given board
bool TouchkeyDevice::internalRGBLEDAllOff() {
	if(!isOpen())
		return false;
    if(!deviceHasRGBLEDs_)
        return false;
    
    unsigned char command[5];
    
    command[0] = ESCAPE_CHARACTER;
    command[1] = kControlCharacterFrameBegin;
    command[2] = kFrameTypeRGBLEDAllOff;
    command[3] = ESCAPE_CHARACTER;
    command[4] = kControlCharacterFrameEnd;
	
	// Send command
	if(deviceWrite((char*)command, 5) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setRGBLEDAllOff command.  errno = " << errno << '\n';
	}
    
	if(verbose_ >= 3)
		std::cout << "Turning off all RGB LEDs" << '\n';
    
	// Return value depends on ACK or NAK received
	return true; //checkForAck(20);
}

// Get board number for MIDI note
int TouchkeyDevice::internalRGBLEDMIDIToBoardNumber(const int midiNote) {
    // lowestMidiNote_ holds the very bottom LED on the bottom board. The boards
    // go up by two-octave sets from there. The top board has one extra LED (high C).
    
    if(midiNote > lowestMidiNote_ + 96)
        return -1;
    if(midiNote >= lowestMidiNote_ + 72)
        return 3;
    else if(midiNote >= lowestMidiNote_ + 48)
        return 2;
    else if(midiNote >= lowestMidiNote_ + 24)
        return 1;
    else if(midiNote >= lowestMidiNote_)
        return 0;
    return -1;
}

// Get LED number for MIDI note (within a board)
int TouchkeyDevice::internalRGBLEDMIDIToLEDNumber(const int midiNote) {
    // Take the note number relative to the lowest note of the whole device.
    // Once it's located within a board (2-octaves each), the offset gives us the LED number.
    // However, the lowest board works differently as it only has 15 LEDs which start at A,
    // not at C.
    const int midiNoteOffset = midiNote - lowestMidiNote_;
    
    if(midiNoteOffset < 9) // Below the bottom A, hence invalid
        return -1;
    if(midiNoteOffset < 24) {
        // Within 2 octaves of bottom --> lowest board --> adjust for 15 LEDs on board
        return midiNoteOffset - 9;
    }
    else if(midiNoteOffset < 48) {
        // Board 1
        return midiNoteOffset - 24;
    }
    else if(midiNoteOffset < 72) {
        // Board 2
        return midiNoteOffset - 48;
    }
    else if(midiNoteOffset < 97) {
        // Board 3 includes a top C (index 24)
        return midiNoteOffset - 72;
    }
    return -1;
}

// ***** Calibration Methods *****

// Start calibrating selected keys and pedals. If argument is NULL, assume it applies to all keys.
void TouchkeyDevice::calibrationStart(std::vector<int>* keysToCalibrate) {
	if(keysToCalibrate == 0) {
		for(int i = 0; i < keyCalibratorsLength_; i++)
			keyCalibrators_[i]->calibrationStart();
	}
	else {
		for( auto it = keysToCalibrate->begin(); it != keysToCalibrate->end(); it++) {
			if(*it >= 0 && *it < keyCalibratorsLength_)
				keyCalibrators_[*it]->calibrationStart();
		}
	}
	
	calibrationInProgress_ = true;
}

// Finish the current calibration in progress.  Pass it on to all Calibrators, and the ones that weren't
// calibrating will just ignore it.
void TouchkeyDevice::calibrationFinish() {
    bool calibratedAtLeastOneKey = false;
    
	for(int i = 0; i < keyCalibratorsLength_; i++) {
        // Check if calibration was successful
		if(keyCalibrators_[i]->calibrationFinish()) {
            calibratedAtLeastOneKey = true;
            // Update the display if available
            if(keyboard_.gui() != 0) {
                keyboard_.gui()->setAnalogCalibrationStatusForKey(i + lowestMidiNote_, true);
            }
        }
    }
	
	calibrationInProgress_ = false;
	isCalibrated_ = calibratedAtLeastOneKey;
}

// Abort a calibration in progress, without saving its results. Pass it on to all Calibrators.
void TouchkeyDevice::calibrationAbort() {
	for(int i = 0; i < keyCalibratorsLength_; i++)
		keyCalibrators_[i]->calibrationAbort();
	
	calibrationInProgress_ = false;
}

// Clear the existing calibration, reverting to an uncalibrated state.
void TouchkeyDevice::calibrationClear() {
	for(int i = 0; i < keyCalibratorsLength_; i++) {
		keyCalibrators_[i]->calibrationClear();
        if(keyboard_.gui() != 0) {
            keyboard_.gui()->setAnalogCalibrationStatusForKey(i + lowestMidiNote_, false);
        }
    }

	calibrationInProgress_ = false;
	isCalibrated_ = false;
}

// Save calibration data to a file
bool TouchkeyDevice::calibrationSaveToFile(std::string const& filename) {
	int i;
	
	if(!isCalibrated()) {
		std::cerr << "TouchKeys not calibrated, so can't save calibration data.\n";
		return false;
	}
	
	// Create an XML structure and save it to file.
	try {
		juce::XmlElement baseElement("TouchkeyDeviceCalibration");
		bool savedValidData = false;
		
		for(i = 0; i < keyCalibratorsLength_; i++) {
            juce::XmlElement *calibrationElement = baseElement.createNewChildElement("Key");
            if(calibrationElement == 0)
                continue;
            
			calibrationElement->setAttribute("id", i);
			
			// Tell each individual calibrator to add its data to the XML tree
			if(keyCalibrators_[i]->saveToXml(*calibrationElement)) {
				savedValidData = true;
			}
		}
		
		if(!savedValidData) {
			std::cerr << "TouchkeyDevice: unable to find valid calibration data to save.\n";
			throw 1;
		}
		
		// Now save the generated tree to a file
        
		if(!baseElement.writeTo( juce::File(filename.c_str()) )) {
			std::cerr << "TouchkeyDevice: could not write calibration file " << filename << "\n";
			throw 1;
		}
		
		//lastCalibrationFile_ = filename;
	}
	catch(...) {
		return false;
	}
	
	return true;
}

// Load calibration from a file
bool TouchkeyDevice::calibrationLoadFromFile(std::string const& filename) {
	//int i, j;
    
	calibrationClear();
	
	// Open the file and read the new values
	try {
        juce::XmlDocument doc(juce::File(filename.c_str()));
		auto baseElement = doc.getDocumentElement();
        juce::XmlElement *deviceCalibrationElement, *calibratorElement;
		
		if(baseElement == nullptr ) {
			std::cerr << "TouchkeyDevice: unable to load patch table file: \"" << filename << "\". Error was:\n";
			std::cerr << doc.getLastParseError() << '\n';
			throw 1;
		}
		
		// All calibration data is encapsulated within the root element <PianoBarCalibration>
		deviceCalibrationElement = baseElement->getChildByName("TouchkeyDeviceCalibration");
		if(deviceCalibrationElement == 0) {
			std::cerr << "TouchkeyDevice: malformed calibration file, aborting.\n";
            //delete baseElement;
			throw 1;
		}
		
		// Go through and find each key's calibration information
		calibratorElement = deviceCalibrationElement->getChildByName("Key");
		if(calibratorElement == 0) {
			std::cerr << "TouchkeyDevice: warning: no keys found\n";
		}
		else {
			while(calibratorElement != 0) {
				int keyId;
                
                if(calibratorElement->hasAttribute("id")) {
                    keyId = calibratorElement->getIntAttribute("id");
					if(keyId >= 0 && keyId < keyCalibratorsLength_)
						keyCalibrators_[keyId]->loadFromXml(*calibratorElement);
                }

				calibratorElement = calibratorElement->getNextElementWithTagName("Key");
			}
		}
        
        calibrationInProgress_ = false;
        isCalibrated_ = true;
        if(keyboard_.gui() != 0) {
            for(int i = lowestMidiNote_; i <  lowestMidiNote_ + 12*numOctaves_; i++) {
                keyboard_.gui()->setAnalogCalibrationStatusForKey(i, true);
            }
        }
		//lastCalibrationFile_ = filename;
        
        //delete baseElement;
	}
	catch(...) {
		return false;
	}
	
	// TODO: reset key states?
	
	return true;
}

// Initialize the calibrators
void TouchkeyDevice::calibrationInit(int numberOfCalibrators) {
    if(keyCalibrators_ != 0)
        calibrationDeinit();
    if(numberOfCalibrators <= 0)
        return;
    keyCalibratorsLength_ = numberOfCalibrators;
    
    // Initialize the calibrator array
    keyCalibrators_ = (PianoKeyCalibrator **)malloc(keyCalibratorsLength_ * sizeof(PianoKeyCalibrator*));
    
    for(int i = 0; i < keyCalibratorsLength_; i++) {
		keyCalibrators_[i] = new PianoKeyCalibrator(true, 0);
	}
    
    calibrationClear();
}

// Free the initialized calibrators
void TouchkeyDevice::calibrationDeinit() {
    if(keyCalibrators_ == 0)
        return;
    
	for(int i = 0; i < keyCalibratorsLength_; i++) {
        if(keyCalibrators_[i] != 0)
            delete keyCalibrators_[i];
        keyCalibrators_[i] = 0;
    }
    free(keyCalibrators_);
    
    keyCalibratorsLength_ = 0;
    isCalibrated_ = calibrationInProgress_ = false;
}

// Update the lowest MIDI note of the TouchKeys device
void TouchkeyDevice::setLowestMidiNote(int note) {
    // If running, save the value in a temporary holding place until
    // the data gathering thread makes the update. Otherwise update right away.
    // This avoids things changing during data processing and other threading problems.
    if(isAutoGathering())
        updatedLowestMidiNote_ = note;
    else {
        lowestKeyPresentMidiNote_ += (note - lowestMidiNote_);
        lowestMidiNote_ = updatedLowestMidiNote_ = note;
        if(isOpen())
            keyboard_.setKeyboardGUIRange(lowestKeyPresentMidiNote_, lowestMidiNote_ + 12*numOctaves_ + lowestNotePerOctave_);
    }
}

// Convert an octave and key designation into a MIDI note
int TouchkeyDevice::octaveKeyToMidi(int octave, int key) {
    int midi = lowestMidiNote_ + octave*12 + key;
    
    if(lowestNotePerOctave_ == 0)
        return midi;
    
    // For keyboards which do not change octaves at C (e.g. E-E and F-F keyboards),
    // the lowest note numbers are actually one octave higher (e.g. an octave might start
    // at E, meaning C-Eb are part of the next higher octave).
    // Also, the "top C" (key 12) has a special designation as being the top of
    // whichever note the keyboard began at.
    
    if(key == 12)
        midi = lowestMidiNote_ + octave*12 + key + lowestNotePerOctave_;
    else if(key < lowestNotePerOctave_)
        midi += 12;
    
    return midi;
}

// Loop for sending LED updates to the device, which must happen
// in a separate thread from data collection so the device's capacity
// to process incoming data doesn't gate its transmission of sensor data
void TouchkeyDevice::ledUpdateLoop(DeviceThread *thread) {
    
    // Run until told to stop, looking for updates to send to the board
    while(!shouldStop_ && !ledShouldStop_ && !thread->threadShouldExit()) {
        while(!ledUpdateQueue_.empty()) {
            // Get the update
            RGBLEDUpdate& updateStructure = ledUpdateQueue_.back();
            
            if(updateStructure.allLedsOff) {
                internalRGBLEDAllOff();
            }
            else {
                // Convert MIDI note number to board/LED pair. If valid, send to device.
                int board = internalRGBLEDMIDIToBoardNumber(updateStructure.midiNote);
                int led = internalRGBLEDMIDIToLEDNumber(updateStructure.midiNote);
                
                if(board >= 0 && board <= 3 && led >= 0)
                    internalRGBLEDSetColor(board, led, updateStructure.red, updateStructure.green, updateStructure.blue);
            }
            
            // Remove the update we just transmitted
            ledUpdateQueue_.pop_back();
        }
        
        juce::Thread::sleep(20);
    }
}

// Main run loop, which runs in its own thread
void TouchkeyDevice::runLoop(DeviceThread *thread) {
	unsigned char buffer[1024];							// Raw data from device
	unsigned char frame[TOUCHKEY_MAX_FRAME_LENGTH];		// Accumulated frame of data
	int frameLength;
	bool controlSeq = false, inFrame = false, frameError = false;

   /* struct timeval currentTime;
    unsigned long long currentTicks = 0, lastTicks = 0;
    int currentNote = 21;*/

	// Continuously read from the input device.  Read as much data as is available, up to
	// 1024 bytes at a time.  If no data is available, wait 0.5ms before trying again.  USB
	// data comes in every 1ms, so this guarantees no more than a 1ms wait for data, and often less.
	
	while(!shouldStop_ && !thread->threadShouldExit()) {
        
/*
            // This code for RGBLED testing
            gettimeofday(&currentTime, 0);
            
            currentTicks = currentTime.tv_sec * 1000000ULL + currentTime.tv_usec;
            if(currentTicks - lastTicks > 50000ULL) {
                lastTicks = currentTicks;
                rgbledSetColor(currentNote, 0, 0, 0);
                currentNote++;
                if(currentNote > highestMidiNote()) {
                    rgbledAllOff();
                    currentNote = 21;
                }
                rgbledSetColorHSV(currentNote, (float)(currentNote - 21)/(float)(highestMidiNote() - 21), 1.0, 1.0);
            }
*/        
 		long count = deviceRead((char *)buffer, 1024);

		if(count == 0) {
#ifdef _MSC_VER
            juce::Thread::sleep(1);
#else
			usleep(500);
#endif
			continue;
		}
		if(count < 0) {
			if(errno != EAGAIN) {	// EAGAIN just means no data was available
                if(verbose_ >= 1)
                    std::cout << "Unable to read from device (error " << errno << ").  Aborting.\n";
                stopAutoGathering(false);
				//shouldStop_ = true;
			}
			
#ifdef _MSC_VER
			juce::Thread::sleep(1);
#else
			usleep(500);
#endif
			continue;
		}	
		
		// Process the received data
		
		for(int i = 0; i < count; i++) {
			unsigned char ch = buffer[i];
		
			if(inFrame) {
				// Receiving a frame
				
				if(controlSeq) {
					controlSeq = false;
					if(ch == kControlCharacterFrameEnd)	{		// frame finished?
						inFrame = false;
						processFrame(frame, frameLength);
					}
					else if(ch == kControlCharacterFrameError) { // device telling us about an internal comm error
						if(verbose_ >= 1)
							std::cout << "Warning: received frame error, continuing anyway.\n";
						frameError = true;
					}
					else if(ch == ESCAPE_CHARACTER) {			// double-escape means a literal escape character
						frame[frameLength++] = ch;
						if(frameLength >= TOUCHKEY_MAX_FRAME_LENGTH) {
							inFrame = false;
							if(verbose_ >= 1)
								std::cout << "Warning: ignoring frame exceeding length limit " << (int)TOUCHKEY_MAX_FRAME_LENGTH << '\n';
						}				
					}
					else if(ch == kControlCharacterNak && verbose_ >= 1) {
                        // TODO: pass this on to a checkForAck() call
						std::cout << "Warning: received NAK (while receiving frame)\n";
					}			
				}
				else {
					if(ch == ESCAPE_CHARACTER)
						controlSeq = true;
					else {
						frame[frameLength++] = ch;
						if(frameLength >= TOUCHKEY_MAX_FRAME_LENGTH) {
							inFrame = false;
							if(verbose_ >= 1)
								std::cout << "Warning: ignoring frame exceeding length limit " << (int)TOUCHKEY_MAX_FRAME_LENGTH << '\n';
						}
					}
				}				
			}
			else {
				// Waiting for a frame beginning control sequence
				
				if(controlSeq) {
					controlSeq = false;
					if(ch == kControlCharacterFrameBegin) {
						inFrame = true;
						frameLength = 0;
						frameError = false;
					}
					else if(ch == kControlCharacterNak && verbose_ >= 1) {
                        // TODO: pass this on to a checkForAck() call
						std::cout << "Warning: received NAK (while waiting for frame)\n";
					}
				}
				else {
					if(ch == ESCAPE_CHARACTER)
						controlSeq = true;
				}
			}
		}
	}
}

// Main run loop for gathering raw data from a particular key, used for debugging
// and testing purposes
void TouchkeyDevice::rawDataRunLoop(DeviceThread *thread) {
	unsigned char buffer[1024];							// Raw data from device
	unsigned char frame[TOUCHKEY_MAX_FRAME_LENGTH];		// Accumulated frame of data
	int frameLength;
	bool controlSeq = false, inFrame = false, frameError = false;
    
    unsigned char gatherDataCommand[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
        kFrameTypeSendI2CCommand, (unsigned char)rawDataCurrentOctave_, (unsigned char)rawDataCurrentKey_,
        0 /* xmit */, 26 /* response */, 
        ESCAPE_CHARACTER, kControlCharacterFrameEnd};
    
    //struct timeval currentTime;
    double currentTime = 0, lastTime = 0;
    //unsigned long long currentTicks = 0, lastTicks = 0;

	// Continuously read from the input device.  Read as much data as is available, up to
	// 1024 bytes at a time.  If no data is available, wait 0.5ms before trying again.  USB
	// data comes in every 1ms, so this guarantees no more than a 1ms wait for data, and often less.
	
	while(!shouldStop_ && !thread->threadShouldExit()) {
        // Every 100ms, request raw data from the active key
        currentTime = juce::Time::getMillisecondCounterHiRes();
        
        if(currentTime - lastTime > 50.0) {
            lastTime = currentTime;
            
            // Check if we need to choose a new key or mode
            if(rawDataShouldChangeMode_) {
                // Prepare the key and update the command
                rawDataPrepareCollection(rawDataCurrentOctave_, rawDataCurrentKey_, rawDataCurrentMode_, rawDataCurrentScaler_);
                gatherDataCommand[3] = rawDataCurrentOctave_;
                gatherDataCommand[4] = rawDataCurrentKey_;
            }
            
            // Request data
            if(deviceWrite((char*)gatherDataCommand, 9) < 0) {
                if(verbose_ >= 1)
                    std::cout << "ERROR: unable to write gather data command.  errno = " << errno << '\n';
            }
        }
      
 		long count = deviceRead((char *)buffer, 1024);

		if(count == 0) {
#ifdef _MSC_VER
			juce::Thread::sleep(1);
#else
			usleep(500);
#endif
			continue;
		}
		if(count < 0) {
			if(errno != EAGAIN) {	// EAGAIN just means no data was available
                if(verbose_ >= 1)
                    std::cout << "Unable to read from device (error " << errno << ").  Aborting.\n";
                stopAutoGathering(false);
				//shouldStop_ = true;
			}
			
#ifdef _MSC_VER
			juce::Thread::sleep(1);
#else
			usleep(500);
#endif
			continue;
		}
		
		// Process the received data
		
		for(int i = 0; i < count; i++) {
			unsigned char ch = buffer[i];
            
			if(inFrame) {
				// Receiving a frame
				
				if(controlSeq) {
					controlSeq = false;
					if(ch == kControlCharacterFrameEnd)	{		// frame finished?
						inFrame = false;
						processFrame(frame, frameLength);
					}
					else if(ch == kControlCharacterFrameError) { // device telling us about an internal comm error
						if(verbose_ >= 1)
							std::cout << "Warning: received frame error, continuing anyway.\n";
						frameError = true;
					}
					else if(ch == ESCAPE_CHARACTER) {			// double-escape means a literal escape character
						frame[frameLength++] = ch;
						if(frameLength >= TOUCHKEY_MAX_FRAME_LENGTH) {
							inFrame = false;
							if(verbose_ >= 1)
								std::cout << "Warning: ignoring frame exceeding length limit " << (int)TOUCHKEY_MAX_FRAME_LENGTH << '\n';
						}
					}
					else if(ch == kControlCharacterNak && verbose_ >= 1) {
                        // TODO: pass this on to a checkForAck() call
						std::cout << "Warning: received NAK (while receiving frame)\n";
					}
				}
				else {
					if(ch == ESCAPE_CHARACTER)
						controlSeq = true;
					else {
						frame[frameLength++] = ch;
						if(frameLength >= TOUCHKEY_MAX_FRAME_LENGTH) {
							inFrame = false;
							if(verbose_ >= 1)
								std::cout << "Warning: ignoring frame exceeding length limit " << (int)TOUCHKEY_MAX_FRAME_LENGTH << '\n';
						}
					}
				}
			}
			else {
				// Waiting for a frame beginning control sequence
				
				if(controlSeq) {
					controlSeq = false;
					if(ch == kControlCharacterFrameBegin) {
						inFrame = true;
						frameLength = 0;
						frameError = false;
					}
					else if(ch == kControlCharacterNak && verbose_ >= 1) {
                        // TODO: pass this on to a checkForAck() call
						std::cout << "Warning: received NAK (while waiting for frame)\n";
					}
				}
				else {
					if(ch == ESCAPE_CHARACTER)
						controlSeq = true;
				}
			}
		}
	}
}

// Process the contents of a frame that has been received from the device
void TouchkeyDevice::processFrame(unsigned char * const frame, int length) {
	if(length == 0)	// Empty frame --> nothing to do here
		return;
	
	switch(frame[0]) { // First character gives frame type
		case kFrameTypeCentroid:
			if(verbose_ >= 3)
				std::cout << "Received centroid data\n";
			processCentroidFrame(&frame[1], length - 1);
			break;
		case kFrameTypeRawKeyData:
			if(verbose_ >= 3)
				std::cout << "Received raw key data\n";
			processRawDataFrame(&frame[1], length - 1);
			break;
        case kFrameTypeAnalog:
			if(verbose_ >= 3)
				std::cout << "Received analog data\n";
            processAnalogFrame(&frame[1], length - 1);
            break;
        case kFrameTypeErrorMessage:
            if(verbose_ >= 3)
				std::cout << "Received error data\n";
            processErrorMessageFrame(&frame[1], length-1);
            break;
        case kFrameTypeI2CResponse:
            if(verbose_ >= 3)
                std::cout << "Received I2C response\n";
            processI2CResponseFrame(&frame[1], length - 1);
            break;
		case kFrameTypeStatus:
		default:
			if(verbose_ >= 3)
				std::cout << "Received frame type " << (int)frame[0] << '\n';			
			break;
	}	
}

// Process a frame of data containing centroid values (the default mode of scanning)
void TouchkeyDevice::processCentroidFrame(unsigned char * const buffer, const int bufferLength) {
    int frame, octave, bufferIndex;
    
    // Old and new generation devices structure the frame differently. 
	if((deviceSoftwareVersion_ <= 0 && bufferLength < 3) || (deviceSoftwareVersion_ > 0 && bufferLength < 5)) {
		if(verbose_ >= 1)
			std::cout << "Warning: ignoring malformed centroid frame of " << bufferLength << " bytes, less than minimum 3\n";
		if(verbose_ >= 2) {
			std::cout << "  Contents: ";
			hexDump(std::cout, buffer, bufferLength);
			std::cout << '\n';
		}
		return;
	}
	
	if(verbose_ >= 4) {
		std::cout << "Centroid frame contents:  ";
		hexDump(std::cout, buffer, bufferLength);
		std::cout << '\n';
	}
	
    // Parse the octave and timestamp differently depending on hardware version
    if(deviceSoftwareVersion_ > 0) {
        octave = buffer[0]; // First byte is octave
        
        // Frame is stored as 32-bit little endian value
        frame = buffer[1] + ((int)buffer[2] << 8) + ((int)buffer[3] << 16) + ((int)buffer[4] << 24);
        bufferIndex = 5;
        
        if(verbose_ >= 3)
            std::cout << "Centroid frame octave " << octave << " timestamp " << frame << '\n';
    }
    else {
        frame = (buffer[0] << 8) + buffer[1];	// First two bytes give us the timestamp in milliseconds (mod 2^16)
        octave = buffer[2];	// Third byte tells us which octave of keys is being addressed
        bufferIndex = 3;
	}
    
	// Convert from device frame number (expressed in USB 1ms SOF intervals) to a system
	// timestamp that can be synchronized with other data streams
	lastTimestamp_ = timestampSynchronizer_.synchronizedTimestamp(frame);
	
	//ioMutex_.enter();
	
	while(bufferIndex < bufferLength) {
		// First byte tells us the number of the key (0-12); next bytes hold the data frame
		int key = (int)buffer[bufferIndex++];
		int bytesParsed = processKeyCentroid(frame,octave, key, lastTimestamp_, &buffer[bufferIndex], bufferLength - bufferIndex);
		
		if(bytesParsed < 0)  {
			if(verbose_ >= 1)
				std::cout << "Warning: malformed data frame (parsing key " << key << " at byte " << bufferIndex << ")\n";
			
			if(verbose_ >= 2) {
				std::cout << "--> Data: ";
				hexDump(std::cout, buffer, bufferLength);
				std::cout << '\n';
			}
			
			break;
		}
		
		bufferIndex += bytesParsed;
	}
    
    if(updatedLowestMidiNote_ != lowestMidiNote_) {
        int keyPresentDifference = (lowestKeyPresentMidiNote_ - lowestMidiNote_);
        
		lowestMidiNote_ = updatedLowestMidiNote_;
        lowestKeyPresentMidiNote_ = lowestMidiNote_ + keyPresentDifference;

        // Turn off all existing touches before changing the octave
        // so we don't end up with orphan touches when the "off" message is
        // sent to a different octave than the "on"
        for(int i = 0; i <= 127; i++)
            if(keyboard_.key(i) != 0)
                if(keyboard_.key(i)->touchIsActive())
                    keyboard_.key(i)->touchOff(lastTimestamp_);
        
        keyboard_.setKeyboardGUIRange(lowestKeyPresentMidiNote_, lowestMidiNote_ + 12*numOctaves_ + lowestNotePerOctave_);
    }
	
	//ioMutex_.exit();
}

// Process a frame containing raw key data, whose configuration was set with startRawDataCollection()
// First byte holds the octave that the data came from.

void TouchkeyDevice::processRawDataFrame(unsigned char * const buffer, const int bufferLength) {
	int octave = buffer[0];
	
	if(verbose_ >= 3)
		std::cout << "Raw data frame from octave " << octave << " contains " << bufferLength - 1 << " samples\n";
	
	if(verbose_ >= 4) {
		std::cout << "  ";
		hexDump(std::cout, &buffer[1], bufferLength - 1);
		std::cout << '\n';		
	}
    
    // Change the first byte to contain the note number this data is expected to have come
    // from (based on which key we are presently querying)
    buffer[0] = lowestMidiNote_ + (rawDataCurrentOctave_ * 12 + rawDataCurrentKey_);

	// Send raw data as an OSC blob
	lo_blob b = lo_blob_new(bufferLength, buffer);
	keyboard_.sendMessage("/touchkeys/rawbytes", "b", b, LO_ARGS_END);
	lo_blob_free(b);	
}

// Extract the floating-point centroid data for a key from packed character input.
// Send OSC features as appropriate

int TouchkeyDevice::processKeyCentroid(int frame, int octave, int key, timestamp_type timestamp, unsigned char * buffer, int maxLength) {
	int touchCount = 0;
	
	float sliderPosition[3];
	float sliderPositionH;
	float sliderSize[3];
	
	int bytesParsed;
	
	if(key < 0 || key > 12 || maxLength < 1)
		return -1;
	
	int white = (kKeyColor[key] == kKeyColorWhite);
	int midiNote = octaveKeyToMidi(octave, key);
	
	// Check that the received data is actually valid and not left over from a previous scan (which
	// can happen when the scan rate is too high).  0x88 is a special "warning" marker for this case
	// since it will never be part of a valid centroid.
	
	if(buffer[0] == 0x88) {
        if(verbose_ >= 1)
            std::cout << "Warning: octave " << octave << " key " << key << " data is not ready.  Check scan rate.\n";
        if(deviceSoftwareVersion_ >= 1)
            return white ? expectedLengthWhite_ : expectedLengthBlack_;
        else
            return 1;
	}
	
	// A value of 0xFF means that no touch is active, and no further data will be present on this key.
	
	if(buffer[0] == 0xFF && deviceSoftwareVersion_ <= 0) {
		bytesParsed = 1;
		sliderPosition[0] = sliderPosition[1] = sliderPosition[2] = -1.0;
		sliderSize[0] = sliderSize[1] = sliderSize[2] = 0.0;
		sliderPositionH = -1.0;
		
		if(verbose_ >= 4) {
			std::cout << "Octave " << octave << " Key " << key << " (TS " << timestamp << "): ff\n";
		}			
	}
	else {		
		bytesParsed = white ? expectedLengthWhite_ : expectedLengthBlack_;
		
		if(bytesParsed > maxLength)	// Make sure there's enough buffer left to process this key
			return -1;
		
		int rawSliderPosition[3];
		int rawSliderPositionH;		
		
		rawSliderPosition[0] = (((buffer[0] & 0xF0) << 4) + buffer[1]);
		rawSliderPosition[1] = (((buffer[0] & 0x0F) << 8) + buffer[2]);
		rawSliderPosition[2] = (((buffer[3] & 0xF0) << 4) + buffer[4]);
		
        if(deviceHardwareVersion_ >= 2)
        {
            // Always an H value with version 2 sensor hardware
            rawSliderPositionH = (((buffer[3] & 0x0F) << 8) + buffer[5]);
            
            if(white) {
                for(int i = 0; i < 3; i++) {
                    if(rawSliderPosition[i] != 0x0FFF) {	// 0x0FFF means no touch
                        sliderPosition[i] = (float)rawSliderPosition[i] / whiteMaxY_;
                        sliderSize[i] = (float)buffer[i + 6] / kSizeMaxValue;
                        touchCount++;
                    }
                    else {
                        sliderPosition[i] = -1.0;
                        sliderSize[i] = 0.0;
                    }
                }
            }
            else {
                for(int i = 0; i < 3; i++) {
                    if(rawSliderPosition[i] != 0x0FFF) {	// 0x0FFF means no touch
                        sliderPosition[i] = (float)rawSliderPosition[i] / blackMaxY_;
                        sliderSize[i] = (float)buffer[i + 6] / kSizeMaxValue;
                        touchCount++;
                    }
                    else {
                        sliderPosition[i] = -1.0;
                        sliderSize[i] = 0.0;
                    }
                }
            }
        }
        else
        {
            // H value only on white keys with version 0-1 sensor hardware
            
            if(white) {
                rawSliderPositionH = (((buffer[3] & 0x0F) << 8) + buffer[5]);
                
                for(int i = 0; i < 3; i++) {
                    if(rawSliderPosition[i] != 0x0FFF) {	// 0x0FFF means no touch
                        sliderPosition[i] = (float)rawSliderPosition[i] / whiteMaxY_;
                        sliderSize[i] = (float)buffer[i + 6] / kSizeMaxValue;
                        touchCount++;
                    }
                    else {
                        sliderPosition[i] = -1.0;
                        sliderSize[i] = 0.0;
                    }
                }
            }
            else {
                rawSliderPositionH = 0x0FFF;
                
                for(int i = 0; i < 3; i++) {
                    if(rawSliderPosition[i] != 0x0FFF) {	// 0x0FFF means no touch
                        sliderPosition[i] = (float)rawSliderPosition[i] / blackMaxY_;
                        sliderSize[i] = (float)buffer[i + 5] / kSizeMaxValue;
                        touchCount++;
                    }
                    else {
                        sliderPosition[i] = -1.0;
                        sliderSize[i] = 0.0;
                    }
                }		
            }
        }
		
		if(rawSliderPositionH != 0x0FFF) {
			sliderPositionH = (float)rawSliderPositionH / whiteMaxX_ ;
		}
		else
			sliderPositionH = -1.0;
		
		if(verbose_ >= 4) {
			std::cout << "Octave " << octave << " Key " << key << ": ";
			hexDump(std::cout, buffer, white ? expectedLengthWhite_ : expectedLengthBlack_);
			std::cout << '\n';
		}	
	}
	
	// Sanity check: do we have the PianoKey structure available to receive this data?
	// If not, no need to proceed further.
	if(keyboard_.key(midiNote) == 0) {
        if(verbose_ >= 1)
            std::cout << "Warning: No PianoKey available for touchkey MIDI note " << midiNote << '\n';
		return bytesParsed;
	}
    
    // From here on out, grab the performance data mutex so no MIDI events can show up in the middle
    juce::ScopedLock ksl(keyboard_.performanceDataMutex_);

	// Turn off touch activity on this key if there's no active touches
	if(touchCount == 0) {
		if(keyboard_.key(midiNote)->touchIsActive())
        {
			keyboard_.key(midiNote)->touchOff(timestamp);
            KeyTouchFrame newFrame(0, sliderPosition, sliderSize, sliderPositionH, white);
            
            if (loggingActive_)
            {
                ////////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////
                //////////////////// BEGIN LOGGING /////////////////////
                
                keyTouchLog_.write((char*)&timestamp, sizeof(timestamp_type));
                keyTouchLog_.write((char*)&frame, sizeof(int));
                keyTouchLog_.write((char*)&midiNote, sizeof(int));
                keyTouchLog_.write((char*)&newFrame, sizeof(KeyTouchFrame));
                
                ///////////////////// END LOGGING //////////////////////
                ////////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////
            }
            
            // Send raw OSC message if enabled
            if(sendRawOscMessages_) {
                keyboard_.sendMessage("/touchkeys/raw-off", "iii",
                                      octave, key, frame,
                                      LO_ARGS_END );
            }
            
        }
        
        return bytesParsed;
	}
	
	// At this point, construct a new frame with this data and pass it to the PianoKey for
	// further processing.  Leave the ID fields empty; these are state-dependent and will
	// be worked out based on the previous frames.
	
	KeyTouchFrame newFrame(touchCount, sliderPosition, sliderSize, sliderPositionH, white);
	
	keyboard_.key(midiNote)->touchInsertFrame(newFrame, timestamp);
    
    
    if (loggingActive_)
    {
        ////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////
        //////////////////// BEGIN LOGGING /////////////////////
        
        keyTouchLog_.write((char*)&timestamp, sizeof(timestamp_type));
        keyTouchLog_.write((char*)&frame, sizeof(int));
        keyTouchLog_.write((char*)&midiNote, sizeof(int));
        keyTouchLog_.write((char*)&newFrame, sizeof(KeyTouchFrame));
        
        ///////////////////// END LOGGING //////////////////////
        ////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////
    }
	
	// Send raw OSC message if enabled
	if(sendRawOscMessages_) {
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
	}
	
	// Verbose logging of key info
	if(verbose_ >= 3) {
		std::cout << "Octave " << octave << " Key " << key << " (TS " << timestamp << "): ";
		std::cout << sliderPositionH << " ";
		std::cout << sliderPosition[0] << " " << sliderPosition[1] << " " << sliderPosition[2] << " ";
		std::cout << sliderSize[0] << " " << sliderSize[1] << " " << sliderSize[2] << '\n';
	}	
	
	return bytesParsed;
}

// Process a frame of data containing analog values (i.e. key angle, Z-axis). These
// always come as a group for a whole board, and should be parsed apart into individual keys
void TouchkeyDevice::processAnalogFrame(unsigned char * const buffer, const int bufferLength) {
    // Format: [Octave] [TS0] [TS1] [TS2] [TS3] [Key0L] [Key0H] [Key1L] [Key1H] ... [Key24L] [Key24H]
    //                   ... (more frames)
    //                  [TS0] [TS1] [TS2] [TS3] [Key0L] [Key0H] [Key1L] [Key1H] ... [Key24L] [Key24H]
    
    if(bufferLength < 1) {
		if(verbose_ >= 1)
			std::cout << "Warning: ignoring malformed analog frame of " << bufferLength << " bytes, less than minimum 1\n";
        return;
    }
    
    int octave = buffer[0];
    int board = octave / 2;
    int frame;
    int bufferIndex = 1;
    int midiNote, value;
    
    // Parse the buffer one frame at a time
    while(bufferIndex < bufferLength) {
        if(bufferLength - bufferIndex < 54) {
            // This condition indicates a malformed analog frame (not enough data)
            if(verbose_ >= 1)
                std::cout << "Warning: ignoring extra analog data of " << bufferLength - bufferIndex << " bytes, less than full frame 54 (total " << bufferLength << ")\n";
            break;
        }
        
        // Find the timestamp (i.e. frame ID generated by the device). 32-bit little-endian.
        frame = buffer[bufferIndex] + ((int)buffer[bufferIndex+1] << 8) +
                ((int)buffer[bufferIndex+2] << 16) + ((int)buffer[bufferIndex+3] << 24);
        
        // Check the timestamp against the last frame from this board to see if any frames have been dropped
        if(frame > analogLastFrame_[board] + 1) {
            if(verbose_ >= 1)
                std::cout << "WARNING: dropped frame(s) on board " << board << " at " << frame << " (last was " << analogLastFrame_[board] << ")" << '\n';
        }
        else if(frame < analogLastFrame_[board] + 1) {
            if(verbose_ >= 1)
                std::cout << "WARNING: repeat frame(s) on board " << board << " at " << frame << " (last was " << analogLastFrame_[board] << ")" << '\n';
        }
        analogLastFrame_[board] = frame;
        
        // TESTING
        /*if(verbose_ >= 3 || (frame % 500 == 0))
            std::cout << "Analog frame octave " << octave << " timestamp " << frame << '\n';
        if(verbose_ >= 4 || (frame % 500 == 0)) {
            std::cout << "Values: ";
            for(int i = 0; i < 25; i++) {
                std::cout << std::setw(5) << (((signed char)buffer[i*2 + 6])*256 + buffer[i*2 + 5]) << " ";
            }
            std::cout << '\n';
        }*/
        
        // Process key values individually and add them to the keyboard data structure
        for(int key = 0; key < 25; key++) {
            // Every analog frame contains 25 values, however only the top board actually uses all 25
            // sensors. There are several "high C" values in the lower boards (i.e. key == 24) which
            // do not correspond to real sensors. These should be ignored.
            if(key == 24 && octave != numberOfOctaves() - 2)
                continue;
            
            midiNote = octaveKeyToMidi(octave, key);
            
            // Check that this note is in range to the available calibrators and keys.
            if(keyboard_.key(midiNote) == 0 || (octave*12 + key) >= keyCalibratorsLength_ || midiNote < 21)
                continue;
            
            // Pull the value out from the packed buffer (little endian 16 bit)
            value = (((signed char)buffer[key*2 + 6])*256 + buffer[key*2 + 5]);
            
            // Calibrate the value, assuming the calibrator is ready and running
            key_position calibratedPosition = keyCalibrators_[octave*12 + key]->evaluate(value);
            if(!missing_value<key_position>::isMissing(calibratedPosition)) {
                timestamp_type timestamp = timestampSynchronizer_.synchronizedTimestamp(frame);
                keyboard_.key(midiNote)->insertSample(calibratedPosition, timestamp);
            }
            else if(keyboard_.gui() != 0){
                
                //keyboard_.key(midiNote)->insertSample((float)value / 4096.0, timestampSynchronizer_.synchronizedTimestamp(frame));
                
                // Update the GUI but don't actually save the value since it's uncalibrated
                keyboard_.gui()->setAnalogValueForKey(midiNote, (float)value / kTouchkeyAnalogValueMax);
                
                if(keyCalibrators_[octave*12 + key]->calibrationStatus() == kPianoKeyCalibrated) {
                    if(verbose_ >= 1)
                        std::cout << "key " << midiNote << " calibrated but missing (raw value " << value << ")\n";
                }
            }
        }
        
        if(loggingActive_) {
            analogLog_.write((char*)&buffer[0], 1); // Octave number
            analogLog_.write((char*)&buffer[bufferIndex], 54);
        }
        
        // Skip to next frame
        bufferIndex += 54;
    }
}

// Process a frame containing a human-readable (and machine-coded) error message generated
// internally by the device
void TouchkeyDevice::processErrorMessageFrame(unsigned char * const buffer, const int bufferLength) {
    char msg[256];
    int len = bufferLength - 5;
    
    // Error on error message frame!
    if(bufferLength < 5) {
        if(verbose_ >= 1)
            std::cout << "Warning: received error message frame of " << bufferLength << " bytes, less than minimum 5\n";
        return;
    }
    
    // Limit length of string for safety reasons
    if(len > 256)
        len = 256;
    memcpy(msg, &buffer[5], len * sizeof(char));
    msg[len - 1] = '\0';
    
    // Print the error
    if(verbose_ >= 1)
        std::cout << "Error frame received: " << msg << '\n';
    
    // Dump the buffer containing error coding information
    if(verbose_ >= 2) {
        std::cout << "Contents: ";
        hexDump(std::cout, buffer, 5);
        std::cout << '\n';
    }
}

// Process a frame containing a response to an I2C command. We can use this to gather
// raw information from the key.
void TouchkeyDevice::processI2CResponseFrame(unsigned char * const buffer, const int bufferLength) {
    // Format: [octave] [key] [length] <data>
    
    if(bufferLength < 3) {
        if(verbose_ >= 1)
            std::cout << "Warning: received I2C response frame of " << bufferLength << " bytes, less than minimum 3\n";
        return;
    }
    
    int octave = buffer[0];
    int key = buffer[1];
    int responseLength = buffer[2];
    
    if(bufferLength < responseLength + 3) {
        if(verbose_ >= 1) {
            std::cout << "Warning: received malformed I2C response (octave " << octave << ", key " << key << ", length " << responseLength;
            std::cout << ") but only " << bufferLength - 3 << " bytes of data\n";
        }
        if(verbose_ >= 4) {
            std::cout << "  ";
            hexDump(std::cout, &buffer[3], bufferLength - 3);
            std::cout << '\n';
        }
        
        responseLength = bufferLength - 3;
    }
    else {
        if(verbose_ >= 3) {
            std::cout << "I2C response from octave " << octave << ", key " << key << ", length " << responseLength << '\n';
        }
        if(verbose_ >= 4) {
            std::cout << "  ";
            hexDump(std::cout, &buffer[3], responseLength);
            std::cout << '\n';
        }
    }
    
    if(sensorDisplay_ != 0) {
        // Copy response data to display
        std::vector<int> data;
        
        for(int i = 3; i < responseLength + 3; i++) {
            data.push_back(buffer[i]);
        }
        sensorDisplay_->setDisplayData(data);
    }
    
    // Change the first byte to contain the note number this data is expected to have come
    // from (based on which key we are presently querying)
    buffer[2] = lowestMidiNote_ + (octave * 12 + key);
    
	// Send raw data as an OSC blob
	lo_blob b = lo_blob_new(responseLength + 1, &buffer[2]);
	keyboard_.sendMessage("/touchkeys/rawbytes", "b", b, LO_ARGS_END);
	lo_blob_free(b);
}

// Parse raw data from a status request.  Buffer should start immediately after the
// frame type byte, and processing will finish either at the end of the expected buffer,
// or at the given length, whichever comes first.  Returns true if a status buffer was
// successfully received.

bool TouchkeyDevice::processStatusFrame(unsigned char * buffer, int maxLength, TouchkeyDevice::ControllerStatus *status) {
	if((status == 0 || maxLength < 5) && verbose_ >= 1) {
		std::cout << "Invalid status frame: ";
		hexDump(std::cout, buffer, maxLength);
		std::cout << '\n';
		return false;
	}
	
	status->hardwareVersion = buffer[0];
	status->softwareVersionMajor = buffer[1];
	status->softwareVersionMinor = buffer[2];
	status->running = ((buffer[3] & kStatusFlagRunning) != 0);
	status->octaves = buffer[4];
	status->connectedKeys = (unsigned int *)malloc(2*status->octaves*sizeof(unsigned int));
	
	int i, oct = 0;				// Get connected key information
    if(status->softwareVersionMajor >= 2) {
        // One extra byte holds lowest physical sensor
        status->lowestHardwareNote = buffer[5];
        status->hasTouchSensors = ((buffer[3] & kStatusFlagHasI2C) != 0);
        status->hasAnalogSensors = ((buffer[3] & kStatusFlagHasAnalog) != 0);
        status->hasRGBLEDs = ((buffer[3] & kStatusFlagHasRGBLED) != 0);
        i = 6;
    }
    else {
        status->lowestHardwareNote = 0;
        status->hasTouchSensors = true;
        status->hasAnalogSensors = true;
        status->hasRGBLEDs = true;
        i = 5;
    }

	while(i+1 < maxLength) {
		status->connectedKeys[oct] = 256*buffer[i] + buffer[i+1];
		i += 2;
		oct++;
	}
	
	if(oct < status->octaves && verbose_ >= 1) {
		std::cout << "Invalid status frame: ";
		hexDump(std::cout, buffer, maxLength);	
		std::cout << '\n';
		return false;
	}
	
	return true;
}

// Prepare the indicated key for raw data collection
void TouchkeyDevice::rawDataPrepareCollection(int octave, int key, int mode, int scaler) {
    juce::Thread::sleep(10);
    
    // Command to set the mode of the key
    unsigned char commandSetMode[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
        kFrameTypeSendI2CCommand, (unsigned char)octave, (unsigned char)key,
        3 /* xmit */, 0 /* response */, 0 /* command offset */, 1 /* mode */, (unsigned char)mode,
        ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	if(deviceWrite((char*)commandSetMode, 12) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setMode command.  errno = " << errno << '\n';
	}

    juce::Thread::sleep(10);
    
    // Command to set the scaler of the key
    unsigned char commandSetScaler[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
        kFrameTypeSendI2CCommand, (unsigned char)octave, (unsigned char)key,
        3 /* xmit */, 0 /* response */, 0 /* command offset */, 3 /* raw scaler */, (unsigned char)scaler,
        ESCAPE_CHARACTER, kControlCharacterFrameEnd};
	
	if(deviceWrite((char*)commandSetScaler, 12) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write setMode command.  errno = " << errno << '\n';
	}
    
    juce::Thread::sleep(10);
    
    unsigned char commandPrepareRead[] = {ESCAPE_CHARACTER, kControlCharacterFrameBegin,
        kFrameTypeSendI2CCommand, (unsigned char)octave, (unsigned char)key,
        1 /* xmit */, 0 /* response */, 6 /* data offset */,
        ESCAPE_CHARACTER, kControlCharacterFrameEnd};
    
	if(deviceWrite((char*)commandPrepareRead, 10) < 0) {
        if(verbose_ >= 1)
            std::cout << "ERROR: unable to write prepareRead command.  errno = " << errno << '\n';
	}

   juce::Thread::sleep(10);
    
    rawDataShouldChangeMode_ = false;
}

// Check for an ACK response from the device.  Returns true if found.  Returns
// false if NAK received, or if a timeout occurs.
// TODO: this implementation needs to change to not chew up other data coming in.

bool TouchkeyDevice::checkForAck(int timeoutMilliseconds) {
	//struct timeval startTime, currentTime;
	bool controlSeq = false;
	unsigned char ch;
	
    double startTime = juce::Time::getMillisecondCounterHiRes();
    double currentTime = startTime;
    
	//gettimeofday(&startTime, 0);
	//gettimeofday(&currentTime, 0);
	
	while(currentTime - startTime < (double)timeoutMilliseconds) {
		long count = deviceRead((char *)&ch, 1);

		if(count < 0) {				// Check if an error occurred on read
			if(errno != EAGAIN) {
                if(verbose_ >= 1)
                    std::cout << "Unable to read from device while waiting for ACK (error " << errno << ").  Aborting.\n";
				return false;
			}
		}
		else if(count > 0) {		// Data received
			// Wait for a sequence {ESCAPE_CHARACTER, ACK} or {ESCAPE_CHARACTER, NAK}
			if(controlSeq) {
				controlSeq = false;
				if(ch == kControlCharacterAck) {
					if(verbose_ >= 2)
						std::cout << "Received ACK\n";
					return true;
				}
				else if(ch == kControlCharacterNak) {
					if(verbose_ >= 1)
						std::cout << "Warning: received NAK\n";
					return false;
				}
			}
			else if(ch == ESCAPE_CHARACTER)
				controlSeq = true;
		}
		
		currentTime = juce::Time::getMillisecondCounterHiRes();
	}
	
    if(verbose_ >= 1)
        std::cout << "Error: timeout waiting for ACK\n";
	return false;
}

// Convenience method to dump hexadecimal output
void TouchkeyDevice::hexDump( std::ostream& str, unsigned char * buffer, int length) {
	if(length <= 0)
		return;
	str << std::hex << (int)buffer[0];
	for(int i = 1; i < length; i++) {
		str << " " << (int)buffer[i];
	}
	str << std::dec;
}

// Read from the TouchKeys device
long TouchkeyDevice::deviceRead(char *buffer, unsigned int count) {
#ifdef _MSC_VER
	int n;

	if(!ReadFile(serialHandle_, buffer, count, (LPDWORD)((void *)&n), NULL))
		return -1;
	return n;
#else
    return read(device_, buffer, count);
#endif
}

// Write to the TouchKeys device
int TouchkeyDevice::deviceWrite(char *buffer, unsigned int count) {
    int result;
    
#ifdef _MSC_VER
    if(!WriteFile(serialHandle_, buffer, count, (LPDWORD)((void *)&result), NULL))
		return -1;
#else
    result = write(device_, buffer, count);
#endif
    deviceDrainOutput();
    return result;
}

// Flush (discard) the TouchKeys device input
void TouchkeyDevice::deviceFlush(bool bothDirections) {
#ifdef _MSC_VER
	// WINDOWS_TODO (?)
#else
    if(bothDirections)
        tcflush(device_, TCIOFLUSH);
    else
        tcflush(device_, TCIFLUSH);							// Flush device input
#endif
}

// Flush the TouchKeys device output
void TouchkeyDevice::deviceDrainOutput() {
#ifdef _MSC_VER
    FlushFileBuffers(serialHandle_);
#else
    tcdrain(device_);
#endif
}


TouchkeyDevice::~TouchkeyDevice() {
    if (logFileCreated_)
    {
        keyTouchLog_.close();
        analogLog_.close();
    }
    
	closeDevice();
    calibrationDeinit();
}