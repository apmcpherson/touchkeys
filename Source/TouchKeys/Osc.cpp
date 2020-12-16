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
 
  Osc.cpp: classes for handling reception and transmission of OSC messages,
  using the liblo library.
*/

#include "Osc.h"

#undef DEBUG_OSC

#pragma mark OscHandler

OscHandler::~OscHandler()
{
	if(oscController_ != NULL)	// Remove (individually) each listener
	{
		for( auto it = oscListenerPaths_.begin(); it != oscListenerPaths_.end(); ++it)
		{
#ifdef DEBUG_OSC
			std::cout << "Deleting path " << *it << '\n';
#endif
			
			std::string pathToRemove = *it;
			oscController_->removeListener(pathToRemove, this);
		}
	}
}

#pragma mark -- Private Methods

// Call this internal method to add a listener to the OSC controller.  Returns true on success.

bool OscHandler::addOscListener(const std::string& path)
{
	if(oscController_ == NULL)
		return false;
	if(oscListenerPaths_.count(path) > 0)
		return false;
	oscListenerPaths_.insert(path);
	oscController_->addListener(path, this);
	return true;
}

bool OscHandler::removeOscListener(const std::string& path)
{
	if(oscController_ == NULL)
		return false;
	if(oscListenerPaths_.count(path) == 0)
		return false;
	oscController_->removeListener(path, this);
	oscListenerPaths_.erase(path);
	return true;
}

bool OscHandler::removeAllOscListeners()
{
	if(oscController_ == NULL)
		return false;
	auto it = oscListenerPaths_.begin();
	
	while(it != oscListenerPaths_.end()) {
		removeOscListener(*it++);
	}
	
	return true;
}

#pragma mark OscMessageSource

// Adds a specific object listening for a specific OSC message.  The object will be
// added to the internal map from strings to objects.  All messages are preceded by
// a global prefix (typically "/mrp").  Returns true on success.

bool OscMessageSource::addListener(const std::string& path, OscHandler *object, bool matchSubpath)
{
	if(object == NULL)
		return false;
    
#ifdef OLD_OSC_MESSAGE_SOURCE
	double before = Time::getMillisecondCounterHiRes();
	oscListenerMutex_.enterWrite();
    std::cout << "addListener(): took " << Time::getMillisecondCounterHiRes() - before << "ms to acquire mutex\n";
	noteListeners_.insert(pair<string, OscHandler*>(path, object));
	oscListenerMutex_.exitWrite();
#else
    juce::ScopedLock sl(oscUpdaterMutex_);
    
    // Add this object to the insertion list
    noteListenersToAdd_.insert(std::pair<std::string, OscHandler*>(path, object));
#endif
    
#ifdef DEBUG_OSC
	std::cout << "Added OSC listener to path '" << path << "'\n";
#endif
	
	return true;
}

// Removes a specific object from listening to a specific OSC message.
// Returns true if at least one path was removed.

bool OscMessageSource::removeListener(const std::string& path, OscHandler *object)
{
	if(object == NULL)
		return false;
	
	bool removedAny = false;
	
#ifdef OLD_OSC_MESSAGE_SOURCE    
	oscListenerMutex_.enterWrite(); // Lock the mutex so no incoming messages happen in the middle
		
	// Every time we remove an element from the multimap, the iterator is potentially corrupted.  Realistically
	// there should never be more than one entry with the same object and same path (we check this on insertion).
	
	auto ret = noteListeners_.equal_range(path);
	
	auto it = ret.first;
	while(it != ret.second)
	{
		if(it->second == object)
		{
			noteListeners_.erase(it++);
			removedAny = true;
			break;
		}
		else
			++it;
	}
	
	oscListenerMutex_.exitWrite();
#else
    juce::ScopedLock sl(oscUpdaterMutex_);
    
    // Add this object to the removal list
    noteListenersToRemove_.insert(std::pair<std::string, OscHandler*>(path, object));
    
    // Also remove this object from anything on the add list, so it doesn't
    // get put back in by a previous add call.
	std::pair<std::multimap<std::string, OscHandler*>::iterator, std::multimap<std::string, OscHandler*>::iterator> ret;
    
    ret = noteListenersToAdd_.equal_range(path);
    auto it = ret.first;
    while(it != ret.second) {
        if(it->second == object) {
            noteListenersToAdd_.erase(it++);
            //break;
        }
        else
            ++it;
    }
    
    removedAny = true; // FIXME: do we still need this?
#endif
    
#ifdef DEBUG_OSC
	if(removedAny)
		std::cout << "Removed OSC listener from path '" << path << "'\n";	
	else
		std::cout << "Removal failed to find OSC listener on path '" << path << "'\n";
#endif
	
	return removedAny;
}

// Removes an object from all OSC messages it was listening to.  Returns true if object
// was found and removed.

