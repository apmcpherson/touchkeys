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
 
  OscMidiConverter.cpp: converts incoming OSC messages to outgoing MIDI
  messages with adjustable ranges and parameters.
*/
#include "OscMidiConverter.h"
#include "MidiKeyboardSegment.h"

#undef DEBUG_OSC_MIDI_CONVERTER

// Main constructor: set up OSC reception from the keyboard
OscMidiConverter::OscMidiConverter(PianoKeyboard& keyboard, MidiKeyboardSegment& segment, int controllerId) :
  keyboard_(keyboard), keyboardSegment_(segment), midiOutputController_(0),
  controller_(controllerId), lastUniqueId_(0),
  incomingController_(MidiKeyboardSegment::kControlDisabled)
{
	setOscController(&keyboard_);
    
    for(int i = 0; i < 16; i++) {
        currentValue_[i] = 0;
        lastOutputValue_[i] = -1;
    }
}

// Set the type of MIDI message (CC, Pitch Wheel, Aftertouch)
// that this particular object is in charge of controlling.
// Optionally specify the default, range, and whether to use a
// 14 bit controller.
void OscMidiConverter::setMidiMessageType(int defaultValue, int minValue,
                                          int maxValue, int centerValue, bool use14BitControl) {
    /*if(controller < 0 || controller >= MidiKeyboardSegment::kControlMax) {
        controller_ = MidiKeyboardSegment::kControlDisabled;
        return;
    }
    
    keyboardSegment_ = segment;*/
    
    // Otherwise, send defaults on the current controller to all channels and update to the new values
    //for(int i = 0; i < 16; i++)
    //    sendDefaultValue(i);
    
    // Clear any existing active inputs, but not the mappings themselves
    lastValues_.clear();
    
    //controller_ = controller;
    if(defaultValue >= 0)
        controlDefaultValue_ = defaultValue;
    else {
        // Default value for MIDI CCs and aftertouch is 0
        // Default value for MIDI pitch wheel is 8192
        if(controller_ == MidiKeyboardSegment::kControlPitchWheel)
            controlDefaultValue_ = 8192;
        else
            controlDefaultValue_ = 0;
    }
    
    if(centerValue >= 0)
        controlCenterValue_ = centerValue;
    else {
        // Set center value with same procedure as default value
        if(controller_ == MidiKeyboardSegment::kControlPitchWheel)
            controlCenterValue_ = 8192;
        else
            controlCenterValue_ = 0;
    }
    
    // Pitch wheel is always 14 bit. Aftertouch is always 7 bit.
    // Other CCs are selectable (though according to MIDI spec not every
    // controller has an LSB defined). And above 96 using "control+32"
    // for fine adjust no longer makes sense
    if(controller_ < 96)
        controllerIs14Bit_ = use14BitControl;
    else if(controller_ == MidiKeyboardSegment::kControlPitchWheel)
        controllerIs14Bit_ = true;
    else
        controllerIs14Bit_ = false;
   
    // Handle the outer ranges of the control, providing defaults if unspecified
    if(controllerIs14Bit_) {
        if(maxValue < 0 || maxValue > 16383)
            controlMaxValue_ = 16383;
        else
            controlMaxValue_ = maxValue;
        if(minValue < 0 || minValue > 16383)
            controlMinValue_ = 0;
        else
            controlMinValue_ = minValue;
    }
    else {
        if(maxValue < 0 || maxValue > 127)
            controlMaxValue_ = 127;
        else
            controlMaxValue_ = maxValue;
        if(minValue < 0 || minValue > 127)
            controlMinValue_ = 0;
        else
            controlMinValue_ = minValue;
    }
}

