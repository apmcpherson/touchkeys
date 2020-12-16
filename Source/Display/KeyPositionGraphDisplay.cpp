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

  KeyPositionGraphDisplay.cpp: implements a graph of key position over time
  for a specific key press, which is useful for analysis of continuous key
  position.
*/

#include "KeyPositionGraphDisplay.h"

// Class constants
// Display margins
const float KeyPositionGraphDisplay::kDisplaySideMargin = 0.5;
const float KeyPositionGraphDisplay::kDisplayBottomMargin = 0.5;
const float KeyPositionGraphDisplay::kDisplayTopMargin = 0.5;

// Size of the graph area
const float KeyPositionGraphDisplay::kDisplayGraphWidth = 20.0;
const float KeyPositionGraphDisplay::kDisplayGraphHeight = 10.0;

KeyPositionGraphDisplay::KeyPositionGraphDisplay() : canvas_(0),
displayPixelWidth_(1.0), displayPixelHeight_(1.0), totalDisplayWidth_(1.0), totalDisplayHeight_(1.0), 
xMin_(0.0), xMax_(1.0), yMin_(-0.2), yMax_(1.2)
{
	// Initialize OpenGL settings: 2D only
    
	//glMatrixMode(GL_PROJECTION);
	//glDisable(GL_DEPTH_TEST);
    
    pressStartTimestamp_ = pressFinishTimestamp_ = releaseStartTimestamp_ = releaseFinishTimestamp_ = missing_value<timestamp_type>::missing();
    pressStartPosition_ = pressFinishPosition_ = releaseStartPosition_ = releaseFinishPosition_ = missing_value<key_position>::missing();
    
    totalDisplayWidth_ = kDisplaySideMargin*2 + kDisplayGraphWidth;
    totalDisplayHeight_ = kDisplayTopMargin + kDisplayBottomMargin + kDisplayGraphHeight;
}

// Tell the underlying canvas to repaint itself
void KeyPositionGraphDisplay::tellCanvasToRepaint() {
    if(canvas_ != 0)
        canvas_->triggerRepaint();
}

void KeyPositionGraphDisplay::setDisplaySize(float width, float height) {
	juce::ScopedLock sl(displayMutex_);
	displayPixelWidth_ = width;
	displayPixelHeight_ = height;
	refreshViewport();
}


// Render the keyboard display

void KeyPositionGraphDisplay::render() {
	// Start with a light gray background
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
    
    float invAspectRatio = totalDisplayWidth_ / totalDisplayHeight_; 
	float scaleValue = 2.0 / totalDisplayWidth_;
	
	glScalef(scaleValue, scaleValue * invAspectRatio, scaleValue);
	glTranslatef(-1.0 / scaleValue, -totalDisplayHeight_ / 2.0, 0);
	glTranslatef(kDisplaySideMargin, kDisplayBottomMargin, 0.0);
	
	juce::ScopedLock sl(displayMutex_);
    
    // Draw the region where the graph will be plotted (white with black outline)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1.0, 1.0, 1.0);
    
    glBegin(GL_POLYGON);
	glVertex2f(0, 0);
    glVertex2f(0, kDisplayGraphHeight);
	glVertex2f(kDisplayGraphWidth, kDisplayGraphHeight);
	glVertex2f(kDisplayGraphWidth, 0);
	glEnd();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(1.0);
    
    glBegin(GL_POLYGON);
	glVertex2f(0, 0);
    glVertex2f(0, kDisplayGraphHeight);
	glVertex2f(kDisplayGraphWidth, kDisplayGraphHeight);
	glVertex2f(kDisplayGraphWidth, 0);
	glEnd();
    
    // Draw gray lines for the 0.0 and 1.0 key positions
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINES);
    glVertex2f(0, graphToDisplayY(0.0));
    glVertex2f(kDisplayGraphWidth, graphToDisplayY(0.0));
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(0, graphToDisplayY(1.0));
    glVertex2f(kDisplayGraphWidth, graphToDisplayY(1.0));
    glEnd();

    // Draw the graph of position over time (if data exists)
    if(!keyPositions_.empty() && !keyTimestamps_.empty()) {
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_STRIP);
        for(int index = 0; index < keyPositions_.size() && index < keyTimestamps_.size(); index++) {
            glVertex2f(graphToDisplayX(keyTimestamps_[index]), graphToDisplayY(keyPositions_[index]));
        }
        glEnd();
    }
    
    // Draw the important features, if they exist
    if(!missing_value<timestamp_type>::isMissing(pressStartTimestamp_)) {
        glColor3f(0.0, 0.0, 1.0);
        glBegin(GL_LINES);
        glVertex2f(graphToDisplayX(pressStartTimestamp_), 0);
        glVertex2f(graphToDisplayX(pressStartTimestamp_), kDisplayGraphHeight);
        glEnd();
    }
    if(!missing_value<timestamp_type>::isMissing(pressFinishTimestamp_)) {
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINES);
        glVertex2f(graphToDisplayX(pressFinishTimestamp_), 0);
        glVertex2f(graphToDisplayX(pressFinishTimestamp_), kDisplayGraphHeight);
        glEnd();
    }
    if(!missing_value<timestamp_type>::isMissing(releaseStartTimestamp_)) {
        glColor3f(0.0, 0.0, 1.0);
        glBegin(GL_LINES);
        glVertex2f(graphToDisplayX(releaseStartTimestamp_), 0);
        glVertex2f(graphToDisplayX(releaseStartTimestamp_), kDisplayGraphHeight);
        glEnd();
    }
    if(!missing_value<timestamp_type>::isMissing(releaseFinishTimestamp_)) {
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINES);
        glVertex2f(graphToDisplayX(releaseFinishTimestamp_), 0);
        glVertex2f(graphToDisplayX(releaseFinishTimestamp_), kDisplayGraphHeight);
        glEnd();
    }
	
	glFlush();
}

