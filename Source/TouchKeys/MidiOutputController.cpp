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
 
  MidiOutputController.cpp: handles outgoing MIDI messages and manages output
  ports; uses Juce MIDI library functions.
*/

#include "MidiOutputController.h"

#undef DEBUG_MIDI_OUTPUT_CONTROLLER
#undef MIDI_OUTPUT_CONTROLLER_DEBUG_RAW

// Constructor
MidiOutputController::MidiOutputController()
{
}

// Iterate through the available MIDI devices.  Return a vector containing
// indices and names for each device.  The index will later be passed back
// to indicate which device to open.

std::vector<std::pair<int, std::string> > MidiOutputController::availableMidiDevices() {
    std::vector<std::pair<int, std::string> > deviceList;
    
	try {
        juce::StringArray deviceStrings = juce::MidiOutput::getDevices();
		
		for(int i = 0; i < deviceStrings.size(); i++) {
            std::pair<int, std::string> p(i, std::string(deviceStrings[i].toUTF8()));
			deviceList.push_back(p);
		}
	}
	catch(...) {
		deviceList.clear();
	}
	
	return deviceList;
}

// Open a new MIDI output port, given an index related to the list from
// availableMidiDevices().  Returns true on success.

bool MidiOutputController::enablePort(int identifier, int deviceNumber) {
	if(deviceNumber < 0)
		return false;
    
    // Check if there is a port for this identifier, and disable it if so
    if(activePorts_.count(identifier) > 0)
        disablePort(identifier);
	
    auto device = juce::MidiOutput::openDevice(deviceNumber);
    
    if( device == nullptr ) {
        std::cout << "Failed to enable MIDI output port " << deviceNumber << ")\n";
        return false;
    }
    
#ifdef DEBUG_MIDI_OUTPUT_CONTROLLER
    std::cout << "Enabling MIDI output port " << deviceNumber << " for ID " << identifier << "\n";
#endif
    
    // Save the device in the set of ports
    //MidiOutputControllerRecord record;
    //record.portNumber = deviceNumber;
    //record.output = std::move( device );
    //
    //activePorts_[identifier] = record;
    activePorts_[identifier] = MidiOutputControllerRecord{ deviceNumber, std::move( device ) };
    
	return true;
}

#ifndef JUCE_WINDOWS
bool MidiOutputController::enableVirtualPort(int identifier, const char *name) {
    // Check if there is a port for this identifier, and disable it if so
    if(activePorts_.count(identifier) > 0)
        disablePort(identifier);
    
    // Try to create a new port
    MidiOutput* device = MidiOutput::createNewDevice(name);
    if(device == nullptr) {
        std::cout << "Failed to enable MIDI virtual output port " << name << ")\n";
        return false;
    }
    
    MidiOutputControllerRecord record;
    record.portNumber = kMidiVirtualOutputPortNumber;
    record.output = device;
    
    activePorts_[identifier] = record;
    
#ifdef DEBUG_MIDI_OUTPUT_CONTROLLER
    std::cout << "Enabling virtual output port " << name << '\n';
#endif
    
	return true;
}
#endif

void MidiOutputController::disablePort(int identifier) {
	if(activePorts_.count(identifier) <= 0)
		return;
	
	//MidiOutput *device = activePorts_[identifier].output;
	auto device = std::move( activePorts_[identifier].output );
    
    if( device == nullptr )
        return;
    
#ifdef DEBUG_MIDI_OUTPUT_CONTROLLER
	std::cout << "Disabling MIDI output " << activePorts_[identifier].portNumber << " for ID " << identifier << "\n";
#endif
    //delete device;
    device.reset();
    
	activePorts_.erase(identifier);
}

void MidiOutputController::disableAllPorts() {
    auto it = activePorts_.begin();
	
#ifdef DEBUG_MIDI_OUTPUT_CONTROLLER
	std::cout << "Disabling all MIDI output ports\n";
#endif
	
	while(it != activePorts_.end()) {
        if( it->second.output == nullptr )
            continue;
		//delete it->second.output;						// free MidiInputCallback
        it->second.output.reset();
		it++;
	}
	
	activePorts_.clear();
}

