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
 
  MidiKeyboardSegment.h: handles incoming MIDI data and certain input-output
  mappings for one segment of a keyboard. The keyboard may be divided up into
  any number of segments with different behaviors. An important role of this
  class is to manage the output channel allocation when using one MIDI channel
  per note (for example, to handle polyphonic pitch bend).
*/

#pragma once

#include "../Mappings/MappingFactorySplitter.h"
#include "PianoKeyboard.h"
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>


class OscMidiConverter;


struct MIDIChannelCompare
{
    explicit MIDIChannelCompare( const int ch );

    int channel;
};

bool operator==( const std::pair< int, int >& p, const MIDIChannelCompare& c );

bool operator==( const MIDIChannelCompare& c, const std::pair< int, int >& p );

// This class handles the processing of MIDI input data for a particular
// segment of the keyboard. It defines the processing mode and stores certain
// state information about active notes for this particular part of the keyboard.
// The MidiInputController class will use one or more of these segments to define
// keyboard behavior. In the case of a split keyboard arrangement, MIDI channel
// or note number might determine which segment takes ownership of a particular note.

class MidiKeyboardSegment : public OscHandler {
private:
    static const int kMidiControllerDamperPedal { 64 };
    static const int kMidiControllerSostenutoPedal { 66 };
    static const int kPedalActiveValue { 64 };

public:
    // Factories to use
    [[deprecated( "Use kMappingFactoryNames.size() instead." )]] const int kNumMappingFactoryTypes = 7;
    static const std::array< std::string, 7 > kMappingFactoryNames;

	// Operating modes for MIDI input on this segment
	enum [[deprecated( "Please use enum class Mode instead." )]] {
		ModeOff = 0,
		ModePassThrough,
		ModeMonophonic,
		ModePolyphonic,
        ModeMPE
	};
	
    // Operating modes for MIDI input on this segment
    enum class Mode
    {
        Off = 0,
        PassThrough,
        Monophonic,
        Polyphonic,
        MPE
    };

    // String names for each entry in the Mode enum
    static const std::map< Mode, std::string > modeNames;

    // MPE Zones
    enum class MPEZone
    {
        // MPE is switched off
        Off,
        // Lower zone: Master channel 0x00; potential member channels [0x01..0x0F]
        Lower,
        // Upper zone: Master channel 0x0F; potential member channels [0x00..0x0E]
        Upper,
        // Both zones enabled with respective master channels 0x00 and 0x0F; inbetween
        // channels can be assigned to one or the other zone (but not both)
        LowerAndUpper
    };

    // The MIDI Pitch Wheel is not handled by control change like the others,
    // but it is something we will want to map to.  Use a special control number
    // to designate mapping OSC to the Pitch Wheel.  Use 14 bit values when mapping
    // to this control. Similarly, we might want to map to channel aftertouch value.
    // The mechanics here are identical to 7-bit controllers.
    enum {
        kControlDisabled = -1,
        kControlPitchWheel = 128,
        kControlChannelAftertouch,
        kControlPolyphonicAftertouch,
        kControlMax
    };

    enum {
        kControlActionPassthrough = 0,
        kControlActionBroadcast,
        kControlActionSendToLatest,
        kControlActionBlock
    };

public:
	// Constructor
	explicit MidiKeyboardSegment(PianoKeyboard& keyboard);
    
    // Destructor
    ~MidiKeyboardSegment();
 
    // Set/query the output controller
	MidiOutputController* midiOutputController() { return midiOutputController_; }
	void setMidiOutputController(MidiOutputController* ct) { midiOutputController_ = ct; }
	
    // Check whether this MIDI message is for this segment
    bool respondsToMessage(const juce::MidiMessage& message);
    bool respondsToNote(int noteNumber);
    
    // Set which channels we listen to
	void enableChannel(const int channelNumber);
	void enableAllChannels();
	void disableChannel(const int channelNumber);
	void disableAllChanels();
    void setChannelMask(const int channelMask) { channelMask_ = channelMask; }
    