void OscMidiConverter::listenToIncomingControl(int controller, int centerValue, bool use14BitControl) {
    if(controller < 0 || controller >= MidiKeyboardSegment::kControlMax) {
        incomingController_ = MidiKeyboardSegment::kControlDisabled;
        return;
    }
    
    incomingController_ = controller;
    
    // Assign the center value that will be subtacted from the incoming controller
    if(centerValue >= 0)
        incomingControllerCenterValue_ = centerValue;
    else if(controller == MidiKeyboardSegment::kControlPitchWheel)
        incomingControllerCenterValue_ = 8192; // Pitch wheel is centered at 8192
    else
        incomingControllerCenterValue_ = 0;
    
    // Pitch wheel is always 14 bit. Aftertouch is always 7 bit.
    // Other CCs are selectable (though according to MIDI spec not every
    // controller has an LSB defined). And above 96 using "control+32"
    // for fine adjust no longer makes sense
    if(controller < 96)
        incomingControllerIs14Bit_ = use14BitControl;
    else if(controller == MidiKeyboardSegment::kControlPitchWheel)
        incomingControllerIs14Bit_ = true;
    else
        incomingControllerIs14Bit_ = false;
}

// Resend the most recent value
void OscMidiConverter::resend(int channel) {
    sendCurrentValue(keyboardSegment_.outputPort(), channel, -1, true);
}

// Send the default value on the specified channel.
void OscMidiConverter::sendDefaultValue(int channel) {    
    if(midiOutputController_ == nullptr || controller_ == MidiKeyboardSegment::kControlDisabled)
        return;
    
    // Modulate the default value by the value of the incoming controller,
    // if one is enabled.
    int defaultValue = controlDefaultValue_;
    if(incomingController_ != MidiKeyboardSegment::kControlDisabled)
        defaultValue += keyboardSegment_.controllerValue(incomingController_) - incomingControllerCenterValue_;
    
    if(controller_ == MidiKeyboardSegment::kControlPitchWheel) {
        //sendPitchWheelRange(keyboardSegment_.outputPort(), channel);
        midiOutputController_->sendPitchWheel(keyboardSegment_.outputPort(), channel, defaultValue);
    }
    else if(controller_ == MidiKeyboardSegment::kControlChannelAftertouch) {
        midiOutputController_->sendAftertouchChannel(keyboardSegment_.outputPort(), channel, defaultValue);
    }
    else if(controller_ == MidiKeyboardSegment::kControlPolyphonicAftertouch) {
        // TODO
    }
    else if(controllerIs14Bit_) {
        midiOutputController_->sendControlChange(keyboardSegment_.outputPort(), channel, controller_, defaultValue >> 7);
        midiOutputController_->sendControlChange(keyboardSegment_.outputPort(), channel, controller_ + 32, defaultValue & 0x7F);
    }
    else
        midiOutputController_->sendControlChange(keyboardSegment_.outputPort(), channel, controller_, defaultValue);
}

int OscMidiConverter::currentControllerValue(int channel) {
    float controlValue = (float)controlCenterValue_ + (float)controlMinValue_
    + currentValue_[channel]*(float)(controlMaxValue_ - controlMinValue_);
    
    if(incomingController_ != MidiKeyboardSegment::kControlDisabled) {
        controlValue += keyboardSegment_.controllerValue(incomingController_) - incomingControllerCenterValue_;
#ifdef DEBUG_OSC_MIDI_CONVERTER
        std::cout << "current value " << currentValue_[channel] << " corresponds to " << controlValue;
        std::cout << " including incoming value " << (keyboardSegment_.controllerValue(incomingController_) - incomingControllerCenterValue_) << std::endl;
#endif
    }
    else {
#ifdef DEBUG_OSC_MIDI_CONVERTER
        std::cout << "current value " << currentValue_[channel] << " corresponds to " << controlValue << std::endl;
#endif
    }
    
    // For 14-bit CC messages, multiply by 128 first to keep the same apparent range as
    // the 7-bit version but adding extra resolution on the second CC number. This should
    // not be done for pitch wheel where the values are already normalised to 14 bits.
    if(controllerIs14Bit_ && controller_ != MidiKeyboardSegment::kControlPitchWheel)
        controlValue *= 128.0;
    
    int roundedControlValue = (int)floorf(controlValue + 0.5f);
    int maxValue = controllerIs14Bit_ ? 16383 : 127;
    if(roundedControlValue > maxValue)
        roundedControlValue = maxValue;
    if(roundedControlValue < 0)
        roundedControlValue = 0;
    
    return roundedControlValue;
}

