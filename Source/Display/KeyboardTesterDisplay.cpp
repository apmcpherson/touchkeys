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

  KeyboardTesterDisplay.cpp: A keyboard display for raw data that can be used
  for testing the functionality of individual TouchKeys sensors
*/

#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST

#include "KeyboardTesterDisplay.h"
#include "../MainApplicationController.h"
#include "../TouchKeys/PianoKeyboard.h"

const int KeyboardTesterDisplay::kNumSensorsPerKey = 26;
const int KeyboardTesterDisplay::kDefaultSensorThreshold = 16;

// Constructor
KeyboardTesterDisplay::KeyboardTesterDisplay(MainApplicationController& controller, PianoKeyboard& keyboard)
: controller_(controller), keyboard_(keyboard),
  currentlyActiveKey_(-1), sensorThreshold_(kDefaultSensorThreshold) {
    for(int i = 0; i < 128; i++)
        resetSensorState(i);
    
    setOscController(&keyboard_);
    addOscListener("/touchkeys/rawbytes");
}

// Render the display starting with the underlying keyboard and
// then adding our own display info on top
void KeyboardTesterDisplay::render() {
	if(lowestMidiNote_ == highestMidiNote_)
		return;
	
	// Start with a light gray background
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
    
	float invAspectRatio = totalDisplayWidth_ / totalDisplayHeight_;
	float scaleValue = 2.0 / totalDisplayWidth_;
	
	glScalef(scaleValue, scaleValue * invAspectRatio, scaleValue);
	glTranslatef(-1.0 / scaleValue, -totalDisplayHeight_ / 2.0, 0);
	glTranslatef(kDisplaySideMargin, kDisplayBottomMargin, 0.0);
	
	glPushMatrix();
    
	// Draw the keys themselves first, with analog values if present, then draw the touches
	for(int key = lowestMidiNote_; key <= highestMidiNote_; key++) {
		if(keyShape(key) >= 0) {
			// White keys: draw and move the frame over for the next key
			drawWhiteKey(0, 0, keyShape(key), key == lowestMidiNote_,
                         key == highestMidiNote_, (key == currentlyActiveKey_) || (key == currentHighlightedKey_), 1);
            // Draw sensor state for this key
            drawSensorState(key, 0, 0, kWhiteKeyBackWidths[keyShape(key)], kWhiteKeyFrontLength + kWhiteKeyBackLength,
                            true, kWhiteKeyBackOffsets[keyShape(key)]);
			glTranslatef(kWhiteKeyFrontWidth + kInterKeySpacing, 0, 0);
		}
		else {
			// Black keys: draw and leave the frame in place
			int previousWhiteKeyShape = keyShape(key - 1);
			float offsetH = -1.0 + kWhiteKeyBackOffsets[previousWhiteKeyShape] + kWhiteKeyBackWidths[previousWhiteKeyShape];
			float offsetV = kWhiteKeyFrontLength + kWhiteKeyBackLength - kBlackKeyLength;
            
			glTranslatef(offsetH, offsetV, 0.0);
			drawBlackKey(0, 0, (key == currentlyActiveKey_) || (key == currentHighlightedKey_), 1);
            // Draw sensor state for this key
            drawSensorState(key, 0, 0, kBlackKeyWidth, kBlackKeyLength, false, 0);
			glTranslatef(-offsetH, -offsetV, 0.0);
		}
	}
	
	// Restore to the original location we used when drawing the keys
	glPopMatrix();
	glFlush();
}

// Called when a given key is clicked by mouse
void KeyboardTesterDisplay::keyClicked(int key) {
    controller_.touchkeySensorTestSetKey(key);
}

// Set the threshold level at which a sensor is considered active
void KeyboardTesterDisplay::setSensorThreshold(int threshold) {
    sensorThreshold_ = threshold;
}

// Set the state of a given sensor on a given key to be on or off,
// based on some externally-computed threshold. Sensors that are on
// will flip the "good" flag to true, which remains set until cleared
// externally.
void KeyboardTesterDisplay::setSensorState(int key, int sensor, bool active) {
    if(key < 0 || key > 127)
        return;
    if(sensor < 0 || sensor >= kNumSensorsPerKey)
        return;
    if(active) {
        keySensorActive_[key] |= (1 << sensor);
        keySensorGood_[key] |= (1 << sensor);
    }
    else
        keySensorActive_[key] &= ~(1 << sensor);
    currentlyActiveKey_ = key;
    tellCanvasToRepaint();
    
    if(allSensorsGood(currentlyActiveKey_)) {
        controller_.touchkeySensorTestSetKey(key + 1);
    }
}

// Indicate whether all sensors have shown an active value on this key
bool KeyboardTesterDisplay::allSensorsGood(int key) {
    if(key < 0 || key > 127)
        return false;
    unsigned int mask = (1 << kNumSensorsPerKey) - 1;
    
    return ((keySensorGood_[key] & mask) == mask);
}

// Reset the sensor state to all off
void KeyboardTesterDisplay::resetSensorState(int key) {
    if(key < 0 || key > 127)
        return;
    keySensorGood_[key] = 0;
    keySensorActive_[key] = 0;
    if(currentlyActiveKey_ == key)
        currentlyActiveKey_ = -1;
}

