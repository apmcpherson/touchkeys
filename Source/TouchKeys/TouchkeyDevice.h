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
 
  TouchkeyDevice.h: handles communication with the TouchKeys hardware
*/

#pragma once

#include "Osc.h"
#include "../Utility/TimestampSynchronizer.h"
#include "PianoKeyCalibrator.h"
#include "../Display/RawSensorDisplay.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <cstdio>
#include <cmath>
#include <deque>
#include <errno.h>
#include <fcntl.h>
#include <limits>
#include <list>
#ifndef _MSC_VER
#include <termios.h>
#endif


#define TOUCHKEY_MAX_FRAME_LENGTH 256	// Maximum data length in a single frame
#define ESCAPE_CHARACTER 0xFE			// Indicates control sequence

//#define TRANSMISSION_LENGTH_WHITE 9
//#define TRANSMISSION_LENGTH_BLACK 8
//#define TRANSMISSION_LENGTH_TOTAL (8*TRANSMISSION_LENGTH_WHITE + 5*TRANSMISSION_LENGTH_BLACK)

const int kTransmissionLengthWhiteOldHardware = 9;
const int kTransmissionLengthBlackOldHardware = 8;
const int kTransmissionLengthWhiteNewHardware = 9;
const int kTransmissionLengthBlackNewHardware = 9;
const int kTransmissionLengthTotalOldHardware = (8 * kTransmissionLengthWhiteOldHardware + 5 * kTransmissionLengthBlackOldHardware);
const int kTransmissionLengthTotalNewHardware = (8 * kTransmissionLengthWhiteNewHardware + 5 * kTransmissionLengthBlackNewHardware);

// Maximum integer values for different types of sliders

//#define WHITE_MAX_VALUE 1280.0		// White keys, vertical	(64 * 20)
//#define WHITE_MAX_H_VALUE 255.0		// Whtie keys, horizontal
//#define BLACK_MAX_VALUE 1024.0		// Black keys, vertical (64 * 16)
//#define SIZE_MAX_VALUE 255.0		// Max touch size for either key type

const float kWhiteMaxYValueOldHardware = 1280.0;    // White keys, vertical	(64 * 20)
const float kWhiteMaxXValueOldHardware = 255.0;     // White keys, horizontal (1 byte)
const float kBlackMaxYValueOldHardware = 1024.0;    // Black keys, vertical (64 * 16)
const float kWhiteMaxYValueNewHardware = 2432.0;    // White keys, vertical (128 * 19)
const float kWhiteMaxXValueNewHardware = 256.0;     // White keys, horizontal (1 byte + 1 bit)
const float kBlackMaxYValueNewHardware = 1536.0;    // Black keys, vertical (128 * 12)
const float kBlackMaxXValueNewHardware = 256.0;     // Black keys, horizontal (1 byte + 1 bit)

const float kSizeMaxValue = 255.0;

enum {
	kControlCharacterFrameBegin = 0x00,
	kControlCharacterAck = 0x01,
	kControlCharacterNak = 0x02,
	kControlCharacterFrameError = 0xFD,
	kControlCharacterFrameEnd = 0xFF
};

// Frame types for data sent over USB.  The first byte following a frame start control sequence gives the type.

enum {
	kFrameTypeStatus = 0,		// Status info: connected keys, current operating modes
	kFrameTypeCentroid = 16,	// Centroid data (default mode of operation)
	kFrameTypeI2CResponse = 17,	// Response from a specific I2C command
	kFrameTypeRawKeyData = 18,	// Raw data from the selected key	
    kFrameTypeAnalog = 19,		// Analog data from Z-axis optical sensors
	