// Add a new OSC input to this MIDI control
void OscMidiConverter::addControl(const std::string& oscPath, int oscParamNumber, float oscMinValue,
                                  float oscMaxValue, float oscCenterValue, int outOfRangeBehavior) {
	// First remove any existing mapping with these exact parameters
	removeControl(oscPath);

#ifdef DEBUG_OSC_MIDI_CONVERTER
    std::cout << "OscMidiConverter: adding path " << oscPath << std::endl;
#endif
    
	// Insert the mapping
	OscInput input;
    
    input.oscParamNumber = oscParamNumber;
    input.oscMinValue = oscMinValue;
    input.oscMaxValue = oscMaxValue;
    
    // Calculate the normalized center value which will be subtracted
    // from the scaled input. Do this once now to save computation.
    if(oscMinValue == oscMaxValue)
        input.oscScaledCenterValue = 0.5;
    else {
        input.oscScaledCenterValue = (oscCenterValue - oscMinValue) / (oscMaxValue - oscMinValue);
        if(input.oscScaledCenterValue < 0)
            input.oscScaledCenterValue = 0;
        if(input.oscScaledCenterValue > 1.0)
            input.oscScaledCenterValue = 1.0;
    }
    input.outOfRangeBehavior = outOfRangeBehavior;
    input.uniqueId = lastUniqueId_++;
    inputs_[oscPath] = input;
	
	// Register for the relevant OSC message
	addOscListener(oscPath);
}

// Remove an existing OSC input
void OscMidiConverter::removeControl(const std::string& oscPath) {
    // Find the affected control and its ID
    if(inputs_.count(oscPath) == 0)
        return;
    int controlId = inputs_[oscPath].uniqueId;
    
#ifdef DEBUG_OSC_MIDI_CONVERTER
    std::cout << "OscMidiConverter: removing path " << oscPath << std::endl;
#endif
    
    // Look for any active inputs on this channel
    for(int i = 0; i < 16; i++) {
        int channelModulatedId = idWithChannel(i, controlId);
        if(lastValues_.count(channelModulatedId) == 0)
            continue;
        
        // Found a last value. Subtract it off and get new value
        float lastValueThisChannel = lastValues_[channelModulatedId];
        currentValue_[i] -= lastValueThisChannel;
        
        // Remove this value from the set of active inputs
        lastValues_.erase(channelModulatedId);
        
        // Send the new value after removing this one
        sendCurrentValue(keyboardSegment_.outputPort(), i, -1, true);
    }
    
    // Having removed any active inputs, now remove the control itself
    // TODO: mutex protection
    inputs_.erase(oscPath);
    
    removeOscListener(oscPath);
}

void OscMidiConverter::removeAllControls() {
    // Clear all active inputs and send default values to all channels
    for(int i = 0; i < 16; i++)
        sendDefaultValue(i);
    lastValues_.clear();
    inputs_.clear();
    lastUniqueId_ = 0;
    removeAllOscListeners();
}

// Update the minimum input value of an existing path
void OscMidiConverter::setControlMinValue(const std::string& oscPath, float newValue) {
    if(inputs_.count(oscPath) == 0)
        return;
    inputs_[oscPath].oscMinValue = newValue;
}

// Update the maximum input value of an existing path
void OscMidiConverter::setControlMaxValue(const std::string& oscPath, float newValue) {
    if(inputs_.count(oscPath) == 0)
        return;
    inputs_[oscPath].oscMaxValue = newValue;
}

// Update the center input value of an existing path
void OscMidiConverter::setControlCenterValue(const std::string& oscPath, float newValue) {
    if(inputs_.count(oscPath) == 0)
        return;
    float minValue, maxValue, scaledCenterValue;
    minValue = inputs_[oscPath].oscMinValue;
    maxValue = inputs_[oscPath].oscMaxValue;
    
    if(minValue == maxValue)
        scaledCenterValue = 0.0;
    else
        scaledCenterValue = (newValue - minValue) / (maxValue - minValue);
    if(scaledCenterValue < 0)
        scaledCenterValue = 0;
    if(scaledCenterValue > 1.0)
        scaledCenterValue = 1.0;
    
    inputs_[oscPath].oscScaledCenterValue = scaledCenterValue;
}

