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
 
  Main.cpp: main startup routines, connecting to Juce library
*/

#include "MainApplicationController.h"

#ifndef TOUCHKEYS_NO_GUI
#include "GUI/MainWindow.h"
#include "GUI/GraphicsDisplayWindow.h"
#include "GUI/PreferencesWindow.h"
#include "GUI/PreferencesComponent.h"
#include "Display/OpenGLJuceCanvas.h"

//==============================================================================
class TouchKeysApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    TouchKeysApplication() {}

    const juce::String getApplicationName()       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion()    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed()       { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) {
        // This method is where you should put your application's initialisation code..

        mainWindow_ = std::make_unique< MainWindow >(controller_);
        keyboardDisplayWindow_ = std::make_unique< GraphicsDisplayWindow >("TouchKeys Display", controller_.keyboardDisplay());
        preferencesWindow_ = std::make_unique< PreferencesWindow >(controller_);
        
        controller_.setKeyboardDisplayWindow(keyboardDisplayWindow_.get());
        controller_.setPreferencesWindow(preferencesWindow_.get());
        controller_.initialise();
    }

    void shutdown() {
        // Add your application's shutdown code here..
        if(controller_.touchkeyDeviceIsRunning())
            controller_.stopTouchkeyDevice();
        
        mainWindow_ = nullptr; // (deletes our window)
        
        controller_.setKeyboardDisplayWindow(0);    // Delete display window and disconnect from controller
        controller_.setPreferencesWindow(0);
        keyboardDisplayWindow_ = nullptr;
        preferencesWindow_ = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

private:
    std::unique_ptr<MainWindow> mainWindow_;
    std::unique_ptr<GraphicsDisplayWindow> keyboardDisplayWindow_;
    std::unique_ptr<PreferencesWindow> preferencesWindow_;
    MainApplicationController controller_;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (TouchKeysApplication)

#else // TOUCHKEYS_NO_GUI

#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

bool programShouldStop_ = false;

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"list", no_argument, NULL, 'l'},
	{"touchkeys", required_argument, NULL, 't'},
    {"midi-input", required_argument, NULL, 'i'},
    {"midi-output", required_argument, NULL, 'o'},
    {"virtual-midi-output", no_argument, NULL, 'V'},
    {"osc-input-port", required_argument, NULL, 'P'},
	{0,0,0,0}
};

void sigint_handler(int s){
    programShouldStop_ = true;
}

void usage(const char * processName)	// Print usage information and exit
{
	cerr << "Usage: " << processName << " [-h] [-l] [-t touchkeys] [-i MIDI-in] [-o MIDI-out]\n";
	cerr << "  -h:   Print this menu\n";
	cerr << "  -l:   List available TouchKeys and MIDI devices\n";
	cerr << "  -t:   Specify TouchKeys device path and autostart\n";
    cerr << "  -i:   Specify MIDI input device\n";
    cerr << "  -o:   Specify MIDI output device\n";
    cerr << "  -V:   Open virtual MIDI output\n";
    cerr << "  -P:   Specify OSC input port (default: " << kDefaultOscReceivePort << ")\n";
}

void list_devices(MainApplicationController& controller)
{
    std::vector<std::string> touchkeysDevices(controller.availableTouchkeyDevices());
    std::vector<std::pair<int, std::string> > midiInputDevices(controller.availableMIDIInputDevices());
    std::vector<std::pair<int, std::string> > midiOutputDevices(controller.availableMIDIOutputDevices());
    
    cerr << "TouchKeys devices: \n";
    if(touchkeysDevices.empty())
        cerr << "  [none found]\n";
    else {
        for(auto it = touchkeysDevices.begin(); it != touchkeysDevices.end(); ++it) {
            cerr << "  /dev/" << *it << "\n";
        }
    }

    cerr << "\nMIDI input devices: \n";
    if(midiInputDevices.empty())
        cerr << "  [none found]\n";
    else {
        for(auto it = midiInputDevices.begin();
            it != midiInputDevices.end();
            ++it) {
            cerr << "  " << it->first << ": " << it->second << "\n";
        }
    }
    
    cerr << "\nMIDI output devices: \n";
    if(midiOutputDevices.empty())
        cerr << "  [none found]\n";
    else {
        for( auto it = midiOutputDevices.begin();
            it != midiOutputDevices.end();
            ++it) {
            cerr << "  " << it->first << ": " << it->second << "\n";
        }
    }
}

