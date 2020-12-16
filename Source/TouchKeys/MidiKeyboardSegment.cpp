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
 
  MidiKeyboardSegment.cpp: handles incoming MIDI data and certain input-output
  mappings for one segment of a keyboard. The keyboard may be divided up into
  any number of segments with different behaviors. An important role of this
  class is to manage the output channel allocation when using one MIDI channel
  per note (for example, to handle polyphonic pitch bend).
*/

#include "MidiKeyboardSegment.h"
#include "MidiOutputController.h"
#include "../Mappings/MappingFactory.h"
#include "../Mappings/Vibrato/TouchkeyVibratoMappingFactory.h"
#include "../Mappings/PitchBend/TouchkeyPitchBendMappingFactory.h"
#include "../Mappings/Control/TouchkeyControlMappingFactory.h"
#include "../Mappings/ReleaseAngle/TouchkeyReleaseAngleMappingFactory.h"
#include "../Mappings/OnsetAngle/TouchkeyOnsetAngleMappingFactory.h"
#include "../Mappings/MultiFingerTrigger/TouchkeyMultiFingerTriggerMappingFactory.h"
#include "../Mappings/KeyDivision/TouchkeyKeyDivisionMappingFactory.h"
#include "OscMidiConverter.h"

#undef DEBUG_MIDI_KEYBOARD_SEGMENT

const std::array< std::string, 7 > MidiKeyboardSegment::kMappingFactoryNames { 
	"Control", "Vibrato", "Pitch Bend", "Split Key", 
	"Multi-Finger Trigger", "Onset Angle", "Release Angle" 
};

const std::map< MidiKeyboardSegment::Mode, std::string > MidiKeyboardSegment::modeNames
{
	{ MidiKeyboardSegment::Mode::Off, "Off" },
	{ MidiKeyboardSegment::Mode::PassThrough, "Pass Through" },
	{ MidiKeyboardSegment::Mode::Monophonic, "Monophonic" },
	{ MidiKeyboardSegment::Mode::Polyphonic, "Polyphonic" },
	{ MidiKeyboardSegment::Mode::MPE, "MPE" }
};

// Constructor
MidiKeyboardSegment::MidiKeyboardSegment(PianoKeyboard& keyboard) : 
	keyboard_(keyboard), 
	mappingFactorySplitter_(keyboard)
{
	// Register for OSC messages from the internal keyboard source
	setOscController(&keyboard_);
	keyboard_.setMappingFactory(this, &mappingFactorySplitter_);
	
	setAllControllerActionsTo(kControlActionBlock);
	resetControllerValues();
	
	for(int i = 0; i < 128; i++)
		noteOnsetTimestamps_[i] = 0;
}

// Destructor
MidiKeyboardSegment::~MidiKeyboardSegment() {
	removeAllMappingFactories();
	keyboard_.removeMappingFactory(this);
}

bool MidiKeyboardSegment::respondsToMessage(const juce::MidiMessage& message) {
	const int channel = message.getChannel();

	// If the message is not something universal, check if it matches our channel
	if(channel > 0) {
		if(!(channelMask_ && (1 << (channel - 1))))
			return false;
	}
	
	// If the message has a note number, see if it's in range
	if(message.isNoteOn() || message.isNoteOff() || message.isAftertouch()) {
		const int noteNumber = message.getNoteNumber();
		if(noteNumber < noteMin_ || noteNumber > noteMax_)
			return false;
	}
	
	return true;
}

bool MidiKeyboardSegment::respondsToNote(const int noteNumber) {
	if(noteNumber < noteMin_ || noteNumber > noteMax_)
		return false;
	return true;
}

// Listen on a given MIDI channel
void MidiKeyboardSegment::enableChannel(const int channelNumber) {
	if(channelNumber >= 0 && channelNumber < 16)
		channelMask_ |= (1 << channelNumber);
}

// Listen on all MIDI channels
void MidiKeyboardSegment::enableAllChannels() {
	channelMask_ = 0xFFFF;
}

// Disable listening to a specific MIDI channel
void MidiKeyboardSegment::disableChannel(const int channelNumber) {
	if(channelNumber >= 0 && channelNumber < 16)
		channelMask_ &= ~(1 << channelNumber);
}

// Disable all MIDI channels
void MidiKeyboardSegment::disableAllChanels() {
	channelMask_ = 0;
}

// Set the range of notes we listen to. Sets the range to between
// minNote and maxNote, inclusive.
void MidiKeyboardSegment::setNoteRange(const int minNote, const int maxNote) {
	// Sanity check
	if(minNote > maxNote)
		return;
	if(minNote < 0)
		noteMin_ = 0;
	else if(minNote > 127)
		noteMin_ = 127;
	else
		noteMin_ = minNote;

	if(maxNote < 0)
		noteMax_ = 0;
	else if(maxNote > 127)
		noteMax_ = 127;
	else
		noteMax_ = maxNote;
}

// Set the MIDI pitch wheel range
void MidiKeyboardSegment::setMidiPitchWheelRange(const float semitones, const bool send) {
	if(semitones < 0)
		pitchWheelRange_ = 0;
	else if(semitones > 48.0)
		pitchWheelRange_ = 48.0;
	else
		pitchWheelRange_ = semitones;
	
	if(send)
		sendMidiPitchWheelRange();
}

// Send the MIDI pitch wheel range RPN
// If in polyphonic mode, send to all channels; otherwise send only
// to the channel in question.
void MidiKeyboardSegment::sendMidiPitchWheelRange() {
	// MPE-DONE?
	if( midiOutputController_ == nullptr )
		return;

	auto sendPWRange = [&]( const int channel ) {
		// Find number of semitones and cents
		const int majorRange = ( int ) floorf( pitchWheelRange_ );
		const int minorRange = ( int ) ( 100.0f * ( pitchWheelRange_ - floorf( pitchWheelRange_ ) ) );

		// Set RPN controller = 0
		midiOutputController_->sendControlChange( outputPortNumber_, channel, 101, 0 );
		midiOutputController_->sendControlChange( outputPortNumber_, channel, 100, 0 );
		// Set data value MSB/LSB for bend range in semitones
		midiOutputController_->sendControlChange( outputPortNumber_, channel, 6, majorRange );
		midiOutputController_->sendControlChange( outputPortNumber_, channel, 38, minorRange );
		// Set RPN controller back to 16383
		midiOutputController_->sendControlChange( outputPortNumber_, channel, 101, 127 );
		midiOutputController_->sendControlChange( outputPortNumber_, channel, 100, 127 );
	};

	switch( mode_ )
	{
		case Mode::Polyphonic :
			for( int i = outputChannelLowest_; i < outputChannelLowest_ + retransmitMaxPolyphony_; i++ ){
				sendPWRange( i );
			}
		break;
		case Mode::MPE :
			sendPWRange( 0x00 );
		break;
		default:
			sendPWRange( outputChannelLowest_ );
		break;
	}
}


// Enable TouchKeys standalone mode (no MIDI input, touch triggers note)
void MidiKeyboardSegment::enableTouchkeyStandaloneMode() {
	if(touchkeyStandaloneMode_)
		return;
	
	addOscListener("/touchkeys/on");
	addOscListener("/touchkeys/off");
	touchkeyStandaloneMode_ = true;
}

// Disable TouchKeys standalone mode (no MIDI input, touch triggers note)
void MidiKeyboardSegment::disableTouchkeyStandaloneMode() {
	if(!touchkeyStandaloneMode_)
		return;

	removeOscListener("/touchkeys/on");
	removeOscListener("/touchkeys/off");
	touchkeyStandaloneMode_ = false;
}

// Disable any currently active notes
void MidiKeyboardSegment::allNotesOff() {
	// TODO: implement me

	for( int i = 0; i != 16; ++i )
		midiOutputController_->sendControlChange( outputPortNumber_, i, 123, 0 );
}

// Reset controller values to defaults
void MidiKeyboardSegment::resetControllerValues() {
	// Most controls default to 0
	for(int i = 0; i < kControlMax; i++)
		controllerValues_[i] = 0;
	// ...except pitch wheel, which defaults to center
	controllerValues_[kControlPitchWheel] = 8192;
}

// Set the operating mode of the controller.  The mode determines the behavior in
// response to incoming MIDI data.

void MidiKeyboardSegment::setMode(const int mode) {
	const Mode newMode { static_cast< Mode >( mode ) };

	if( newMode == Mode::PassThrough)
		setModePassThrough();
	else if( newMode == Mode::Monophonic)
		setModeMonophonic();
	else if( newMode == Mode::Polyphonic)
		setModePolyphonic();
	else if( newMode == Mode::MPE)
		setModeMPE();
	else
		setModeOff();
}

void MidiKeyboardSegment::setModeOff() {
	allNotesOff();
	removeOscListener("/midi/noteon");
	setAllControllerActionsTo(kControlActionBlock);
	mode_ = Mode::Off;
}

void MidiKeyboardSegment::setModePassThrough() {
	allNotesOff();
	removeOscListener("/midi/noteon");
	setAllControllerActionsTo(kControlActionPassthrough);
	mode_ = Mode::PassThrough;
}

void MidiKeyboardSegment::setModeMonophonic() {
	allNotesOff();
	removeOscListener("/midi/noteon");
	setAllControllerActionsTo(kControlActionPassthrough);
	mode_ = Mode::Monophonic;
}