// Update the out of range behavior for an existing path
void OscMidiConverter::setControlOutOfRangeBehavior(const std::string& oscPath, int newBehavior) {
    if(inputs_.count(oscPath) == 0)
        return;
    inputs_[oscPath].outOfRangeBehavior = newBehavior;
}

// Reset any active previous values on the given channel
// 'send' indicates whether to send the value when finished
// if items were erased
void OscMidiConverter::clearLastValues(int channel, bool send) {
    auto it = lastValues_.begin();
    
    bool erased = false;
    
    while(it != lastValues_.end()) {
        // Look for any last values matching this channel
        if((it->first & 0x0F) == channel) {
            lastValues_.erase(it++);
            erased = true;
        }
        else
            it++;
    }
 
    currentValue_[channel] = 0;
    lastOutputValue_[channel] = -1;
    
    // If any last values were erased and send is enabled,
    // resend the current values
    if(erased && send)
        sendDefaultValue(channel);
}

// OSC Handler, called by the data source (PianoKeyboard in this case). Check path against stored
// inputs and map to MIDI accordingly

bool OscMidiConverter::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
#ifdef DEBUG_OSC_MIDI_CONVERTER
    std::cout << "OscMidiConverter: received path " << path << std::endl;
#endif
	if(midiOutputController_ == nullptr || controller_ == MidiKeyboardSegment::kControlDisabled)
		return false;
    
    //double before = Time::getMillisecondCounterHiRes();  // DEBUG
    
	// First value should always be MIDI note number (integer type) so we can retrieve
	// information from the PianoKeyboard class
	if(numValues < 1)
		return false;
	if(types[0] != 'i')
		return false;
	int midiNoteNumber = values[0]->i;
    
	// Get the MIDI retransmission channel from the note number
	if(keyboard_.key(midiNoteNumber) == 0)
		return false;
	int midiChannel = keyboard_.key(midiNoteNumber)->midiChannel();
	if(midiChannel < 0 || midiChannel > 15) {
#ifdef DEBUG_OSC_MIDI_CONVERTER
        std::cout << "OscMidiConverter: no retransmission channel on note " << midiNoteNumber << std::endl;
#endif
		return false;
    }
    
    // Find the relevant input and make sure this OSC message has enough parameters
    if(inputs_.count(path) == 0)
        return false;
    OscInput const& input = inputs_[path];
    if(input.oscParamNumber >= numValues)
        return false;
    
    // Find the relevant input value from this OSC message
    float oscParamValue;
    if(types[input.oscParamNumber] == 'f')
        oscParamValue = values[input.oscParamNumber]->f;
    else if(types[input.oscParamNumber] == 'i')
        oscParamValue = (float)values[input.oscParamNumber]->i;
    else
        return false;
    
    // Scale input to a 0-1 range, then to the output range. There's a special case for MIDI pitch wheel,
    // where if the range is set to 0, it means to use the segment-wide pitch wheel range. This is done so
    // we don't have to cache multiple copies of the pitch wheel range in every OSC-MIDI converter.
    float scaledValue;
    if(controller_ == MidiKeyboardSegment::kControlPitchWheel &&
       input.oscMaxValue == 0 && input.oscMinValue == 0) {
        float pitchWheelRange = keyboardSegment_.midiPitchWheelRange();
        scaledValue = (oscParamValue + pitchWheelRange) / (2.0 * pitchWheelRange);
    }
    else
        scaledValue = (oscParamValue - input.oscMinValue) / (input.oscMaxValue - input.oscMinValue);

#ifdef DEBUG_OSC_MIDI_CONVERTER
    std::cout << "port " << keyboardSegment_.outputPort() << " received input " << oscParamValue << " which scales to " << scaledValue << std::endl;
