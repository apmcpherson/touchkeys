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
 
  MainApplicationController.cpp: contains the overall glue that holds
  together the various parts of the TouchKeys code. It works together
  with the user interface to let the user configure the hardware and 
  manage the mappings, but it is kept separate from any particular user 
  interface configuration.
*/

#include "MainApplicationController.h"
#ifndef TOUCHKEYS_NO_GUI
#include "Display/KeyboardTesterDisplay.h"
#endif

// Strings for pitch classes (two forms for sharps), for static methods
const char* kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char* kNoteNamesAlternate[12] = {"C", "Db", "D ", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

MainApplicationController::MainApplicationController()
: midiInputController_(keyboardController_),
  oscReceiver_(0, "/touchkeys"),
  touchkeyController_(keyboardController_),
  touchkeyEmulator_(keyboardController_, oscReceiver_),
  logPlayback_(0),
#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
  touchkeyEntropyGenerator_(keyboardController_),
  entropyGeneratorSelected_(false),
#endif
  touchkeyErrorOccurred_(false),
  touchkeyErrorMessage_(""),
  touchkeyAutodetecting_(false),
  touchkeyStandaloneModeEnabled_(false),
  deviceUpdateCounter_(0),
  oscReceiveEnabled_(false),
  oscReceivePort_(kDefaultOscReceivePort),
  experimentalMappingsEnabled_(false),
#ifndef TOUCHKEYS_NO_GUI
  keyboardDisplayWindow_(0),
  keyboardTesterDisplay_(0),
  keyboardTesterWindow_(0),
  preferencesWindow_(0),
#endif
  segmentCounter_(0),
  loggingActive_(false),
  isPlayingLog_(false)
{
    // Set our OSC controller
    setOscController(&keyboardController_);
    oscTransmitter_.setEnabled(false);
    //oscTransmitter_.setDebugMessages(true);
    
    // Initialize the links between objects
    keyboardController_.setOscTransmitter(&oscTransmitter_);
    keyboardController_.setMidiOutputController(&midiOutputController_);
    keyboardController_.setGUI(&keyboardDisplay_);
	midiInputController_.setMidiOutputController(&midiOutputController_);

    // Set up default logging directory
    loggingDirectory_ = ( juce::File::getSpecialLocation( juce::File::userHomeDirectory).getFullPathName() + "/Desktop").toUTF8();
    
    // Configure application properties
    juce::PropertiesFile::Options options;
    options.applicationName = "TouchKeys";
    options.folderName = "TouchKeys";
    options.filenameSuffix = ".properties";
    options.osxLibrarySubFolder = "Application Support";
    applicationProperties_.setStorageParameters(options);
    
    // Defaults for display, until we get other information
    keyboardDisplay_.setKeyboardRange(36, 72);
    
    // Add one keyboard segment at the beginning
    midiSegmentAdd();
    
    // Load the current preferences
    loadApplicationPreferences();
    
    // Set up an initial OSC transmit host/port if none has been loaded
    if(oscTransmitter_.addresses().size() == 0)
        oscTransmitter_.addAddress(kDefaultOscTransmitHost, kDefaultOscTransmitPort);
    
    // Listen for control messages by OSC
    mainOscController_ = new MainApplicationOSCController(*this, oscReceiver_);
}

MainApplicationController::~MainApplicationController() {
#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST
    if(touchkeySensorTestIsRunning())
        touchkeySensorTestStop();
#endif
    if(logPlayback_ != nullptr)
        delete logPlayback_;
    removeAllOscListeners();
    midiInputController_.removeAllSegments();   // Remove segments now to avoid deletion-order problems
    delete mainOscController_;
}

// Actions here run in the JUCE initialise() method once the application is loaded
void MainApplicationController::initialise() {
    // Load a preset if enabled
    if(getPrefsStartupPresetLastSaved()) {
        if(applicationProperties_.getUserSettings()->containsKey("LastSavedPreset")) {
            juce::String presetFile = applicationProperties_.getUserSettings()->getValue("LastSavedPreset");
            if(presetFile != "") {
                loadPresetFromFile(presetFile.toUTF8());
            }
        }
    }
    else if(getPrefsStartupPresetVibratoPitchBend()) {
        if(midiInputController_.numSegments() > 0) {
            MidiKeyboardSegment *segment = midiInputController_.segment(0);
            
            MappingFactory *factory = new TouchkeyVibratoMappingFactory(keyboardController_, *segment);
            if(factory != nullptr)
                segment->addMappingFactory(factory, true);
            factory = new TouchkeyPitchBendMappingFactory(keyboardController_, *segment);
            if(factory != nullptr)
                segment->addMappingFactory(factory, true);
        }
    }
    else if(!getPrefsStartupPresetNone()) {
        juce::String presetFile = getPrefsStartupPreset();
        if(presetFile != "") {
            loadPresetFromFile(presetFile.toUTF8());
        }
    }
    
    // Automatically start the TouchKeys if the preferences are enabled
    if(getPrefsAutoStartTouchKeys() && applicationProperties_.getUserSettings()->containsKey("TouchKeysDevice")) {
        juce::String tkDevicePath = applicationProperties_.getUserSettings()->getValue("TouchKeysDevice");
        if(touchkeyDeviceExists(tkDevicePath.toUTF8())) {
            // Exists: try to open and run
            touchkeyDeviceStartupSequence(tkDevicePath.toUTF8());
        }
    }
}

bool MainApplicationController::touchkeyDeviceStartupSequence(const char * path) {
#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
    if(!strcmp(path, "/dev/Entropy Generator") || !strcmp(path, "\\\\.\\Entropy Generator")) {
        entropyGeneratorSelected_ = true;
        touchkeyEntropyGenerator_.start();
    }
    else {
        entropyGeneratorSelected_ = false;
#endif
        
    // Step 1: attempt to open device
    if(!openTouchkeyDevice(path)) {
        touchkeyErrorMessage_ = "Failed to open";
        touchkeyErrorOccurred_ = true;
        return false;
    }
    
    // Step 2: see if a real TouchKeys device is present at the other end
    if(!touchkeyDeviceCheckForPresence()) {
        touchkeyErrorMessage_ = "Device not recognized";
        touchkeyErrorOccurred_ = true;
        return false;
    }
    
    // Step 3: update the display
    keyboardDisplay_.setKeyboardRange(touchkeyController_.lowestKeyPresentMidiNote(), touchkeyController_.highestMidiNote());
#ifndef TOUCHKEYS_NO_GUI
    if(keyboardDisplayWindow_ != nullptr) {
        keyboardDisplayWindow_->getConstrainer()->setFixedAspectRatio(keyboardDisplay_.keyboardAspectRatio());
        
        juce::Rectangle<int> bounds = keyboardDisplayWindow_->getBounds();
        if(bounds.getY() < 44)
            bounds.setY(44);
        keyboardDisplayWindow_->setBoundsConstrained(bounds);
    }
#endif
    
    // Step 4: suppress stray touches if enabled
    touchkeyController_.setSuppressStrayTouches(getPrefsSuppressStrayTouches());
        
    // Step 5: start data collection from the device
    if(!startTouchkeyDevice()) {
        touchkeyErrorMessage_ = "Failed to start";
        touchkeyErrorOccurred_ = true;
    }

#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
    }
#endif
    
    // Success!
    touchkeyErrorMessage_ = "";
    touchkeyErrorOccurred_ = false;
    
#ifndef TOUCHKEYS_NO_GUI
    showKeyboardDisplayWindow();
#endif
    
    // Automatically detect the lowest octave if set
    if(getPrefsAutodetectOctave())
        touchkeyDeviceAutodetectLowestMidiNote();
    
    return true;
}

std::string MainApplicationController::touchkeyDevicePrefix() {
#ifdef _MSC_VER
	return "\\\\.\\";
#else
    if(juce::SystemStats::getOperatingSystemType() == juce::SystemStats::Linux) {
        return "/dev/serial/by-id/";
    }
    else {
        return "/dev/";
    }
#endif
}

// Return a list of available TouchKey devices
std::vector<std::string> MainApplicationController::availableTouchkeyDevices() {
    std::vector<std::string> devices;

#ifdef _MSC_VER
    for(int i = 1; i <= 128; i++) {
        juce::String comPortName("COM");
		comPortName += i;

        DWORD dwSize = 0;
        LPCOMMCONFIG lpCC = (LPCOMMCONFIG) new BYTE[1];
        BOOL ret = GetDefaultCommConfig(comPortName.toUTF8(), lpCC, &dwSize);
        delete [] lpCC;
        
		if(ret) 
			devices.push_back(comPortName.toStdString());
		else {
			if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				//Logger::writeToLog(juce::String::formatted("Found " + comPortName));
				lpCC = (LPCOMMCONFIG) new BYTE[dwSize];
				ret = GetDefaultCommConfig(comPortName.toUTF8(), lpCC, &dwSize);
				if(ret)
					devices.push_back(comPortName.toStdString());
				else {
					int error = GetLastError();
					//Logger::writeToLog(juce::String("2Didn't find " + comPortName + "; error " + juce::String(error)));
				}
				delete [] lpCC;
			}
			else {
				int error = GetLastError();
				//Logger::writeToLog(juce::String("Didn't find " + comPortName + "; error " + juce::String(error)));
			}
		}
    }
#else
    if(juce::SystemStats::getOperatingSystemType() == juce::SystemStats::Linux) {
        juce::DirectoryIterator devDirectory(juce::File("/dev/serial/by-id"),false,"*");
        
        while(devDirectory.next()) {
            devices.push_back(std::string(devDirectory.getFile().getFileName().toUTF8()));
        }
    }
    else {
        juce::DirectoryIterator devDirectory(juce::File("/dev"),false,"cu.usbmodem*");
        
        while(devDirectory.next()) {
            devices.push_back(std::string(devDirectory.getFile().getFileName().toUTF8()));
        }
    }
#endif
    
#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
    devices.push_back("Entropy Generator");
#endif
    
    return devices;
}