void MidiKeyboardSegment::setModePolyphonic() {
	// First turn off any notes in the current mode
	allNotesOff();
	setAllControllerActionsTo(kControlActionBroadcast);
	
	// Register a callback for touchkey data.  When we get a note-on message,
	// we request this callback occur once touch data is available.  In this mode,
	// we know the eventual channel before any touch data ever occurs: thus, we
	// only listen to the MIDI onset itself, which happens after all the touch
	// data is sent out.
	addOscListener("/midi/noteon");
	
	mode_ = Mode::Polyphonic;
	
	if(retransmitMaxPolyphony_ < 1)
		retransmitMaxPolyphony_ = 1;
	modePolyphonicSetupHelper();
}

void MidiKeyboardSegment::setModeMPE() 
{
	// "When a receiver receives an MPE Configuration Message, it must set the Master Pitch Bend Sensitivity to
	// ±2 semitones, and the Pitch Bend Sensitivity of the Member Channels to ±48 semitones."

	// First turn off any notes in the current mode
	allNotesOff();
	
	// MPE-TODO some things need to be set to master-zone retransmit
	// also reset pitch wheel value to 0 since it's sent separately
	setAllControllerActionsTo(kControlActionBroadcast);
	
	// Register a callback for touchkey data.  When we get a note-on message,
	// we request this callback occur once touch data is available.  In this mode,
	// we know the eventual channel before any touch data ever occurs: thus, we
	// only listen to the MIDI onset itself, which happens after all the touch
	// data is sent out.
	addOscListener("/midi/noteon");
	
	mode_ = Mode::MPE;
	mpeZone_ = MPEZone::Lower;

	// MPE-DONE - currently supports only the Lower Zone
	// Set RPN 6 to enable MPE with the appropriate zone
	modeMPEsendConfigurationMessage( mpeZone_ );
}

// Set the maximum polyphony, affecting polyphonic mode only
void MidiKeyboardSegment::setPolyphony(const int polyphony) {
	// First turn off any notes if this affects current polyphonic mode
	// (other modes unaffected so we can make these changes in background)
	if(mode_ == Mode::Polyphonic)
		allNotesOff();

	if( mode_ == Mode::MPE )
	{
		// Currently only supporting MPE Lower Zone, where MIDI channel 0x00 is reserved
		// as the MPE Master Channel. Channels 0x01-0x0E (15 in total) can be used as 
		// Member Channels
		if( polyphony < 1 )
			retransmitMaxPolyphony_ = 1;
		else if( polyphony > 15 )
			retransmitMaxPolyphony_ = 15;
		else
			retransmitMaxPolyphony_ = polyphony;

		// MPE-DONE
		// Send RPN 6 to change the zone configuration
		// -- maybe in modePolyphonicSetupHelper()
		modeMPEsendConfigurationMessage( MPEZone::Lower, retransmitMaxPolyphony_ );

	}else{
		if( polyphony < 1 )
			retransmitMaxPolyphony_ = 1;
		else if( polyphony > 16 )
			retransmitMaxPolyphony_ = 16;
		else
			retransmitMaxPolyphony_ = polyphony;

		if( mode_ == Mode::Polyphonic )
			modePolyphonicSetupHelper();
	}
}

// Set whether the damper pedal is enabled or not
void MidiKeyboardSegment::setDamperPedalEnabled(const bool enable) {
	if(damperPedalEnabled_ && !enable) {
		// Pedal was enabled before, now it isn't. Clear out any notes
		// currently in the pedal so they can be retaken.
		damperPedalWentOff();
	}
	
	damperPedalEnabled_ = enable;
}

// Set the lowest output channel
void MidiKeyboardSegment::setOutputChannelLowest(const int ch) {

	// FIXME this is probably broken for polyphonic mode!

	if( mode_ == Mode::MPE )
	{
		// MPE-DONE: send new RPN 6 for disabling old zone and creating new one
		modeMPEsendConfigurationMessage( MPEZone::Lower );
		mpeZone_ = MPEZone::Lower;
		outputChannelLowest_ = 0x01;
	}else{
		outputChannelLowest_ = ch;
	}
}

// Handle an incoming MIDI message
void MidiKeyboardSegment::midiHandlerMethod(juce::MidiInput* source, const juce::MidiMessage& message) {
	// Log the timestamps of note onsets and releases, regardless of the mode
	// of processing
	if(message.isNoteOn()) {
		const int noteNumber { message.getNoteNumber() };

		if(noteNumber >= 0 && noteNumber < 128)
			noteOnsetTimestamps_[ noteNumber ] = keyboard_.schedulerCurrentTimestamp();
	}
	else if(message.isNoteOff()) {
		const int noteNumber { message.getNoteNumber() };
		// Remove the onset timestamp unless we have the specific condition:
		// (damper pedal enabled) && (pedal is down) && (polyphonic mode)
		// In this condition, onsets will be removed when note goes off
		if( noteNumber >= 0 && noteNumber < 128) {
			if(!damperPedalEnabled_ || controllerValues_[kMidiControllerDamperPedal] < kPedalActiveValue ||
			   (mode_ != Mode::Polyphonic && mode_ != Mode::MPE)) {
				noteOnsetTimestamps_[ noteNumber ] = 0;
			}
		}
	}
	else if(message.isAllNotesOff() || message.isAllSoundOff()) {
		for(int i = 0; i < 128; i++)
			noteOnsetTimestamps_[i] = 0;
	}
	
	// Log the values of incoming control changes in case mappings need to use them later
	if(message.isController() && !(message.isAllNotesOff() || message.isAllSoundOff())) {
		// Handle damper pedal specially: it may affect note allocation
		const int ccNumber { message.getControllerNumber() };
		const int ccValue { message.getControllerValue() };

		if(ccNumber == kMidiControllerDamperPedal) {
			if(ccValue < kPedalActiveValue &&
			   controllerValues_[kMidiControllerDamperPedal] >= kPedalActiveValue) {
				damperPedalWentOff();
			}
		}
		
		if(ccNumber >= 0 && ccNumber < 128) {
			if(ccNumber == 1 && usesKeyboardModWheel_) {
				controllerValues_[ccNumber] = ccValue;
				handleControlChangeRetransit(ccNumber, message);
			}
			else if(ccNumber >= 64 && ccNumber <= 69 && usesKeyboardPedals_) {
				// MPE-DONE send this on master zone
				midiOutputController_->sendControlChange( outputPortNumber_, 0x00, ccNumber, ccValue );

				controllerValues_[ccNumber] = ccValue;
				handleControlChangeRetransit(ccNumber, message);
			}
			else if(usesKeyboardMidiControllers_) {
				controllerValues_[ccNumber] = ccValue;
				handleControlChangeRetransit(ccNumber, message);
			}
		}
	}
	else if(message.isChannelPressure()) {
		if(usesKeyboardChannelPressure_) {
			controllerValues_[kControlChannelAftertouch] = message.getChannelPressureValue();
			handleControlChangeRetransit(kControlChannelAftertouch, message);
		}
	}
	else if(message.isPitchWheel()) {
		if(usesKeyboardPitchWheel_) {
			if(mode_ == Mode::MPE) {
				// MPE-DONE send this on master zone instead of putting it into the calculations
				switch( mpeZone_ ) 
				{
					case MPEZone::Lower :
						midiOutputController_->sendPitchWheel( outputPortNumber_, 0x00, message.getPitchWheelValue() );
					break;
					case MPEZone::Upper :
						midiOutputController_->sendPitchWheel( outputPortNumber_, 0x0F, message.getPitchWheelValue() );
					break;
					case MPEZone::LowerAndUpper :
						// ??
					break;
					case MPEZone::Off :
					default:
						// ??
					break;
				}
			}
			else {
				controllerValues_[kControlPitchWheel] = message.getPitchWheelValue();
				handleControlChangeRetransit(kControlPitchWheel, message);
			}
		}
	}
	else {
		// Process the message differently depending on the current mode
		switch(mode_) {
			case Mode::PassThrough:
				modePassThroughHandler(source, message);
				break;
			case Mode::Monophonic:
				modeMonophonicHandler(source, message);
				break;
			case Mode::Polyphonic:
				modePolyphonicHandler(source, message);
				break;
			case Mode::MPE:
				modeMPEHandler(source, message);
				break;
			case Mode::Off:
			default:
				// Ignore message
				break;
		}
	}
}

// OscHandler method which parses incoming OSC messages we've registered for.  In this case,
// we use OSC callbacks to find out about touch data for notes we want to trigger.

bool MidiKeyboardSegment::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
	if(touchkeyStandaloneMode_) {
		if(!strcmp(path, "/touchkeys/on") && numValues > 0) {
			int noteNumber = values[0]->i;
			if(!respondsToNote(noteNumber))
				return true;
			if(noteNumber >= 0 && noteNumber < 128) {
				// Generate MIDI note on for this message
				juce::MidiMessage msg( juce::MidiMessage::noteOn(1, noteNumber, ( juce::uint8)64));
				midiHandlerMethod(0, msg);
			}
			return true;
		}
		else if(!strcmp(path, "/touchkeys/off") && numValues > 0) {
			int noteNumber = values[0]->i;
			if(!respondsToNote(noteNumber))
				return true;
			if(noteNumber >= 0 && noteNumber < 128) {
				// Generate MIDI note off for this message
				juce::MidiMessage msg( juce::MidiMessage::noteOff(1, noteNumber));
				midiHandlerMethod(0, msg);
			}
			return true;
		}
	}
	
	if(mode_ == Mode::Polyphonic || mode_ == Mode::MPE) {
		modePolyphonicMPENoteOnCallback(path, types, numValues, values);
	}
	
	return true;
}

