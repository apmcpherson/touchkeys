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

  Mapping.cpp: base class for a single-note mapping. The mapping will take in
  MIDI, touch and (optionally) continuous key position data, and generate
  specific OSC or MIDI messages. TouchKeys-specific mappings generally
  inherit from the TouchkeyBaseMapping subclass, which provides other
  useful generic methods.
*/

#include "Mapping.h"
#include "MappingFactory.h"
#include "MappingScheduler.h"

// Main constructor takes references/pointers from objects which keep track
// of touch location, continuous key position and the state detected from that
// position. The PianoKeyboard object is strictly required as it gives access to
// Scheduler and OSC methods. The others are optional since any given system may
// contain only one of continuous key position or touch sensitivity
Mapping::Mapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                       Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker)
: keyboard_(keyboard), factory_(factory), noteNumber_(noteNumber), touchBuffer_(touchBuffer),
positionBuffer_(positionBuffer), positionTracker_(positionTracker), engaged_(false),
suspended_(false), updateInterval_(kDefaultUpdateInterval),
nextScheduledTimestamp_(0)
{
    // Create a statically bound call to the performMapping() method that
    // we use each time we schedule a new mapping
    mappingAction_ = boost::bind(&Mapping::performMapping, this);
}

// Copy constructor
Mapping::Mapping(Mapping const& obj) : keyboard_(obj.keyboard_), factory_(obj.factory_), noteNumber_(obj.noteNumber_),
touchBuffer_(obj.touchBuffer_), positionBuffer_(obj.positionBuffer_), positionTracker_(obj.positionTracker_),
engaged_(obj.engaged_), updateInterval_(obj.updateInterval_),
nextScheduledTimestamp_(obj.nextScheduledTimestamp_)
{
    // Create a statically bound call to the performMapping() method that
    // we use each time we schedule a new mapping
    mappingAction_ = boost::bind(&Mapping::performMapping, this);
    
    // Register ourself if already engaged since the scheduler won't have a copy of this object
    if(engaged_) {
#ifdef NEW_MAPPING_SCHEDULER
        keyboard_.mappingScheduler().scheduleNow(this);
#else
        keyboard_.scheduleEvent(this, mappingAction_, keyboard_.schedulerCurrentTimestamp());
#endif
    }
}

// Destructor. IMPORTANT NOTE: any derived class of Mapping() needs to call disengage() in its
// own destructor. It can't be called here, or there is a risk that the scheduled action will be
// called between the destruction of the derived class and the destruction of Mapping. This
// will result in a pure virtual function call and a crash.
Mapping::~Mapping() {
    //std::cerr << "~Mapping(): " << this << std::endl;
}

// Turn on mapping of data. Register for a callback and set a flag so
// we continue to receive updates
void Mapping::engage() {
    engaged_ = true;
    
    //std::cout << "Mapping::engage(): before TS " << keyboard_.schedulerCurrentTimestamp() << std::endl;
    // Register for trigger updates from touch data and state updates if either one is present.
    // Don't register for triggers on each new key sample
    if(touchBuffer_ != nullptr)
        registerForTrigger(touchBuffer_);
    if(positionTracker_ != nullptr)
        registerForTrigger(positionTracker_);
    nextScheduledTimestamp_ = keyboard_.schedulerCurrentTimestamp();
    //std::cout << "Mapping::engage(): mid TS " << keyboard_.schedulerCurrentTimestamp() << std::endl;
#ifdef NEW_MAPPING_SCHEDULER
    keyboard_.mappingScheduler().registerMapping(this);
    keyboard_.mappingScheduler().scheduleNow(this);
#else
    keyboard_.scheduleEvent(this, mappingAction_, nextScheduledTimestamp_);
#endif
    //std::cout << "Mapping::engage(): after TS " << keyboard_.schedulerCurrentTimestamp() << std::endl;
}

// Turn off mapping of data. Remove our callback from the scheduler
void Mapping::disengage(bool shouldDelete) {
    //std::cerr << "Mapping::disengage(): " << this << std::endl;
    
    engaged_ = false;
#ifndef NEW_MAPPING_SCHEDULER
    keyboard_.unscheduleEvent(this/*, nextScheduledTimestamp_*/);
#endif
    // Unregister for updates from touch data
    if(touchBuffer_ != nullptr)
        unregisterForTrigger(touchBuffer_);
    if(positionTracker_ != nullptr)
        unregisterForTrigger(positionTracker_);
    //std::cerr << "Mapping::disengage(): done\n";
    
#ifdef NEW_MAPPING_SCHEDULER
    if(shouldDelete)
        keyboard_.mappingScheduler().unregisterAndDelete(this);
    else
        keyboard_.mappingScheduler().unregisterMapping(this);
#endif
}

// Reset state back to defaults
void Mapping::reset() {
    updateInterval_ = kDefaultUpdateInterval;
}