int main (int argc, char* argv[])
{
    MainApplicationController controller;
    
    int ch, option_index;
    int midiInputNum = 0, midiOutputNum = 0;
    bool useVirtualMidiOutput = false;
    bool shouldStart = true;
    bool autostartTouchkeys = false;
    bool autoopenMidiOut = false, autoopenMidiIn = false;
    int oscInputPort = kDefaultOscReceivePort;
    std::string touchkeysDevicePath;
    
	while((ch = getopt_long(argc, argv, "hli:o:t:VP:", long_options, &option_index)) != -1)
	{
        if(ch == 'l') { // List devices
            list_devices(controller);
            shouldStart = false;
            break;
        }
        else if(ch == 't') { // TouchKeys device
            touchkeysDevicePath = optarg;
            autostartTouchkeys = true;
        }
        else if(ch == 'i') { // MIDI input device
            midiInputNum = atoi(optarg);
            autoopenMidiIn = true;
        }
        else if(ch == 'o') { // MIDI output device
            midiOutputNum = atoi(optarg);
            autoopenMidiOut = true;
        }
        else if(ch == 'V') { // Virtual MIDI output
            useVirtualMidiOutput = true;
            autoopenMidiOut = true;
        }
        else if(ch == 'P') { // OSC port
            oscInputPort = atoi(optarg);
        }
        else {
            usage(basename(argv[0]));
            shouldStart = false;
            break;
		}
	}
    
    
    if(shouldStart) {
        // Main initialization: open TouchKeys and MIDI devices
        controller.initialise();
        
        // Always enable OSC input without GUI, since it is how we control
        // the system
        controller.oscReceiveSetPort(oscInputPort);
        controller.oscReceiveSetEnabled(true);
        
        try {
            // Open MIDI devices
            if(autoopenMidiIn) {
                std::cout << "Opening MIDI input device " << midiInputNum << '\n';
                controller.enableMIDIInputPort(midiInputNum, true);
            }

            // TODO: enable multiple keyboard segments
            if(autoopenMidiOut) {
                if(useVirtualMidiOutput) {
#ifndef JUCE_WINDOWS
                    std::cout << "Opening virtual MIDI output\n";
                    controller.enableMIDIOutputVirtualPort(0, "TouchKeys");
#endif
                }
                else {
                    std::cout << "Opening MIDI output device " << midiOutputNum << '\n';
                    controller.enableMIDIOutputPort(0, midiOutputNum);
                }
            }
            
            // Start the TouchKeys
            if(autostartTouchkeys) {
                std::cout << "Starting the TouchKeys on " << touchkeysDevicePath << " ... ";
                if(!controller.touchkeyDeviceStartupSequence(touchkeysDevicePath.c_str())) {
                    std::cout << "failed: " << controller.touchkeyDeviceErrorMessage() << '\n';
                    throw new exception;
                }
                else
                    std::cout << "succeeded!\n";
            }
            
            // Set up interrupt catching so we can stop with Ctrl-C
            struct sigaction sigIntHandler;
            
            sigIntHandler.sa_handler = sigint_handler;
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = 0;
            sigaction(SIGINT, &sigIntHandler, NULL);

            // Wait until interrupt signal is received
            while(!programShouldStop_) {
                juce::Thread::sleep(50);
            }
        }
        catch(...) {
            
        }
        
        // Stop TouchKeys if still running
        if(controller.touchkeyDeviceIsRunning())
            controller.stopTouchkeyDevice();
    }
    
    // Clean up any MessageManager instance that JUCE creates
    DeletedAtShutdown::deleteAll();
    MessageManager::deleteInstance();
    return 0;
}

#endif // TOUCHKEYS_NO_GUI