#endif
    
    // Figure out what to do with an out of range value...
    if(scaledValue < 0.0 || scaledValue > 1.0) {
        if(input.outOfRangeBehavior == kOutOfRangeClip) {
            if(scaledValue < 0.0)
                scaledValue = 0.0;
            if(scaledValue > 1.0)
                scaledValue = 1.0;
        }
        else if(input.outOfRangeBehavior == kOutOfRangeExtrapolate) {
            // Do nothing
        }
        else	// Ignore or unknown behavior
            return false;
    }
    
    //double midpoint = Time::getMillisecondCounterHiRes(); // DEBUG
    
    // Now subtract the normalized center value, which may put the range outside 0-1
    // but this is expected. For example, in a pitch wheel situation, we might move
    // to a range of -0.5 to 0.5.
    scaledValue -= input.oscScaledCenterValue;

    // Look for previous input with this path and channel and remove it
    int channelModulatedId = idWithChannel(midiChannel, input.uniqueId);
    if(lastValues_.count(channelModulatedId) > 0) {
        // Found a last value. Subtract it off and replace with our current value
        float lastValueThisChannel = lastValues_[channelModulatedId];
        currentValue_[midiChannel] -= lastValueThisChannel;
#ifdef DEBUG_OSC_MIDI_CONVERTER
        std::cout << "found and removed " << lastValueThisChannel << ", now have " << currentValue_[midiChannel] << std::endl;
#endif
    }
    
    lastValues_[channelModulatedId] = scaledValue;
    currentValue_[midiChannel] += scaledValue;

    // Send the total current value as a MIDI controller
    sendCurrentValue(keyboardSegment_.outputPort(), midiChannel, midiNoteNumber, false);
    
    //double after = Time::getMillisecondCounterHiRes(); // DEBUG
    
    //std::cout << "OscMidiConverter: total " << after - before << "ms, of which " << after - midpoint << " in 2nd half\n";
    
	return true;
}

// Send the current sum value of all OSC inputs as a MIDI message
void OscMidiConverter::sendCurrentValue(int port, int channel, int note, bool force) {
    if(midiOutputController_ == nullptr || channel < 0 || channel > 15)
        return;
    
    // TODO: what about values that are centered by default e.g. pitch?
    /*float controlValue = (float)controlCenterValue_ + (float)controlMinValue_
                            + currentValue_[channel]*(float)(controlMaxValue_ - controlMinValue_);
    
    if(incomingController_ != MidiKeyboardSegment::kControlDisabled)
        controlValue += keyboardSegment_.controllerValue(incomingController_) - incomingControllerCenterValue_;
    
    std::cout << "sending value " << currentValue_[channel] << " as " << controlValue << std::endl;
    int roundedControlValue = (int)roundf(controlValue);
    if(roundedControlValue > controlMaxValue_)
        roundedControlValue = controlMaxValue_;
    if(roundedControlValue < controlMinValue_)
        roundedControlValue = controlMinValue_;*/
    
    int roundedControlValue = currentControllerValue(channel);
    
    // If this is the same ultimate CC value as before, don't resend unless forced
    if(roundedControlValue == lastOutputValue_[channel] && !force)
        return;
    lastOutputValue_[channel] = roundedControlValue;
    
    // Four cases: Pitch Wheel messages, aftertouch, 14-bit controls (major and minor controllers), ordinary 7-bit controls
    if(controller_ == MidiKeyboardSegment::kControlPitchWheel) {
        midiOutputController_->sendPitchWheel(port, channel, roundedControlValue);
    }
    else if(controller_ == MidiKeyboardSegment::kControlChannelAftertouch) {
        midiOutputController_->sendAftertouchChannel(port, channel, roundedControlValue);
    }
    else if(controller_ == MidiKeyboardSegment::kControlPolyphonicAftertouch && note >= 0) {
        midiOutputController_->sendAftertouchPoly(port, channel, note + keyboardSegment_.outputTransposition(),
                                                    roundedControlValue);
    }
    else if(controllerIs14Bit_) {
        // LSB for controllers 0-31 are found on controllers 32-63
        int lsb = roundedControlValue & 0x007F;
        int msb = (roundedControlValue >> 7) & 0x007F;
        
        midiOutputController_->sendControlChange(port, channel, controller_, msb);
        midiOutputController_->sendControlChange(port, channel, controller_ + 32, lsb);
    }
    else {	// 7 bit
        midiOutputController_->sendControlChange(port, channel, controller_, roundedControlValue);
    }
}

