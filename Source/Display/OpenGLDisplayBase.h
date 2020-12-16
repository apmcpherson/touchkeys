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

  OpenGLDisplayBase.h: virtual base class for a renderer that handles
  arbitrary display sizes and user mouse events.
*/

#pragma once

class OpenGLJuceCanvas;

// Virtual base class that implements some basic methods that the OS-specific
// GUI can attach to. Specific displays are subclasses of this

class OpenGLDisplayBase {
public:
    OpenGLDisplayBase() {}
    
    virtual ~OpenGLDisplayBase() {}
    
    // Canvas reference method
    virtual void setCanvas(OpenGLJuceCanvas *canvas) = 0;
	
	// Setup method for display size
	virtual void setDisplaySize(float width, float height) = 0;
	
	// Drawing methods
	virtual void render() = 0;
	
	// Interaction methods
	virtual void mouseDown(const float x, const float y) = 0;
	virtual void mouseDragged(const float x, const float y) = 0;
	virtual void mouseUp(const float x, const float y) = 0;
	virtual void rightMouseDown(const float x, const float y) = 0;
	virtual void rightMouseDragged(const float x, const float y) = 0;
	virtual void rightMouseUp(const float x, const float y) = 0;
};