bool OscMessageSource::removeListener(OscHandler *object)
{
	if(object == NULL)
		return false;

	bool removedAny = false;

#ifdef OLD_OSC_MESSAGE_SOURCE
	oscListenerMutex_.enterWrite();	// Lock the mutex so no incoming messages happen in the middle
	
	// Every time we remove an element from the multimap, the iterator is potentially corrupted.  Realistically
	// there should never be more than one entry with the same object and same path (we check this on insertion).
	
	auto it = noteListeners_.begin();
	while(it != noteListeners_.end())
	{
		if(it->second == object)
		{
			noteListeners_.erase(it++);
			removedAny = true;
			//break;
		}
		else
			++it;
	}
	
	oscListenerMutex_.exitWrite();
#else
    juce::ScopedLock sl(oscUpdaterMutex_);
    
    // Add this object to the removal list
    noteListenersForBlanketRemoval_.insert(object);
    
    // Also remove this object from anything on the add list, so it doesn't
    // get put back in by a previous add call.
    auto it = noteListenersToAdd_.begin();

    while(it != noteListenersToAdd_.end()) {
        if(it->second == object) {
            noteListenersToAdd_.erase(it++);
        }
        else
            ++it;
    }
    
    removedAny = true; // FIXME: do we still need this?
#endif
	
#ifdef DEBUG_OSC
	if(removedAny)
		std::cout << "Removed OSC listener from all paths\n";	
	else
		std::cout << "Removal failed to find OSC listener on any path\n";
#endif
	
	return removedAny;
}

// Propagate changes to the listeners to the main noteListeners_ object

void OscMessageSource::updateListeners()
{
    juce::ScopedLock sl2(oscListenerMutex_);    
    juce::ScopedLock sl(oscUpdaterMutex_);
    
    // Step 1: remove any objects that need complete removal from all paths
    for(auto blanketRemovalIterator = noteListenersForBlanketRemoval_.begin();
        blanketRemovalIterator != noteListenersForBlanketRemoval_.end();
        ++blanketRemovalIterator) {

        auto it = noteListeners_.begin();

        while(it != noteListeners_.end()) {
            if(it->second == *blanketRemovalIterator) {
                noteListeners_.erase(it++);
            }
            else
                ++it;
        }
    }
    
    // Step 2: remove any specific path listeners
    for( auto it = noteListenersToRemove_.begin(); it != noteListenersToRemove_.end(); ++it) {
		std::pair< std::multimap< std::string, OscHandler*>::iterator, std::multimap<std::string, OscHandler*>::iterator> ret;

		std::string const& path = it->first;
        OscHandler *object = it->second;
        
        // Find all the objects that match this string and remove ones that correspond to this particular OscHandler
        ret = noteListeners_.equal_range(path);
        
        auto it2 = ret.first;
        while(it2 != ret.second)
        {
            if(it2->second == object) {
                noteListeners_.erase(it2++);
                //break;
            }
            else
                ++it2;
        }
    }

    // Step 3: add any listeners
    for(auto it = noteListenersToAdd_.begin(); it != noteListenersToAdd_.end(); ++it) {
        noteListeners_.insert( std::pair<std::string, OscHandler*>(it->first, it->second));
    }
    
    // Step 4: clear the buffers of pending listeners
    noteListenersForBlanketRemoval_.clear();
    noteListenersToRemove_.clear();
    noteListenersToAdd_.clear();
}

#pragma mark OscReceiver

// OscReceiver::handler()
// The main handler method for incoming OSC messages.  From here, we farm out the processing depending
// on the path. Return 0 if the message has been adequately handled, 1 otherwise (so the server can look
// for other functions to pass it to).

int OscReceiver::handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *data)
{
	bool matched = false;
	
	std::string pathString(path);
	
	if(useThru_)
	{
		// Rebroadcast any matching messages
		
		if(!pathString.compare(0, thruPrefix_.length(), thruPrefix_))
			lo_send_message(thruAddress_, path, msg);
	}
	
	// Check if the incoming message matches the global prefix for this program.  If not, discard it.
	if(pathString.compare(0, globalPrefix_.length(), globalPrefix_))
	{
#ifdef DEBUG_OSC
		std::cout << "OSC message '" << path << "' received\n";
#endif
		return 1;
	}
	
    // Update the list of OSC listeners to propagate any changes
    updateListeners();
    
	// Lock the mutex so the list of listeners doesn't change midway through
    oscListenerMutex_.enter();
	
	// Now remove the global prefix and compare the rest of the message to the registered handlers.
	std::pair<std::multimap<std::string, OscHandler*>::iterator, std::multimap<std::string, OscHandler*>::iterator> ret;
	std::string truncatedPath = pathString.substr(globalPrefix_.length(),
											 pathString.length() - globalPrefix_.length());
	std::string subpath = truncatedPath;
	ret = noteListeners_.equal_range(truncatedPath);

    while(ret.first == ret.second) {
        // No handlers match this range. But maybe there are higher-level handlers
        // that match all subpaths.
        
        // Strip off the last component of the path
        int pathSeparator = subpath.find_last_of('/');

        if(pathSeparator == std::string::npos)   // Not found --> no match
            break;
        else {
            // Reduce string by one path level and add *; compare again
            subpath = subpath.substr(0, pathSeparator);
            subpath.push_back('*');
            ret = noteListeners_.equal_range(subpath);
        }
    }

    auto it = ret.first;
    while(it != ret.second) {
        OscHandler *object = (*it++).second;
        
#ifdef DEBUG_OSC
        std::cout << "Matched OSC path '" << path << "' to handler " << object << '\n';
#endif
        object->oscHandlerMethod(truncatedPath.c_str(), types, argc, argv, data);
        matched = true;
    }
	
    oscListenerMutex_.exit();
    
	if(matched)		// This message has been handled
		return 0;
	
#ifdef DEBUG_OSC    
	printf("Unhandled OSC path: <%s>\n", path);
	
    for (int i=0; i<argc; i++) {
		printf("arg %d '%c' ", i, types[i]);
		lo_arg_pp((lo_type)types[i], argv[i]);
		printf("\n");
    }
#endif
	
    return 1;
}

