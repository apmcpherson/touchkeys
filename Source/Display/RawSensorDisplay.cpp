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

  RawSensorDisplay.cpp: simple graph for showing raw TouchKeys sensor values
*/

#include "RawSensorDisplay.h"
#include <OpenGL/gl.h>

RawSensorDisplay::RawSensorDisplay() {
	// Initialize OpenGL settings: 2D only
    
	//glMatrixMode(GL_PROJECTION);
	//glDisable(GL_DEPTH_TEST);
    
    totalDisplayWidth_ = kDisplaySideMargin*2 + kDisplayBarWidth;
    totalDisplayHeight_ = kDisplayTopMargin + kDisplayBottomMargin + kDisplayBarHeight;
}

// Tell the underlying canvas to repaint itself
void RawSensorDisplay::tellCanvasToRepaint() {
    if(canvas_ != 0)
        canvas_->triggerRepaint();
}

void RawSensorDisplay::setDisplaySize(float width, float height) {
    juce::ScopedLock sl(displayMutex_);
    
	displayPixelWidth_ = width;
	displayPixelHeight_ = height;
	refreshViewport();
}


// Render the keyboard display

void RawSensorDisplay::render() {
	// Start with a light gray background
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
    
    float invAspectRatio = totalDisplayWidth_ / totalDisplayHeight_;
	float scaleValue = 2.0 / totalDisplayWidth_;
	
	glScalef(scaleValue, scaleValue * invAspectRatio, scaleValue);
	glTranslatef(-1.0 / scaleValue, -totalDisplayHeight_ / 2.0, 0);
	glTranslatef(0.0, kDisplayBottomMargin, 0.0);
	
    juce::ScopedLock sl(displayMutex_);
    
    // Draw the line for zero value
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINES);
    glVertex2f(0, graphToDisplayY(0.0));
    glVertex2f(totalDisplayWidth_, graphToDisplayY(0.0));
    glEnd();
    
    glTranslatef(kDisplaySideMargin, 0.0, 0.0);
    
    for(int i = 0; i < displayValues_.size(); i++) {
        // Draw each bar in sequence
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor3f(1.0, 0.0, 0.0);
        
        glBegin(GL_POLYGON);
        glVertex2f(0, graphToDisplayY(0));
        glVertex2f(0, graphToDisplayY(displayValues_[i]));
        glVertex2f(kDisplayBarWidth, graphToDisplayY(displayValues_[i]));
        glVertex2f(kDisplayBarWidth, graphToDisplayY(0));
        glEnd();
        
        glTranslatef(kDisplayBarWidth + kDisplayBarSpacing, 0.0, 0.0);
    }
	
	glFlush();
}

// Copy new data into the display buffer
void RawSensorDisplay::setDisplayData(std::vector<int> const& values) {
    displayValues_ = values;
    
    // Update display width according to number of data points
    totalDisplayWidth_ = kDisplaySideMargin*2 + (kDisplayBarWidth + kDisplayBarSpacing) * displayValues_.size();
    
    tellCanvasToRepaint();
}

// Mouse interaction methods

void RawSensorDisplay::mouseDown(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void RawSensorDisplay::mouseDragged(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void RawSensorDisplay::mouseUp(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void RawSensorDisplay::rightMouseDown(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void RawSensorDisplay::rightMouseDragged(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

void RawSensorDisplay::rightMouseUp(const float x, const float y) {
	//Point mousePoint = {x, y};
	//Point scaledPoint = screenToInternal(mousePoint);
}

float RawSensorDisplay::graphToDisplayY(const float y) {
    return kDisplayBarHeight*(y - yMin_)/(yMax_ - yMin_);
}

void RawSensorDisplay::refreshViewport() {
	glViewport(0, 0, displayPixelWidth_, displayPixelHeight_);
}

// Conversion from internal coordinate space to external pixel values and back

// Pixel values go from 0,0 (lower left) to displayPixelWidth_, displayPixelHeight_ (upper right)
// Internal values go from -totalDisplayWidth_/2, -totalDisplayHeight_/2 (lower left)
//   to totalDisplayWidth_/2, totalDisplayHeight_/2 (upper right)

// Pixel value in --> OpenGL value out
RawSensorDisplay::Point RawSensorDisplay::screenToInternal(Point& inPoint) {
	Point out;
	
	out.x = -totalDisplayWidth_*0.5 + (inPoint.x/displayPixelWidth_) * totalDisplayWidth_;
	out.y = -totalDisplayHeight_*0.5 + (inPoint.y/displayPixelHeight_) * totalDisplayHeight_;
	
	return out;
}

// OpenGL value in --> Pixel value out
RawSensorDisplay::Point RawSensorDisplay::internalToScreen(Point& inPoint) {
	Point out;
	
	out.x = ((inPoint.x + totalDisplayWidth_*0.5)/totalDisplayWidth_) * displayPixelWidth_;
	out.y = ((inPoint.y + totalDisplayHeight_*0.5)/totalDisplayHeight_) * displayPixelHeight_;
    
	return out;
}