    // Set which notes we listen to
    void setNoteRange(const int minNote, const int maxNote);
    std::pair<int, int> noteRange() const { return std::make_pair(noteMin_, noteMax_); }
    
    // Set whether or not we use aftertouch, pitchwheel or other controls
    // directly from the keyboard
    bool usesKeyboardChannnelPressure() const { return usesKeyboardChannelPressure_; }
    void setUsesKeyboardChannelPressure(const bool use) {
        usesKeyboardChannelPressure_ = use;
        // Reset to default if not using
        if(!use)
            controllerValues_[kControlChannelAftertouch] = 0;
    }
    
    bool usesKeyboardPitchWheel() const { return usesKeyboardPitchWheel_; }
    void setUsesKeyboardPitchWheel(const bool use) {
        usesKeyboardPitchWheel_ = use;
        // Reset to default if not using
        if(!use)
            controllerValues_[kControlPitchWheel] = 8192;
    }

    bool usesKeyboardModWheel() const { return usesKeyboardModWheel_; }
    void setUsesKeyboardModWheel(const bool use) {
        usesKeyboardModWheel_ = use;
        // Reset to default if not using
        if(!use) {
            controllerValues_[1] = 0;
        }
    }
    
    bool usesKeyboardPedals() const { return usesKeyboardPedals_; }
    void setUsesKeyboardPedals(const bool use) {
        usesKeyboardPedals_ = use;
        // Reset to default if not using
        if(!use) {
            // MIDI CCs 64 to 69 are for pedals
            for(int i = 64; i <= 69; i++)
                controllerValues_[i] = 0;
        }
    }
    
    bool usesKeyboardMIDIControllers() const { return usesKeyboardMidiControllers_; }
    void setUsesKeyboardMIDIControllers(const bool use) {
        usesKeyboardMidiControllers_ = use;
        // Reset to default if not using
        if(!use) {
            for(int i = 2; i < 128; i++)
                controllerValues_[i] = 0;
        }
    }
    
    // Get or set the MIDI pitch wheel range in semitones, and optionally send an RPN
    // message announcing its new value.
    float midiPitchWheelRange() const { return pitchWheelRange_; }
    void setMidiPitchWheelRange(const float semitones, const bool send = false);
    void sendMidiPitchWheelRange();
    
    // TouchKeys standalone mode generates MIDI note onsets from touch data
    // without needing a MIDI keyboard
    void enableTouchkeyStandaloneMode();
    void disableTouchkeyStandaloneMode();
    bool touchkeyStandaloneModeEnabled() const { return touchkeyStandaloneMode_; }
    
    // All Notes Off: can be sent by MIDI or controlled programmatically
	void allNotesOff();
    
    // Query the value of a controller
    int controllerValue( const int index) {
        if(index < 0 || index >= kControlMax)
            return 0;
        return controllerValues_[index];
    }
    
    // Reset MIDI controller values to defaults
    void resetControllerValues();
	
	// Change or query the operating mode of the controller
	Mode mode() const { return mode_; }
    void setMode(const int mode);
	void setModeOff();
	void setModePassThrough();
    void setModeMonophonic();
	void setModePolyphonic();
    void setModeMPE();
    
    // Get/set polyphony and voice stealing for polyphonic mode
    int polyphony() const { return retransmitMaxPolyphony_; }
    void setPolyphony(const int polyphony);
    bool voiceStealingEnabled() const { return useVoiceStealing_; }
    void setVoiceStealingEnabled(const bool enable) { useVoiceStealing_ = enable; }
    
    // Get/set the number of the output port that messages on this segment should go to
    int outputPort() const { return outputPortNumber_; }
    void setOutputPort(const int port) { outputPortNumber_ = port; }
    
    // Get/Set the minimum MIDI channel that should be used for output
    // MPE Mode, Lower Zone: 1-15
    // Everything else: 0-15
    int outputChannelLowest() const { return outputChannelLowest_; }
    void setOutputChannelLowest(const int ch);
    
    // Get set the output transposition in semitones, relative to input MIDI notes
    int outputTransposition() const { return outputTransposition_; }
    void setOutputTransposition(const int trans) { outputTransposition_ = trans; }
    