void MainApplicationController::touchkeyDeviceClearErrorMessage() {
    touchkeyErrorMessage_ = "";
    touchkeyErrorOccurred_ = false;
}

// Check whether a given touchkey device exists
bool MainApplicationController::touchkeyDeviceExists(const char * path) {
    juce::String pathString(path);
    juce::File tkDeviceFile(pathString);
    return tkDeviceFile.existsAsFile();
}

// Select a particular touchkey device
bool MainApplicationController::openTouchkeyDevice(const char * path) {
    bool success = touchkeyController_.openDevice(path);
    
    if(success)
        applicationProperties_.getUserSettings()->setValue("TouchKeysDevice", juce::String(path));
    return success;
}

// Close the currently open TouchKeys device
void MainApplicationController::closeTouchkeyDevice() {
#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
    if(entropyGeneratorSelected_)
        touchkeyEntropyGenerator_.stop();
    else
        touchkeyController_.closeDevice();
#else
    touchkeyController_.closeDevice();
#endif
}

// Check whether a TouchKey device is present. Returns true if device found.
bool MainApplicationController::touchkeyDeviceCheckForPresence(int waitMilliseconds, int tries) {
    
    int count = 0;
    while(1) {
        if(touchkeyController_.checkIfDevicePresent(waitMilliseconds))
            break;
        if(++count >= tries) {
            return false;
        }
    }
    
    return true;
}

// Start/stop the TouchKeys data collection
bool MainApplicationController::startTouchkeyDevice() {
    return touchkeyController_.startAutoGathering();
}

void MainApplicationController::stopTouchkeyDevice() {
    touchkeyController_.stopAutoGathering();
}

// Status queries on TouchKeys
// Returns true if device has been opened
bool MainApplicationController::touchkeyDeviceIsOpen() {
    return touchkeyController_.isOpen();
}

// Return true if device is collecting data
bool MainApplicationController::touchkeyDeviceIsRunning() {
#ifdef TOUCHKEY_ENTROPY_GENERATOR_ENABLE
    if(entropyGeneratorSelected_)
        return touchkeyEntropyGenerator_.isRunning();
    else
        return touchkeyController_.isAutoGathering();
#else
    return touchkeyController_.isAutoGathering();
#endif
}

// Returns true if an error has occurred
bool MainApplicationController::touchkeyDeviceErrorOccurred() {
    return touchkeyErrorOccurred_;
}

// Return the error message if one occurred
std::string MainApplicationController::touchkeyDeviceErrorMessage() {
    return touchkeyErrorMessage_;
}

// How many octaves on the current device
int MainApplicationController::touchkeyDeviceNumberOfOctaves() {
    return touchkeyController_.numberOfOctaves();
}

// Return the lowest MIDI note
int MainApplicationController::touchkeyDeviceLowestMidiNote() {
    return touchkeyController_.lowestMidiNote();
}

// Set the lowest MIDI note for the TouchKeys
void MainApplicationController::touchkeyDeviceSetLowestMidiNote(int note) {
    keyboardDisplay_.clearAllTouches();
    touchkeyEmulator_.setLowestMidiNote(note);
    touchkeyController_.setLowestMidiNote(note);
    
    applicationProperties_.getUserSettings()->setValue("TouchKeysLowestMIDINote", note);
}

// Start an autodetection routine to match touch data to MIDI
void MainApplicationController::touchkeyDeviceAutodetectLowestMidiNote() {
    if(touchkeyAutodetecting_)
        return;
    
    touchkeyAutodetecting_ = true;
    addOscListener("/midi/noteon");
}

// Abort an autodetection routine
void MainApplicationController::touchkeyDeviceStopAutodetecting() {
    if(!touchkeyAutodetecting_)
        return;
    
    removeOscListener("/midi/noteon");
    touchkeyAutodetecting_ = false;
}

bool MainApplicationController::touchkeyDeviceIsAutodetecting() {
    return touchkeyAutodetecting_;
}

// Start logging TouchKeys/MIDI data to a file. Filename is autogenerated
// based on current time.
void MainApplicationController::startLogging() {
    if(loggingActive_)
        stopLogging();
    
    std::stringstream out;
    out << time(NULL);
    std::string fileId = out.str();
    

    std::string midiLogFileName = "midiLog_" + fileId + ".bin";
    std::string keyTouchLogFileName = "keyTouchLog_" + fileId + ".bin";
    std::string analogLogFileName = "keyAngleLog_" + fileId + ".bin";
    
    // Create log files with these names
    midiInputController_.createLogFile(midiLogFileName, loggingDirectory_);
    touchkeyController_.createLogFiles(keyTouchLogFileName, analogLogFileName, loggingDirectory_);
    
    // Enable logging from each controller
    midiInputController_.startLogging();
    touchkeyController_.startLogging();
    
    loggingActive_ = true;
}

// Stop a currently running log.
void MainApplicationController::stopLogging() {
    if(!loggingActive_)
        return;
    
    // stop logging data
    midiInputController_.stopLogging();
    touchkeyController_.stopLogging();
    
    // close the log files
    midiInputController_.closeLogFile();
    touchkeyController_.closeLogFile();
    
    loggingActive_ = false;
}

void MainApplicationController::setLoggingDirectory(const char *directory) {
    loggingDirectory_ = directory;
}