    kFrameTypeErrorMessage = 127, // Error message from controller
	// These types are for incoming (computer -> us) data
	kFrameTypeStartScanning = 128,	// Start auto-scan
	kFrameTypeStopScanning = 129,	// Stop auto-scan
	kFrameTypeSendI2CCommand = 130,	// Send a specific I2C command
	kFrameTypeResetDevices = 131,	// Physically reset the system
	kFrameTypeScanRate = 132,		// Set the scan rate (in milliseconds)	
	kFrameTypeNoiseThreshold = 133,
	kFrameTypeSensitivity = 134,
	kFrameTypeSizeScaler = 135,
	kFrameTypeMinimumSize = 136,
	kFrameTypeSetEnabledKeys = 137,
	kFrameTypeMonitorRawFromKey = 138,
	kFrameTypeUpdateBaselines = 139,	// Reinitialize baseline values
	kFrameTypeRescanKeyboard = 140,	// Rescan what keys are connected
    kFrameTypeEncapsulatedMIDI = 167, // MIDI messages to pass to MIDI standalone firmware
    kFrameTypeRGBLEDSetColors = 168, // Set RGBLEDs of given index to specific values
	kFrameTypeRGBLEDAllOff = 169,    // All LEDs off
	kFrameTypeEnterISPMode = 192,
    kFrameTypeEnterSelfProgramMode = 193
};

enum {
	kKeyColorWhite = 0,
	kKeyColorBlack
};

enum {
	kStatusFlagRunning = 0x01,
	kStatusFlagRawMode = 0x02,
	kStatusFlagHasI2C = 0x04,
	kStatusFlagHasAnalog = 0x08,
	kStatusFlagHasRGBLED = 0x10,
	kStatusFlagComError = 0x80
};


const int kKeyColor[13] = { kKeyColorWhite, kKeyColorBlack, kKeyColorWhite,
	kKeyColorBlack, kKeyColorWhite, kKeyColorWhite, kKeyColorBlack,
	kKeyColorWhite, kKeyColorBlack, kKeyColorWhite, kKeyColorBlack,
	kKeyColorWhite, kKeyColorWhite };

const int kWhiteKeyIndices[13] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6, 7};

const unsigned char kCommandStatus[] = { ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeStatus,
	ESCAPE_CHARACTER, kControlCharacterFrameEnd };
const unsigned char kCommandStartScanning[] = { ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeStartScanning,
	ESCAPE_CHARACTER, kControlCharacterFrameEnd };
const unsigned char kCommandStopScanning[] = { ESCAPE_CHARACTER, kControlCharacterFrameBegin, kFrameTypeStopScanning,
	ESCAPE_CHARACTER, kControlCharacterFrameEnd };

#define octaveNoteToIndex(octave, note) (100*octave + note)	// Generate indices for containers
#define indexToOctave(index) (int)(index / 100)
#define indexToNote(index) (index % 100)

const float kTouchkeyAnalogValueMax = 4095.0; // Maximum value any analog sample can take

// This class implements device access to the touchkey hardware.

class TouchkeyDevice /*: public OscHandler*/
{
    // ***** Class to implement the Juce thread *****
private:
    class DeviceThread : public juce::Thread {
    public:
        DeviceThread(boost::function<void (DeviceThread*)> action, juce::String name = "DeviceThread")
        : juce::Thread(name), actionFunction_(action) {}
        
        ~DeviceThread() {}
        
        void run() {
            actionFunction_(this);
        }
        
    private:
        boost::function<void (DeviceThread*)> actionFunction_;
    };
    
public:
	class ControllerStatus {
	public:
		ControllerStatus() : connectedKeys(0) {}
		~ControllerStatus() {
			if(connectedKeys != 0)
				free(connectedKeys);
		}
		
		int hardwareVersion;		// Hardware version
		int softwareVersionMajor;	// Controller firmware major version
		int softwareVersionMinor;	// Controller firmware minor version
		bool running;				// Is the system currently gathering centroid data?
        bool hasTouchSensors;       // Whether the device has I2C touch sensors
        bool hasAnalogSensors;      // Whether the device has analog optical position sensors
        bool hasRGBLEDs;            // Whether the device has RGB LEDs for display
		int octaves;				// Number of octaves connected [two octaves per board]
        int lowestHardwareNote;     // Note number (0-12) of lowest connector or sensor on lowest board
		unsigned int *connectedKeys;// Which keys are connected to each octave
	};
	
