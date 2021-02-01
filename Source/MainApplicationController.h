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
 
  MainApplicationController.h: contains the overall glue that holds
  together the various parts of the TouchKeys code. It works together
  with the user interface to let the user configure the hardware and
  manage the mappings, but it is kept separate from any particular user
  interface configuration.
*/

#pragma once

#undef TOUCHKEY_ENTROPY_GENERATOR_ENABLE

#include "TouchKeys/MidiInputController.h"
#include "TouchKeys/MidiOutputController.h"
#include "TouchKeys/TouchkeyDevice.h"
#include "TouchKeys/TouchkeyOscEmulator.h"
#include "TouchKeys/LogPlayback.h"
#include "Mappings/Vibrato/TouchkeyVibratoMappingFactory.h"
#include "Mappings/PitchBend/TouchkeyPitchBendMappingFactory.h"
#include "Mappings/Control/TouchkeyControlMappingFactory.h"
#include "Mappings/ReleaseAngle/TouchkeyReleaseAngleMappingFactory.h"
#include "Mappings/OnsetAngle/TouchkeyOnsetAngleMappingFactory.h"
#include "Mappings/MultiFingerTrigger/TouchkeyMultiFingerTriggerMappingFactory.h"
#include "Mappings/KeyDivision/TouchkeyKeyDivisionMappingFactory.h"
#include "Mappings/MappingFactorySplitter.h"

#include <cstdlib>
#include <sstream>

#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
#include "TouchKeys/TouchkeyEntropyGenerator.h"
#endif

#ifndef TOUCHKEYS_NO_GUI
#include "GUI/GraphicsDisplayWindow.h"
#include "GUI/PreferencesWindow.h"
class KeyboardTesterDisplay;
#endif

const char kDefaultOscTransmitHost[] = "127.0.0.1";
const char kDefaultOscTransmitPort[] = "8000";
const int kDefaultOscReceivePort = 8001;

class MainApplicationOSCController;

class MainApplicationController : public OscHandler {
    friend class MainApplicationOSCController;
    
public:
    // *** Constructor ***
    MainApplicationController();
    
    // *** Destructor ***
    ~MainApplicationController();
    
    // *** Startup actions ***
    void initialise();
    
    // *** TouchKeys device methods ***
    
    // Return the path prefix of the TouchKeys device
    std::string touchkeyDevicePrefix();
    
    // Return a list of paths to all available touchkey devices
    std::vector<std::string> availableTouchkeyDevices();

    // Run the main startup sequence: open device, check its presence,
    // start data collection, all in one method. Returns true if successful.
    // Will set the error message string if not
    bool touchkeyDeviceStartupSequence(const char * path);
    void touchkeyDeviceClearErrorMessage();
    
    // Check whether a given touchkey device exists
    bool touchkeyDeviceExists(const char * path);
    
    // Select a particular touchkey device
    bool openTouchkeyDevice(const char * path);
    
    void closeTouchkeyDevice();
    
    // Check for device present
    bool touchkeyDeviceCheckForPresence(int waitMilliseconds = 250, int tries = 10);
    
    // Start/stop the TouchKeys data collection
    bool startTouchkeyDevice();
    void stopTouchkeyDevice();
    
    // Status queries on TouchKeys
    // Returns true if device has been opened
    bool touchkeyDeviceIsOpen();
    
    // Return true if device is collecting data
    bool touchkeyDeviceIsRunning();
    
    // Returns true if an error has occurred
    bool touchkeyDeviceErrorOccurred();
    
    // Return the error message if one occurred
    std::string touchkeyDeviceErrorMessage();
    
    // How many octaves on the current device
    int touchkeyDeviceNumberOfOctaves();
    
    // Return the lowest MIDI note
    int touchkeyDeviceLowestMidiNote();
    
    // Set the lowest MIDI note for the TouchKeys
    void touchkeyDeviceSetLowestMidiNote(int note);
    
    // Attempt to autodetect the correct TouchKey octave from MIDI data
    void touchkeyDeviceAutodetectLowestMidiNote();
    void touchkeyDeviceStopAutodetecting();
    bool touchkeyDeviceIsAutodetecting();
    
    // *** MIDI device methods ***
    
    // Return a list of IDs and paths to all available MIDI devices
    std::vector<std::pair<int, std::string> > availableMIDIInputDevices() {
        return midiInputController_.availableMidiDevices();
    }
    
    std::vector<std::pair<int, std::string> > availableMIDIOutputDevices() {
        return midiOutputController_.availableMidiDevices();
    }
    
    // Return the number of keyboard segments
    int midiSegmentsCount() {
        return midiInputController_.numSegments();
    }
    // Return the pointer to a specific segment
    MidiKeyboardSegment* midiSegment(int index) {
        return midiInputController_.segment(index);
    }
    // Return a unique signature of segment configuration which
    // tells any listeners whether an update has happened
    int midiSegmentUniqueIdentifier() {
        return midiInputController_.segmentUniqueIdentifier();
    }
    // Add a new segment, returning the result. Segments are
    // stored 
    MidiKeyboardSegment* midiSegmentAdd();
    // Remove a segment
    void midiSegmentRemove(MidiKeyboardSegment *segment);