void MainApplicationController::playLogWithDialog() {
    if(isPlayingLog_)
        return;
    
    juce::FileChooser tkChooser ("Select TouchKeys log...",
                           juce::File{}, // File::getSpecialLocation (File::userHomeDirectory),
                           "*.bin");
    if(tkChooser.browseForFileToOpen()) {
        juce::FileChooser midiChooser ("Select MIDI log...",
                               juce::File{}, // File::getSpecialLocation (File::userHomeDirectory),
                               "*.bin");
        if(midiChooser.browseForFileToOpen()) {
            logPlayback_ = new LogPlayback(keyboardController_, midiInputController_);
            if(logPlayback_ == nullptr)
                return;
            
            if(logPlayback_->openLogFiles(tkChooser.getResult().getFullPathName().toRawUTF8(), midiChooser.getResult().getFullPathName().toRawUTF8())) {
                logPlayback_->startPlayback();
                isPlayingLog_ = true;
#ifndef TOUCHKEYS_NO_GUI
                // Always show 88 keys for log playback since we won't know which keys were actually recorded
                keyboardDisplay_.setKeyboardRange(21, 108);
                if(keyboardDisplayWindow_ != nullptr) {
                    keyboardDisplayWindow_->getConstrainer()->setFixedAspectRatio(keyboardDisplay_.keyboardAspectRatio());
                    
                    juce::Rectangle<int> bounds = keyboardDisplayWindow_->getBounds();
                    if(bounds.getY() < 44)
                        bounds.setY(44);
                    keyboardDisplayWindow_->setBoundsConstrained(bounds);
                }
                showKeyboardDisplayWindow();
#endif
            }
        }
    }
}

void MainApplicationController::stopPlayingLog() {
    if(!isPlayingLog_)
        return;
    
    if(logPlayback_ != nullptr) {
        logPlayback_->stopPlayback();
        logPlayback_->closeLogFiles();
        delete logPlayback_;
        logPlayback_ = 0;
    }
    
#ifndef TOUCHKEYS_NO_GUI
    keyboardDisplay_.clearAllTouches();
#endif
    midiInputController_.allNotesOff();
    isPlayingLog_ = false;
}

// Add a new MIDI keyboard segment. This method also handles numbering of the segments
MidiKeyboardSegment* MainApplicationController::midiSegmentAdd() {
    // For now, the segment counter increments with each new segment. Eventually, we could
    // consider renumbering every time a segment is removed so that we always have an index
    // 0-N which corresponds to the indexes within MidiInputController (and also the layout
    // of the tabs).
    MidiKeyboardSegment *newSegment = midiInputController_.addSegment(segmentCounter_, 12, 127);
    
    // Set up defaults
    newSegment->setModePassThrough();
    newSegment->setPolyphony(8);
    newSegment->setVoiceStealingEnabled(false);
    newSegment->enableAllChannels();
    newSegment->setOutputTransposition(0);
    newSegment->setUsesKeyboardPitchWheel(true);
    
    // Enable the MIDI output for this segment if it exists in the preferences
    loadMIDIOutputFromApplicationPreferences(segmentCounter_);
    
    // Enable standalone mode on the new segment if generally enabled
    if(touchkeyStandaloneModeEnabled_)
        newSegment->enableTouchkeyStandaloneMode();
    
    segmentCounter_++;
    
    return newSegment;
}

// Remove a MIDI keyboard segment.
void MainApplicationController::midiSegmentRemove(MidiKeyboardSegment *segment) {
    if(segment == nullptr)
        return;
    // Check if this segment uses a virtual output port. Right now, we have a unique
    // output per segment. If it does, then disable the virtual output port.
    int identifier = segment->outputPort();
    if(midiOutputController_.enabledPort(identifier) == MidiOutputController::kMidiVirtualOutputPortNumber)
        midiOutputController_.disablePort(identifier);
    midiInputController_.removeSegment(segment);
}

// Enable one MIDI input port either as primary or auxiliary
void MainApplicationController::enableMIDIInputPort(int portNumber, bool isPrimary) {
    midiInputController_.enablePort(portNumber, isPrimary);
    if(isPrimary)
        applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary",
                                                           midiInputController_.deviceName(portNumber));
    else
        applicationProperties_.getUserSettings()->setValue("MIDIInputAuxiliary",
                                                           midiInputController_.deviceName(portNumber));
}

// Enable all available MIDI input ports, with one in particular selected as primary
void MainApplicationController::enableAllMIDIInputPorts(int primaryPortNumber) {
    midiInputController_.enableAllPorts(primaryPortNumber);
    applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary",
                                                       midiInputController_.deviceName(primaryPortNumber));
    applicationProperties_.getUserSettings()->setValue("MIDIInputAuxiliary", "__all__");
}

// Disable a particular MIDI input port number
// For now, the preferences for auxiliary ports don't update; could add a complete list of enabled aux ports
void MainApplicationController::disableMIDIInputPort(int portNumber) {
    if(portNumber == selectedMIDIPrimaryInputPort())
        applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary", "");
    midiInputController_.disablePort(portNumber);
}

// Disable the current primary MIDI input port
void MainApplicationController::disablePrimaryMIDIInputPort() {
    applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary", "");
    midiInputController_.disablePrimaryPort();
}

// Disable either all MIDI input ports or all auxiliary inputs
void MainApplicationController::disableAllMIDIInputPorts(bool auxiliaryOnly) {
    applicationProperties_.getUserSettings()->setValue("MIDIInputAuxiliary", "");
    if(!auxiliaryOnly)
        applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary", "");
    midiInputController_.disableAllPorts(auxiliaryOnly);
}

// Enable a particular MIDI output port, associating it with a segment
void MainApplicationController::enableMIDIOutputPort(int identifier, int deviceNumber) {
    midiOutputController_.enablePort(identifier, deviceNumber);
    
    juce::String zoneName = "MIDIOutputZone";
    zoneName += identifier;
    applicationProperties_.getUserSettings()->setValue(zoneName, midiOutputController_.deviceName(deviceNumber));
}

#ifndef JUCE_WINDOWS
// Create a virtual (inter-application) MIDI output port
void MainApplicationController::enableMIDIOutputVirtualPort(int identifier, const char *name) {
    midiOutputController_.enableVirtualPort(identifier, name);
    
    juce::String zoneName = "MIDIOutputZone";
    zoneName += identifier;
    juce::String zoneValue = "__virtual__";
    zoneValue += juce::String(name);
    applicationProperties_.getUserSettings()->setValue(zoneName, zoneValue);
}
#endif

// Disable a particular MIDI output port
void MainApplicationController::disableMIDIOutputPort(int identifier) {
    juce::String zoneName = "MIDIOutputZone";
    zoneName += identifier;
    applicationProperties_.getUserSettings()->setValue(zoneName, "");
    
    midiOutputController_.disablePort(identifier);
}

// Disable all MIDI output ports
void MainApplicationController::disableAllMIDIOutputPorts() {
    std::vector<std::pair<int, int> > enabledPorts = midiOutputController_.enabledPorts();
    for(int i = 0; i < enabledPorts.size(); i++) {
        // For each active zone, set output port to disabled in preferences
        juce::String zoneName = "MIDIOutputZone";
        zoneName += enabledPorts[i].first;
        applicationProperties_.getUserSettings()->setValue(zoneName, "");
    }
    
    midiOutputController_.disableAllPorts();
}

// Enable TouchKeys standalone mode
void MainApplicationController::midiTouchkeysStandaloneModeEnable() {
    touchkeyStandaloneModeEnabled_ = true;
    // Go through all segments and enable standalone mode
    for(int i = 0; i < midiInputController_.numSegments(); i++) {
        midiInputController_.segment(i)->enableTouchkeyStandaloneMode();
    }
    
    applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary", "__standalone__");
}

