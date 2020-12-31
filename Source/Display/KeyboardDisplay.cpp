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

  KeyboardDisplay.cpp: displays the keyboard state, including active MIDI
  notes and current touch position and size.
*/

#include "KeyboardDisplay.h"
#include <OpenGL/gl.h>

// Class constants

// Individual geometry for C, D, E, F, G, A, B, c'

const float KeyboardDisplay::kWhiteKeyBackOffsets[9] = {0, 0.22, 0.42, 0, 0.14, 0.3, 0.44, 0.22, 0};
const float KeyboardDisplay::kWhiteKeyBackWidths[9] = {0.6, 0.58, 0.58, 0.56, 0.56, 0.56, 0.56, 0.58, 1.0};

// Key shape constants

const int KeyboardDisplay::kShapeForNote[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
const int KeyboardDisplay::kWhiteToChromatic[7] = {0, 2, 4, 5, 7, 9, 11};

KeyboardDisplay::KeyboardDisplay() {
	// Initialize OpenGL settings: 2D only
	  
	//glMatrixMode(GL_PROJECTION);
	//glDisable(GL_DEPTH_TEST);
	
    clearAllTouches();
    for(int i = 0; i < 128; i++)
        midiActiveForKey_[i] = false;
    
    recalculateKeyDivisions();
}

// Tell the underlying canvas to repaint itself
void KeyboardDisplay::tellCanvasToRepaint() {
    if(canvas_ != 0)
        canvas_->triggerRepaint();
}

void KeyboardDisplay::setKeyboardRange(int lowest, int highest) {
	if(lowest < 0 || highest < 0)
		return;
	
    juce::ScopedLock sl(displayMutex_);
    
	lowestMidiNote_ = lowest;
	if(keyShape(lowest) < 0)	// Lowest key must always be a white key for display to
		lowest++;				// render properly
	
	highestMidiNote_ = highest;
	
	// Recalculate relevant display parameters
	// Display size is based on the number of white keys
	
	int numKeys = 0;
	for(int i = lowestMidiNote_; i <= highestMidiNote_; i++) {
		if(keyShape(i) >= 0)
			numKeys++;
        if(i >= 0 && i < 128) {
            analogValueForKey_[i] = 0.0;
            analogValueIsCalibratedForKey_[i] = false;
        }
	}
	
	if(numKeys == 0) {
		return;
	}

	// Width: N keys, N-1 interkey spaces, 2 side margins
	totalDisplayWidth_ = (float)numKeys * (kWhiteKeyFrontWidth + kInterKeySpacing) 
	- kInterKeySpacing + 2.0 * kDisplaySideMargin;
	
	// Height: white key height plus top and bottom margins
    if(analogSensorsPresent_)
        totalDisplayHeight_ = kDisplayTopMargin + kDisplayBottomMargin + kWhiteKeyFrontLength + kWhiteKeyBackLength
                                + kAnalogSliderVerticalSpacing + kAnalogSliderLength;
    else
        totalDisplayHeight_ = kDisplayTopMargin + kDisplayBottomMargin + kWhiteKeyFrontLength + kWhiteKeyBackLength;
}

void KeyboardDisplay::setDisplaySize(float width, float height) { 
    juce::ScopedLock sl(displayMutex_);
    
	displayPixelWidth_ = width; 
	displayPixelHeight_ = height; 
	refreshViewport();
}


// Render the keyboard display

void KeyboardDisplay::render() {
	if(lowestMidiNote_ == highestMidiNote_)
		return;
	
	// Start with a light gray background
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	float invAspectRatio = totalDisplayWidth_ / totalDisplayHeight_; //displayPixelWidth_ / displayPixelHeight_;
	float scaleValue = 2.0 / totalDisplayWidth_;	
	
	glScalef(scaleValue, scaleValue * invAspectRatio, scaleValue);
	glTranslatef(-1.0 / scaleValue, -totalDisplayHeight_ / 2.0, 0);
	glTranslatef(kDisplaySideMargin, kDisplayBottomMargin, 0.0);
	
    //ScopedLock sl(displayMutex_);
    
	glPushMatrix();
    
	// Draw the keys themselves first, with analog values if present, then draw the touches
	for(int key = lowestMidiNote_; key <= highestMidiNote_; key++) {
		if(keyShape(key) >= 0) {
            if(key < 0 || key > 127) {
                // Safety check
                drawWhiteKey(0, 0, keyShape(key), key == lowestMidiNote_, key == highestMidiNote_,
                             false, 1);
                // Analog slider should be centered with respect to the back of the white key
                if(analogSensorsPresent_ && keyShape(key) >= 0) {
                    float sliderOffset = kWhiteKeyBackOffsets[keyShape(key)] + (kWhiteKeyBackWidths[keyShape(key)] - kAnalogSliderWidth) * 0.5;
                    drawAnalogSlider(sliderOffset, kWhiteKeyFrontLength + kWhiteKeyBackLength + kAnalogSliderVerticalSpacing,
                                     false, true, 0);
                }
            }
            else {
                // White keys: draw and move the frame over for the next key
                drawWhiteKey(0, 0, keyShape(key), key == lowestMidiNote_, key == highestMidiNote_,
                             /*(key == currentHighlightedKey_) ||*/ midiActiveForKey_[key], keyDivisionsForNote_[key]);
                // Analog slider should be centered with respect to the back of the white key
                if(analogSensorsPresent_ && keyShape(key) >= 0) {
                    float sliderOffset = kWhiteKeyBackOffsets[keyShape(key)] + (kWhiteKeyBackWidths[keyShape(key)] - kAnalogSliderWidth) * 0.5;
                    drawAnalogSlider(sliderOffset, kWhiteKeyFrontLength + kWhiteKeyBackLength + kAnalogSliderVerticalSpacing,
                                     analogValueIsCalibratedForKey_[key], true, analogValueForKey_[key]);
                }
            }
			glTranslatef(kWhiteKeyFrontWidth + kInterKeySpacing, 0, 0);
		}
		else {
			// Black keys: draw and leave the frame in place
			int previousWhiteKeyShape = keyShape(key - 1);
			float offsetH = -1.0 + kWhiteKeyBackOffsets[previousWhiteKeyShape] + kWhiteKeyBackWidths[previousWhiteKeyShape];
			float offsetV = kWhiteKeyFrontLength + kWhiteKeyBackLength - kBlackKeyLength;

			glTranslatef(offsetH, offsetV, 0.0);
            
            if(key < 0 || key > 127) {
                // Safety check
                drawBlackKey(0, 0, false, 1);
                if(analogSensorsPresent_) {
                    drawAnalogSlider((kBlackKeyWidth - kAnalogSliderWidth) * 0.5, kBlackKeyLength + kAnalogSliderVerticalSpacing,
                                     false, false, 0);
                }
            }
            else {
                drawBlackKey(0, 0, /*(key == currentHighlightedKey_) ||*/ midiActiveForKey_[key], keyDivisionsForNote_[key]);
                if(analogSensorsPresent_) {
                    drawAnalogSlider((kBlackKeyWidth - kAnalogSliderWidth) * 0.5, kBlackKeyLength + kAnalogSliderVerticalSpacing,
                                     analogValueIsCalibratedForKey_[key], false, analogValueForKey_[key]);
                }
            }
			glTranslatef(-offsetH, -offsetV, 0.0);
		}
	}
	
	// Restore to the original location we used when drawing the keys
	glPopMatrix();
	
    // Transfer the touch display data from storage buffer to a local copy we can use for display.
    // This avoids OpenGL calls happening while the mutex is locked, which stalls the data producer thread
    displayMutex_.enter();
    memcpy(currentTouchesMirror_, currentTouches_, 128*sizeof(TouchInfo));
    displayMutex_.exit();
    
	// Draw touches
	for(int key = lowestMidiNote_; key <= highestMidiNote_; key++) {
		if(keyShape(key) >= 0) {
			// Check whether there are any current touches for this key
			//if(currentTouches_.count(key) > 0) {
            if(currentTouchesMirror_[key].active) {
				TouchInfo& t = currentTouchesMirror_[key];
				
				if(t.locV1 >= 0)
					drawWhiteTouch(0, 0, keyShape(key), t.locH, t.locV1, t.size1);
				if(t.locV2 >= 0)
					drawWhiteTouch(0, 0, keyShape(key), t.locH, t.locV2, t.size2);
				if(t.locV3 >= 0)
					drawWhiteTouch(0, 0, keyShape(key), t.locH, t.locV3, t.size3);
			}
			
			glTranslatef(kWhiteKeyFrontWidth + kInterKeySpacing, 0, 0);			
		}
		else {
			// Black keys: draw and leave the frame in place
			int previousWhiteKeyShape = keyShape(key - 1);
			float offsetH = -1.0 + kWhiteKeyBackOffsets[previousWhiteKeyShape] + kWhiteKeyBackWidths[previousWhiteKeyShape];
			float offsetV = kWhiteKeyFrontLength + kWhiteKeyBackLength - kBlackKeyLength;
			
			glTranslatef(offsetH, offsetV, 0.0);
			
			// Check whether there are any current touches for this key
			//if(currentTouches_.count(key) > 0) {
            if(currentTouchesMirror_[key].active) {
				TouchInfo& t = currentTouchesMirror_[key];
				
				if(t.locV1 >= 0)
					drawBlackTouch(0, 0, t.locH, t.locV1, t.size1);
				if(t.locV2 >= 0)
					drawBlackTouch(0, 0, t.locH, t.locV2, t.size2);
				if(t.locV3 >= 0)
					drawBlackTouch(0, 0, t.locH, t.locV3, t.size3);
			}
			
			glTranslatef(-offsetH, -offsetV, 0.0);
		}
	}
    
	glFlush();
}

// Mouse interaction methods

void KeyboardDisplay::mouseDown(const float x, const float y) {
	Point mousePoint = {x, y};
	Point scaledPoint = screenToInternal(mousePoint);	
	
	currentHighlightedKey_ = keyForLocation(scaledPoint);
	tellCanvasToRepaint();
}

void KeyboardDisplay::mouseDragged(const float x, const float y) {
	Point mousePoint = {x, y};
	Point scaledPoint = screenToInternal(mousePoint);	
	
	currentHighlightedKey_ = keyForLocation(scaledPoint);
	tellCanvasToRepaint();
}

void KeyboardDisplay::mouseUp(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
	
	// When the mouse is released, see if it was over a key.  If so, take any action
	// associated with clicking that key.
	
	if(currentHighlightedKey_ != -1)
		keyClicked(currentHighlightedKey_);
	
	currentHighlightedKey_ = -1;
	tellCanvasToRepaint();	
}

void KeyboardDisplay::rightMouseDown(const float x, const float y) {
	Point mousePoint = {x, y};	
	Point scaledPoint = screenToInternal(mousePoint);
	
	int key = keyForLocation(scaledPoint);
	if(key != -1)
		keyRightClicked(key);
	
	tellCanvasToRepaint();
}

void KeyboardDisplay::rightMouseDragged( const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyboardDisplay::rightMouseUp( const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyboardDisplay::keyClicked( const int key) {
	
}

void KeyboardDisplay::keyRightClicked( const int key) {
	
}

// Insert new touch information for the given key and request a display update.

void KeyboardDisplay::setTouchForKey( const int key, const KeyTouchFrame& touch) {
	if(key < lowestMidiNote_ || key > highestMidiNote_)
		return;
    
    juce::ScopedLock sl(displayMutex_);
	
	//TouchInfo t = {touch.locH, touch.locs[0], touch.locs[1], touch.locs[2], touch.sizes[0], touch.sizes[1], touch.sizes[2]};
	//currentTouches_[key] = t;
	//currentTouches_[key] = {true, touch.locH, touch.locs[0], touch.locs[1], touch.locs[2], touch.sizes[0], touch.sizes[1], touch.sizes[2]};
    currentTouches_[key].active = true;
	currentTouches_[key].locH = touch.locH;
	currentTouches_[key].locV1 = touch.locs[0];
	currentTouches_[key].locV2 = touch.locs[1];
	currentTouches_[key].locV3 = touch.locs[2];
	currentTouches_[key].size1 = touch.sizes[0];
	currentTouches_[key].size2 = touch.sizes[1];
	currentTouches_[key].size3 = touch.sizes[2];

	tellCanvasToRepaint();
}

// Clear touch information for this key

void KeyboardDisplay::clearTouchForKey( const int key) {
    juce::ScopedLock sl(displayMutex_);
	
	//currentTouches_.erase(key);
    currentTouches_[key].active = 0;
    
	tellCanvasToRepaint();
}

// Clear all current touch information

void KeyboardDisplay::clearAllTouches() {
    juce::ScopedLock sl(displayMutex_);
	
	//currentTouches_.clear();
    for(int i = 0; i < 128; i++)
        currentTouches_[i].active = false;
    
	tellCanvasToRepaint();
}

// Indicate whether the given key is calibrated or not

void KeyboardDisplay::setAnalogCalibrationStatusForKey(int key, bool isCalibrated) {
    if(key < 0 || key > 127)
        return;
    analogValueIsCalibratedForKey_[key] = isCalibrated;
    tellCanvasToRepaint();
}

// Set the current value of the analog sensor for the given key.
// Whether calibrated or not, the data should be in the range 0.0-1.0
// with a bit of room for deviation outside that range (i.e. for extra key
// pressure > 1.0, or for resting key states slightly miscalibrated < 0.0).

void KeyboardDisplay::setAnalogValueForKey(int key, float value) {
    if(key < 0 || key > 127)
        return;
    analogValueForKey_[key] = value;
    tellCanvasToRepaint();
}

// Clear all the analog data for all keys
void KeyboardDisplay::clearAnalogData() {
    for(int key = 0; key < 128; key++) {
        analogValueForKey_[key] = 0.0;
    }
    tellCanvasToRepaint();
}

void KeyboardDisplay::setMidiActive(int key, bool active) {
    if(key < 0 || key > 127)
        return;
    midiActiveForKey_[key] = active;
    tellCanvasToRepaint();
}

void KeyboardDisplay::clearMidiData() {
    for(int key = 0; key < 128; key++)
        midiActiveForKey_[key] = false;
    tellCanvasToRepaint();
}

// Indicate whether a given key has touch sensing capability

void KeyboardDisplay::setTouchSensorPresentForKey(int key, bool present) {
	if(key < 0 || key > 127 || !touchSensingEnabled_)
		return;
	touchSensingPresentOnKey_[key] = present;
}

// Indicate whether touch sensing is active at all on the keyboard.
// Clear all key-specific information on whether a touch-sensing key is connected

void KeyboardDisplay::setTouchSensingEnabled(bool enabled) {
	touchSensingEnabled_ = enabled;
	
	for(int i = 0; i < 128; i++)
		touchSensingPresentOnKey_[i] = false;
}

// Key division methods: indicate that certain keys are divided into more than
// one segment on the display. Useful for certain mappings.

void KeyboardDisplay::addKeyDivision(void *who, int noteLow, int noteHigh, int divisions) {
    KeyDivision div;
    
    div.noteLow = noteLow;
    div.noteHigh = noteHigh;
    div.divisions = divisions;
    
    keyDivisions_[who] = div;
    
    recalculateKeyDivisions();
    tellCanvasToRepaint();
}

void KeyboardDisplay::removeKeyDivision(void *who) {
    if(keyDivisions_.count(who) == 0)
        return;
    keyDivisions_.erase(who);
    
    recalculateKeyDivisions();
    tellCanvasToRepaint();
}

// Draw the outline of a white key.  Shape ranges from 0-7, giving the type of white key to draw
// Coordinates give the lower-left corner of the key

void KeyboardDisplay::drawWhiteKey(float x, float y, int shape, bool first, bool last, bool highlighted, int divisions) {
	// First and last keys will have special geometry since there is no black key below
	// Figure out the precise geometry in this case...
	
	float backOffset, backWidth;
	
	if(first) {
		backOffset = 0.0;
		backWidth = kWhiteKeyBackOffsets[shape] + kWhiteKeyBackWidths[shape];
	}
	else if(last) {
		backOffset = kWhiteKeyBackOffsets[shape];
		backWidth = 1.0 - kWhiteKeyBackOffsets[shape];
	}
	else {
		backOffset = kWhiteKeyBackOffsets[shape];
		backWidth = kWhiteKeyBackWidths[shape]; 
	}
	
	// First draw white fill as two squares
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
	if(highlighted)
		glColor3f(1.0, 0.8, 0.8);
	else
		glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	
	glVertex2f(x, y);
	glVertex2f(x, y + kWhiteKeyFrontLength);
	glVertex2f(x + kWhiteKeyFrontWidth, y + kWhiteKeyFrontLength);
	glVertex2f(x + kWhiteKeyFrontWidth, y);
	
	glVertex2f(x + backOffset, y + kWhiteKeyFrontLength);
	glVertex2f(x + backOffset, y + kWhiteKeyFrontLength + kWhiteKeyBackLength);
	glVertex2f(x + backOffset + backWidth, y + kWhiteKeyFrontLength + kWhiteKeyBackLength);
	glVertex2f(x + backOffset + backWidth, y + kWhiteKeyFrontLength);
	
	glEnd();
	
	// Now draw the outline as black line segments
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.0, 0.0, 0.0);			
	glBegin(GL_POLYGON);
	
	glVertex2f(x, y);
    glVertex2f(x, y + kWhiteKeyFrontLength);
    glVertex2f(x + backOffset, y + kWhiteKeyFrontLength);
    glVertex2f(x + backOffset, y + kWhiteKeyFrontLength + kWhiteKeyBackLength);
    glVertex2f(x + backOffset + backWidth, y + kWhiteKeyFrontLength + kWhiteKeyBackLength);
    glVertex2f(x + backOffset + backWidth, y + kWhiteKeyFrontLength);
    glVertex2f(x + kWhiteKeyFrontWidth, y + kWhiteKeyFrontLength);
    glVertex2f(x + kWhiteKeyFrontWidth, y);

	glEnd();
    
    if(divisions > 1) {
        glColor3f(0.0, 0.0, 0.0);
        
        for(int i = 1; i < divisions; i++) {
            float ratio = (float)i / (float)divisions;
            if(ratio > kWhiteFrontBackCutoff) {
                glBegin(GL_LINES);
                glVertex2d(x + backOffset, y + (kWhiteKeyFrontLength + kWhiteKeyBackLength) * ratio);
                glVertex2d(x + backOffset + backWidth, y + (kWhiteKeyFrontLength + kWhiteKeyBackLength) * ratio);
                glEnd();
            }
            else {
                glBegin(GL_LINES);
                glVertex2d(x, y + (kWhiteKeyFrontLength + kWhiteKeyBackLength) * ratio);
                glVertex2d(x + kWhiteKeyFrontWidth, y + (kWhiteKeyFrontLength + kWhiteKeyBackLength) * ratio);
                glEnd();
            }
        }
    }
}

// Draw the outline of a black key, given its lower-left corner

void KeyboardDisplay::drawBlackKey(float x, float y, bool highlighted, int divisions) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if(highlighted)
		glColor3f(0.7, 0.0, 0.0);
	else
		glColor3f(0.0, 0.0, 0.0);					// Display color black
	glBegin(GL_POLYGON);
	
	glVertex2f(x, y);
    glVertex2f(x, y + kBlackKeyLength);
	glVertex2f(x + kBlackKeyWidth, y + kBlackKeyLength);
	glVertex2f(x + kBlackKeyWidth, y);
	
	glEnd();

    if(divisions > 1) {
        glColor3f(1.0, 1.0, 1.0);
        for(int i = 1; i < divisions; i++) {
            glBegin(GL_LINES);
            glVertex2d(x, y + kBlackKeyLength * (float)i / (float)divisions);
            glVertex2d(x + kBlackKeyWidth, y + kBlackKeyLength * (float)i / (float)divisions);
            glEnd();
        }
    }
}

// Draw a circle indicating a touch on the white key surface

void KeyboardDisplay::drawWhiteTouch(float x, float y, int shape, float touchLocH, float touchLocV, float touchSize) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(1.0, 0.0, 1.0);
	
	glBegin(GL_POLYGON);
	if(/*touchLocV < kWhiteKeyFrontBackCutoff && */touchLocH >= 0.0) { // FIXME: find a more permanent solution
		// Here, the touch is in a location that has both horizontal and vertical information.
		for(int i = 0; i < 360; i += 5) {
			glVertex2f(x + cosf((float)i*3.14159/180.0)*(kDisplayMinTouchSize + touchSize*kDisplayTouchSizeScaler)
					   + touchLocH*kWhiteKeyFrontWidth,
					   y + sinf((float)i*3.14159/180.0)*(kDisplayMinTouchSize + touchSize*kDisplayTouchSizeScaler)
					   + kWhiteKeyFrontLength*(touchLocV/kWhiteKeyFrontBackCutoff));
		}
	}
	else {
		// The touch is in the back part of the key, or for some reason lacks horizontal information
		for(int i = 0; i < 360; i += 5) {
			glVertex2f(x + cosf((float)i*3.14159/180.0)*(kDisplayMinTouchSize + touchSize*kDisplayTouchSizeScaler)
					   + kWhiteKeyBackOffsets[shape] + kWhiteKeyBackWidths[shape]/2,
					   y + sinf((float)i*3.14159/180.0)*(kDisplayMinTouchSize + touchSize*kDisplayTouchSizeScaler) 
					   + kWhiteKeyFrontLength + (kWhiteKeyBackLength*
												 ((touchLocV-kWhiteKeyFrontBackCutoff)/(1.0-kWhiteKeyFrontBackCutoff))));
		}		
	}
	glEnd();
}

