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

  RawSensorDisplay.h: simple graph for showing raw TouchKeys sensor values
*/

#pragma once

#include "../TouchKeys/PianoTypes.h"
#include "../Utility/Node.h"
#include "OpenGLJuceCanvas.h"
//#include <OpenGL/gl.h>
#include <cmath>
#include <iostream>
#include <vector>

// This class uses OpenGL to draw a bar graph of key sensor data,
// for the purpose of debugging and validating individual keys

class RawSensorDisplay : public OpenGLDisplayBase {
	// Internal data structures and constants
private:
	// Display margins
	static constexpr float kDisplaySideMargin = 0.5;
	static constexpr float kDisplayBottomMargin = 0.5;
	static constexpr float kDisplayTopMargin = 0.5;

	// Size of bar graphs and spacing
	static constexpr float kDisplayBarWidth = 0.5;
	static constexpr float kDisplayBarSpacing = 0.25;
	static constexpr float kDisplayBarHeight = 10.0;

	typedef struct {
		float x;
		float y;
	} Point;
	
public:
	RawSensorDisplay();
	
    // Set canvas for triggering rendering;
    void setCanvas(OpenGLJuceCanvas *canvas) { canvas_ = canvas; }
    void tellCanvasToRepaint();
    
	// Setup methods for display size and keyboard range
	void setDisplaySize(float width, float height);
    float aspectRatio() { return totalDisplayWidth_ / totalDisplayHeight_; }
	
	// Drawing methods
	void render();
	
	// Interaction methods
	void mouseDown(const float x, const float y);
	void mouseDragged(const float x, const float y);
	void mouseUp(const float x, const float y);
	void rightMouseDown(const float x, const float y);
	void rightMouseDragged(const float x, const float y);
	void rightMouseUp(const float x, const float y);
    
    // Data update methods
    void setDisplayData(std::vector<int> const& values);
    
private:
    // Convert mathematical XY coordinate space to drawing positions
    float graphToDisplayX(const float x);
    float graphToDisplayY(const float y);
    
	void refreshViewport();
	
	// Conversion from internal coordinate space to external pixel values and back
	Point screenToInternal(Point& inPoint);
	Point internalToScreen(Point& inPoint);
	
    
private:
	// Reference to object which handles rendering
	OpenGLJuceCanvas *canvas_ { nullptr };
    
	// Pixel resolution of the surrounding window
	float displayPixelWidth_ { 1.0f };
	float displayPixelHeight_ { 1.0f };

	// Size of the internal view (centered around origin)
	float totalDisplayWidth_ { 1.0f };
	float totalDisplayHeight_ { 1.0f };
	
	// Range of the graph axes
    float yMin_ { -10.0f };
	float yMax_ { 256.0f };                 
    
	// Synchronize access between data and display threads
	juce::CriticalSection displayMutex_;

	// Values to display as a bar graph
    std::vector<int> displayValues_;
};