void MainApplicationController::midiTouchkeysStandaloneModeDisable() {
    touchkeyStandaloneModeEnabled_ = false;
    // Go through all segments and disable standalone mode
    for(int i = 0; i < midiInputController_.numSegments(); i++) {
        midiInputController_.segment(i)->disableTouchkeyStandaloneMode();
    }
    
    if(applicationProperties_.getUserSettings()->getValue("MIDIInputPrimary") == "__standalone__")
        applicationProperties_.getUserSettings()->setValue("MIDIInputPrimary", "");
}

// *** OSC device methods ***

// Return whether OSC transmission is enabled
bool MainApplicationController::oscTransmitEnabled() {
    return oscTransmitter_.enabled();
}

// Set whether OSC transmission is enabled
void MainApplicationController::oscTransmitSetEnabled(bool enable) {
    oscTransmitter_.setEnabled(enable);
    applicationProperties_.getUserSettings()->setValue("OSCTransmitEnabled", enable);
}

// Return whether raw frame transmission is enabled
bool MainApplicationController::oscTransmitRawDataEnabled() {
    return touchkeyController_.transmitRawDataEnabled();
}

// Set whether raw frame transmission is enabled
void MainApplicationController::oscTransmitSetRawDataEnabled(bool enable) {
    touchkeyController_.setTransmitRawData(enable);
    applicationProperties_.getUserSettings()->setValue("OSCTransmitRawDataEnabled", enable);
}

// Return the addresses to which OSC messages are sent
std::vector<lo_address> MainApplicationController::oscTransmitAddresses() {
    return oscTransmitter_.addresses();
}

// Add a new address for sending OSC messages to
int MainApplicationController::oscTransmitAddAddress(const char * host, const char * port, int proto) {
    int indexOfNewAddress = oscTransmitter_.addAddress(host, port, proto);
    
    if(indexOfNewAddress >= 0) {
        // Successfully added; update preferences
        juce::String keyName = "OSCTransmitHost";
        keyName += indexOfNewAddress;
        applicationProperties_.getUserSettings()->setValue(keyName, juce::String(host));
        
        keyName = "OSCTransmitPort";
        keyName += indexOfNewAddress;
        applicationProperties_.getUserSettings()->setValue(keyName, juce::String(port));

        keyName = "OSCTransmitProtocol";
        keyName += indexOfNewAddress;
        applicationProperties_.getUserSettings()->setValue(keyName, proto);
    }
    
    return indexOfNewAddress;
}

// Remove a particular OSC address from the send list
void MainApplicationController::oscTransmitRemoveAddress(int index) {
    oscTransmitter_.removeAddress(index);
    
    // Remove this destination from the preferences, if it exists
    juce::String keyName = "OSCTransmitHost";
    keyName += index;
    
    if(applicationProperties_.getUserSettings()->containsKey(keyName)) {
        applicationProperties_.getUserSettings()->setValue(keyName, "");
        
        keyName = "OSCTransmitPort";
        keyName += index;
        applicationProperties_.getUserSettings()->setValue(keyName, "");
        
        keyName = "OSCTransmitProtocol";
        keyName += index;
        applicationProperties_.getUserSettings()->setValue(keyName, (int)0);
    }
}

// Remove all OSC addresses from the send list
void MainApplicationController::oscTransmitClearAddresses() {
    oscTransmitter_.clearAddresses();
    
    for(int index = 0; index < 16; index++) {
        // Go through and clear preferences for recent OSC hosts;
        // 16 hosts is a sanity check
        
        juce::String keyName = "OSCTransmitHost";
        keyName += index;
        
        if(applicationProperties_.getUserSettings()->containsKey(keyName)) {
            applicationProperties_.getUserSettings()->setValue(keyName, "");
            
            keyName = "OSCTransmitPort";
            keyName += index;
            applicationProperties_.getUserSettings()->setValue(keyName, "");
            
            keyName = "OSCTransmitProtocol";
            keyName += index;
            applicationProperties_.getUserSettings()->setValue(keyName, (int)0);
        }
    }

}

// OSC Input (receiver) methods
// Enable or disable on the OSC receive, and report is status
bool MainApplicationController::oscReceiveEnabled() {
    return oscReceiveEnabled_;
}

// Enable method returns true on success (false only if it was
// unable to set the port)
bool MainApplicationController::oscReceiveSetEnabled(bool enable) {
    applicationProperties_.getUserSettings()->setValue("OSCReceiveEnabled", enable);
    
    if(enable && !oscReceiveEnabled_) {
        oscReceiveEnabled_ = true;
        return oscReceiver_.setPort(oscReceivePort_);
    }
    else if(!enable && oscReceiveEnabled_) {
        oscReceiveEnabled_ = false;
        return oscReceiver_.setPort(0);
    }
    return true;
}

// Whether the OSC server is running (false means couldn't open port)
bool MainApplicationController::oscReceiveRunning() {
    return oscReceiver_.running();
}

// Get the current OSC receive port
int MainApplicationController::oscReceivePort() {
    return oscReceivePort_;
}

// Set the current OSC receive port (returns true on success)
bool MainApplicationController::oscReceiveSetPort(int port) {
    applicationProperties_.getUserSettings()->setValue("OSCReceivePort", port);
    
    oscReceivePort_ = port;
    return oscReceiver_.setPort(port);
}

// OSC handler method
bool MainApplicationController::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
	if(!strcmp(path, "/midi/noteon")) {
        if(touchkeyAutodetecting_ && numValues > 0) {
            // Found a MIDI note. Look for a unique touch on this pitch class to
            // determine which octave the keyboard is set to
            if(types[0] != 'i')
                return false;   // Ill-formed message
            int midiNote = values[0]->i;
            if(midiNote < 0 || midiNote > 127)
                return false;
            
            // Go through each octave and see if a touch is present
            int midiTestNote = midiNote % 12;
            int count = 0;
            int lastFoundTouchNote = 0;
            while(midiTestNote <= 127) {
                if(keyboardController_.key(midiTestNote) != 0) {
                    if(keyboardController_.key(midiTestNote)->touchIsActive()) {
                        count++;
                        lastFoundTouchNote = midiTestNote;
                    }
                }
                midiTestNote += 12;
            }
            
            // We return success if exactly one note had a touch on this pitch class
            if(count == 1) {
                int noteDifference = lastFoundTouchNote - midiNote;
                int currentMinNote = touchkeyController_.lowestMidiNote();

                // std::cout << "Found difference of " << noteDifference << std::endl;

                currentMinNote -= noteDifference;
                if(currentMinNote >= 0 && currentMinNote <= 127) {
                    touchkeyController_.setLowestMidiNote(currentMinNote);
                    applicationProperties_.getUserSettings()->setValue("TouchKeysLowestMIDINote", currentMinNote);
                }
                
                touchkeyDeviceStopAutodetecting();
            }
            return false; // Others may still want to handle this message
        }
    }
    
    return false;
}

// Save the current settings to an XML file
// Returns true on success
bool MainApplicationController::savePresetToFile(const char *filename) {
    juce::File outputFile(filename);
    
    return savePresetHelper(outputFile);
}

// Load settings from a saved XML file
// Returns true on success
bool MainApplicationController::loadPresetFromFile(const char *filename) {
    juce::File inputFile(filename);

    return loadPresetHelper(inputFile);
}

