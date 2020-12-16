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
 
  Osc.h: classes for handling reception and transmission of OSC messages,
  using the liblo library.
*/

#pragma once

//#include <cstdint>
#include "lo/lo.h"
#include <JuceHeader.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>


class OscMessageSource;

// This is an abstract base class implementing a single function oscHandlerMethod().  Objects that
// want to register to receive OSC messages should inherit from OscHandler.  Notice that all listener
// add/remove methods are private or protected.  The subclass of OscHandler should add any relevant 
// listeners, or optionally expose a public interface to add listeners.  (Never call the methods in
// OscMessageSource externally.)

class OscHandler
{
public:
	OscHandler() : oscController_(NULL) {}
	
	// The OSC controller will call this method when it gets a matching message that's been registered
	virtual bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) = 0;
	void setOscController(OscMessageSource *c) { oscController_ = c; }
	
	virtual ~OscHandler();	// In the destructor, remove all OSC listeners
protected:
	bool addOscListener(const std::string& path);
	bool removeOscListener(const std::string& path);
	bool removeAllOscListeners();
	
	OscMessageSource *oscController_;
    std::set<std::string> oscListenerPaths_;
};

// Base class for anything that acts as a source of OSC messages.  Could be
// received externally or internally generated.

class OscMessageSource
{
	friend class OscHandler;
	
public:
	OscMessageSource() {}
	
protected:
	bool addListener(const std::string& path, OscHandler *object,
                     bool matchSubpath = false);                    // Add a listener object for a specific path
	bool removeListener(const std::string& path, OscHandler *object);	// Remove a listener object	from a specific path
	bool removeListener(OscHandler *object);						// Remove a listener object from all paths
	
    void updateListeners();                                         // Propagate changes to the listeners to the main object
    
	//ReadWriteLock oscListenerMutex_;                // This mutex protects the OSC listener table from being modified mid-message
    juce::CriticalSection oscListenerMutex_;                // This mutex protects the OSC listener table from being modified mid-message
    juce::CriticalSection oscUpdaterMutex_;                 // This mutex controls the insertion of objects in add/removeListener
    
    std::multimap<std::string, OscHandler*> noteListeners_;	// Map from OSC path name to handler (possibly multiple handlers per object)
    std::multimap<std::string, OscHandler*> noteListenersToAdd_;    // Collection of listeners to add on the next cycle
    std::multimap<std::string, OscHandler*> noteListenersToRemove_; // Collection of listeners to remove on the next cycle
    std::set<OscHandler*> noteListenersForBlanketRemoval_;     // Collection of listeners to remove from all paths
};

// This class specifically implements OSC messages coming from external sources

class OscReceiver : public OscMessageSource
{
public:
	OscReceiver(const int port, const char *prefix) {
        globalPrefix_.assign(prefix);
		useThru_ = false;
        
        // Only start the server if the port is positive
        if(port > 0) {
            char portStr[16];
#ifdef _MSC_VER
			_snprintf_s(portStr, 16, _TRUNCATE, "%d", port);
#else
            snprintf(portStr, 16, "%d", port);
#endif

            oscServerThread_ = lo_server_thread_new(portStr, staticErrorHandler);
            if(oscServerThread_ != 0) {
                lo_server_thread_add_method(oscServerThread_, NULL, NULL, OscReceiver::staticHandler, (void *)this);
                lo_server_thread_start(oscServerThread_);
            }
        }
        else
            oscServerThread_ = 0;
	}
	
	void setThruAddress(lo_address thruAddr, const char *prefix) {
		thruAddress_ = thruAddr;
		thruPrefix_.assign(prefix);
		useThru_ = true;
	}
    
    // Check whether the server is operating
    bool running() { return (oscServerThread_ != 0); }
    
    // Get or set the current port. Setting the port requires restarting the server.
    // setPort() returns true on success; false if an error occurred (which will leave the server not running).
    const int port() {
        if(oscServerThread_ == 0)
            return 0;
        return lo_server_get_port(oscServerThread_);
    }
    bool setPort(const int port);
	
	// staticHandler() is called by liblo with new OSC messages.  Its only function is to pass control
	// to the object-specific handler method, which has access to all internal variables.
	
	int handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data);
	static int staticHandler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *userData) {
		return ((OscReceiver *)userData)->handler(path, types, argv, argc, msg, userData);
	}
    
    // staticErrorHandler() is called by liblo when an error occurs. For now, ignore errors.
    
    static void staticErrorHandler(int num, const char *msg, const char *path) {}
	
	~OscReceiver() {
        if(oscServerThread_ != 0) {
            lo_server_thread_del_method(oscServerThread_, NULL, NULL);
            lo_server_thread_stop(oscServerThread_);
            lo_server_thread_free(oscServerThread_);
        }
	}
	
private:
	lo_server_thread oscServerThread_;		// Thread that handles received OSC messages
	
	// OSC thru
	bool useThru_;							// Whether or not we retransmit any messages
	lo_address thruAddress_;				// Address to which we retransmit
    std::string thruPrefix_;						// Prefix that must be matched to be retransmitted
	
	// State variables
    std::string globalPrefix_;					// Prefix for all OSC paths	
};

// Simple class to hold a message alongw ith a path and a type
class OscMessage
{
public:
    OscMessage(const char *path, const char *type, lo_message& message)
    : path_(path), type_(type), message_(message) {}
    
    ~OscMessage() {
        lo_message_free(message_);
    }
    
    const char *path() { return path_.c_str(); }
    const char *type() { return type_.c_str(); }
    lo_message message() { return message_; }
    
    // Add a prefix to the message path
    void prependPath(const char *prefix) {
        path_.insert(0, prefix); // TODO: check that this is right
    }
    
private:
    std::string path_;
    std::string type_;
    lo_message message_;
};


class OscTransmitter
{
public:
	OscTransmitter() : enabled_(true), debugMessages_(false) {}
    
    // Enable or disable transmission
    void setEnabled(bool enable) { enabled_ = enable; }
    bool enabled() { return enabled_; }
	
	// Add and remove addresses to send to
	int addAddress(const char * host, const char * port, int proto = LO_UDP);
	void removeAddress(int index);
	void clearAddresses();
    std::vector<lo_address> addresses() { return addresses_; }
	
	void sendMessage(const char * path, const char * type, ...);
	void sendMessage(const char * path, const char * type, const lo_message& message);
	void sendByteArray(const char * path, const unsigned char * data, int length);
	
	void setDebugMessages(bool debug) { debugMessages_ = debug; }
	
	~OscTransmitter();
    
    // Static methods
    static OscMessage* createMessage(const char * path, const char * type, ...);
    static OscMessage* createSuccessMessage() { return createMessage("/result", "i", 0, LO_ARGS_END); }
    static OscMessage* createFailureMessage() { return createMessage("/result", "i", 1, LO_ARGS_END); }
	
private:
    std::vector<lo_address> addresses_;
    bool enabled_;
	bool debugMessages_;
};