    // Whether the damper pedal is enabled in note channel allocation
    bool damperPedalEnabled() const { return damperPedalEnabled_; }
    void setDamperPedalEnabled(const bool enable);
    
    // MIDI handler routine
    void midiHandlerMethod(juce::MidiInput* source, const juce::MidiMessage& message);
    
    // OSC method: used to get touch callback data from the keyboard
	bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
    // OSC control method: called separately via the MidiInputController to manipulate
    // control parameters of this object
    OscMessage* oscControlMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
    // **** Mapping-related methods *****
    
    // OSC-MIDI converters: request and release methods. The acquire method
    // will create a converter if it does not already exist, or return an existing
    // one if it does. The release method will release the object when the
    // acquirer no longer needs it.    
    OscMidiConverter* acquireOscMidiConverter(int controlId);
    void releaseOscMidiConverter(int controlId);
    
    // *** Mapping methods ***
    // Return the number of mapping factory types available
    static int numberOfMappingFactories();
    
    // Return the name of a given mapping factory type
    static juce::String mappingFactoryNameForIndex(int index);
    
    // Whether a given mapping is experimental
    static bool mappingIsExperimental(int index);
    
    // Create a new mapping factory of the given type, attached to
    // the supplied segment
    MappingFactory* createMappingFactoryForIndex(int index);
    
    // Create a new mapping factory for this segment. A pointer should be passed in
    // of a newly-allocated object. It will be released upon removal.
    void addMappingFactory(MappingFactory* factory, bool autoGenerateName = false);
    
    // Remove a mapping factory, releasing the associated object.
    void removeMappingFactory(MappingFactory* factory);
    
    // Remove all mapping factories, releasing each one
    void removeAllMappingFactories();
    
    // Return a list of current mapping factories.
    std::vector<MappingFactory*> const& mappingFactories();

    // Return the specific index of this mapping factory
    int indexOfMappingFactory(MappingFactory *factory);
    
    // Return a unique identifier of the mapping state, so we know when something has changed
    int mappingFactoryUniqueIdentifier() const { return mappingFactoryUniqueIdentifier_; }
    
    // **** Preset methods ****
    
    // Get an XML element describing current settings (for saving presets)
    std::unique_ptr< juce::XmlElement > getPreset();
    
    // Load settings from an XML element
    bool loadPreset(juce::XmlElement const* preset);

private:
	// Mode-specific MIDI input handlers
	void modePassThroughHandler(juce::MidiInput* source, const juce::MidiMessage& message);
	void modeMonophonicHandler(juce::MidiInput* source, const juce::MidiMessage& message);
    
	void modePolyphonicHandler(juce::MidiInput* source, const juce::MidiMessage& message);
	void modePolyphonicNoteOn(const uint8_t note, const uint8_t velocity);
	void modePolyphonicNoteOff(const uint8_t note, const bool forceOff = false);
	void modePolyphonicMPENoteOnCallback(const char *path, const char *types, int numValues, lo_arg **values);

    /*
    The MPE Configuration message( AMEI / MMA CA - 034 ) is defined as RPN "00 06".
    The MSB of Data Entry represents the number of MIDI Channels assigned, as explained below. The
    LSB of Data Entry is not used.

    [ REGISTERED PARAMETER NUMBER ]
    MSB     LSB     Function
    =======================================
    00      06      MPE Configuration RPN
    Message Format : [ Bn 64 06 ] [ Bn 65 00 ] [ Bn 06 < mm > ]

    n = MIDI Channel Number
    n = 0 : Lower Zone Master Channel
    n = F : Upper Zone Master Channel
    All other values are invalid and should be ignored

    mm = Number of Member MIDI Channels in the Zone
    mm = 0 : MPE is Off( No Channels )
    mm = 1 to F : Assigns that number of MIDI Channels to the Zone ( see below )
    */
    void modeMPEsendConfigurationMessage( const MPEZone& z = MPEZone::Lower, const int singleZoneRange = 0x0F, const int dualZoneRange = 0x07 );
    void modeMPEHandler(juce::MidiInput* source, const juce::MidiMessage& message);
    void modeMPENoteOn(const uint8_t note, const uint8_t velocity);
    void modeMPENoteOff(const uint8_t note, const bool forceOff = false);

