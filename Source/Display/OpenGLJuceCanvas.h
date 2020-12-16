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

  OpenGLJuceCanvas.h: Juce Component subclass which connects to the
  OpenGLDisplayBase class. Provides a generic interface between Juce and
  the specific OpenGL renderer.
*/

#pragma once

#include "OpenGLDisplayBase.h"
#include <JuceHeader.h>

#if JUCE_OPENGL

//==============================================================================
class OpenGLJuceCanvas  : public juce::Component,
                          public juce::OpenGLRenderer
{
public:
    // *** Constructor / Destructor ***
    OpenGLJuceCanvas(OpenGLDisplayBase& display) : display_(display) {
        openGLContext_.setRenderer (this);
        openGLContext_.setComponentPaintingEnabled (false);
        // Funny OpenGL bugs in Linux version that results in improper drawing when in background
        // For other OSes, save the cycles of continuous repainting.
        if(juce::SystemStats::getOperatingSystemType() == juce::SystemStats::Linux)
            openGLContext_.setContinuousRepainting (true);
        else
            openGLContext_.setContinuousRepainting (false);
        openGLContext_.attachTo (*this);
        display_.setCanvas(this);
    }
    ~OpenGLJuceCanvas() {
        openGLContext_.detach();
    }
    
    // *** OpenGL Context Methods ***
    void newOpenGLContextCreated() {}
    void openGLContextClosing() {}
    
    void mouseDown (const juce::MouseEvent& e) {
        if(e.mods.isLeftButtonDown())
            display_.mouseDown(e.x, e.y);
        else if(e.mods.isRightButtonDown())
            display_.rightMouseDown(e.x, e.y);
    }
    
    void mouseDrag (const juce::MouseEvent& e) {
        if(e.mods.isLeftButtonDown())
            display_.mouseDragged(e.x, e.y);
        else if(e.mods.isRightButtonDown())
            display_.rightMouseDragged(e.x, e.y);
    }
    
    void mouseUp (const juce::MouseEvent& e) {
        if(e.mods.isLeftButtonDown())
            display_.mouseUp(e.x, e.y);
        else if(e.mods.isRightButtonDown())
            display_.rightMouseUp(e.x, e.y);
    }
    
    
    void resized() override {
        display_.setDisplaySize(getWidth(), getHeight());
    }
    
    void paint (juce::Graphics&) override {}
    
    void renderOpenGL() override {
        display_.render();
    }
    
    // Method to tell the OpenGLContext to repaint
    void triggerRepaint() {
        openGLContext_.triggerRepaint();
    }
    
private:
    OpenGLDisplayBase& display_;    // Reference to the TouchKeys-specific OpenGL Display
    juce::OpenGLContext openGLContext_;
};

#endif /* JUCE_OPENGL */