// Draw a circle indicating a touch on the black key surface

void KeyboardDisplay::drawBlackTouch(float x, float y, float touchLocH, float touchLocV, float touchSize) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(0.0, 1.0, 0.0);
	
	glBegin(GL_POLYGON);
    
    if(touchLocH < 0.0)
        touchLocH = 0.5;

	for(int i = 0; i < 360; i += 5) {
		glVertex2f(x + cosf((float)i*3.14159/180.0)*(kDisplayMinTouchSize + touchSize*kDisplayTouchSizeScaler)
                   + touchLocH * kBlackKeyWidth,
				   y + sinf((float)i*3.14159/180.0)*(kDisplayMinTouchSize + touchSize*kDisplayTouchSizeScaler) + kBlackKeyLength*touchLocV);
	}	

	glEnd();
}

// Draw a slider bar indicating the current key analog position

void KeyboardDisplay::drawAnalogSlider(float x, float y, bool calibrated, bool whiteKey, float value) {
    // First some gray lines indicating the 0.0 and 1.0 marks
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.5, 0.5, 0.5);
    
	glBegin(GL_POLYGON);
	glVertex2f(x, y + kAnalogSliderZeroLocation);
    glVertex2f(x, y + kAnalogSliderOneLocation);
	glVertex2f(x + kAnalogSliderWidth, y + kAnalogSliderOneLocation);
	glVertex2f(x + kAnalogSliderWidth, y + kAnalogSliderZeroLocation);
	glEnd();
    
    // Draw a red box at the top for uncalibrated values
    if(!calibrated) {
        glColor3f(1.0, 0.0, 0.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_POLYGON);
        glVertex2f(x, y + kAnalogSliderOneLocation);
        glVertex2f(x, y + kAnalogSliderLength);
        glVertex2f(x + kAnalogSliderWidth, y + kAnalogSliderLength);
        glVertex2f(x + kAnalogSliderWidth, y + kAnalogSliderOneLocation);
        glEnd();
    }
    
    // Next the filled part indicating the specific value (same color as touches), then the outline
    if(whiteKey)
        glColor3f(1.0, 0.0, 1.0);
    else
        glColor3f(0.0, 1.0, 0.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_POLYGON);
    
    float locationForValue = kAnalogSliderLength * (value - kAnalogSliderMinimumValue) / (kAnalogSliderMaximumValue - kAnalogSliderMinimumValue);
    if(locationForValue < 0.0)
        locationForValue = 0.0;
    if(locationForValue > kAnalogSliderLength)
        locationForValue = kAnalogSliderLength;
    
    // Draw solid box from 0.0 to current value
    glVertex2f(x, y + kAnalogSliderZeroLocation);
    glVertex2f(x, y + locationForValue);
    glVertex2f(x + kAnalogSliderWidth, y + locationForValue);
    glVertex2f(x + kAnalogSliderWidth, y + kAnalogSliderZeroLocation);
	glEnd();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.0, 0.0, 0.0);
    
	glBegin(GL_POLYGON);
	glVertex2f(x, y);
    glVertex2f(x, y + kAnalogSliderLength);
	glVertex2f(x + kAnalogSliderWidth, y + kAnalogSliderLength);
	glVertex2f(x + kAnalogSliderWidth, y);
	glEnd();
}

