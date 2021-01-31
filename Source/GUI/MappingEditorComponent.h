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

  MappingEditorComponent.h: Juce Component subclass from which all
  mapping editors inherit.
*/

#pragma once

#ifndef TOUCHKEYS_NO_GUI

#include <JuceHeader.h>

//==============================================================================

class TKLabel : public juce::Label
{
public:
    explicit TKLabel( const juce::String& lbl ) :
        TKLabel { "new label", lbl }
    {
    }

    TKLabel( const juce::String& componentName, const juce::String& lbl ) :
        juce::Label { componentName, lbl }
    {
        setFont( juce::Font( 15.00f, juce::Font::plain ) );
        setJustificationType( juce::Justification::centredLeft );
        setEditable( false, false, false );
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( TKLabel )
};


class TKTextEditor : public juce::TextEditor
{
public:
    explicit TKTextEditor( const juce::String& s ) :
        TKTextEditor { s, juce::String{ } }
    {
    }

    TKTextEditor( const juce::String& componentString, const juce::String& displayText ) :
        TextEditor { componentString }
    {
        setMultiLine( false );
        setReturnKeyStartsNewLine( false );
        setReadOnly( false );
        setScrollbarsShown( true );
        setCaretVisible( true );
        setPopupMenuEnabled( true );
        setText( displayText );
    }


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( TKTextEditor )
};


//==============================================================================
/* class TouchKeysLookAndFeel - Look & Feel class for JUCE UI components
*/
class TouchKeysLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TouchKeysLookAndFeel()
    {
        setColour( juce::Label::ColourIds::textColourId, juce::Colours::black );
        //setColour (juce::Label::ColourIds::backgroundColourId, juce::Colours::black);

        setColour( juce::GroupComponent::ColourIds::textColourId, juce::Colours::black );

        setColour( juce::ToggleButton::ColourIds::textColourId, juce::Colours::black );
        setColour( juce::ToggleButton::ColourIds::tickColourId, juce::Colours::black );
    }
};


//==============================================================================
/*
*/
class MappingEditorComponent : public juce::Component
{
public:
    MappingEditorComponent()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

    }

    virtual ~MappingEditorComponent()
    {
    }
    
    // Method to synchronize the GUI state to the underlying
    // state of the mapping. Implemented in the subclass.
    virtual void synchronize() {}
    
    // Get a human-readable name for the mapping
    // Generally, extended editors should implement this but short editors
    // don't need to
    virtual juce::String getDescription() { return "Mapping"; }

    void paint (juce::Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

        g.fillAll(juce::Colours::black);   // clear the background

        g.setColour(juce::Colours::grey);
        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

        g.setColour(juce::Colours::lightblue);
        g.setFont (14.0f);
        g.drawText ("MappingEditorComponent", getLocalBounds(),
                    juce::Justification::centred, true);   // draw some placeholder text
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..

    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingEditorComponent)
};

#endif  // TOUCHKEYS_NO_GUI