#ifndef TOUCHKEYS_NO_GUI
// Present the user with a Save dialog and then save the preset
bool MainApplicationController::savePresetWithDialog() {
    juce::FileChooser myChooser ("Save preset...",
                           juce::File{}, // File::getSpecialLocation (File::userHomeDirectory),
                           "*.tkpreset");
    if(myChooser.browseForFileToSave(true)) {
        juce::File outputFile(myChooser.getResult());
        return savePresetHelper(outputFile);
    }
    // User clicked cancel...
    return true;
}


// Present the user with a Load dialog and then save the preset
bool MainApplicationController::loadPresetWithDialog() {
    juce::FileChooser myChooser ("Select a preset...",
                           juce::File{}, // File::getSpecialLocation (File::userHomeDirectory),
                           "*.tkpreset");
    if(myChooser.browseForFileToOpen()) {
        return loadPresetHelper(myChooser.getResult());
    }
    // User clicked cancel...
    return true;
}
#endif

bool MainApplicationController::loadPresetHelper( juce::File const& inputFile) {
    if(!inputFile.existsAsFile())
        return false;
    
    // Load the XML element from the file and check that it is valid
    juce::XmlDocument document(inputFile);
    auto mainElement { document.getDocumentElement() };
    
    if(mainElement == nullptr)
        return false;
    if(mainElement->getTagName() != "TouchKeysPreset")
        return false;
    juce::XmlElement *segmentsElement = mainElement->getChildByName("KeyboardSegments");
    if(segmentsElement == nullptr)
        return false;
        
    // Load the preset from this element
    bool result = midiInputController_.loadSegmentPreset(segmentsElement);
    
    // Enable any necessary MIDI outputs
    for(int i = 0; i < midiInputController_.numSegments(); i++)
        loadMIDIOutputFromApplicationPreferences(i);
    
    // Loading a preset won't set standalone mode; so re-enable it when finished
    // if needed
    if(touchkeyStandaloneModeEnabled_) {
        midiTouchkeysStandaloneModeEnable();
    }
    
    return result;
}

bool MainApplicationController::savePresetHelper( juce::File& outputFile) {
    juce::XmlElement mainElement("TouchKeysPreset");
    mainElement.setAttribute("format", "0.1");
    
    juce::XmlElement* segmentsElement = midiInputController_.getSegmentPreset();
    mainElement.addChildElement(segmentsElement);
    
    bool result = mainElement.writeTo(outputFile);
    
    if(result) {
        applicationProperties_.getUserSettings()->setValue("LastSavedPreset", outputFile.getFullPathName());
    }
    
    return result;
}

// Clear the current preset and restore default settings
void MainApplicationController::clearPreset() {
    midiInputController_.removeAllSegments();
    //midiOutputController_.disableAllPorts();
    segmentCounter_ = 0;
    
    // Re-add a new segment, starting at 0
    midiSegmentAdd();
}

// Whether to automatically start the TouchKeys on startup
bool MainApplicationController::getPrefsAutoStartTouchKeys() {
    if(!applicationProperties_.getUserSettings()->containsKey("StartupStartTouchKeys"))
        return false;
    return applicationProperties_.getUserSettings()->getBoolValue("StartupStartTouchKeys");
}

void MainApplicationController::setPrefsAutoStartTouchKeys(bool autoStart) {
    applicationProperties_.getUserSettings()->setValue("StartupStartTouchKeys", autoStart);
}

// Whether to automatically detect the TouchKeys octave when they start
bool MainApplicationController::getPrefsAutodetectOctave() {
    if(!applicationProperties_.getUserSettings()->containsKey("StartupAutodetectTouchKeysOctave"))
        return false;
    return applicationProperties_.getUserSettings()->getBoolValue("StartupAutodetectTouchKeysOctave");
}

void MainApplicationController::setPrefsAutodetectOctave(bool autoDetect) {
    applicationProperties_.getUserSettings()->setValue("StartupAutodetectTouchKeysOctave", autoDetect);
}

// Which preset (if any) to load at startup
void MainApplicationController::setPrefsStartupPresetNone() {
    applicationProperties_.getUserSettings()->setValue("StartupPreset", "__none__");
}
bool MainApplicationController::getPrefsStartupPresetNone() {
    // By default, no prefs means no preset
    if(!applicationProperties_.getUserSettings()->containsKey("StartupPreset"))
        return true;
    if(applicationProperties_.getUserSettings()->getValue("StartupPreset") == "__none__")
        return true;
    return false;
}

void MainApplicationController::setPrefsStartupPresetVibratoPitchBend() {
    applicationProperties_.getUserSettings()->setValue("StartupPreset", "__vib_pb__");
}
bool MainApplicationController::getPrefsStartupPresetVibratoPitchBend() {
    if(!applicationProperties_.getUserSettings()->containsKey("StartupPreset"))
        return false;
    if(applicationProperties_.getUserSettings()->getValue("StartupPreset") == "__vib_pb__")
        return true;
    return false;
}

void MainApplicationController::setPrefsStartupPresetLastSaved() {
    applicationProperties_.getUserSettings()->setValue("StartupPreset", "__last__");
}
bool MainApplicationController::getPrefsStartupPresetLastSaved() {
    if(!applicationProperties_.getUserSettings()->containsKey("StartupPreset"))
        return false;
    if(applicationProperties_.getUserSettings()->getValue("StartupPreset") == "__last__")
        return true;
    return false;
}

void MainApplicationController::setPrefsStartupPreset(juce::String const& path) {
    applicationProperties_.getUserSettings()->setValue("StartupPreset", path);
}
juce::String MainApplicationController::getPrefsStartupPreset() {
    if(!applicationProperties_.getUserSettings()->containsKey("StartupPreset"))
        return "";
    return applicationProperties_.getUserSettings()->getValue("StartupPreset");
}

// Whether to suppress stray touches from the TouchKeys device
int MainApplicationController::getPrefsSuppressStrayTouches() {
    if(!applicationProperties_.getUserSettings()->containsKey("TouchKeysSuppressStrayTouches"))
        return 0;
    return applicationProperties_.getUserSettings()->getIntValue("TouchKeysSuppressStrayTouches");
}
void MainApplicationController::setPrefsSuppressStrayTouches(int level) {
    applicationProperties_.getUserSettings()->setValue("TouchKeysSuppressStrayTouches", level);
    
    // Update the TouchKeys device right away in case it's already running
    touchkeyController_.setSuppressStrayTouches(level);
}

// Reset application preferences to defaults
void MainApplicationController::resetPreferences() {
    // TODO: reset settings now, not after restart
    applicationProperties_.getUserSettings()->clear();
    
    setPrefsStartupPresetVibratoPitchBend();
    setPrefsAutodetectOctave(true);
    setPrefsSuppressStrayTouches(0);
}