	class MultiKeySweep {
	public:
		
		int sweepId;
		int sweepOctave;
		float sweepNote;
		int keyCount;
		int keyOctave[2];
		int keyNote[2];
		int keyTouchId[2];
		float keyPosition[2];
	};
    
    // Structure to hold changes to RGB LEDs on relevant hardware
    class RGBLEDUpdate {
    public:
        bool allLedsOff;        // Set to true if all LEDs should turn off on indicated board
        int midiNote;           // MIDI note number to change
        int red;                // RGB color
        int green;
        int blue;
    };
    
private:
    // Data structure to keep track of stray touches. Implements a state machine
    // to check what state each touch is in along with some other information on where
    // it is located and when it started
    class StrayTouchRecord {
    public:
        typedef enum {
            StateOff = 0,
            StateWaitingOff,
            StateWaitingOn,
            StateSuppressed,
            StateActive,
        } TouchState;
        
        StrayTouchRecord(TouchState _state,
                         timestamp_type _startingTimestamp,
                         float _startingPosition, float _currentPosition,
                         bool _midiNoteIsOn)
        : state(_state), startingTimestamp(_startingTimestamp),
          startingPosition(_startingPosition), currentPosition(_currentPosition),
          midiNoteIsOn(_midiNoteIsOn), matched(false)
        {}
        
        TouchState state;
        timestamp_type startingTimestamp;
        float startingPosition;
        float currentPosition;
        bool midiNoteIsOn;
        bool matched;
    };
	
public:
	// ***** Constructor *****
	TouchkeyDevice(PianoKeyboard& keyboard);
    
    // ***** Destructor *****
	~TouchkeyDevice();
    
    // ***** Device Management *****
	// Open a new device.  Returns true on success
	bool openDevice(const char * inputDevicePath);
	void closeDevice();
	
	// Start or stop the processing.  startAutoGathering() returns
	// true on success.
	bool startAutoGathering();
	void stopAutoGathering(bool writeStopCommandToDevice = true);
	
	// Status query methods
	bool isOpen();
	bool isAutoGathering() { return autoGathering_; }
	int numberOfOctaves() { return numOctaves_; }
    
	// Ping the device, to see if it is ready to respond
	bool checkIfDevicePresent(int millisecondsToWait);
	
	// Start collecting raw data from a given key
	bool startRawDataCollection(int octave, int key, int mode, int scaler);
    void rawDataChangeKeyAndMode(int octave, int key, int mode, int scaler);
    
    // ***** RGB LED updates *****
    void rgbledSetColor(const int midiNote, const float red, const float green, const float blue);
    void rgbledSetColorHSV(const int midiNote, const float hue, const float saturation, const float value);
    void rgbledAllOff();
    
    // ***** Device Parameters *****
    
	// Set the scan interval in milliseconds
	bool setScanInterval(int intervalMilliseconds);
	
	// Key parameters.  Setting octave or key to -1 means all octaves or all keys, respectively.
	bool setKeySensitivity(int octave, int key, int value);
	bool setKeyCentroidScaler(int octave, int key, int value);
	bool setKeyMinimumCentroidSize(int octave, int key, int value);
	bool setKeyNoiseThreshold(int octave, int key, int value);
    bool setKeyUpdateBaseline(int octave, int key);
    
    // Set whether to ignore stray touches that might not be actual finger touch events
    void setSuppressStrayTouches(int level);
    
    // Jump to device internal bootloader
    void jumpToBootloader();
    
    // ***** Calibration Methods *****
    
	// Return whether or not the controller has been calibrated, and whether it's currently calibrating
	bool isCalibrated() { return isCalibrated_; }
	bool calibrationInProgress() { return calibrationInProgress_; }
	
