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

  KeyPositionGraphDisplay.h: implements a graph of key position over time
  for a specific key press, which is useful for analysis of continuous key
  position.
*/

#pragma once

#include "../TouchKeys/PianoTypes.h"
#include "../Utility/Node.h"
#include "OpenGLJuceCanvas.h"
#include <cmath>
#include <iostream>
#include <vector>

// This class uses OpenGL to handle drawing of a graph showing key position
// over time, for debugging or analysis purposes.

class KeyPositionGraphDisplay : public OpenGLDisplayBase {
	// Internal data structures and constants
private:

   // Display margins
    static constexpr float kDisplaySideMargin = 0.5;
    static constexpr float kDisplayBottomMargin = 0.5;
    static constexpr float kDisplayTopMargin = 0.5;

    // Size of the graph area
    static constexpr float kDisplayGraphWidth = 20.0;
    static constexpr float kDisplayGraphHeight = 10.0;

	typedef struct {
		float x;
		float y;
	} Point;
	
public:
	KeyPositionGraphDisplay();
	
    // Set canvas for triggering rendering;
    void setCanvas(OpenGLJuceCanvas *canvas) override { canvas_ = canvas; }
    void tellCanvasToRepaint();
    
	// Setup methods for display size and keyboard range
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
    
    // Data update methods
    void copyKeyDataFromBuffer(Node<key_position>& keyBuffer, const Node<key_position>::size_type startIndex,
                               const Node<key_position>::size_type endIndex);
    void setKeyPressStart(key_position position, timestamp_type timestamp) {
        pressStartTimestamp_ = timestamp;
        pressStartPosition_ = position;
        tellCanvasToRepaint();
    }
    void setKeyPressFinish(key_position position, timestamp_type timestamp) {
        pressFinishTimestamp_ = timestamp;
        pressFinishPosition_ = position;
        tellCanvasToRepaint();
    }
    void setKeyReleaseStart(key_position position, timestamp_type timestamp) {
        releaseStartTimestamp_ = timestamp;
        releaseStartPosition_ = position;
        tellCanvasToRepaint();
    }
    void setKeyReleaseFinish(key_position position, timestamp_type timestamp) {
        releaseFinishTimestamp_ = timestamp;
        releaseFinishPosition_ = position;
        tellCanvasToRepaint();
    }
    
private:
    // Convert mathematical XY coordinate space to drawing positions
    float graphToDisplayX(float x);
    float graphToDisplayY(float y);
    
	void refreshViewport();
	
	// Conversion from internal coordinate space to external pixel values and back
	Point screenToInternal(Point& inPoint);
	Point internalToScreen(Point& inPoint);
	
private:
    OpenGLJuceCanvas *canvas_ { nullptr };                      // Reference to canvas that renders OpenGL
    
	float displayPixelWidth_ { 1.0f };
    float displayPixelHeight_ { 1.0f };	// Pixel resolution of the surrounding window
	float totalDisplayWidth_ { 1.0f };
    float totalDisplayHeight_ { 1.0f };	// Size of the internal view (centered around origin)
    float xMin_ { 0.0f };
    float xMax_ { 1.0f };
    float yMin_ { -0.2f };
    float yMax_ { 1.2f };               // Coordinates for the graph axes

	juce::CriticalSection displayMutex_;					// Synchronize access between data and display threads
    
    std::vector<key_position> keyPositions_;        // Positions (0-1 normalized) of the key
    std::vector<timestamp_type> keyTimestamps_;     // Timestamps corresponding to the above positions
    
    // Details on features of key motion: start, end, release, etc.
    key_position pressStartPosition_, pressFinishPosition_;
    timestamp_type pressStartTimestamp_, pressFinishTimestamp_;
    key_position releaseStartPosition_, releaseFinishPosition_;
    timestamp_type releaseStartTimestamp_, releaseFinishTimestamp_;
};
