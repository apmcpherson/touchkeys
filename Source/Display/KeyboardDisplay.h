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

  KeyboardDisplay.h: displays the keyboard state, including active MIDI
  notes and current touch position and size.
*/
#pragma once

#include "../TouchKeys/KeyTouchFrame.h"
#include "OpenGLJuceCanvas.h"
#include <cmath>
#include <iostream>
#include <map>
//#include <OpenGL/gl.h>

// This class uses OpenGL to implement the actual drawing of the piano keyboard graphics.
// Graphics include the current state of each key and the touches on the surface.

class KeyboardDisplay : public OpenGLDisplayBase {
	// Internal data structures and constants
protected:
    // Display dimensions, normalized to the width of one white key
 
	static constexpr float kWhiteKeyFrontWidth = 1.0;
	static constexpr float kBlackKeyWidth = 0.5;
	static constexpr float kWhiteKeyFrontLength = 2.3;
	static constexpr float kWhiteKeyBackLength = 4.1;
	static constexpr float kBlackKeyLength = 4.0;
	static constexpr float kInterKeySpacing = 0.1;
	static constexpr float kAnalogSliderVerticalSpacing = 0.2;
	static constexpr float kAnalogSliderLength = 3.0;
	static constexpr float kAnalogSliderWidth = 0.4;
	static constexpr float kAnalogSliderMinimumValue = -0.2;
	static constexpr float kAnalogSliderMaximumValue = 1.2;
	static constexpr float kAnalogSliderZeroLocation = kAnalogSliderLength * ( 0.0 - kAnalogSliderMinimumValue ) / ( kAnalogSliderMaximumValue - kAnalogSliderMinimumValue );
	static constexpr float kAnalogSliderOneLocation = kAnalogSliderLength * ( 1.0 - kAnalogSliderMinimumValue ) / ( kAnalogSliderMaximumValue - kAnalogSliderMinimumValue );
    // Individual geometry for C, D, E, F, G, A, B, c'
    
    static const float kWhiteKeyBackOffsets[9];
    static const float kWhiteKeyBackWidths[9];
    
	// Display margins

	static constexpr float kDisplaySideMargin = 0.4;
	static constexpr float kDisplayBottomMargin = 0.8;
	static constexpr float kDisplayTopMargin = 0.8;
    
    // Key shape constants
    
    static const int kShapeForNote[12];
    static const int kWhiteToChromatic[7];
	static constexpr float kWhiteKeyFrontBackCutoff = ( 6.5 / 19.0 );
    
	// Touch constants
	static constexpr float kDisplayMinTouchSize = 0.1;
	static constexpr float kDisplayTouchSizeScaler = 0.5;
    
    
	typedef struct {
        bool  active;
		float locH;
		float locV1;
		float locV2;
		float locV3;
		float size1;
		float size2;
		float size3;
	} TouchInfo;
	
	typedef struct {
		float x;
		float y;
	} Point;
    
    typedef struct {
        int noteLow;
        int noteHigh;
        int divisions;
    } KeyDivision;
	
public:
	KeyboardDisplay();
    virtual ~KeyboardDisplay() {}
    
    // Set canvas for triggering rendering;
    void setCanvas(OpenGLJuceCanvas *canvas) { canvas_ = canvas; }
    void tellCanvasToRepaint();
	
	// Setup methods for display size and keyboard range
	void setKeyboardRange(int lowest, int highest);
	float keyboardAspectRatio() { return totalDisplayWidth_ / totalDisplayHeight_; }
	void setDisplaySize(float width, float height) override;
	
	// Drawing methods
	void render() override;
	
	// Interaction methods
	void mouseDown(const float x, const float y) override;
	void mouseDragged(const float x, const float y) override;
	void mouseUp(const float x, const float y) override;
	void rightMouseDown(const float x, const float y) override;
	void rightMouseDragged(const float x, const float y) override;
	void rightMouseUp(const float x, const float y) override;
	
	// Take action associated with clicking a key.  These are called within the mouse
	// methods but may also be called externally.
	virtual void keyClicked(const int key);
	virtual void keyRightClicked(const int key);
	
	// State-change methods
	void setTouchForKey(const int key, const KeyTouchFrame& touch);
	void clearTouchForKey(const int key);
	void clearAllTouches();
    void setAnalogCalibrationStatusForKey(const int key, const bool isCalibrated);
	void setAnalogValueForKey(const int key, const float value);
    void clearAnalogData();
    void setMidiActive(const int key, const bool active);
    void clearMidiData();
    
    void setAnalogSensorsPresent(const bool present) { analogSensorsPresent_ = present; }
	void setTouchSensorPresentForKey(const int key, const bool present);
	void setTouchSensingEnabled(const bool enabled);
    
    // Key division methods
    void addKeyDivision(void *who, int noteLow, int noteHigh, int divisions);
    void removeKeyDivision(void *who);
	
protected:
	void drawWhiteKey(float x, float y, int shape, bool first, bool last, bool highlighted, int divisions);
	void drawBlackKey(float x, float y, bool highlighted, int divisions);
	
	void drawWhiteTouch(float x, float y, int shape, float touchLocH, float touchLocV, float touchSize);
	void drawBlackTouch(float x, float y, float touchLocH, float touchLocV, float touchSize);
    
    void drawAnalogSlider(float x, float y, bool calibrated, bool whiteKey, float value);
	
	// Indicate the shape of the given MIDI note.  0-6 for white keys C-B, -1 for black keys.
	// We handle unusual shaped keys at the top or bottom of the keyboard separately.
	
	int keyShape(int key) { 
		if(key < 0) return -1;
		return kShapeForNote[key % 12]; 
	}
	
	void refreshViewport();
	
	// Conversion from internal coordinate space to external pixel values and back
	Point screenToInternal(Point& inPoint);
	Point internalToScreen(Point& inPoint);
	
	// Figure out which key (if any) the current point corresponds to
	int keyForLocation(Point& internalPoint);
    
    // Convert key division map into a number of divisions for each key
    void recalculateKeyDivisions();
		
protected:
	OpenGLJuceCanvas *canvas_ { nullptr };        // Reference to object which handles rendering
    
	int lowestMidiNote_ { 0 };
	int highestMidiNote_ { 0 };			// Which keys should be displayed (use MIDI note numbers)	
	float totalDisplayWidth_ { 1.0f };
	float totalDisplayHeight_ { 1.0f };	// Size of the internal view (centered around origin)
    float displayPixelWidth_ { 1.0f };
	float displayPixelHeight_ { 1.0f };	// Pixel resolution of the surrounding window
	int currentHighlightedKey_ { -1 };						// What key is being clicked on at the moment
	bool touchSensingEnabled_ { false };						// Whether touch-sensitive keys are being used
	bool touchSensingPresentOnKey_[128];			// Whether the key with this MIDI note has a touch sensor
    
    bool analogSensorsPresent_ { false };                     // Whether the given device has analog sensors at all
    bool analogValueIsCalibratedForKey_[128];       // Whether the analog sensor is calibrated on this key
    float analogValueForKey_[128];                  // Latest analog sensor value for each key
    bool midiActiveForKey_[128];                    // Whether the MIDI note is on for each key
	
    TouchInfo currentTouches_[128];                 // Touch data for each key
    TouchInfo currentTouchesMirror_[128];           // Mirror of the above, used for active display
    std::map<void*, KeyDivision> keyDivisions_;     // Division of keys into more than one segment, for certain mappings
    int keyDivisionsForNote_[128];                  // Number of key divisions per note
	juce::CriticalSection displayMutex_;					// Synchronize access between data and display threads
};