// Load the current devices from a global preferences file
void MainApplicationController::loadApplicationPreferences(){
    juce::PropertiesFile *props = applicationProperties_.getUserSettings();
    
    if(props == nullptr )
        return;
    
    // A few first-time defaults if the properties file is missing
    if(props->getAllProperties().size() == 0) {
        resetPreferences();
    }
    
    // Load TouchKeys settings
    if(props->containsKey("TouchKeysDevice")) {
        // TODO
    }
    if(props->containsKey("TouchKeysLowestMIDINote")) {
        int note = props->getIntValue("TouchKeysLowestMIDINote");
        if(note >= 0 && note <= 127)
            touchkeyDeviceSetLowestMidiNote(note);
    }
    
    // Load MIDI input settings
    if(props->containsKey("MIDIInputPrimary")) {
        juce::String deviceName = props->getValue("MIDIInputPrimary");
        if(deviceName == "__standalone__") {
            midiTouchkeysStandaloneModeEnable();
        }
        else {
            int index = midiInputController_.indexOfDeviceNamed(deviceName);
            // std::cout << "primary input id " << index << " name " << deviceName << '\n';
            if(index >= 0)
                enableMIDIInputPort(index, true);
        }
    }
    if(props->containsKey("MIDIInputAuxiliary")) {
        juce::String deviceName = props->getValue("MIDIInputAuxiliary");
        int index = midiInputController_.indexOfDeviceNamed(deviceName);
        // std::cout << "aux input id " << index << " name " << deviceName << '\n';
        if(index >= 0)
            enableMIDIInputPort(index, false);
    }
    
    // MIDI output settings are loaded when segments are created
    
    // OSC settings
    if(props->containsKey("OSCTransmitEnabled")) {
        bool enable = props->getBoolValue("OSCTransmitEnabled");
        oscTransmitSetEnabled(enable);
    }
    if(props->containsKey("OSCTransmitRawDataEnabled")) {
        bool enable = props->getBoolValue("OSCTransmitRawDataEnabled");
        oscTransmitSetRawDataEnabled(enable);
    }
    
    for(int i = 0; i < 16; i++) {
        juce::String keyName = "OSCTransmitHost";
        juce::String host, port;
        int protocol = LO_UDP;
        
        keyName += i;
        if(props->containsKey(keyName)) {
            host = props->getValue(keyName);
        }
        else
            continue;
           
        keyName = "OSCTransmitPort";
        keyName += i;
        if(props->containsKey(keyName)) {
            port = props->getValue(keyName);
        }
        else
            continue;
        
        keyName = "OSCTransmitProtocol";
        keyName += i;
        if(props->containsKey(keyName)) {
            protocol = props->getIntValue(keyName);
        }
        // okay to go ahead without protocol; use default
        
        // Check for validity
        if(host != "" && port != "" && (protocol == LO_UDP || protocol == LO_TCP)) {
            oscTransmitter_.addAddress(host.toUTF8(), port.toUTF8(), protocol);
        }
    }
    
    if(props->containsKey("OSCReceiveEnabled")) {
        bool enable = props->getBoolValue("OSCReceiveEnabled");
        oscReceiveSetEnabled(enable);
    }
    if(props->containsKey("OSCReceivePort")) {
        int port = props->getIntValue("OSCReceivePort");
        if(port >= 1 && port <= 65535)
            oscReceiveSetPort(port);
    }
    
}

// Load the MIDI output device for a given zone
void MainApplicationController::loadMIDIOutputFromApplicationPreferences(int zone) {
    juce::PropertiesFile *props = applicationProperties_.getUserSettings();
    
    juce::String keyName = "MIDIOutputZone";
    keyName += zone;
    
    if(props->containsKey(keyName)) {
        juce::String output = props->getValue(keyName);
        if(output.startsWith("__virtual__")) {
#ifndef JUCE_WINDOWS
            // Open virtual port with the name that follows
            juce::String virtualPortName = output.substring(11); // length of "__virtual__"
            midiOutputController_.enableVirtualPort(zone, virtualPortName.toUTF8());
#endif
        }
        else {
            juce::String deviceName = props->getValue(keyName);
            int index = midiOutputController_.indexOfDeviceNamed(deviceName);
            // std::cout << "zone " << zone << " id " << index << " name " << deviceName << '\n';
            if(index >= 0)
                enableMIDIOutputPort(zone, index);
        }
    }
}

#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST
// Start testing the TouchKeys sensors. Returns true on success.
bool MainApplicationController::touchkeySensorTestStart(const char *path, int firstKey) {
#ifndef TOUCHKEYS_NO_GUI
    if(path == 0 || firstKey < touchkeyController_.lowestMidiNote())
        return false;
    if(keyboardTesterDisplay_ != 0)
        return true;
    
    // First, close the existing device which stops the data autogathering
    closeTouchkeyDevice();
    
    // Now reopen the TouchKeys device
    if(!touchkeyController_.openDevice(path)) {
        touchkeyErrorMessage_ = "Failed to open";
        touchkeyErrorOccurred_ = true;
        return false;
    }
    
    // Next, see if a real TouchKeys device is present at the other end
    if(!touchkeyDeviceCheckForPresence()) {
        touchkeyErrorMessage_ = "Device not recognized";
        touchkeyErrorOccurred_ = true;
        return false;
    }
    
    // Now, create the KeyboardTesterDisplay object which will handle processing
    // raw data and displaying the results. Also a new window to hold it.
    keyboardTesterDisplay_ = new KeyboardTesterDisplay(*this, keyboardController_);
    keyboardTesterDisplay_->setKeyboardRange(touchkeyController_.lowestKeyPresentMidiNote(), touchkeyController_.highestMidiNote());
    keyboardTesterWindow_ = new GraphicsDisplayWindow("TouchKeys Sensor Test", *keyboardTesterDisplay_);
    
    // Start raw data gathering from the indicated key (converted to octave/key notation)
    int keyOffset = firstKey - touchkeyController_.lowestMidiNote();
    if(keyOffset < 0) // Shouldn't happen...
        keyOffset = 0;
    
    if(!touchkeyController_.startRawDataCollection(keyOffset / 12, keyOffset % 12, 3, 2)) {
        touchkeyErrorMessage_ = "Failed to start";
        touchkeyErrorOccurred_ = true;
        return false;
    }
    
    keyboardTesterWindow_->addToDesktop(keyboardTesterWindow_->getDesktopWindowStyleFlags()
                                         | ComponentPeer::windowHasCloseButton);
    keyboardTesterWindow_->setVisible(true);
    keyboardTesterWindow_->toFront(true);
    
    touchkeyErrorMessage_ = "";
    touchkeyErrorOccurred_ = false;
    return true;
#endif
}

// Stop testing the TouchKeys sensors
void MainApplicationController::touchkeySensorTestStop() {
#ifndef TOUCHKEYS_NO_GUI
    if(keyboardTesterDisplay_ == 0)
        return;
    
    // Stop raw data gathering first and close device
    touchkeyController_.stopAutoGathering();
    touchkeyController_.closeDevice();
    
    // Delete the testing objects
    delete keyboardTesterWindow_;
    delete keyboardTesterDisplay_;
    
    keyboardTesterWindow_ = 0;
    keyboardTesterDisplay_ = 0;
    
    touchkeyErrorMessage_ = "";
    touchkeyErrorOccurred_ = false;
#endif
}

// Is the sensor test running?
bool MainApplicationController::touchkeySensorTestIsRunning() {
#ifdef TOUCHKEYS_NO_GUI   
    return false;
#else
    return (keyboardTesterDisplay_ != 0);
#endif
}

// Set the current key that is begin tested
void MainApplicationController::touchkeySensorTestSetKey(int key) {
#ifndef TOUCHKEYS_NO_GUI
    if(keyboardTesterDisplay_ == 0 || key < touchkeyController_.lowestMidiNote())
        return;

    int keyOffset = key - touchkeyController_.lowestMidiNote();
    if(keyOffset < 0) // Shouldn't happen...
        keyOffset = 0;
    
    touchkeyController_.rawDataChangeKeyAndMode(keyOffset / 12, keyOffset % 12, 3, 2);
#endif
}

// Reset the testing state to all sensors off
void MainApplicationController::touchkeySensorTestResetState() {
#ifndef TOUCHKEYS_NO_GUI
    if(keyboardTesterDisplay_ == 0)
        return;
    for(int key = 0; key < 128; key++)
        keyboardTesterDisplay_->resetSensorState(key);
#endif
}

#endif // ENABLE_TOUCHKEYS_SENSOR_TEST