void KeyboardDisplay::refreshViewport() {
	//glViewport(0, 0, displayPixelWidth_, displayPixelHeight_);
}

// Conversion from internal coordinate space to external pixel values and back

// Pixel values go from 0,0 (lower left) to displayPixelWidth_, displayPixelHeight_ (upper right)
// Internal values go from -totalDisplayWidth_/2, -totalDisplayHeight_/2 (lower left)
//   to totalDisplayWidth_/2, totalDisplayHeight_/2 (upper right)

// Pixel value in --> OpenGL value out
KeyboardDisplay::Point KeyboardDisplay::screenToInternal(Point& inPoint) {
	Point out;
	
	out.x = -totalDisplayWidth_*0.5 + (inPoint.x/displayPixelWidth_) * totalDisplayWidth_;
	out.y = -totalDisplayHeight_*0.5 + (inPoint.y/displayPixelHeight_) * totalDisplayHeight_;
	
	return out;
}

// OpenGL value in --> Pixel value out
KeyboardDisplay::Point KeyboardDisplay::internalToScreen(Point& inPoint) {
	Point out;
	
	out.x = ((inPoint.x + totalDisplayWidth_*0.5)/totalDisplayWidth_) * displayPixelWidth_;
	out.y = ((inPoint.y + totalDisplayHeight_*0.5)/totalDisplayHeight_) * displayPixelHeight_;
			  
	return out;
}

