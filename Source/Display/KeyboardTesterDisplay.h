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

  KeyboardTesterDisplay.h: A keyboard display for raw data that can be used
  for testing the functionality of individual TouchKeys sensors
*/

#pragma once

#ifdef ENABLE_TOUCHKEYS_SENSOR_TEST

#include "KeyboardDisplay.h"
#include "../TouchKeys/Osc.h"

class MainApplicationController;
class PianoKeyboard;

class KeyboardTesterDisplay : public KeyboardDisplay, public OscHandler {
private:
    static const int kNumSensorsPerKey;
    static const int kDefaultSensorThreshold;
    
public:
    KeyboardTesterDisplay(MainApplicationController& controller, PianoKeyboard& keyboard);
    ~KeyboardTesterDisplay() {}
    
    // Render the display
    void render();

    // Called when a given key is clicked by mouse
    void keyClicked(int key);
    
    // Set the threshold for a value considered active
    void setSensorThreshold(int threshold);
    
    // Set whether the given sensor is active or not
    void setSensorState(int key, int sensor, bool active);
    
    // Reset the detection state for a given key to all sensors off
    void resetSensorState(int key);
    
    // Check whether all the sensors are good on a given key
    bool allSensorsGood(int key);
    
    // OSC callback
    bool oscHandlerMethod(const char *path, const char *types, int numValues, lo_arg **values, void *data);
    
private:
    void drawSensorState(int key, float x, float y, float width, float height, bool white, float whiteOffset);

    MainApplicationController& controller_;
    PianoKeyboard& keyboard_;
    
    // Keep track of which sensors have registered a positive reading;
    // one bit per sensor (up to 26)
    unsigned int keySensorActive_[128];
    unsigned int keySensorGood_[128];
    int currentlyActiveKey_;
    int sensorThreshold_;
};

#endif  // ENABLE_TOUCHKEYS_SENSOR_TEST