    // Helper functions for polyphonic mode
    void modePolyphonicSetupHelper();
    int oldestNote();
    int oldestNoteInPedal();
    int newestNote();
    
    // Methods for managing controllers
    void handleControlChangeRetransit(int controllerNumber, const juce::MidiMessage& message);
    void setAllControllerActionsTo(const int action);
    
    // Handle action of the damper pedal: when released, clear out any notes held there
    void damperPedalWentOff();
    
    // Send pitch wheel range to a specific channel
    [[deprecated( "Defined as lambda inside sendMidiPitchWheelRange()." )]]
    void sendMidiPitchWheelRangeHelper(const int channel);
    
    // ***** Member Variables *****
    
    PianoKeyboard& keyboard_;						// Reference to main keyboard data
	MidiOutputController *midiOutputController_;	// Destination for MIDI output
    int outputPortNumber_ {};                       // Which port to use on the output controller
    std::vector<MappingFactory*> mappingFactories_; // Collection of mappings for this segment
    MappingFactorySplitter mappingFactorySplitter_; // ...and a splitter class to facilitate communication
    int mappingFactoryUniqueIdentifier_ {};         // Unique ID indicating mapping factory changes

	Mode mode_ { Mode::Off };                       // Current operating mode of the segment

    // If mode_ is set to Mode::MPE, mpeZone_ becomes MPEZone::Lower
    MPEZone mpeZone_ { MPEZone::Off };              
                                                    
    unsigned int channelMask_ {};                   // Which channels we listen to (1 bit per channel)
    int noteMin_ { 0 };                             // Ranges of the notes we respond to
    int noteMax_ { 127 };                         
    int outputChannelLowest_ {};                    // Lowest (or only) MIDI channel we send to
    int outputTransposition_ {};                    // Transposition of notes at output
    bool damperPedalEnabled_ { true };              // Whether to handle damper pedal events in allocating channels
    bool touchkeyStandaloneMode_ { false };         // Whether we emulate MIDI data from TouchKeys
    bool usesKeyboardChannelPressure_ { false };    // Whether this segment passes aftertouch from the keyboard
    bool usesKeyboardPitchWheel_ { false };         // Whether this segment passes pitchwheel from the keyboard
    bool usesKeyboardModWheel_ { false };           // Whether this segment passes CC 1 (mod wheel) from keyboard
    bool usesKeyboardPedals_ { true };              // Whether this segment passes CCs 64-69 (pedals) from the keyboard
    bool usesKeyboardMidiControllers_ { false };    // Whether this segment passes other controllers
    float pitchWheelRange_ { 2.0f };                // Range of MIDI pitch wheel (in semitones)
    
    int controllerValues_[kControlMax];             // Values of MIDI controllers from input device
    int controllerActions_[kControlMax];            // What to do with MIDI CCs when they come in

    // Mapping between input notes and output channels.  Depending on the mode of operation,
	// each note may be rebroadcast on its own MIDI channel.  Need to keep track of what goes where.
	// key is MIDI note #, value is output channel (0-15)
    // NOTE When in MPE Mode, value is output channel [1..15] (channel 0 is Master Channel in Lower Zone)
    std::map<int, int> retransmitChannelForNote_;
    std::set<int> retransmitChannelsAvailable_;
    std::set<int> retransmitNotesHeldInPedal_;
	int retransmitMaxPolyphony_;
    bool useVoiceStealing_ { false };
    timestamp_type noteOnsetTimestamps_[128];       // When each currently active note began, for stealing
    
    // OSC-MIDI conversion objects for use with data mapping. These are stored in each
    // keyboard segment and specific mapping factories can request one when needed.
    std::map<int, OscMidiConverter*> oscMidiConverters_;
    std::map<int, int> oscMidiConverterReferenceCounts_;
};