// Mouse interaction methods

void KeyPositionGraphDisplay::mouseDown( const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyPositionGraphDisplay::mouseDragged(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyPositionGraphDisplay::mouseUp(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyPositionGraphDisplay::rightMouseDown(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyPositionGraphDisplay::rightMouseDragged(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void KeyPositionGraphDisplay::rightMouseUp(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

float KeyPositionGraphDisplay::graphToDisplayX( const float x) {
    return kDisplayGraphWidth*(x - xMin_)/(xMax_ - xMin_);
}

float KeyPositionGraphDisplay::graphToDisplayY( const float y) {
    return kDisplayGraphHeight*(y - yMin_)/(yMax_ - yMin_);
}

void KeyPositionGraphDisplay::refreshViewport() {
	glViewport(0, 0, displayPixelWidth_, displayPixelHeight_);
}

// Copy data from the circular buffer to our internal buffer for 
void KeyPositionGraphDisplay::copyKeyDataFromBuffer(Node<key_position>& keyBuffer,
                                                    const Node<key_position>::size_type startIndex,
                                                    const Node<key_position>::size_type endIndex) {
    // Clear existing information
    keyPositions_.clear();
    keyTimestamps_.clear();
    
    Node<key_position>::size_type index = startIndex;
    if(index < keyBuffer.beginIndex())
        index = keyBuffer.beginIndex();
    
    // Iterate through the buffer, copying positions and timestamps
    while(index < endIndex && index < keyBuffer.endIndex()) {
        keyPositions_.push_back(keyBuffer[index]);
        keyTimestamps_.push_back(keyBuffer.timestampAt(index));
        index++;
    }
    
    // Scale the axes according to what's being displayed. The Y axis should stay
    // more or less constant (for now), but X will change depending on timestamps
    xMin_ = keyTimestamps_.front();
    xMax_ = keyTimestamps_.back();
    yMin_ = -0.2f;
    yMax_ = 1.2f;
    tellCanvasToRepaint();
}

// Conversion from internal coordinate space to external pixel values and back

// Pixel values go from 0,0 (lower left) to displayPixelWidth_, displayPixelHeight_ (upper right)
// Internal values go from -totalDisplayWidth_/2, -totalDisplayHeight_/2 (lower left)
//   to totalDisplayWidth_/2, totalDisplayHeight_/2 (upper right)

// Pixel value in --> OpenGL value out
KeyPositionGraphDisplay::Point KeyPositionGraphDisplay::screenToInternal(Point& inPoint) {
	Point out;
	
	out.x = -totalDisplayWidth_*0.5f + (inPoint.x/displayPixelWidth_) * totalDisplayWidth_;
	out.y = -totalDisplayHeight_*0.5f + (inPoint.y/displayPixelHeight_) * totalDisplayHeight_;
	
	return out;
}

// OpenGL value in --> Pixel value out
KeyPositionGraphDisplay::Point KeyPositionGraphDisplay::internalToScreen(Point& inPoint) {
	Point out;
	
	out.x = ((inPoint.x + totalDisplayWidth_*0.5f)/totalDisplayWidth_) * displayPixelWidth_;
	out.y = ((inPoint.y + totalDisplayHeight_*0.5f)/totalDisplayHeight_) * displayPixelHeight_;
    
	return out;
}