    // Select MIDI input/output devices
    void enableMIDIInputPort(int portNumber, bool isPrimary);
    void enableAllMIDIInputPorts(int primaryPortNumber);
    void disableMIDIInputPort(int portNumber);
    void disablePrimaryMIDIInputPort();
    void disableAllMIDIInputPorts(bool auxiliaryOnly);
    void enableMIDIOutputPort(int identifier, int deviceNumber);
#ifndef JUCE_WINDOWS
    void enableMIDIOutputVirtualPort(int identifier, const char *name);
#endif
    void disableMIDIOutputPort(int identifier);
    void disableAllMIDIOutputPorts();
    
    // Get selected MIDI input/output devices by ID
    int selectedMIDIPrimaryInputPort() {
        return midiInputController_.primaryActivePort();
    }
    std::vector<int> selectedMIDIAuxInputPorts() {
        return midiInputController_.auxiliaryActivePorts();
    }
    int selectedMIDIOutputPort(int identifier) {
        return midiOutputController_.enabledPort(identifier);
    }
    
    void midiTouchkeysStandaloneModeEnable();
    void midiTouchkeysStandaloneModeDisable();
    bool midiTouchkeysStandaloneModeIsEnabled() { return touchkeyStandaloneModeEnabled_; }
    
    // *** Update sync methods ***
    // The controller maintains a variable that tells when the devices should be updated
    // by the control window component. Whenever it changes value, the devices should be rescanned.
    
    int devicesShouldUpdate() { return deviceUpdateCounter_; }
    void tellDevicesToUpdate() { deviceUpdateCounter_++; }
    
    // *** OSC device methods ***
    
    bool oscTransmitEnabled();
    void oscTransmitSetEnabled(bool enable);
    bool oscTransmitRawDataEnabled();
    void oscTransmitSetRawDataEnabled(bool enable);
    std::vector<lo_address> oscTransmitAddresses();
    int oscTransmitAddAddress(const char * host, const char * port, int proto = LO_UDP);
	void oscTransmitRemoveAddress(int index);
	void oscTransmitClearAddresses();
    
    // OSC Input (receiver) methods
    // Enable or disable on the OSC receive, and report is status
    bool oscReceiveEnabled();
    
    // Enable method returns true on success (false only if it was
    // unable to set the port)
    bool oscReceiveSetEnabled(bool enable);
    
    // Whether the OSC server is running (false means couldn't open port)
    bool oscReceiveRunning();
    
    // Get the current OSC receive port
    int oscReceivePort();
    
    // Set the current OSC receive port (returns true on success)
    bool oscReceiveSetPort(int port);
    
    // *** Display methods ***
    
    KeyboardDisplay& keyboardDisplay() { return keyboardDisplay_; }
#ifndef TOUCHKEYS_NO_GUI
    void setKeyboardDisplayWindow( juce::DocumentWindow *window) { keyboardDisplayWindow_ = window; }
    void showKeyboardDisplayWindow() {
        if(keyboardDisplayWindow_ != nullptr) {
            keyboardDisplayWindow_->addToDesktop(keyboardDisplayWindow_->getDesktopWindowStyleFlags() 
						 | juce::ComponentPeer::windowHasCloseButton);
            keyboardDisplayWindow_->setVisible(true);
            keyboardDisplayWindow_->toFront(true);
        }
    }
    void setPreferencesWindow(PreferencesWindow *window) { preferencesWindow_ = window; }
    void showPreferencesWindow() {
        if(preferencesWindow_ != nullptr) {
            preferencesWindow_->addToDesktop(preferencesWindow_->getDesktopWindowStyleFlags() 
					     | juce::ComponentPeer::windowHasCloseButton);
            preferencesWindow_->setVisible(true);
            preferencesWindow_->toFront(true);
        }
    }
#endif
    
    // *** Logging methods ***
    // Logging methods which record TouchKeys and MIDI data to files for
    // later analysis/playback
    
    void startLogging();
    void stopLogging();
    bool isLogging() { return loggingActive_; }
    void setLoggingDirectory(const char *directory);
    
    // Playback methods for log files

    void playLogWithDialog();
    void stopPlayingLog();
    bool isPlayingLog() { return isPlayingLog_; }
    
    // *** OSC handler method (different from OSC device selection) ***
    
	bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
    // *** Mapping methods ***

    // Whether experimental (not totally finished/tested) mappings are available
    bool experimentalMappingsEnabled() { return experimentalMappingsEnabled_; }
    void setExperimentalMappingsEnabled(bool enable) { experimentalMappingsEnabled_ = enable; }
    