#ifdef ENABLE_TOUCHKEYS_FIRMWARE_UPDATE
// Put TouchKeys controller board into bootloader mode, for receiving firmware updates
// (supplied by a different utility)
bool MainApplicationController::touchkeyJumpToBootloader(const char *path) {
    // First, close the existing device which stops the data autogathering
    closeTouchkeyDevice();
    
    // Now reopen the TouchKeys device
    if(!touchkeyController_.openDevice(path)) {
        touchkeyErrorMessage_ = "Failed to open";
        touchkeyErrorOccurred_ = true;
        return false;
    }
    
    touchkeyController_.jumpToBootloader();
    
    // Set an "error" condition to display this message, and because
    // after jumping to bootloader mode, the device will not open properly
    // until it has been reset.
    touchkeyErrorMessage_ = "Firmware update mode";
    touchkeyErrorOccurred_ = true;
    return true;
}
#endif // ENABLE_TOUCHKEYS_FIRMWARE_UPDATE

// Return the name of a MIDI note given its number
std::string MainApplicationController::midiNoteName(int noteNumber) {
    if(noteNumber < 0 || noteNumber > 127)
        return "";
    char name[6];
#ifdef _MSC_VER
	_snprintf_s(name, 6, _TRUNCATE, "%s%d", kNoteNames[noteNumber % 12], (noteNumber / 12) - 1);
#else
    snprintf(name, 6, "%s%d", kNoteNames[noteNumber % 12], (noteNumber / 12) - 1);
#endif

    return name;
}

// Get the number of a MIDI note given its name
int MainApplicationController::midiNoteNumberForName(std::string const& name) {
    // Any valid note name will have at least two characters
    if(name.length() < 2)
        return -1;
    
    // Find the pitch class first, then the octave
    int pitchClass = -1;
    int startIndex = 1;
    if(!name.compare(0, 2, "C#") ||
       !name.compare(0, 2, "c#") ||
       !name.compare(0, 2, "Db") ||
       !name.compare(0, 2, "db")) {
        pitchClass = 1;
        startIndex = 2;
    }
    else if(!name.compare(0, 2, "D#") ||
            !name.compare(0, 2, "d#") ||
            !name.compare(0, 2, "Eb") ||
            !name.compare(0, 2, "eb")) {
        pitchClass = 3;
        startIndex = 2;
    }
    else if(!name.compare(0, 2, "F#") ||
            !name.compare(0, 2, "f#") ||
            !name.compare(0, 2, "Gb") ||
            !name.compare(0, 2, "gb")){
        pitchClass = 6;
        startIndex = 2;
    }
    else if(!name.compare(0, 2, "G#") ||
            !name.compare(0, 2, "g#") ||
            !name.compare(0, 2, "Ab") ||
            !name.compare(0, 2, "ab")){
        pitchClass = 8;
        startIndex = 2;
    }
    else if(!name.compare(0, 2, "A#") ||
            !name.compare(0, 2, "a#") ||
            !name.compare(0, 2, "Bb") ||
            !name.compare(0, 2, "bb")){
        pitchClass = 10;
        startIndex = 2;
    }
    else if(!name.compare(0, 1, "C") ||
            !name.compare(0, 1, "c"))
        pitchClass = 0;
    else if(!name.compare(0, 1, "D") ||
            !name.compare(0, 1, "d"))
        pitchClass = 2;
    else if(!name.compare(0, 1, "E") ||
            !name.compare(0, 1, "e"))
        pitchClass = 4;
    else if(!name.compare(0, 1, "F") ||
            !name.compare(0, 1, "f"))
        pitchClass = 5;
    else if(!name.compare(0, 1, "G") ||
            !name.compare(0, 1, "g"))
        pitchClass = 7;
    else if(!name.compare(0, 1, "A") ||
            !name.compare(0, 1, "a"))
        pitchClass = 9;
    else if(!name.compare(0, 1, "B") ||
            !name.compare(0, 1, "b"))
        pitchClass = 11;
    
    if(pitchClass < 0) // No valid note found
        return -1;
    
    int octave = atoi(name.substr(startIndex).c_str());
    int noteNumber = (octave + 1) * 12 + pitchClass;
    
    if(noteNumber < 0 || noteNumber > 127)
        return -1;
    return noteNumber;
}


// ***** External OSC Control *****