int MidiOutputController::enabledPort(int identifier) {
	if(activePorts_.count(identifier) <= 0)
		return -1;
    return activePorts_[identifier].portNumber;
}

std::vector<std::pair<int, int> > MidiOutputController::enabledPorts() {
    std::vector<std::pair<int, int> > ports;
    //std::map<int, MidiOutputControllerRecord>::iterator it;

    for( auto it = activePorts_.begin(); it != activePorts_.end(); ++it) {
        ports.push_back(std::pair<int,int>(it->first, it->second.portNumber));
    }
    
    return ports;
}

// Get the name of a particular MIDI input port
juce::String MidiOutputController::deviceName(int portNumber) {
    juce::StringArray const& deviceStrings = juce::MidiOutput::getDevices();
    if(portNumber < 0 || portNumber >= deviceStrings.size())
        return "";
    return deviceStrings[portNumber];
}

// Find the index of a device with a given name; return -1 if not found
int MidiOutputController::indexOfDeviceNamed(juce::String const& name) {
    juce::StringArray const& deviceStrings = juce::MidiOutput::getDevices();
    
    for(int i = 0; i < deviceStrings.size(); i++) {
        if(name == deviceStrings[i])
            return i;
    }
    
    return -1;
}

// Send a MIDI Note On message
void MidiOutputController::sendNoteOn(int port, unsigned char channel, unsigned char note, unsigned char velocity) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessageNoteOn),
                            (int)(note & 0x7F),
                            (int)(velocity & 0x7F)));
}

// Send a MIDI Note Off message
void MidiOutputController::sendNoteOff(int port, unsigned char channel, unsigned char note, unsigned char velocity) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessageNoteOff),
                            (int)(note & 0x7F),
                            (int)(velocity & 0x7F)));
}

// Send a MIDI Control Change message
void MidiOutputController::sendControlChange(int port, unsigned char channel, unsigned char control, unsigned char value) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessageControlChange),
                            (int)(control & 0x7F),
                            (int)(value & 0x7F)));
}

// Send a MIDI Program Change message
void MidiOutputController::sendProgramChange(int port, unsigned char channel, unsigned char value) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessageProgramChange),
                            (int)(value & 0x7F)));
}

// Send a Channel Aftertouch message
void MidiOutputController::sendAftertouchChannel(int port, unsigned char channel, unsigned char value) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessageAftertouchChannel),
                            (int)(value & 0x7F)));
}

// Send a Polyphonic Aftertouch message
void MidiOutputController::sendAftertouchPoly(int port, unsigned char channel, unsigned char note, unsigned char value) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessageAftertouchPoly),
                            (int)(note & 0x7F),
                            (int)(value & 0x7F)));
}

// Send a Pitch Wheel message
void MidiOutputController::sendPitchWheel(int port, unsigned char channel, unsigned int value) {
	sendMessage(port,
        juce::MidiMessage((int)((channel & 0x0F) | kMidiMessagePitchWheel),
                            (int)(value & 0x7F),
                            (int)((value >> 7) & 0x7F)));
}

// Send a MIDI system reset message
void MidiOutputController::sendReset(int port) {
	sendMessage(port, juce::MidiMessage(kMidiMessageReset));
}

// Send a generic MIDI message (pre-formatted data)
void MidiOutputController::sendMessage(int port, const juce::MidiMessage& message) {
#ifdef MIDI_OUTPUT_CONTROLLER_DEBUG_RAW
    int dataSize = message.getRawDataSize();
    const unsigned char *data = message.getRawData();
    
	std::cout << "MIDI Output " << port << ": ";
	for(int debugPrint = 0; debugPrint < dataSize; debugPrint++)
		printf("%x ", data[debugPrint]);
	std::cout << '\n';
#endif /* MIDI_OUTPUT_CONTROLLER_DEBUG_RAW */
    
	if(activePorts_.count(port) == 0) {
#ifdef MIDI_OUTPUT_CONTROLLER_DEBUG_RAW
        std::cout << "MIDI Output: no port on " << port << '\n';
#endif
		return;
    }
    
	activePorts_[port].output->sendMessageNow(message);
}