    // *** Preset Save/Load ***
    // These methods save the current settings to file or load settings
    // from a file. They return true on success.
    bool savePresetToFile(const char *filename);
    bool loadPresetFromFile(const char *filename);

#ifndef TOUCHKEYS_NO_GUI
    bool savePresetWithDialog();
    bool loadPresetWithDialog();
#endif
    
    // Clears the current preset and restores default settings to zones/mappings
    void clearPreset();
    
    // *** Preferences ***
    
    // Whether to automatically start the TouchKeys on startup
    bool getPrefsAutoStartTouchKeys();
    void setPrefsAutoStartTouchKeys(bool autoStart);
    
    // Whether to automatically detect the TouchKeys octave when they start
    bool getPrefsAutodetectOctave();
    void setPrefsAutodetectOctave(bool autoDetect);
    
    // Which preset (if any) to load at startup
    void setPrefsStartupPresetNone();
    bool getPrefsStartupPresetNone();
    
    void setPrefsStartupPresetLastSaved();
    bool getPrefsStartupPresetLastSaved();
    
    void setPrefsStartupPresetVibratoPitchBend();
    bool getPrefsStartupPresetVibratoPitchBend();
    
    void setPrefsStartupPreset(juce::String const& path);
    juce::String getPrefsStartupPreset();
    
    // Reset all preferences
    void resetPreferences();
    
    // Load global preferences from file
    void loadApplicationPreferences();
    
    // Load a MIDI output device from preexisting application preferences
    void loadMIDIOutputFromApplicationPreferences(int zone);
    
#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST
    // *** TouchKeys sensor testing methods ***
    // Start testing the TouchKeys sensors
    bool touchkeySensorTestStart(const char *path, int firstKey);
    
    // Stop testing the TouchKeys sensors
    void touchkeySensorTestStop();
    
    // Is the sensor test running?
    bool touchkeySensorTestIsRunning();
    
    // Set the current key that is begin tested
    void touchkeySensorTestSetKey(int key);
    
    // Reset the testing state to all sensors off
    void touchkeySensorTestResetState();
#endif
    
#ifdef ENABLE_TOUCHKEYS_FIRMWARE_UPDATE
    // Put TouchKeys controller board into bootloader mode
    bool touchkeyJumpToBootloader(const char *path);
#endif
    
    // *** Static utility methods ***
    static std::string midiNoteName(int noteNumber);
    static int midiNoteNumberForName(std::string const& name);
    
private:
    bool savePresetHelper( juce::File& outputFile);
    bool loadPresetHelper( juce::File const& inputFile);
    
    // Application properties: for managing preferences
    juce::ApplicationProperties applicationProperties_;
    
    // TouchKeys objects
    MainApplicationOSCController *mainOscController_;
    PianoKeyboard keyboardController_;
    MidiInputController midiInputController_;
    MidiOutputController midiOutputController_;
    OscTransmitter oscTransmitter_;
    OscReceiver oscReceiver_;
    TouchkeyDevice touchkeyController_;
    TouchkeyOscEmulator touchkeyEmulator_;
    LogPlayback *logPlayback_;
#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
    TouchkeyEntropyGenerator touchkeyEntropyGenerator_;
    bool entropyGeneratorSelected_;
#endif
    
    bool touchkeyErrorOccurred_;
    std::string touchkeyErrorMessage_;
    bool touchkeyAutodetecting_;
    bool touchkeyStandaloneModeEnabled_;
    int deviceUpdateCounter_;               // Unique number that increments every time devices should
                                            // be rescanned
    
    // OSC information
    bool oscReceiveEnabled_;
    int oscReceivePort_;
    
    // Mapping objects
    bool experimentalMappingsEnabled_;
    
    // Display objects
    KeyboardDisplay keyboardDisplay_;
#ifndef TOUCHKEYS_NO_GUI
    juce::DocumentWindow *keyboardDisplayWindow_;
    KeyboardTesterDisplay *keyboardTesterDisplay_;
    GraphicsDisplayWindow *keyboardTesterWindow_;
    PreferencesWindow *preferencesWindow_;
#endif
    
    // Segment info
    int segmentCounter_;
    
    // Logging info
    bool loggingActive_, isPlayingLog_;
    std::string loggingDirectory_;
};


// Separate class for handling external OSC control messages since
// one class cannot have two receivers. This one is for all external
// OSC messages which OscHandler on MainApplicationController is for
// internally-generated messages via the PianoKeyboard class.

class MainApplicationOSCController : public OscHandler {
public:
    MainApplicationOSCController(MainApplicationController& controller,
                                 OscMessageSource& source) :
    controller_(controller), source_(source) {
        setOscController(&source_);
        addOscListener("/control*");
    }
    
    // *** OSC handler method (different from OSC device selection) ***
    
    bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
private:
    // Reply to OSC messages with a status
    void oscControlTransmitResult(int result);
    
    MainApplicationController& controller_;
    OscMessageSource& source_;
};