// Draw the given key sensors as being active, good, or inactive
void KeyboardTesterDisplay::drawSensorState(int key, float x, float y, float width, float height, bool white, float whiteOffset) {
    float heightInset = height / ((float)kNumSensorsPerKey * 10);
    
    glPushMatrix();
    glTranslatef(x, y, 0);
    
    if(white) {
        float hSensorWidth = kWhiteKeyFrontWidth * 0.25;
        
        // Draw the first four sensors horizontally for the white keys
        glPushMatrix();
        
        for(int i = 0; i < 4; i++) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if(keySensorActive_[key] & (1 << i) && key == currentlyActiveKey_)
                glColor3f(1.0, 1.0, 0.0);   // Sensor active right now = yellow
            else if(keySensorGood_[key] & (1 << i))
                glColor3f(0.0, 1.0, 0.0);   // Sensor has been active (good) = green
            else
                glColor3f(1.0, 0.0, 0.0);   // Sensor has not yet been active = red
            
            glBegin(GL_POLYGON);
            glVertex2f(hSensorWidth * 0.1, heightInset);
            glVertex2f(hSensorWidth * 0.1, 4.0*height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(hSensorWidth * 0.9, 4.0*height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(hSensorWidth * 0.9, heightInset);
            glEnd();
            glTranslatef(hSensorWidth, 0, 0);
        }
        
        glPopMatrix();
        glTranslatef(whiteOffset, 4.0*height / (float)kNumSensorsPerKey, 0);
        
        for(int i = 4; i < kNumSensorsPerKey; i++) {
            // Draw each sensor in sequence: red = never activated; yellow = active; green = previously
            // activated (good)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if(keySensorActive_[key] & (1 << i) && key == currentlyActiveKey_)
                glColor3f(1.0, 1.0, 0.0);   // Sensor active right now = yellow
            else if(keySensorGood_[key] & (1 << i))
                glColor3f(0.0, 1.0, 0.0);   // Sensor has been active (good) = green
            else
                glColor3f(1.0, 0.0, 0.0);   // Sensor has not yet been active = red
            
            glBegin(GL_POLYGON);
            glVertex2f(width * 0.1, heightInset);
            glVertex2f(width * 0.1, height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(width * 0.9, height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(width * 0.9, heightInset);
            glEnd();
            
            glTranslatef(0, height / (float)kNumSensorsPerKey, 0);
        }
    }
    else { // Black
        // Draw in two rows
        glPushMatrix();
        for(int i = 0; i < kNumSensorsPerKey / 2; i++) {
            // Draw each sensor in sequence: red = never activated; yellow = active; green = previously
            // activated (good)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if(keySensorActive_[key] & (1 << i) && key == currentlyActiveKey_)
                glColor3f(1.0, 1.0, 0.0);   // Sensor active right now = yellow
            else if(keySensorGood_[key] & (1 << i))
                glColor3f(0.0, 1.0, 0.0);   // Sensor has been active (good) = green
            else
                glColor3f(1.0, 0.0, 0.0);   // Sensor has not yet been active = red
            
            glBegin(GL_POLYGON);
            glVertex2f(width * 0.55, heightInset);
            glVertex2f(width * 0.55, 2.0*height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(width * 0.9, 2.0*height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(width * 0.9, heightInset);
            glEnd();
            
            glTranslatef(0, 2.0*height / (float)kNumSensorsPerKey, 0);
        }
        glPopMatrix();
        glPushMatrix();
        for(int i = kNumSensorsPerKey / 2; i < kNumSensorsPerKey; i++) {
            // Draw each sensor in sequence: red = never activated; yellow = active; green = previously
            // activated (good)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if(keySensorActive_[key] & (1 << i) && key == currentlyActiveKey_)
                glColor3f(1.0, 1.0, 0.0);   // Sensor active right now = yellow
            else if(keySensorGood_[key] & (1 << i))
                glColor3f(0.0, 1.0, 0.0);   // Sensor has been active (good) = green
            else
                glColor3f(1.0, 0.0, 0.0);   // Sensor has not yet been active = red
            
            glBegin(GL_POLYGON);
            glVertex2f(width * 0.1, heightInset);
            glVertex2f(width * 0.1, 2.0*height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(width * 0.45, 2.0*height / (float)kNumSensorsPerKey - heightInset);
            glVertex2f(width * 0.45, heightInset);
            glEnd();
            
            glTranslatef(0, 2.0*height / (float)kNumSensorsPerKey, 0);
        }
        glPopMatrix();
    }
    glPopMatrix();
}

// OSC callback method, for when data comes in
bool KeyboardTesterDisplay::oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data) {
    // Look for a blob in value 0 holding the raw data
    if(numValues < 1)
        return false;
    if(types[0] != 'b')
        return false;
    
    // Get OSC blob which holds raw data
    lo_blob blob = values[0];
    int bufferSize = lo_blob_datasize(blob);
    const unsigned char *buffer = (const unsigned char *)lo_blob_dataptr(blob);
    
    // buffer[0] holds the key number from which this data came. Make sure it's sane.
    if(bufferSize == 0)
        return false;
    if(buffer[0] > 127)
        return false;
    
    // The remainder is raw data, with each single byte corresponding to a sensor.
    for(int i = 1; i < bufferSize; i++) {
        bool active = (buffer[i] >= sensorThreshold_);
        setSensorState(buffer[0], i - 1, active);
    }
    
    return true;
}

#endif // ENABLE_TOUCHKEYS_SENSOR_TEST