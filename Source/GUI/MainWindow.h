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
 
  MainWindow.h: the control window, plus menu bar and Juce application methods
*/

#pragma once

#ifndef TOUCHKEYS_NO_GUI

#include "ControlWindowMainComponent.h"

//==============================================================================
/*
 */

class MainWindow : public juce::DocumentWindow, public juce::Timer,
                      public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
private:
    // Commands this application responds to
    enum CommandIDs
    {
        // File menu
        kCommandClearPreset    = 0x2001,
        kCommandOpenPreset,
        kCommandSavePreset,
        
        // Edit menu
        // (all standard)
        
        // Control menu
        kCommandRescanDevices = 0x2020,
        kCommandLoggingStartStop,
        kCommandLoggingPlay,
        kCommandEnableExperimentalMappings,
        kCommandTestTouchkeySensors,
        kCommandJumpToBootloader,
        kCommandPreferences,
        
        // Window menu
        kCommandShowControlWindow = 0x2030,
        kCommandShowKeyboardWindow
    };
    
public:
    MainWindow(MainApplicationController& controller);
    ~MainWindow();
    
    void closeButtonPressed() override
    {
        // This is called when the user tries to close this window. Here, we'll just
        // ask the app to quit when this happens, but you can change this to do
        // whatever you need.
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
    // Method used by Juce timer which we will use for periodic UI updates
    // from the underlying system state, in a configuration similar to how the
    // Juce audio plugins work
    void timerCallback() override;
    
    // ***** Menu Bar methods *****
    
    juce::StringArray getMenuBarNames();
    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& menuName);
    void menuItemSelected (int menuItemID, int topLevelMenuIndex);
    
    // ***** Application Command Manager methods *****
    
    // this will return the next parent component that is an ApplicationCommandTarget (in this
    // case, there probably isn't one, but it's best to use this method in your own apps).
    ApplicationCommandTarget* getNextCommandTarget();
    
    // this returns the set of all commands that this target can perform..
    void getAllCommands (juce::Array <juce::CommandID>& commands);
    
    // This method is used when something needs to find out the details about one of the commands
    // that this object can perform..
    void getCommandInfo ( juce::CommandID commandID, juce::ApplicationCommandInfo& result);
    
    // Perform a command
    bool perform (const InvocationInfo& info);
    
    // Callback for alert box
    static void alertBoxResultChosen(int result, MainWindow *item);
    
    // Clear preset (called from alert box
    void clearPreset() { controller_.clearPreset(); }
    
    /* Note: Be careful if you override any juce::DocumentWindow methods - the base
     class uses a lot of them, so by overriding you might break its functionality.
     It's best to do all your work in your content component instead, but if
     you really have to override any juce::DocumentWindow methods, make sure your
     subclass also calls the superclass's method.
     */
    
private:
    // The command manager object used to dispatch command events
    MainApplicationController& controller_;
    juce::ApplicationCommandManager commandManager_;
    ControlWindowMainComponent mainComponent_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

#endif  // TOUCHKEYS_NO_GUI