// Control method via OSC. This comes in via MainApplicationController to MidiInputController
// and is used specifically for querying and modifying the status of the zone and its mappings,
// as opposed to the more frequent OSC messages to oscHandlerMethod() which provide touch and
// MIDI data. Return true if message was successfully handled.
OscMessage* MidiKeyboardSegment::oscControlMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
	// First check if this message is destined for a mapping within the segment
	// e.g. /mapping/my_mapping_name/message_for_mapping
	if(!strncmp(path, "/mapping/", 9) && strlen(path) > 9) {
		std::string subpath(&path[9]);
		
		int separatorLoc = subpath.find_first_of('/');
		if(separatorLoc == std::string::npos || separatorLoc == subpath.length() - 1) {
			// Malformed input (no slash or it's the last character): ignore
			return 0;
		}
		
		// Find the name of the mapping in the nextsegment
		std::string mappingName = subpath.substr(0, separatorLoc);
		
		// Look for a matching factory. TODO: this should probably be mutex-protected
		for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
			if((*it)->getShortName() == mappingName) {
				std::string mappingAction = subpath.substr(separatorLoc);
				
				if(mappingAction == "/delete") {
					removeMappingFactory(*it);
					return OscTransmitter::createSuccessMessage();
				}
				else {
					// Pass message to mapping factory here
					OscMessage *response = (*it)->oscControlMethod(mappingAction.c_str(), types, numValues, values, data);
					
					// Prepend the mapping name to the response except in case of simple status response
					if(response == 0)
						return 0;
					else if(!strcmp(response->path(), "/result"))
						return response;
					response->prependPath(mappingName.c_str());
					response->prependPath("/mapping/");
					return response;
				}
			}
		}
	}
	else if(!strcmp(path, "/list-mappings")) {
		// Return a list of mapping names and types
		// TODO: this should be mutex-protected
		
		OscMessage *response = OscTransmitter::createMessage("/list-mappings/result", "i", mappingFactories_.size(), LO_ARGS_END);
		
		for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
			lo_message_add_string(response->message(), (*it)->getShortName().c_str());
		}
		
		return response;
	}
	else if(!strcmp(path, "/add-mapping")) {
		// Add a new mapping of a given type
		if(numValues >= 1) {
			if(types[0] == 'i') {
				int type = values[0]->i;
				
				if(type < 0 || type >= kMappingFactoryNames.size() )
					return OscTransmitter::createFailureMessage();
 
				// Create mapping factory of the requested type
				MappingFactory *newFactory = createMappingFactoryForIndex(type);
				if(newFactory == 0)
					return OscTransmitter::createFailureMessage();
 
				// Add the mapping factory to this segment, autogenerating the
				// name unless it is specified
				if(numValues >= 2) {
					if(types[1] == 's') {
						// Set the name as it was passed in
						newFactory->setName(&values[1]->s);
						addMappingFactory(newFactory, false);
					}
					else
						addMappingFactory(newFactory, true);
				}
				else
					addMappingFactory(newFactory, true);
				
				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/set-range")) {
		// Set the MIDI note range
		if(numValues >= 2) {
			if(types[0] == 'i' && types[1] == 'i') {
				int rangeLow = values[0]->i;
				int rangeHigh = values[1]->i;
				
				if(rangeLow < 0 || rangeLow > 127 || rangeHigh < 0 || rangeHigh > 127)
					return OscTransmitter::createFailureMessage();
				if(rangeLow > rangeHigh) {
					// Swap values so lowest one is always first
					int temp = rangeLow;
					rangeLow = rangeHigh;
					rangeHigh = temp;
				}
				
				setNoteRange(rangeLow, rangeHigh);
				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/set-transpose")) {
		// Set the transposition of the output
		if(numValues >= 1) {
			if(types[0] == 'i') {
				int transpose = values[0]->i;
				
				if(transpose < -48 || transpose > 48)
					return OscTransmitter::createFailureMessage();

				setOutputTransposition(transpose);
				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/set-transpose-octave-up")) {
		// Set the transposition of the output
		int transpose = outputTransposition() + 12;
		if(transpose > 48)
			transpose = 48;
		setOutputTransposition(transpose);

		return OscTransmitter::createSuccessMessage();
	}
	else if(!strcmp(path, "/set-transpose-octave-down")) {
		// Set the transposition of the output
		int transpose = outputTransposition() - 12;
		if(transpose < -48)
			transpose = -48;
		setOutputTransposition(transpose);
		
		return OscTransmitter::createSuccessMessage();
	}
	else if(!strcmp(path, "/set-controller-pass")) {
		// Set which controllers to pass through
		// Arguments: (channel pressure), (pitch wheel), (mod wheel), (pedals), (other CCs)
		
		if(numValues >= 5) {
			if(types[0] == 'i' && types[1] == 'i' && types[2] == 'i' && types[3] == 'i' && types[4] == 'i') {
				setUsesKeyboardChannelPressure(values[0]->i != 0);
				setUsesKeyboardPitchWheel(values[1]->i != 0);
				setUsesKeyboardModWheel(values[2]->i != 0);
				setUsesKeyboardPedals(values[3]->i != 0);
				setUsesKeyboardMIDIControllers(values[4]->i != 0);
				
				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/set-pitchwheel-range")) {
		// Set the MIDI pitchwheel range in semitones
		if(numValues >= 1) {
			if(types[0] == 'i') {
				int range = values[0]->i;
				
				setMidiPitchWheelRange(range);
				return OscTransmitter::createSuccessMessage();
			}
			else if(types[0] == 'f') {
				float range = values[0]->f;
				
				setMidiPitchWheelRange(range);
				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/send-pitchwheel-range")) {
		// Send an RPN value with the current pitchwheel range
		sendMidiPitchWheelRange();
		return OscTransmitter::createSuccessMessage();
	}
	else if(!strcmp(path, "/set-midi-mode")) {
		// Set the MIDI mode (mono, poly etc.)
		if(numValues >= 1) {
			if(types[0] == 's') {
				char *mode = &values[0]->s;
				
				if(!strcmp(mode, "off"))
					setModeOff();
				else if(!strncmp(mode, "pass", 4))
					setModePassThrough();
				else if(!strncmp(mode, "mono", 4))
					setModeMonophonic();
				else if(!strncmp(mode, "poly", 4))
					setModePolyphonic();
				else if(!strncmp(mode, "mpe", 3))
					setModeMPE();
				else
					return OscTransmitter::createFailureMessage();

				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/set-midi-channels")) {
		// Set the MIDI channels
		if(numValues >= 2) {
			if(types[0] == 'i' && types[1] == 'i') {
				int channelLow = values[0]->i;
				int channelHigh = values[1]->i;
				
				if(channelLow < 1 || channelLow > 16 || channelHigh < 1 || channelHigh > 16)
					return OscTransmitter::createFailureMessage();
				if(channelLow > channelHigh) {
					// Swap values so lowest one is always first
					int temp = channelLow;
					channelLow = channelHigh;
					channelHigh = temp;
				}
				
				setOutputChannelLowest(channelLow - 1); // 1-16 --> 0-15 indexing
				int polyphony = channelHigh - channelLow + 1;
				if(polyphony < 1)
					polyphony = 1;
				setPolyphony(polyphony);
				
				return OscTransmitter::createSuccessMessage();
			}
		}
	}
	else if(!strcmp(path, "/set-midi-stealing")) {
		// Set whether MIDI voice stealing is enabled
		if(numValues >= 1) {
			if(types[0] == 'i') {
				setVoiceStealingEnabled(values[0]->i != 0);
				return OscTransmitter::createSuccessMessage();
			}
		}
	}

	// No match
	return 0;
}

// Acquire an OSC-MIDI converter. If a converter for this control already exists,
// return it. If not, create it. This method keeps track of how many objects have
// acquired the converter. When all acquirers have released ihe converter, it is
// destroyed.
OscMidiConverter* MidiKeyboardSegment::acquireOscMidiConverter(int controlId) {
	OscMidiConverter *converter;

	if(oscMidiConverters_.count(controlId) == 0) {
		converter = new OscMidiConverter(keyboard_, *this, controlId);
		converter->setMidiOutputController(midiOutputController_);
		
		oscMidiConverters_[controlId] = converter;
		oscMidiConverterReferenceCounts_[controlId] = 1;
	}
	else {
		if(oscMidiConverterReferenceCounts_.count(controlId) == 0) {
			std::cerr << "BUG: found a converter with missing reference counter\n";
		}
		else if(oscMidiConverterReferenceCounts_[controlId] <= 0) {
			std::cerr << "BUG: found a converter with no references\n";
			oscMidiConverterReferenceCounts_[controlId] = 1;
		}
		else
			oscMidiConverterReferenceCounts_[controlId]++;
		converter = oscMidiConverters_[controlId];
	}
	
	return converter;
}

// Release a previously acquired OSC-MIDI converter. This call must be paired with
// acquireOscMidiConverter.
void MidiKeyboardSegment::releaseOscMidiConverter(int controlId) {
	if(oscMidiConverters_.count(controlId) == 0 ||
	   oscMidiConverterReferenceCounts_.count(controlId) == 0) {
		std::cerr << "BUG: releasing a missing OSC-MIDI converter\n";
		return;
	}
	
	oscMidiConverterReferenceCounts_[controlId]--;
	if(oscMidiConverterReferenceCounts_[controlId] == 0) {
		// This was the last release of this converter. Delete it.
		OscMidiConverter *converter = oscMidiConverters_[controlId];
		delete converter;
		oscMidiConverters_.erase(controlId);
		oscMidiConverterReferenceCounts_.erase(controlId);
	}
}

// Helper predicate function for filtering strings
inline bool char_is_not_alphanumeric(int c) {
#ifdef _MSC_VER
	return !isalnum(c);
#else
	return !std::isalnum(c);
#endif
}

// Return the number of mapping factory types available
int MidiKeyboardSegment::numberOfMappingFactories() {
	return kMappingFactoryNames.size();
}

// Return the name of the given mapping factory type
juce::String MidiKeyboardSegment::mappingFactoryNameForIndex(int index) {
	if(index < 0 || index >= kMappingFactoryNames.size() )
		return {};

	return kMappingFactoryNames[index];
}

// Return a new object of the given mapping factory type
MappingFactory* MidiKeyboardSegment::createMappingFactoryForIndex(int index) {
	switch(index) {
		case 0:
			return new TouchkeyControlMappingFactory(keyboard_, *this);
		case 1:
			return new TouchkeyVibratoMappingFactory(keyboard_, *this);
		case 2:
			return new TouchkeyPitchBendMappingFactory(keyboard_, *this);
		case 3:
			return new TouchkeyKeyDivisionMappingFactory(keyboard_, *this);
		case 4:
			return new TouchkeyMultiFingerTriggerMappingFactory(keyboard_, *this);
		case 5:
			return new TouchkeyOnsetAngleMappingFactory(keyboard_, *this);
		case 6:
			return new TouchkeyReleaseAngleMappingFactory(keyboard_, *this);
		default:
			return 0;
	}
}

// Return whethera  given mapping is experimental or not
bool MidiKeyboardSegment::mappingIsExperimental(int index) {
	if(index == 5)
		return true;
	return false;
}

// Create a new mapping factory for this segment. A pointer should be passed in
// of a newly-allocated object. It will be released upon removal.
void MidiKeyboardSegment::addMappingFactory(MappingFactory* factory, bool autoGenerateName) {
	// Check for duplicates-- can't add the same factory twice
	for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
		if(*it == factory)
			return;
	}
	
	// If the name should autogenerate, find a unique name for this factory
	if(autoGenerateName) {
		std::string baseName = factory->factoryTypeName();
		
		// Remove spaces, newlines, other invalid characters, leaving only alphanumerics
		baseName.erase(std::remove_if(baseName.begin(), baseName.end(), char_is_not_alphanumeric),
					   baseName.end());
		std::transform(baseName.begin(), baseName.end(), baseName.begin(), ::tolower);

		// Now look for an OSC path with this name. If found, add a number to the end, incrementing
		// it until a unique name is found
		std::string name = baseName;
		bool isUnique = false;
		int appendDigit = 2;
		
		while(!isUnique) {
			isUnique = true;
			
			for(int i = 0; i < mappingFactories_.size(); i++) {
				std::string compareName = mappingFactories_[i]->getName();
				int lastSeparator = compareName.find_last_of('/');
				if((lastSeparator != std::string::npos) && (lastSeparator < compareName.size() - 1))
					compareName = compareName.substr(lastSeparator + 1);
				
				if(name == compareName) {
					isUnique = false;
					
					// Try a new name with a new digit at the end...
					std::stringstream ss;
					ss << baseName << appendDigit;
					name = ss.str();
					appendDigit++;
					
					break;
				}
			}
		}
		
		factory->setName(name);
	}
	
	// Add factory to internal vector, and add it to splitter class
	mappingFactories_.push_back(factory);
	mappingFactorySplitter_.addFactory(factory);
	mappingFactoryUniqueIdentifier_++;
}

// Remove a mapping factory, releasing the associated object.
void MidiKeyboardSegment::removeMappingFactory(MappingFactory* factory) {
	
	for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
		if(*it == factory) {
			mappingFactorySplitter_.removeFactory(factory);
			delete factory;
			mappingFactories_.erase(it);
			break;
		}
	}
	
	mappingFactoryUniqueIdentifier_++;
}

// Remove all mapping factories, releasing each one
void MidiKeyboardSegment::removeAllMappingFactories() {
	
	mappingFactorySplitter_.removeAllFactories();
	for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
		delete *it;
	}
	
	mappingFactories_.clear();
	mappingFactoryUniqueIdentifier_++;
}

// Return a list of current mapping factories.
std::vector<MappingFactory*> const& MidiKeyboardSegment::mappingFactories(){
	return mappingFactories_;
}

// Return the specific index of a mapping factory for this segment
int MidiKeyboardSegment::indexOfMappingFactory(MappingFactory *factory) {
	int i = 0;
	
	for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
		if(*it == factory)
			return i;
		i++;
	}
	
	return -1;
}

// Get an XML element describing current settings (for saving presets)
// This element will need to be released (or added to another XML element
// that is released) by the caller
std::unique_ptr< juce::XmlElement > MidiKeyboardSegment::getPreset() {
	auto segmentElement = std::make_unique< juce::XmlElement >("Segment");

	// Add segment settings
	juce::PropertySet properties;
	properties.setValue("outputPort", outputPortNumber_);
	properties.setValue("mode", static_cast< int >( mode_ ) );
	properties.setValue("channelMask", (int)channelMask_);
	properties.setValue("noteMin", noteMin_);
	properties.setValue("noteMax", noteMax_);
	properties.setValue("outputChannelLowest", outputChannelLowest_);
	properties.setValue("outputTransposition", outputTransposition_);
	properties.setValue("damperPedalEnabled", damperPedalEnabled_);
	// Don't set standalone mode; that's an input parameter
	properties.setValue("usesKeyboardChannelPressure", usesKeyboardChannelPressure_);
	properties.setValue("usesKeyboardPitchWheel", usesKeyboardPitchWheel_);
	properties.setValue("usesKeyboardModWheel", usesKeyboardModWheel_);
	properties.setValue("usesKeyboardPedals", usesKeyboardPedals_);
	properties.setValue("usesKeyboardMidiControllers", usesKeyboardMidiControllers_);
	properties.setValue("pitchWheelRange", pitchWheelRange_);
	properties.setValue("retransmitMaxPolyphony", retransmitMaxPolyphony_);
	properties.setValue("useVoiceStealing", useVoiceStealing_);

	segmentElement->addChildElement(properties.createXml("Properties").get() );
	
	// Go through mapping factories and add their settings
	for( auto it = mappingFactories_.begin(); it != mappingFactories_.end(); ++it) {
		auto factoryElement = (*it)->getPreset();
		segmentElement->addChildElement( factoryElement.get() );
	}
	
	return segmentElement;
}

// Load settings from an XML element
bool MidiKeyboardSegment::loadPreset( juce::XmlElement const* preset) {
	removeAllMappingFactories();
	
	juce::XmlElement *propertiesElement = preset->getChildByName("Properties");
	if(propertiesElement == 0)
		return false;
	
	// Load segment settings
	juce::PropertySet properties;
	properties.restoreFromXml(*propertiesElement);
	
	if(!properties.containsKey("outputPort"))
		return false;
	outputPortNumber_ = properties.getIntValue("outputPort");
	if(!properties.containsKey("channelMask"))
		return false;
	channelMask_ = properties.getIntValue("channelMask");
	if(!properties.containsKey("noteMin"))
		return false;
	noteMin_ = properties.getIntValue("noteMin");
	if(!properties.containsKey("noteMax"))
		return false;
	noteMax_ = properties.getIntValue("noteMax");
	if(!properties.containsKey("outputChannelLowest"))
		return false;
	outputChannelLowest_ = properties.getIntValue("outputChannelLowest");
	if(!properties.containsKey("outputTransposition"))
		return false;
	outputTransposition_ = properties.getIntValue("outputTransposition");
	if(!properties.containsKey("damperPedalEnabled"))
		return false;
	damperPedalEnabled_ = properties.getBoolValue("damperPedalEnabled");
	if(!properties.containsKey("usesKeyboardChannelPressure"))
		return false;
	usesKeyboardChannelPressure_ = properties.getBoolValue("usesKeyboardChannelPressure");
	if(!properties.containsKey("usesKeyboardPitchWheel"))
		return false;
	usesKeyboardPitchWheel_ = properties.getBoolValue("usesKeyboardPitchWheel");
	if(!properties.containsKey("usesKeyboardModWheel"))
		return false;
	usesKeyboardModWheel_ = properties.getBoolValue("usesKeyboardModWheel");
	if(properties.containsKey("usesKeyboardPedals"))
		usesKeyboardPedals_ = properties.getBoolValue("usesKeyboardPedals");
	else
		usesKeyboardPedals_ = false;    // For backwards compatibility with older versions
	if(!properties.containsKey("usesKeyboardMidiControllers"))
		return false;
	usesKeyboardMidiControllers_ = properties.getBoolValue("usesKeyboardMidiControllers");
	if(!properties.containsKey("pitchWheelRange"))
		return false;
	pitchWheelRange_ = properties.getDoubleValue("pitchWheelRange");
	if(!properties.containsKey("retransmitMaxPolyphony"))
		return false;
	setPolyphony(properties.getIntValue("retransmitMaxPolyphony"));
	if(!properties.containsKey("useVoiceStealing"))
		return false;
	useVoiceStealing_ = properties.getBoolValue("useVoiceStealing");
	
	if(!properties.containsKey("mode"))
		return false;
	const int mode = properties.getIntValue( "mode" );
	// Setting the mode affects a few other variables so use the
	// functions rather than setting mode_ directly
	setMode( mode );
	
	// Load each mapping factory
	juce::XmlElement *element = preset->getChildByName("MappingFactory");
	
	while(element != 0) {
		if(!element->hasAttribute("type"))
			return false;
		
		// Create a new factory whose type depends on the XML tag
		MappingFactory *factory;
		juce::String const& factoryType = element->getStringAttribute("type");
		
		if(factoryType == "Control")
			factory = new TouchkeyControlMappingFactory(keyboard_, *this);
		else if(factoryType == "Vibrato")
			factory = new TouchkeyVibratoMappingFactory(keyboard_, *this);
		else if(factoryType == "PitchBend")
			factory = new TouchkeyPitchBendMappingFactory(keyboard_, *this);
		else if(factoryType == "KeyDivision")
			factory = new TouchkeyKeyDivisionMappingFactory(keyboard_, *this);
		else if(factoryType == "MultiFingerTrigger")
			factory = new TouchkeyMultiFingerTriggerMappingFactory(keyboard_, *this);
		else if(factoryType == "OnsetAngle")
			factory = new TouchkeyOnsetAngleMappingFactory(keyboard_, *this);
		else if(factoryType == "ReleaseAngle")
			factory = new TouchkeyReleaseAngleMappingFactory(keyboard_, *this);
		else {
			// Type unknown or unsupported; ignore and continue
			element = element->getNextElementWithTagName("MappingFactory");
			continue;
		}
		
		// Tell factory to load its settings from this element
		if(!factory->loadPreset(element)) {
			delete factory;
			return false;
		}
		
		// Add factory; don't autogenerate name as it will be saved
		addMappingFactory(factory, false);
		
		element = element->getNextElementWithTagName("MappingFactory");
	}
	
	return true;
}

// Mode-specific MIDI handlers.  These methods handle incoming MIDI data according to the rules
// defined by a particular mode of operation.

// Pass-Through: Retransmit any input data to the output unmodified.
void MidiKeyboardSegment::modePassThroughHandler( juce::MidiInput* source, const juce::MidiMessage& message) {
	// Check if there is a note on or off, and update the keyboard class accordingly
	if(message.isNoteOn()) {
		int note = message.getNoteNumber();
		if(keyboard_.key(note) != nullptr)
			keyboard_.key(note)->midiNoteOn(this, message.getVelocity(), message.getChannel() - 1,
											keyboard_.schedulerCurrentTimestamp());
		
		// Retransmit, possibly with transposition
		if(midiOutputController_ != nullptr) {
			auto newMessage = juce::MidiMessage::noteOn(message.getChannel(), message.getNoteNumber() + outputTransposition_, message.getVelocity());
			midiOutputController_->sendMessage(outputPortNumber_, newMessage);
		}
	}
	else if(message.isNoteOff()) {
		int note = message.getNoteNumber();
		if(keyboard_.key(note) != nullptr)
			keyboard_.key(note)->midiNoteOff(this, keyboard_.schedulerCurrentTimestamp());
		
		// Retransmit, possibly with transposition
		if(midiOutputController_ != nullptr) {
			auto newMessage = juce::MidiMessage::noteOff(message.getChannel(), message.getNoteNumber() + outputTransposition_);
			midiOutputController_->sendMessage(outputPortNumber_, newMessage);
		}
	}
	else if(message.isAftertouch()) { // Polyphonic aftertouch: adjust to transposition
		if(midiOutputController_ != nullptr) {
			auto newMessage = juce::MidiMessage::aftertouchChange(message.getChannel(), message.getNoteNumber() + outputTransposition_, message.getAfterTouchValue());
			midiOutputController_->sendMessage(outputPortNumber_, newMessage);
		}
	}
	else if(midiOutputController_ != nullptr) // Anything else goes through unchanged
		midiOutputController_->sendMessage(outputPortNumber_, message);
}

// Monophonic: all MIDI messages pass through to the output, which is assumed to be a monosynth.
// However the most recent key which determines the currently sounding note will have its mapping
// active; all others are suspended.
void MidiKeyboardSegment::modeMonophonicHandler(juce::MidiInput* source, const juce::MidiMessage& message) {
	if(message.isNoteOn()) {
		// First suspend any other mappings. This might include the current note if the touch
		// data has caused a mapping to be created.
		if(keyboard_.mappingFactory(this) != nullptr) {
			keyboard_.mappingFactory(this)->suspendAllMappings();
		}
		
		// And turn on note on MIDI controller
		if(midiOutputController_ != nullptr) {
			juce::MidiMessage newMessage = juce::MidiMessage::noteOn(outputChannelLowest_ + 1, message.getNoteNumber() + outputTransposition_, message.getVelocity());
			midiOutputController_->sendMessage(outputPortNumber_, newMessage);
		}
		
		// Now start the next one
		int note = message.getNoteNumber();
		if(keyboard_.key(note) != nullptr)
			keyboard_.key(note)->midiNoteOn(this, message.getVelocity(),
											outputChannelLowest_, keyboard_.schedulerCurrentTimestamp());
		
		// Now resume the current note's mapping
		if(keyboard_.mappingFactory(this) != nullptr) {
			keyboard_.mappingFactory(this)->resumeMapping(note, true);
		}
	}
	else if(message.isNoteOff()) {
		// First stop this note
		int note = message.getNoteNumber();
		if(keyboard_.key(note) != nullptr)
			keyboard_.key(note)->midiNoteOff(this, keyboard_.schedulerCurrentTimestamp());
		
		// Then reactivate the most recent note's mappings
		if(keyboard_.mappingFactory(this) != nullptr) {
			int newest = newestNote();
			if(newest >= 0)
				keyboard_.mappingFactory(this)->resumeMapping(newest, true);
		}
		
		// And turn off note on MIDI controller
		if(midiOutputController_ != nullptr) {
			juce::MidiMessage newMessage = juce::MidiMessage::noteOff(outputChannelLowest_ + 1, message.getNoteNumber() + outputTransposition_, message.getVelocity());
			midiOutputController_->sendMessage(outputPortNumber_, newMessage);
		}
	}
}

// Polyphonic: Each incoming note gets its own unique MIDI channel so its controllers
// can be manipulated separately (e.g. by touchkey data).  Keep track of available channels
// and currently active notes.
void MidiKeyboardSegment::modePolyphonicHandler(juce::MidiInput* source, const juce::MidiMessage& message) {
	
	if(message.getRawDataSize() <= 0)
		return;

	const unsigned char *rawData = message.getRawData();
	
	if(rawData[0] == kMidiMessageReset)
	{
		// Reset state and pass along to all relevant channels
		
		retransmitChannelForNote_.clear();	// Clear current note information
		retransmitChannelsAvailable_.clear();
		retransmitNotesHeldInPedal_.clear();
		for(int i = 0; i < retransmitMaxPolyphony_; i++) {
			retransmitChannelsAvailable_.insert(i);
		}
		if(midiOutputController_ != nullptr)
			midiOutputController_->sendReset(outputPortNumber_);

	}else if(message.isNoteOn()){
		
		const int noteNumber { message.getNoteNumber() };

		if(retransmitChannelForNote_.count( noteNumber ) > 0 && retransmitNotesHeldInPedal_.count( noteNumber ) == 0)
		{
			// Case (2)-- retrigger an existing note
			if(midiOutputController_ != nullptr) {
				midiOutputController_->sendNoteOn(outputPortNumber_, retransmitChannelForNote_[ noteNumber ],
					noteNumber + outputTransposition_, message.getVelocity());
			}
		}else{
			// New note
			modePolyphonicNoteOn( noteNumber, message.getVelocity());
		}

	}else if(message.isNoteOff()) {

		modePolyphonicNoteOff(message.getNoteNumber());

	}else if(message.isAllNotesOff() || message.isAllSoundOff()) {

		retransmitChannelForNote_.clear();	// Clear current note information
		retransmitChannelsAvailable_.clear();
		retransmitNotesHeldInPedal_.clear();

		for(int i = 0; i < retransmitMaxPolyphony_; i++)
			retransmitChannelsAvailable_.insert(i);

	}else if(message.isAftertouch()) { // polyphonic aftertouch
		const int noteNumber { message.getNoteNumber() };

		if(retransmitChannelForNote_.count( noteNumber ) > 0) {
			int retransmitChannel = retransmitChannelForNote_[ noteNumber ];

			if(midiOutputController_ != nullptr) {
				midiOutputController_->sendAftertouchPoly(outputPortNumber_, retransmitChannel,
					noteNumber + outputTransposition_, message.getAfterTouchValue());
			}
		}
	}
}

// Handle note on message in polyphonic mode.  Allocate a new channel
// for this note and rebroadcast it.
void MidiKeyboardSegment::modePolyphonicNoteOn( const uint8_t note, const uint8_t velocity) {

	int newChannel = -1;
	
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
	std::cout << "Channels available: ";
	for( auto it = retransmitChannelsAvailable_.begin();
		it != retransmitChannelsAvailable_.end(); ++it) {
		std::cout << *it << " ";
	}
	std::cout << '\n';
	
	std::cout << "Channels allocated: ";
	for( auto it = retransmitChannelForNote_.begin();
		it != retransmitChannelForNote_.end(); ++it) {
		std::cout << it->second << "(" << it->first << ") ";
	}
	std::cout << '\n';
#endif
	
	if(retransmitNotesHeldInPedal_.count(note) > 0) {
		// For notes that are still sounding in the pedal, reuse the same MIDI channel
		// they had before.
		if(retransmitChannelForNote_.count(note) > 0)
			newChannel = retransmitChannelForNote_[note];
		else {
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
			std::cout << "BUG: note " << note << " held in pedal but has no channel\n";
#endif
			retransmitNotesHeldInPedal_.erase(note);
			return;
		}
		
		// No longer held in pedal: it will be an active note again with the same channel
		retransmitNotesHeldInPedal_.erase(note);
	}
	else {
		// Otherwise, allocate a new channel to this note
		if(retransmitChannelsAvailable_.size() == 0) {
			if(damperPedalEnabled_) {
				// First priority is always to take a note that is being sustained
				// in the pedal but not actively held. This is true whether or not
				// voice stealing is enabled.
				const int oldNote = oldestNoteInPedal();
				int oldChannel = -1;
				if(retransmitChannelForNote_.count(oldNote) > 0)
					oldChannel = retransmitChannelForNote_[oldNote];
				if(oldNote >= 0) {
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
					std::cout << "Stealing note " << oldNote << " from pedal for note " << (int)note << '\n';
#endif
					modePolyphonicNoteOff(oldNote, true);
				}
			}
			
			// Now try again...
			if(retransmitChannelsAvailable_.size() == 0) {
				if(useVoiceStealing_) {
					// Find the voice with the oldest timestamp and turn it off
					const int oldNote = oldestNote();
					int oldChannel = -1;
					if(retransmitChannelForNote_.count(oldNote) > 0)
						oldChannel = retransmitChannelForNote_[oldNote];
					if(oldNote < 0) {
						// Shouldn't happen...
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
						std::cout << "No notes present, but no MIDI output channel available for note " << (int)note << '\n';
#endif
						return;
					}
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
					std::cout << "Stealing note " << oldNote << " for note " << (int)note << '\n';
#endif
					modePolyphonicNoteOff(oldNote, true);
				}
				else {
					// No channels available.  Print a warning and finish
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
					std::cout << "No MIDI output channel available for note " << (int)note << '\n';
#endif
					return;
				}
			}
		}
		
		// Request the first available channel
		newChannel = *retransmitChannelsAvailable_.begin();
		retransmitChannelsAvailable_.erase(newChannel);
		retransmitChannelForNote_[note] = newChannel;
	}
	
	if(keyboard_.key(note) != nullptr) {
		keyboard_.key(note)->midiNoteOn(this, velocity, newChannel, keyboard_.schedulerCurrentTimestamp());
	}
	
	// The above function will cause a callback to be generated, which in turn will generate
	// the Note On message.
}

// Handle note off message in polyphonic mode.  Release any channel
// associated with this note.
void MidiKeyboardSegment::modePolyphonicNoteOff(const uint8_t note, const bool forceOff) {    
	// If no channel associated with this note, ignore it
	if(retransmitChannelForNote_.count(note) == 0) {
		if(note >= 0 && note < 128)
			noteOnsetTimestamps_[note] = 0;
		return;
	}
	
	if(keyboard_.key(note) != nullptr) {
		keyboard_.key(note)->midiNoteOff(this, keyboard_.schedulerCurrentTimestamp());
	}

	int oldNoteChannel = retransmitChannelForNote_[note];
	
	if(midiOutputController_ != nullptr) {
		if(forceOff) {
			// To silence a note, we need to clear any pedals that might be holding it
			if(controllerValues_[kMidiControllerDamperPedal] >= kPedalActiveValue) {
				midiOutputController_->sendControlChange(outputPortNumber_, oldNoteChannel,
														 kMidiControllerDamperPedal, 0);
			}
			if(controllerValues_[kMidiControllerSostenutoPedal] >= kPedalActiveValue) {
				midiOutputController_->sendControlChange(outputPortNumber_, oldNoteChannel,
														 kMidiControllerSostenutoPedal, 0);
			}
			
			// Send All Notes Off and All Sound Off
			midiOutputController_->sendControlChange(outputPortNumber_, oldNoteChannel, kMidiControlAllNotesOff, 0);
			midiOutputController_->sendControlChange(outputPortNumber_, oldNoteChannel, kMidiControlAllSoundOff, 0);
		}
		else {
			// Send a Note Off message to the appropriate channel
			midiOutputController_->sendNoteOff(outputPortNumber_, oldNoteChannel, note + outputTransposition_);
		}
	}
	
	// If the pedal is enabled and currently active, don't re-enable this channel
	// just yet. Instead, let the note continue ringing until we have to steal it later.
	if(damperPedalEnabled_ && controllerValues_[kMidiControllerDamperPedal] >= kPedalActiveValue && !forceOff) {
		retransmitNotesHeldInPedal_.insert(note);
	}
	else {
		// Otherwise release the channel mapping associated with this note
		if(retransmitNotesHeldInPedal_.count(note) > 0)
			retransmitNotesHeldInPedal_.erase(note);
		retransmitChannelsAvailable_.insert(retransmitChannelForNote_[note]);
		retransmitChannelForNote_.erase(note);
		if(note >= 0 && note < 128)
			noteOnsetTimestamps_[note] = 0;
	}
	
	if(forceOff) {
		// Now re-enable any pedals that we might have temporarily lifted on this channel
		if(controllerValues_[kMidiControllerDamperPedal] >= kPedalActiveValue) {
			midiOutputController_->sendControlChange(outputPortNumber_, oldNoteChannel,
													 kMidiControllerDamperPedal,
													 controllerValues_[kMidiControllerDamperPedal]);
		}
		if(controllerValues_[kMidiControllerSostenutoPedal] >= kPedalActiveValue) {
			midiOutputController_->sendControlChange(outputPortNumber_, oldNoteChannel,
													 kMidiControllerSostenutoPedal,
													 controllerValues_[kMidiControllerSostenutoPedal]);
		}
	}
}

// Callback function after we request a note on.  PianoKey class will respond
// with touch data (if available within a specified timeout), or with a frame
// indicating an absence of touch data.  Once we receive this, we can send the
// MIDI note on message.

void MidiKeyboardSegment::modePolyphonicMPENoteOnCallback(const char *path, const char *types, int numValues, lo_arg **values) {
	if(numValues < 3)	// Sanity check: first 3 values hold MIDI information
		return;
	if(types[0] != 'i' || types[1] != 'i' || types[2] != 'i')
		return;
	
	int midiNote = values[0]->i;
	int midiChannel = values[1]->i;
	int midiVelocity = values[2]->i;
	
	if(midiNote < 0 || midiNote > 127)
		return;
	// If there are multiple segments of the keyboard active, there may be OSC
	// messages generated from keys that didn't come from us. Don't grab them by mistake.
	// FIXME: the real fix here is to include a source ID with the OSC message
	if(!respondsToNote(midiNote))
		return;
	
	// Send the Note On message to the correct channel
	if(midiOutputController_ != nullptr) {
		midiOutputController_->sendNoteOn(outputPortNumber_, midiChannel, midiNote + outputTransposition_, midiVelocity);
	}
}

void MidiKeyboardSegment::modeMPEsendConfigurationMessage( const MPEZone& z, const int singleZoneRange, const int dualZoneRange )
// Expects:
// - singleZoneRange between [1..15]
// - dualZoneRange between [1..14]
// NOTE "It is necessary to send only one MCM if a device intends to use only one Zone."
{
	static const uint8_t lowerZoneChannel { 0x00 };
	static const uint8_t upperZoneChannel { 0x0F };

	if( midiOutputController_ != nullptr ) {
		// throw std::runtime_error{ "No MIDI output port present" }
		return;
	}

	auto sendRPN6 = [&]( const int masterZoneChannel, const int memberChannelRange ) 
	{
		midiOutputController_->sendControlChange( outputPortNumber_, masterZoneChannel, 0x79, 0x00 );
		midiOutputController_->sendControlChange( outputPortNumber_, masterZoneChannel, 0x64, 0x06 );
		midiOutputController_->sendControlChange( outputPortNumber_, masterZoneChannel, 0x65, 0x00 );
		midiOutputController_->sendControlChange( outputPortNumber_, masterZoneChannel, 0x06, memberChannelRange );
	};

	switch( z )
	{
	case MPEZone::Lower:
		// "The Lower Zone is controlled by Master Channel 1, with Member Channels assigned sequentially from
		// Channel 2 upwards."
		// - Master Channel 1 (0x00)
		// - up to 15 Member Channels [2..16] (i.e., [0x01..0x0F]
		// e.g., 0x0F implies a range of 15 channels)
		sendRPN6( lowerZoneChannel, singleZoneRange );
		break;
	case MPEZone::Upper:
		// "The Upper Zone is controlled by Master Channel 16, with Member Channels assigned
		// sequentially from Channel 15 downwards."
		// - Master channel 16 ( 0x0F ); 
		// - up to 15 Member channels [1..15] (i.e., [0x00..0x0E])
		sendRPN6( upperZoneChannel, singleZoneRange );
		break;
	case MPEZone::LowerAndUpper: // Two MPE Zones
		
		// - Lower zone: Master Channel 1, Member Channels [2..15]
		// - Upper zone: Master Channel 16, Member Channels [2..15]
		// NOTE Implemented so as to assign x channels in lower zone, and 14 - x channels
		// in upper zone.
		sendRPN6( lowerZoneChannel, dualZoneRange );
		sendRPN6( upperZoneChannel, 14 - dualZoneRange );
		break;
	case MPEZone::Off:
	default:
		// "Sending an MCM with the number of Member Channels set to zero deactivates that Zone."
		// MPE switched off by setting both lower and upper zones member channel range to 0
		sendRPN6( lowerZoneChannel, 0x00 );
		sendRPN6( upperZoneChannel, 0x00 );
		break;
	}
}

// MPE (Multidimensional Polyphonic Expression): Each incoming note gets its own unique MIDI channel.
// Like polyphonic mode but implementing the details of the MPE specification which differ subtly
// from a straightforward polyphonic allocation
void MidiKeyboardSegment::modeMPEHandler(juce::MidiInput* source, const juce::MidiMessage& message) {
	// MPE-TODO
	if( message.getRawDataSize() <= 0 )
		return;

	auto resetState = [&]() {
		retransmitChannelForNote_.clear();	// Clear current note information
		retransmitChannelsAvailable_.clear();
		retransmitNotesHeldInPedal_.clear();

		if( mpeZone_ == MPEZone::Lower )
		{
			// Channel 0x00 is reserved as the Lower Zone's Master Channel.
			// Available channels start on outputChannelLowest_ which is 0x01
			for( int i = outputChannelLowest_; i != outputChannelLowest_ + retransmitMaxPolyphony_; ++i )
				retransmitChannelsAvailable_.insert( i );
		}
	};

	const uint8_t* rawData = message.getRawData();

	if( rawData[ 0 ] == kMidiMessageReset )
	{
		// Reset state and pass along to all relevant channels
		resetState();

		if( midiOutputController_ != nullptr )
			midiOutputController_->sendReset( outputPortNumber_ );

	} else if( message.isNoteOn() ) {

		const int note { message.getNoteNumber() };

		if( retransmitChannelForNote_.count( note ) > 0 && retransmitNotesHeldInPedal_.count( note ) == 0 )
		{
			// Case (2)-- retrigger an existing note
			if( midiOutputController_ != nullptr ) {
				midiOutputController_->sendNoteOn( outputPortNumber_, retransmitChannelForNote_[ note ],
					note + outputTransposition_, message.getVelocity() );
			}
		} else {
			// New note
			modeMPENoteOn( note, message.getVelocity() );
		}

	} else if( message.isNoteOff() ) {

		modeMPENoteOff( message.getNoteNumber() );

	} else if( message.isAllNotesOff() || message.isAllSoundOff() ) {

		resetState();

	} else if( message.isAftertouch() ) { // polyphonic aftertouch
		const int note { message.getNoteNumber() };

		if( retransmitChannelForNote_.count( note ) > 0 ) {
			int retransmitChannel = retransmitChannelForNote_[ note ];

			if( midiOutputController_ != nullptr ) {
				midiOutputController_->sendAftertouchPoly( outputPortNumber_, retransmitChannel,
					note + outputTransposition_, message.getAfterTouchValue() );
			}
		}
	}
}

// Handle note on message in MPE mode.  Allocate a new channel
// for this note and rebroadcast it.
void MidiKeyboardSegment::modeMPENoteOn(const uint8_t note, const uint8_t velocity) {
	// MPE-TODO
	// allocate notes to channels like polyphonic mode, with certain changes:
	// -- round-robin as default rather than first available
	// -- different stealing behaviour:
	// ---- when no channels are available, add to an existing one with the fewest sounding notes
	// ---- old note doesn't need to be turned off, but it could(?) have its mappings disabled
	int newChannel = -1;

#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
	std::cout << "Channels available: ";
	for( const int channel : retransmitChannelsAvailable_ )
		std::cout << channel << " ";

	std::cout << '\n';

	std::cout << "Channels allocated: ";
	for( const auto entry : retransmitChannelForNote_ ) {
		std::cout << entry.second << "(" << entry.first << ") ";
	}
	std::cout << '\n';
#endif

	if( retransmitNotesHeldInPedal_.count( note ) > 0 ) {
		// For notes that are still sounding in the pedal, reuse the same MIDI channel
		// they had before.
		if( retransmitChannelForNote_.count( note ) > 0 )
			newChannel = retransmitChannelForNote_[ note ];
		else {
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
			std::cout << "BUG: note " << note << " held in pedal but has no channel\n";
#endif
			retransmitNotesHeldInPedal_.erase( note );
			return;
		}

		// No longer held in pedal: it will be an active note again with the same channel
		retransmitNotesHeldInPedal_.erase( note );
	} else {
		// Otherwise, allocate a new channel to this note
		if( retransmitChannelsAvailable_.size() == 0 ) {
			if( damperPedalEnabled_ ) {
				// First priority is always to take a note that is being sustained
				// in the pedal but not actively held. This is true whether or not
				// voice stealing is enabled.
				const int oldNote = oldestNoteInPedal();
				int oldChannel = -1;
				if( retransmitChannelForNote_.count( oldNote ) > 0 )
					oldChannel = retransmitChannelForNote_[ oldNote ];
				if( oldNote >= 0 ) {
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
					std::cout << "Stealing note " << oldNote << " from pedal for note " << ( int ) note << '\n';
#endif
					modeMPENoteOff( oldNote, true );
				}
			}

			// Now try again...
			if( retransmitChannelsAvailable_.size() == 0 ) {
				if( useVoiceStealing_ ) {
					// Find the voice with the oldest timestamp and turn it off
					const int oldNote = oldestNote();
					int oldChannel = -1;
					if( retransmitChannelForNote_.count( oldNote ) > 0 )
						oldChannel = retransmitChannelForNote_[ oldNote ];
					if( oldNote < 0 ) {
						// Shouldn't happen...
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
						std::cout << "No notes present, but no MIDI output channel available for note " << ( int ) note << '\n';
#endif
						return;
					}
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
					std::cout << "Stealing note " << oldNote << " for note " << ( int ) note << '\n';
#endif
					modeMPENoteOff( oldNote, true );
				} else {
					// No channels available.  Print a warning and finish
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
					std::cout << "No MIDI output channel available for note " << ( int ) note << '\n';
#endif
					return;
				}
			}
		}

		// MPE-TODO round-robin as default rather than first available
		// "Simple circular assignment of new notes to Member Channels of a Zone will not usually provide satisfactory
		// results. In the simplest workable implementation, a new note will be assigned to the Channel with the lowest
		// count of active notes. Then, all else being equal, the Channel with the oldest last Note Off would be
		// preferred. This set of rules has at least one working real - world implementation."
		newChannel = *retransmitChannelsAvailable_.begin();
		retransmitChannelsAvailable_.erase( newChannel );
		retransmitChannelForNote_[ note ] = newChannel;
	}

	if( keyboard_.key( note ) != nullptr ) {
		keyboard_.key( note )->midiNoteOn( this, velocity, newChannel, keyboard_.schedulerCurrentTimestamp() );
	}

	// The above function will cause a callback to be generated, which in turn will generate
	// the Note On message.
}

// MPE-TODO Handle note off message in MPE mode.  Release any channel
// associated with this note.
void MidiKeyboardSegment::modeMPENoteOff( const uint8_t note, const bool forceOff ) {
	// If no channel associated with this note, ignore it
	if( retransmitChannelForNote_.count( note ) == 0 ) {
		if( note >= 0 && note < 128 )
			noteOnsetTimestamps_[ note ] = 0;
		return;
	}

	if( keyboard_.key( note ) != nullptr ) {
		keyboard_.key( note )->midiNoteOff( this, keyboard_.schedulerCurrentTimestamp() );
	}

	int oldNoteChannel = retransmitChannelForNote_[ note ];

	auto sendCC = [ & ]( const int byte1, const int byte2 )
	{
		midiOutputController_->sendControlChange( outputPortNumber_, oldNoteChannel, byte1, byte2 );
	};

	if( midiOutputController_ != nullptr ) {
		if( forceOff ) {
			// To silence a note, we need to clear any pedals that might be holding it
			if( controllerValues_[ kMidiControllerDamperPedal ] >= kPedalActiveValue ) {
				sendCC( kMidiControllerDamperPedal, 0 );
			}
			if( controllerValues_[ kMidiControllerSostenutoPedal ] >= kPedalActiveValue ) {
				sendCC( kMidiControllerSostenutoPedal, 0 );
			}

			// Send All Notes Off and All Sound Off
			sendCC( kMidiControlAllNotesOff, 0 );
			sendCC( kMidiControlAllSoundOff, 0 );
		} else {
			// Send a Note Off message to the appropriate channel
			midiOutputController_->sendNoteOff( outputPortNumber_, oldNoteChannel, note + outputTransposition_ );
		}
	}

	// If the pedal is enabled and currently active, don't re-enable this channel
	// just yet. Instead, let the note continue ringing until we have to steal it later.
	if( damperPedalEnabled_ && controllerValues_[ kMidiControllerDamperPedal ] >= kPedalActiveValue && !forceOff ) {
		retransmitNotesHeldInPedal_.insert( note );
	} else {
		// Otherwise release the channel mapping associated with this note
		if( retransmitNotesHeldInPedal_.count( note ) > 0 )
			retransmitNotesHeldInPedal_.erase( note );
		retransmitChannelsAvailable_.insert( retransmitChannelForNote_[ note ] );
		retransmitChannelForNote_.erase( note );
		if( note >= 0 && note < 128 )
			noteOnsetTimestamps_[ note ] = 0;
	}

	if( forceOff ) {
		// Now re-enable any pedals that we might have temporarily lifted on this channel
		if( controllerValues_[ kMidiControllerDamperPedal ] >= kPedalActiveValue ) {
			sendCC( kMidiControllerDamperPedal, controllerValues_[ kMidiControllerDamperPedal ] );
		}
		if( controllerValues_[ kMidiControllerSostenutoPedal ] >= kPedalActiveValue ) {
			sendCC( kMidiControllerSostenutoPedal, controllerValues_[ kMidiControllerSostenutoPedal ] );
		}
	}
}

// Private helper method to handle changes in polyphony
void MidiKeyboardSegment::modePolyphonicSetupHelper() {
	// Limit polyphony to 16 (number of MIDI channels) or fewer if starting above channel 1
	if(retransmitMaxPolyphony_ + outputChannelLowest_ > 16)
		retransmitMaxPolyphony_ = 16 - outputChannelLowest_;
	retransmitChannelsAvailable_.clear();
	for(int i = outputChannelLowest_; i < outputChannelLowest_ + retransmitMaxPolyphony_; ++i )
		retransmitChannelsAvailable_.insert(i);
	retransmitChannelForNote_.clear();
	retransmitNotesHeldInPedal_.clear();
}

// Find the oldest onset of the currently playing notes. Used for voice stealing.
// Returns -1 if no notes are playing.
int MidiKeyboardSegment::oldestNote() {
	int oldestNoteNumber = -1;
	timestamp_type oldestTimestamp = missing_value<timestamp_type>::missing();
	
	for(int i = 0; i < 128; i++) {
		const timestamp_type onsetTimestamp { noteOnsetTimestamps_[ i ] };

		if(missing_value<timestamp_type>::isMissing(oldestTimestamp) && onsetTimestamp != 0) {
			oldestNoteNumber = i;
			oldestTimestamp = onsetTimestamp;
		}
		else if( onsetTimestamp < oldestTimestamp && onsetTimestamp != 0) {
			oldestNoteNumber = i;
			oldestTimestamp = onsetTimestamp;
		}
	}
	
	return oldestNoteNumber;
}

// Finds the oldest onset of the notes currently finished but sustaining in the pedal.
// Used for voice stealing. Returns -1 if no notes are held in the pedal.
int MidiKeyboardSegment::oldestNoteInPedal() {
	if(!damperPedalEnabled_ || retransmitNotesHeldInPedal_.empty())
		return -1;
	
	int oldestNoteNumber = -1;
	timestamp_type oldestTimestamp = missing_value<timestamp_type>::missing();
	
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
	std::cout << "notes in pedal: ";
#endif
	
	for( const int note : retransmitNotesHeldInPedal_ ) {
		timestamp_type timestamp;
		if(noteOnsetTimestamps_[note] != 0)
			timestamp = noteOnsetTimestamps_[note];
		else
			timestamp = 0; // Why is there a note held in pedal with no onset? Steal it!
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
		std::cout << note << " (" << timestamp << ") ";
#endif
		
		if(missing_value<timestamp_type>::isMissing(oldestTimestamp)) {
			oldestNoteNumber = note;
			oldestTimestamp = timestamp;
		}
		else if(timestamp < oldestTimestamp) {
			oldestNoteNumber = note;
			oldestTimestamp = timestamp;
		}
	}
#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
	std::cout << '\n';
#endif
	
	return oldestNoteNumber;
}

// Find the newest onset of the currently playing notes. Used for monophonic mode.
// Returns -1 if no notes are playing.
int MidiKeyboardSegment::newestNote() {
	int newestNoteNumber = -1;
	timestamp_type newestTimestamp = missing_value<timestamp_type>::missing();
	
	for(int i = 0; i < 128; i++) {
		const timestamp_type timestamp { noteOnsetTimestamps_[i] };

		if(missing_value<timestamp_type>::isMissing(newestTimestamp) && timestamp != 0) {
			newestNoteNumber = i;
			newestTimestamp = timestamp;
		}
		else if(noteOnsetTimestamps_[i] > newestTimestamp && timestamp != 0) {
			newestNoteNumber = i;
			newestTimestamp = timestamp;
		}
	}
	
	return newestNoteNumber;
}

// Given a controller number (including special "controllers" channel-pressure and pitch-wheel),
// retransit or not to outgoing MIDI channels depending on the current behaviour defined in
// controllerActions_.
void MidiKeyboardSegment::handleControlChangeRetransit(int controllerNumber, const juce::MidiMessage& message) {
	// MPE-TODO need a new mode for sending on master zone, e.g. for pitch wheel
	if(midiOutputController_ == nullptr)
		return;

	if(controllerActions_[controllerNumber] == kControlActionPassthrough) {
		// Tell OSC-MIDI converter to resend if present, otherwise pass through
		if(oscMidiConverters_.count(controllerNumber) != 0) {
			oscMidiConverters_[controllerNumber]->resend(message.getChannel() - 1);
		}
		else {
			// Send this control change through unchanged
			midiOutputController_->sendMessage(outputPortNumber_, message);
		}
	}
	else if(controllerActions_[controllerNumber] == kControlActionBroadcast) {
		// Send this control change to all active channels
		juce::MidiMessage newMessage(message); // Modifiable copy of the original message
		
		if(oscMidiConverters_.count(controllerNumber) != 0) {
			for(int i = 0; i < retransmitMaxPolyphony_; i++)
				oscMidiConverters_[controllerNumber]->resend(i);
		}
		else {
			for(int i = 0; i < retransmitMaxPolyphony_; i++) {
				newMessage.setChannel(i + 1); // Juce uses 1-16, we use 0-15
				midiOutputController_->sendMessage(outputPortNumber_, newMessage);
			}
		}
	}
	else if(controllerActions_[controllerNumber] == kControlActionSendToLatest) {
		// Send this control change to the channel of the most recent note
		int noteNumber = newestNote();
		if(noteNumber < 0)
			return;
		if(keyboard_.key(noteNumber) != nullptr ) {
			int channel = keyboard_.key(noteNumber)->midiChannel();
			
			if(oscMidiConverters_.count(controllerNumber) != 0)
				oscMidiConverters_[controllerNumber]->resend(channel);
			else {
				juce::MidiMessage newMessage(message); // Modifiable copy of the original message
				newMessage.setChannel(channel + 1);  // Juce uses 1-16, we use 0-15
				midiOutputController_->sendMessage(outputPortNumber_, newMessage);
			}
		}
	}
	else {} // Block or unknown action
}

// Set all controllers to behave a particular way when messages received
void MidiKeyboardSegment::setAllControllerActionsTo(const int action) {
	for(int i = 0; i < kControlMax; i++)
		controllerActions_[i] = action;
}

// Pedal went off. If we're saving notes in the pedal, release them
void MidiKeyboardSegment::damperPedalWentOff() {
	if(!damperPedalEnabled_)
		return;
	// Go through a list of any notes currently in the damper pedal and release them
	for( const int note : retransmitNotesHeldInPedal_ ) {

#ifdef DEBUG_MIDI_KEYBOARD_SEGMENT
		std::cout << "releasing note " << note << " on channel " << retransmitChannelForNote_[note] << '\n';
#endif
		retransmitChannelsAvailable_.insert(retransmitChannelForNote_[note]);
		retransmitChannelForNote_.erase(note);
		noteOnsetTimestamps_[note] = 0;
	}
	retransmitNotesHeldInPedal_.clear();
}

// Handle the actual sending of the pitch wheel range RPN to a specific channel
void MidiKeyboardSegment::sendMidiPitchWheelRangeHelper(const int channel) {
	if(midiOutputController_ == nullptr)
		return;
	
	// Find number of semitones and cents
	const int majorRange = (int)floorf(pitchWheelRange_);
	const int minorRange = (int)(100.0f * (pitchWheelRange_ - floorf(pitchWheelRange_)));
	
	// Set RPN controller = 0
	midiOutputController_->sendControlChange(outputPortNumber_, channel, 101, 0);
	midiOutputController_->sendControlChange(outputPortNumber_, channel, 100, 0);
	// Set data value MSB/LSB for bend range in semitones
	midiOutputController_->sendControlChange(outputPortNumber_, channel, 6, majorRange);
	midiOutputController_->sendControlChange(outputPortNumber_, channel, 38, minorRange);
	// Set RPN controller back to 16383
	midiOutputController_->sendControlChange(outputPortNumber_, channel, 101, 127);
	midiOutputController_->sendControlChange(outputPortNumber_, channel, 100, 127);
}