	// Start: begin calibrating; finish: end and save results; abort: end and discard results
	void calibrationStart(std::vector<int>* keysToCalibrate);
	void calibrationFinish();
	void calibrationAbort();
	void calibrationClear();
	
	bool calibrationSaveToFile(std::string const& filename);
	bool calibrationLoadFromFile(std::string const& filename);
    
    // ***** Data Logging *****
    void createLogFiles( std::string keyTouchLogFilename, std::string analogLogFilename, std::string path);
    void closeLogFile();
    void startLogging();
    void stopLogging();

    // ***** Debugging and Utility *****
    
	// Set logging level
	void setVerboseLevel(int v) { verbose_ = v; }
	void setTransmitRawData(bool raw) { sendRawOscMessages_	= raw; }
    bool transmitRawDataEnabled() { return sendRawOscMessages_; }
    
	// Conversion between touchkey # and MIDI note
	int lowestMidiNote() { return lowestMidiNote_; }
    int highestMidiNote() { return lowestMidiNote_ + 12*numOctaves_ + lowestNotePerOctave_; }
    int lowestKeyPresentMidiNote() { return lowestKeyPresentMidiNote_; } // What is the lowest key actually connected?
	void setLowestMidiNote(int note);
    int octaveKeyToMidi(int octave, int key);
    
    // Sensor data display
    void setSensorDisplay(RawSensorDisplay *display) { sensorDisplay_ = display; }
    
	// ***** Run Loop Functions *****
    void ledUpdateLoop(DeviceThread *thread);
	void runLoop(DeviceThread *thread);
    void rawDataRunLoop(DeviceThread *thread);
    
    // for debugging
    void testStopLeds() { ledShouldStop_ = true; }
	
private:
	// Read and parse new data from the device, splitting out by frame type
	void processFrame(unsigned char * const frame, int length);

	// Specific data type parsing
	void processCentroidFrame(unsigned char * const buffer, const int bufferLength);
	int processKeyCentroid(int frame,int octave, int key, timestamp_type timestamp, unsigned char * buffer, int maxLength);
    void processAnalogFrame(unsigned char * const buffer, const int bufferLength);
	void processRawDataFrame(unsigned char * const buffer, const int bufferLength);
	bool processStatusFrame(unsigned char * buffer, int maxLength, ControllerStatus *status);
    void processI2CResponseFrame(unsigned char * const buffer, const int bufferLength);
    void processErrorMessageFrame(unsigned char * const buffer, const int bufferLength);

	// Helper methods for centroid processing
	//pair<float, list<int> > matchClosestPoints(float* oldPoints, float *newPoints, float count,
	//										   int oldIndex, set<int>& availableNewPoints, float currentTotalDistance);
	void processTwoFingerGestures(int octave, int key, KeyTouchFrame& previousPosition, KeyTouchFrame& newPosition);
	void processThreeFingerGestures(int octave, int key, KeyTouchFrame& previousPosition, KeyTouchFrame& newPosition);
	
	// Utility method for parsing multi-key gestures
	std::pair<int, int> whiteKeyAbove(int octave, int note);
	
    // Write the commands to prepare a given key for raw data collection
    void rawDataPrepareCollection(int octave, int key, int mode, int scaler);
    
	// After writing a command, check whether it was acknolwedged by the controller
	bool checkForAck(int timeoutMilliseconds);
	
	// Utility method for debugging
	void hexDump( std::ostream& str, unsigned char * buffer, int length);
    
    // Internal calibration methods
    void calibrationInit(int numberOfCalibrators);
    void calibrationDeinit();
    
    // Set RGB LED color (for piano scanner boards)
    bool internalRGBLEDSetColor(const int device, const int led, const int red, const int green, const int blue);
    bool internalRGBLEDAllOff();                        // RGB LEDs off
    int  internalRGBLEDMIDIToBoardNumber(const int midiNote);   // Get board number for MIDI note
    int  internalRGBLEDMIDIToLEDNumber(const int midiNote);     // Get LED number for MIDI note
    
