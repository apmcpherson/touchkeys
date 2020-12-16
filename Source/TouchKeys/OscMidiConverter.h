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
 
  OscMidiConverter.h: converts incoming OSC messages to outgoing MIDI
  messages with adjustable ranges and parameters.
*/

#pragma once

#include "Osc.h"
#include "MidiOutputController.h"

/* OscMidiConverter
 *
 * This class handles the sending of MIDI output messages of a particular type
 * (control change, aftertouch, pitch wheel) in response to incoming OSC messages.
 * Each object takes responsibility for one type of MIDI output but can take
 * several types of OSC message to control it.
 */

class OscMidiConverter : public OscHandler {
public:
    // Behavior for out-of-range inputs.
    enum {
        kOutOfRangeIgnore = 1,
        kOutOfRangeClip,
        kOutOfRangeExtrapolate
    };

private:
    // Structure holding information about a given OSC source
	struct OscInput {
        int uniqueId;               // ID to keep track of recent values
		int oscParamNumber;         // Parameter number in the OSC message we map
		float oscMinValue;          // Min and max of its input range
		float oscMaxValue;
        float oscScaledCenterValue; // Value of the input that should correspond to control center,
                                    // pre-normalized to 0-1 range
		int outOfRangeBehavior;     // What happens at the edge of the range
	};
    
public:
	// Constructor
	OscMidiConverter(PianoKeyboard& keyboard, MidiKeyboardSegment& segment, int controllerId);
	
	// ***** MIDI methods *****
    
    // Provide a reference to the MidiOutputController which actually sends the messages
	void setMidiOutputController(MidiOutputController* m) { midiOutputController_ = m; }
	
    // Set which control this object is handling. If minimum or maximum values are specified,
    // then the control will never exceed this range no matter what OSC messages come in.
    // use14BitControl specifies whether a 14-bit (paired) MIDI CC message should be used,
    // for relevant values of message.
    void setMidiMessageType(int defaultValue = -1, int minValue = -1,
                            int maxValue = -1, int centerValue = -1, bool use14BitControl = false);
    
    // Set whether this converter passes through incoming CC messages from the MIDI input,
    // and if so, which one. Doesn't need to be the same CC coming in as going out.
    void listenToIncomingControl(int controller, int centerValue = -1, bool use14BitControl = false);
    
    // Force a resend of the current value
    void resend(int channel);
    
    // Send the default controller value on the specified channel. Typically used
    // before note onset.
    void sendDefaultValue(int channel);
    
    // Return the current value of the MIDI controller without sending it
    int currentControllerValue(int channel);
    
	// ***** OSC methods *****
	
    // This message specifies an OSC path to be mapped to MIDI, along with its input ranges which
    // correspond to the complete specified MIDI range.
	void addControl(const std::string& oscPath, int oscParamNumber, float oscMinValue, float oscMaxValue,
                    float oscCenterValue, int outOfRangeBehavior);
	void removeControl(const std::string& oscPath);
	void removeAllControls();
    
    // These methods update the range for an existing control
    void setControlMinValue(const std::string& oscPath, float newValue);
    void setControlMaxValue(const std::string& oscPath, float newValue);
    void setControlCenterValue(const std::string& oscPath, float newValue);
    void setControlOutOfRangeBehavior(const std::string& oscPath, int newBehavior);
	
    // Reset any active previous values on the given channel
    void clearLastValues(int channel, bool send = true);
    
	// OSC Handler Method: called by PianoKeyboard (or other OSC source)
	bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
	
	// Destructor
	~OscMidiConverter() {}
	
private:
    // ***** Private Methods *****
    int idWithChannel(int channel, int inputId) { return (inputId << 4) + channel; }
    void sendCurrentValue(int port, int channel, int note, bool force);
    
	// ***** Member Variables *****
	
	PianoKeyboard& keyboard_;						// Main piano keyboard controller
    MidiKeyboardSegment& keyboardSegment_;          // Which segment of the keyboard this mapping is using
	MidiOutputController* midiOutputController_;	// Class handling MIDI output
    
    int controller_;                                // Which MIDI control to use
    bool controllerIs14Bit_;                        // Whether to use a paired MIDI CC
    int controlMinValue_, controlMaxValue_;         // Ranges control can take
    int controlCenterValue_;                        // The center value to use when all OSC inputs are 0
    int controlDefaultValue_;                       // Default value for the control on new notes

    int lastUniqueId_;                              // Global unique ID for input messages
    
    int incomingController_;                         // Which controller we listen to from the MIDI input
    bool incomingControllerIs14Bit_;                // Whether the input controller is 14 bit
    int incomingControllerCenterValue_;             // The center value to subtract from the incoming controller
    
    std::map<std::string, OscInput> inputs_;        // OSC sources for this MIDI output
    std::map<int, float> lastValues_;               // Recently received values from each OSC input
    
    float currentValue_[16];                        // Current sum value of all inputs for each channel
    int lastOutputValue_[16];                       // The last value we sent out; saved to avoid duplicate messages
};