// Given an internal-coordinate representation, return the number of the key that it belongs
// in, otherwise return -1 if no key matches.

int KeyboardDisplay::keyForLocation(Point& internalPoint) {
    // std::cout << "(" << internalPoint.x << "," << internalPoint.y << ")\n";
    
	// First, check that the point is within the overall bounding box of the keyboard
	if(internalPoint.y < -totalDisplayHeight_*0.5 + kDisplayBottomMargin ||
	   internalPoint.y > totalDisplayHeight_*0.5 - kDisplayTopMargin)
		return -1;
	if(internalPoint.x < -totalDisplayWidth_*0.5 + kDisplaySideMargin ||
	   internalPoint.x > totalDisplayWidth_*0.5 - kDisplaySideMargin)
		return -1;
	
	// Now, look for the key region corresponding to this horizontal location
	// hLoc indicates the relative distance from the beginning of the first key
	
	float hLoc = internalPoint.x + totalDisplayWidth_*0.5 - kDisplaySideMargin;
	
	if(hLoc < 0.0)
		return -1;
	
	// normalizedHLoc indicates the index of the white key this touch is near.
	float normalizedHLoc = hLoc / (kWhiteKeyFrontWidth + kInterKeySpacing);
	
	// Two relevant regions: front of the white keys, back of the white keys with black keys
	// Distinguish them by vertical position.
	
	int shapeOfBottomKey = keyShape(lowestMidiNote_);						// White key index of lowest key
	int lowestC = (lowestMidiNote_ / 12) * 12;								// C below lowest key
	int whiteKeyNumber = floorf(normalizedHLoc);							// Number of white key
	int whiteOctaveNumber = (whiteKeyNumber + shapeOfBottomKey) / 7;		// Octave the key is in
	int chromaticKeyNumber = 12 * whiteOctaveNumber + kWhiteToChromatic[(whiteKeyNumber + shapeOfBottomKey) % 7];
	
	// Check if we're on the front area of the white keys, and if so, ignore points located in the gaps
	// between the keys
    
    // std::cout << "norm " << (-internalPoint.y + totalDisplayHeight_*0.5) << std::endl;
	
	if(-internalPoint.y + totalDisplayHeight_*0.5 - kDisplayBottomMargin <= kWhiteKeyFrontLength) {
		if(normalizedHLoc - floorf(normalizedHLoc) > kWhiteKeyFrontWidth / (kWhiteKeyFrontWidth + kInterKeySpacing))
			return -1;		
		return lowestC + chromaticKeyNumber;		
	}
	else {
		// Back of white keys, or black keys
		
		int whiteKeyShape = keyShape(chromaticKeyNumber);
		if(whiteKeyShape < 0)	// Shouldn't happen
			return -1;
		
		float locRelativeToLeft = (normalizedHLoc - floorf(normalizedHLoc)) * (kWhiteKeyFrontWidth + kInterKeySpacing);
		
		// Check if we are in the back region of the white key.  Handle the lowest and highest notes specially since
		// the white keys are generally wider on account of no adjacent black key.
		if(lowestC + chromaticKeyNumber == lowestMidiNote_) {
			if(locRelativeToLeft <= kWhiteKeyBackOffsets[whiteKeyShape] + kWhiteKeyBackWidths[whiteKeyShape])
				return lowestC + chromaticKeyNumber;
		}
		else if(lowestC + chromaticKeyNumber == highestMidiNote_) {
			if(locRelativeToLeft >= kWhiteKeyBackOffsets[whiteKeyShape])
				return lowestC + chromaticKeyNumber;
		}
		else if(locRelativeToLeft >= kWhiteKeyBackOffsets[whiteKeyShape] &&
		   locRelativeToLeft <= kWhiteKeyBackOffsets[whiteKeyShape] + kWhiteKeyBackWidths[whiteKeyShape]) {
			return lowestC + chromaticKeyNumber;	
		}
		
		// By now, we've established that we're not on top of a white key.  See if we align to a black key.
		// Watch the vertical gap between white and black keys
		if(-internalPoint.y + totalDisplayHeight_*0.5 - kDisplayBottomMargin <=
		   kWhiteKeyFrontLength + kWhiteKeyBackLength - kBlackKeyLength)
			return -1;
		
		// Is there a black key below this white key?
		if(keyShape(chromaticKeyNumber - 1) < 0) {		
			if(locRelativeToLeft <= kWhiteKeyBackOffsets[whiteKeyShape] - kInterKeySpacing &&
			   lowestC + chromaticKeyNumber > lowestMidiNote_)
				return lowestC + chromaticKeyNumber - 1;
		}
		// Or is there a black key above this white key?
		if(keyShape(chromaticKeyNumber + 1) < 0) {
			if(locRelativeToLeft >= kWhiteKeyBackOffsets[whiteKeyShape] + kWhiteKeyBackWidths[whiteKeyShape] + kInterKeySpacing
			   && lowestC + chromaticKeyNumber < highestMidiNote_)
				return lowestC + chromaticKeyNumber + 1;
		}
	}

	// If all else fails, assume we're not on any key
	return -1;
}

// Convert the map of keyboard segments with divisions into an array ordered by
// note number, to save time during display

void KeyboardDisplay::recalculateKeyDivisions() {
    // By default, 1 division per key
    for(int i = 0; i < 128; i++)
        keyDivisionsForNote_[i] = 1;
    
    // Increase divisions wherever we find a relevant mapping
    for( auto it = keyDivisions_.begin(); it != keyDivisions_.end(); ++it) {
        int start = it->second.noteLow;
        int end = it->second.noteHigh;
        int div = it->second.divisions;
        
        if(start < 0)
            start = 0;
        if(end > 127)
            end = 127;
        for(int i = start; i <= end; i++) {
            if(div > keyDivisionsForNote_[i])
                keyDivisionsForNote_[i] = div;
        }
    }
}