    // Device low-level access methods
    long deviceRead(char *buffer, unsigned int count);
    int deviceWrite(char *buffer, unsigned int count);
    void deviceFlush(bool bothDirections);
    void deviceDrainOutput();
	
private:
	PianoKeyboard& keyboard_;	// Main keyboard controller

#ifdef _MSC_VER
	HANDLE serialHandle_;		// Serial port handle
#else
	int device_;				// File descriptor
#endif
	DeviceThread ioThread_;		// Thread that handles the communication from the device
    DeviceThread rawDataThread_;// Thread that handles raw data collection
	//CriticalSection ioMutex_;	// Mutex synchronizing access between internal and external threads
	bool autoGathering_;		// Whether auto-scanning is enabled
	volatile bool shouldStop_;	// Communication variable between threads
	bool sendRawOscMessages_;	// Whether we should transmit the raw frame data by OSC
	int verbose_;				// Logging level
	int numOctaves_;			// Number of connected octaves (determined from device)
	int lowestMidiNote_;		// MIDI note number for the lowest C on the lowest octave
    int lowestKeyPresentMidiNote_; // MIDI note number for the lowest key actually attached
    int updatedLowestMidiNote_; // Lowest MIDI note if changed; held separately for thread sync
    int lowestNotePerOctave_;   // Note which starts each octave, for non C-to-C keyboards
	std::set<int> keysPresent_;	// Which keys (octave and note) are present on this device?
    int deviceSoftwareVersion_; // Which version of the device we're talking to
    int deviceHardwareVersion_; // Which version of the device hardware is running
    int expectedLengthWhite_;   // How long the white key data blocks are
    int expectedLengthBlack_;   // How long the black key data blocks are
    float whiteMaxX_, whiteMaxY_;   // Maximum sensor values for white keys
    float blackMaxX_, blackMaxY_;   // Maximum sensor values for black keys
    
    int strayTouchSuppression_; // Whether to suppress stray touches on the keys
    bool strayTouchSuppressionWasEnabled_; // Internal cache of whether suppression was enabled, in case it turns off on the fly
    std::vector<StrayTouchRecord> strayTouchRegister_[128];  // Information on active and stray touches
    
    // Frame counter for analog data, to detect dropped frames
    unsigned int analogLastFrame_[4];    // Max 4 boards
	
	// Synchronization between frame time and system timestamp, allowing interaction
	// with other simultaneous streams using different clocks.  Also save the last timestamp
	// we've processed to other functions can access it.
	TimestampSynchronizer timestampSynchronizer_;	
	timestamp_type lastTimestamp_;
    
    // For raw data collection, this information keeps track of which key we're reading
    bool rawDataShouldChangeMode_;
    int rawDataCurrentOctave_, rawDataCurrentKey_;
    int rawDataCurrentMode_, rawDataCurrentScaler_;
    
    // ***** RGB LED management *****
    bool deviceHasRGBLEDs_;                 // Whether the device has RGB LEDs
    DeviceThread ledThread_;                   // Thread that handles LED updates (communication to the device)
    volatile bool ledShouldStop_;           // testing
	std::deque<RGBLEDUpdate> ledUpdateQueue_; // Queue that holds new LED messages to be sent to device
    
    // ***** Calibration *****
    bool isCalibrated_;
	bool calibrationInProgress_;
    
    PianoKeyCalibrator** keyCalibrators_;	// Calibration information for each key
    int keyCalibratorsLength_;              // How many calibrators
    
    // ***** Logging *****
	std::ofstream keyTouchLog_;
	std::ofstream analogLog_;
    bool logFileCreated_;
    bool loggingActive_;
    
    // ***** Sensor Data Display (for debugging) *****
    RawSensorDisplay *sensorDisplay_;
};