// Set the current port for the OSC receiver object. This implies stopping and
// restarting the server. Returns true on success.
bool OscReceiver::setPort(const int port)
{
    // Stop existing server if running
    if(oscServerThread_ != 0) {
        lo_server_thread_del_method(oscServerThread_, NULL, NULL);
        lo_server_thread_stop(oscServerThread_);
        lo_server_thread_free(oscServerThread_);
        oscServerThread_ = 0;
    }
    
    // Port value 0 indicates to turn off; this always succeeds.
    if(port == 0) {
        return true;
    }
    
    // Now create a new one on the new port
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
        return true;
    }
    
    return false;
}

#pragma mark OscTransmitter

// Add a new transmit address.  Returns the index of the new address.

int OscTransmitter::addAddress(const char * host, const char * port, int proto)
{
	lo_address addr = lo_address_new_with_proto(proto, host, port);
	
	if(addr == 0)
		return -1;
	addresses_.push_back(addr);
	
	return (int)addresses_.size() - 1;
}

// Delete a current transmit address

void OscTransmitter::removeAddress(int index)
{
	if(index >= addresses_.size() || index < 0)
		return;
	addresses_.erase(addresses_.begin() + index);
}

// Delete all destination addresses

void OscTransmitter::clearAddresses()
{
	auto it = addresses_.begin();
	
	while(it != addresses_.end()) {
		lo_address_free(*it++);
	}
	
	addresses_.clear();
}

void OscTransmitter::sendMessage(const char * path, const char * type, ...)
{
    if(!enabled_)
        return;
    
	va_list v;
	
	va_start(v, type);
	lo_message msg = lo_message_new();
	lo_message_add_varargs(msg, type, v);

	/*if(debugMessages_) {
		std::cout << path << " " << type << ": ";
		
		lo_arg **args = lo_message_get_argv(msg);
		
		for(int i = 0; i < lo_message_get_argc(msg); i++) {
			switch(type[i]) {
				case 'i':
					std::cout << args[i]->i << " ";
					break;
				case 'f':
					std::cout << args[i]->f << " ";
					break;
				default:
					std::cout << "? ";
			}
		}
		
		std::cout << '\n';
		//lo_message_pp(msg);
	}*/
	
	sendMessage(path, type, msg);

	lo_message_free(msg);
	va_end(v);
}

void OscTransmitter::sendMessage(const char * path, const char * type, const lo_message& message)
{
    if(!enabled_)
        return;
    
    if(debugMessages_) {
        std::cout << path << " " << type << " ";

        int argc = lo_message_get_argc(message);
        lo_arg **argv = lo_message_get_argv(message);
        for (int i=0; i<argc; i++) {
            lo_arg_pp((lo_type)type[i], argv[i]);
            std::cout << " ";
        }
        std::cout << '\n';
    }
    
	// Send message to everyone who's currently listening
	for(auto it = addresses_.begin(); it != addresses_.end(); it++) {
		lo_send_message(*it, path, message);
	}
}

// Send an array of bytes as an OSC message.  Bytes will be sent as a blob.

void OscTransmitter::sendByteArray(const char * path, const unsigned char * data, int length)
{
    if(!enabled_)
        return;
	if(length == 0)
		return;
	
	lo_blob b = lo_blob_new(length, data);
	
	lo_message msg = lo_message_new();
	lo_message_add_blob(msg, b);
	
	if(debugMessages_) {
		std::cout << path << " ";
		lo_message_pp(msg);
	}
	
	// Send message to everyone who's currently listening
	for( auto it = addresses_.begin(); it != addresses_.end(); it++) {
		lo_send_message(*it, path, msg);
	}	
	
	lo_blob_free(b);
}

OscTransmitter::~OscTransmitter()
{
	clearAddresses();
}

OscMessage* OscTransmitter::createMessage(const char * path, const char * type, ...)
{
    va_list v;
    
    va_start(v, type);
    lo_message msg = lo_message_new();
    lo_message_add_varargs(msg, type, v);
    va_end(v);
    
    return new OscMessage(path, type, msg);
}
