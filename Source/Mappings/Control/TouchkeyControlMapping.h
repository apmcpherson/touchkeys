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

  TouchkeyControlMapping.h: per-note mapping for the TouchKeys control
  mapping, which converts an arbitrary touch parameter into a MIDI or
  OSC control message.
*/

#pragma once

#include "../TouchkeyBaseMapping.h"
#include "../../Utility/IIRFilter.h"
#include <boost/bind.hpp>
#include <climits>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>

// This class handles the implementation of a basic MIDI/OSC control message
// based on touch data. It can use absolute or relative position in the X
// or Y axis.

class TouchkeyControlMapping : public TouchkeyBaseMapping /*public Mapping, public OscHandler*/ {
    friend class TouchkeyControlMappingFactory;
    
public:
    enum {
        kInputParameterXPosition = 1,
        kInputParameterYPosition,
        kInputParameterTouchSize,
        kInputParameter2FingerMean,
        kInputParameter2FingerDistance,
        kInputParameterMaxValue
    };
    
    enum {
        kTypeAbsolute = 1,
        kTypeFirstTouchRelative,
        kTypeNoteOnsetRelative,
        kTypeMaxValue
    };
    
    enum {
        kDirectionPositive = 1,
        kDirectionNegative,
        kDirectionBoth,
        kDirectionMaxValue
    };
    
private:
    // Useful constants for mapping MRP messages
    static constexpr int kDefaultMIDIChannel = 0;
    static constexpr int kDefaultFilterBufferLength = 300;

    static constexpr bool kDefaultIgnoresTwoFingers = false;
    static constexpr bool kDefaultIgnoresThreeFingers = false;
    static constexpr int kDefaultDirection = kDirectionPositive;
public:
	// ***** Constructors *****
	
	// Default constructor, passing the buffer on which to trigger
	TouchkeyControlMapping(PianoKeyboard &keyboard, MappingFactory *factory, int noteNumber, Node<KeyTouchFrame>* touchBuffer,
                             Node<key_position>* positionBuffer, KeyPositionTracker* positionTracker);
	
    // ***** Destructor *****
    
    ~TouchkeyControlMapping();
	
    // ***** Modifiers *****
    
    // Enable mappings to be sent
    //void engage();
    
    // Disable mappings from being sent
    //void disengage(bool shouldDelete = false);
	
    // Reset the state back initial values
	void reset();
    
    // Resend the current state of all parameters
    void resend();
    
    // Name for this control, used in the OSC path
    //void setName(const std::string& name);
    
    // Parameters for the controller handling
    // Input parameter to use for this control mapping and whether it is absolute or relative
    void setInputParameter(int parameter, int type);
    
    // Input/output range for this parameter
    void setRange(float inputMin, float inputMax, float outputMin, float outputMax, float outputDefault);
    
    // Threshold which must be exceeded for the control to engage (for relative position), or 0 if not used
    void setThreshold(float threshold);
    
    // Set whether the mapping should ignore multiple touches
    void setIgnoresMultipleFingers(bool ignoresTwo, bool ignoresThree);
    
    // Set whether the mapping should use the absolute value of a relative position
    void setDirection(int direction);

	// ***** Evaluators *****
    
    // OSC Handler Method: called by PianoKeyboard (or other OSC source)
	//bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
	
    // This method receives triggers whenever events occur in the touch data or the
    // continuous key position (state changes only). It alters the behavior and scheduling
    // of the mapping but does not itself send OSC messages
	void triggerReceived(TriggerSource* who, timestamp_type timestamp);
	
    // This method handles the OSC message transmission. It should be run in the Scheduler
    // thread provided by PianoKeyboard.
    timestamp_type performMapping();
    
private:
    // ***** Private Methods *****
    void midiNoteOnReceived(int channel, int velocity);
    void midiNoteOffReceived(int channel);
    
    void resetDetectionState();
    void clearBuffers();
    
    float getValue(const KeyTouchFrame& frame);
    int locateTouchId(KeyTouchFrame const& frame, int index);
    int lowestUnassignedTouch(KeyTouchFrame const& frame, int *indexWithinFrame);
    void sendControlMessage(float value, bool force = false);
    
	// ***** Member Variables *****
    
    bool controlIsEngaged_;                     // Whether the control has
    
    float inputMin_, inputMax_;                 // Input ranges
    float outputMin_, outputMax_;               // Output ranges
    float outputDefault_;                       // Default value to send in absence of any input
    int inputParameter_;                        // Parameter to be used for mapping
    int inputType_;                             // Type of mapping (absolute, relative, etc.)
    float threshold_;                           // Threshold that must be exceeded before mapping engages
    bool ignoresTwoFingers_;                    // Whether this mapping supresses all messages when two
    bool ignoresThreeFingers_;                  // or three fingers are present
    int direction_;                             // Whether the mapping goes up, down or both directions (for relative motion)
    
    float touchOnsetValue_, midiOnsetValue_;    // Where the touch began initially and at MIDI note on
    float lastValue_;                          // Where the touch was at the last frame we received
    int idsOfCurrentTouches_[3];                // Which touch ID(s) we're currently following
    timestamp_type lastTimestamp_;              // When the last data point arrived
    Node<float>::size_type lastProcessedIndex_; // Index of the last filtered position sample we've handled
    
    float controlEngageLocation_;                  // Where the controller was engaged (i.e. where the threshold was crossed, if relative)
    float controlScalerPositive_, controlScalerNegative_; // Translation between position and control values for upward and downward motions

    float lastControlValue_;                    // The last value we sent out
    
    Node<float> rawValues_;                     // Most recent values
    //CriticalSection rawValueAccessMutex_;       // Mutex protecting access to raw values buffer
};