bool MainApplicationOSCController::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
    if(!strncmp(path, "/control", 8)) {
        // OSC messages that start with /touchkeys/control are used to control the operation of the
        // software and mappings
        
        // First check if the message belongs to one of the segments
        if(!strncmp(path, "/control/segment", 16) && strlen(path) > 16) {
            // Pick out which segment based on the following number: e.g. /control/segment0/...
            
            std::string subpath(&path[16]);
            int separatorLoc = subpath.find_first_of('/');
            if(separatorLoc == std::string::npos || separatorLoc == subpath.length() - 1) {
                // Malformed input (no slash or it's the last character): ignore
                return false;
            }
            std::stringstream segmentNumberSStream(subpath.substr(0, separatorLoc));
            
            int segmentNumber = 0;
            segmentNumberSStream >> segmentNumber;
            
            if(segmentNumber < 0)  // Unknown segment number
                return false;
            
            // Pass this message onto the corresponding segment in MidiInputController
            // If the segment doesn't exist, it will return false. All further handling is
            // done within MidiInputController with its corresponding mutex locked so
            // the segments can't change while this message is processed.
            
            subpath = subpath.substr(separatorLoc); // Start at the '/'
            
            if(subpath == "/set-midi-out") {
                // Special case for setting the MIDI output, which is done in the main controller
                // rather than in the segment itself

                if(numValues >= 1) {
                    if(types[0] == 'i') {
                        MidiKeyboardSegment* segment = controller_.midiInputController_.segment(segmentNumber);
                        if(segment == nullptr)
                            oscControlTransmitResult(1); // Failure response
                        else {
                            if(values[0]->i < 0) {
                                // Negative value means disable
                                controller_.disableMIDIOutputPort(segment->outputPort());
                            }
                            else {
                                controller_.enableMIDIOutputPort(segment->outputPort(), values[0]->i);
                            }
                            oscControlTransmitResult(0);
                        }
                    }
#ifndef JUCE_WINDOWS
                    else if(types[0] == 's') {
                        MidiKeyboardSegment* segment = controller_.midiInputController_.segment(segmentNumber);
                        if(segment == nullptr )
                            oscControlTransmitResult(1); // Failure response
                        else {
                            if(!strcmp(&values[0]->s, "virtual")) {
                                char st[20];
                                snprintf(st, 20, "TouchKeys %d", segment->outputPort());
                                controller_.enableMIDIOutputVirtualPort(segment->outputPort(), st);
                                oscControlTransmitResult(0);
                            }
                        }
                    }
#endif
                }
            }
            else {
                // All other segment messages are handled within MidiKeyboardSegment
                
                OscMessage* response = controller_.midiInputController_.oscControlMessageForSegment(segmentNumber, subpath.c_str(), types, numValues, values, data);
                if(response != nullptr) {
                    // Add the right prefix to the response. If it is a simple result status,
                    // then give it the generic prefix. Otherwise add the zone beforehand
                    if(!strcmp(response->path(), "/result"))
                        response->prependPath("/touchkeys/control");
                    else {
                        char prefix[28];
#ifdef _MSC_VER
                        _snprintf_s(prefix, 28, _TRUNCATE, "/touchkeys/control/segment%d", segmentNumber);
#else
                        snprintf(prefix, 28,  "/touchkeys/control/segment%d", segmentNumber);
#endif
                        response->prependPath(prefix);
                    }
                    
                    // Send the message and free it
                    controller_.oscTransmitter_.sendMessage(response->path(), response->type(), response->message());
                    delete response;
                }
            }
        }
        else if(!strcmp(path, "/control/preset-load")) {
            // Load preset from file
            // Argument 0 is the path to the file
            
            if(numValues > 0) {
                if(types[0] == 's') {
                    bool result = controller_.loadPresetFromFile(&values[0]->s);
                    
                    // Send back a message on success/failure
                    oscControlTransmitResult(result == true ? 0 : 1);
                    return true;
                }
                else if(types[0] == 'i') {
                    // TODO: take a second form of the message which has a numerical
                    //       input for selecting presets
                }
            }
            return false;
        }
        else if(!strcmp(path, "/control/preset-save")) {
            // Save preset to file
            
            if(numValues > 0) {
                if(types[0] == 's') {
                    bool result = controller_.savePresetToFile(&values[0]->s);
                    
                    // Send back a message on success/failure
                    oscControlTransmitResult(result == true ? 0 : 1);
                    return true;
                }
                else if(types[0] == 'i') {
                    // TODO: take a second form of the message which has a numerical
                    //       input for selecting presets
                }
            }
            return false;
        }
        else if(!strcmp(path, "/control/preset-clear")) {
            // Clear everything in preset
            controller_.clearPreset();
            oscControlTransmitResult(0);
            return true;
        }
        else if(!strcmp(path, "/control/tk-list-devices")) {
            // Return a list of TouchKeys devices
            
            OscMessage *response = OscTransmitter::createMessage("/touchkeys/control/tk-list-devices/result", "i",
                                                                controller_.availableTouchkeyDevices().size(), LO_ARGS_END);
            
            std::vector<std::string> devices = controller_.availableTouchkeyDevices();

            for( auto it = devices.begin(); it != devices.end(); ++it) {
                lo_message_add_string(response->message(), it->c_str());
            }
            
            controller_.oscTransmitter_.sendMessage(response->path(), response->type(), response->message());
            delete response;
            
            return true;
        }
        else if(!strcmp(path, "/control/tk-start")) {
            // Start the TouchKeys device with the given path
            if(numValues > 0) {
                if(types[0] == 's') {
                    char *device = &values[0]->s;
                    bool result = controller_.touchkeyDeviceStartupSequence(device);
                    
                    oscControlTransmitResult(result == true ? 0 : 1);
                    return true;
                }
            }
        }
        else if(!strcmp(path, "/control/tk-stop")) {
            // Stop TouchKeys
            if(controller_.touchkeyDeviceIsOpen()) {
                controller_.closeTouchkeyDevice();
                oscControlTransmitResult(0);
            }
            else {
                // Not running, can't close
                oscControlTransmitResult(1);
            }
            return true;
        }
        else if(!strcmp(path, "/control/tk-set-lowest-midi-note")) {
            // Set TouchKeys octave such that the lowest key is at the
            // given note. Only 'C' notes are valid.
            
            if(numValues > 0) {
                if(types[0] == 'i') {
                    int note = values[0]->i;
                    
                    if(note % 12 == 0 && note >= 12 && note < 127) {
                        controller_.touchkeyDeviceSetLowestMidiNote(note);
                        oscControlTransmitResult(0);
                    }
                    else {
                        // Invalid note
                        oscControlTransmitResult(1);
                    }
                    return true;
                }
            }
        }
        else if(!strcmp(path, "/control/tk-autodetect")) {
            // Autodetect lowest TouchKeys octave
            controller_.touchkeyDeviceAutodetectLowestMidiNote();
            oscControlTransmitResult(0);
            return true;
        }
        else if(!strcmp(path, "/control/tk-autodetect-stop")) {
            // Stop autodetecting TouchKeys octave
            controller_.touchkeyDeviceStopAutodetecting();
            oscControlTransmitResult(0);
            return true;
        }
        else if(!strcmp(path, "/control/list-midi-in")) {
            // List available MIDI input devices
            std::vector<std::pair<int, std::string> > midiInputs = controller_.availableMIDIInputDevices();
            
            OscMessage *response = OscTransmitter::createMessage("/list-midi-in/result", "i", midiInputs.size(), LO_ARGS_END);
            
            for( auto it = midiInputs.begin(); it != midiInputs.end(); ++it) {
                lo_message_add_int32(response->message(), it->first);
                lo_message_add_string(response->message(), it->second.c_str());
            }
            
            controller_.oscTransmitter_.sendMessage(response->path(), response->type(), response->message());
            delete response;
            
            return true;
        }
        else if(!strcmp(path, "/control/list-midi-out")) {
            // List available MIDI output devices
            std::vector<std::pair<int, std::string> > midiOutputs = controller_.availableMIDIOutputDevices();
            
            OscMessage *response = OscTransmitter::createMessage("/list-midi-out/result", "i", midiOutputs.size(), LO_ARGS_END);
            
            for( auto it = midiOutputs.begin(); it != midiOutputs.end(); ++it) {
                lo_message_add_int32(response->message(), it->first);
                lo_message_add_string(response->message(), it->second.c_str());
            }
            
            controller_.oscTransmitter_.sendMessage(response->path(), response->type(), response->message());
            delete response;
            
            return true;
        }
        else if(!strcmp(path, "/control/set-midi-in-keyboard")) {
            // Set MIDI input device for keyboard
            if(numValues > 0) {
                if(types[0] == 'i') {
                    if(controller_.midiTouchkeysStandaloneModeIsEnabled())
                        controller_.midiTouchkeysStandaloneModeDisable();
                    if(values[0]->i < 0) {
                        // Negative means disable
                        controller_.disablePrimaryMIDIInputPort();
                    }
                    else {
                        controller_.enableMIDIInputPort(values[0]->i, true);
                    }
                    
                    oscControlTransmitResult(0);
                    return true;
                }
                else if(types[0] == 's') {
                    if(!strncmp(&values[0]->s, "stand", 5)) {
                        // Enable TouchKeys standalone mode in place of MIDI input
                        controller_.disablePrimaryMIDIInputPort();
                        controller_.midiTouchkeysStandaloneModeEnable();
                        
                        oscControlTransmitResult(0);
                        return true;
                    }
                }
            }
        }
        else if(!strcmp(path, "/control/set-midi-in-aux")) {
            // Set MIDI auxiliary input device
            if(numValues > 0) {
                if(types[0] == 'i') {
                    controller_.disableAllMIDIInputPorts(true);
                    if(values[0]->i >= 0) {
                        // Negative values mean leave the port disabled
                        controller_.enableMIDIInputPort(values[0]->i, false);
                    }
                    
                    oscControlTransmitResult(0);
                    return true;
                }
            }
            
        }
        else if(!strcmp(path, "/control/add-segment")) {
            // Add a new keyboard segment
            if(controller_.midiSegmentsCount() >= 8) {
                // Max of 8 segments possible
                oscControlTransmitResult(1);
            }
            else {
                controller_.midiSegmentAdd();
                oscControlTransmitResult(0);
            }
            return true;
        }
        else if(!strcmp(path, "/control/delete-segment")) {
            // Remove a keyboard segment by number
            if(numValues > 0) {
                if(types[0] == 'i') {
                    int segmentNumber = values[0]->i;
                    
                    bool result = controller_.midiInputController_.removeSegment(segmentNumber);
                    oscControlTransmitResult(result == true ? 0 : 1);
                    return true;
                }
            }
        }
    }
    
    return false;
}

// Send back an OSC message to indicate the result of a control command
void MainApplicationOSCController::oscControlTransmitResult(int result) {
    controller_.oscTransmitter_.sendMessage("/touchkeys/control/result", "i", result, LO_ARGS_END);
}
