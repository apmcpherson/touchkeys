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
*/
#pragma once

#ifndef TOUCHKEYS_NO_GUI

#include "../Mappings/MappingFactory.h"

class MappingListComponent;


class MappingListItem  : public juce::Component,
                         public juce::Button::Listener
{
public:
    MappingListItem (MappingListComponent& listComponent);
    ~MappingListItem();

    static void alertBoxResultChosen(int result, MappingListItem *item);
    void deleteMapping();

    MappingFactory* mappingFactory() { return factory_; }
    void setMappingFactory(MappingFactory *factory);
    void synchronize();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

private:
    MappingFactory *factory_;
    MappingListComponent& listComponent_;

    TouchKeysLookAndFeel lnf;

    juce::ToggleButton bypassToggleButton;
    juce::TextButton showDetailsButton;
    TKLabel mappingTypeLabel;
    std::unique_ptr< MappingEditorComponent > mappingShortEditorComponent;
    TKLabel noSettingsLabel;
    juce::TextButton deleteButton;
    juce::Path internalPath1;
    juce::Path internalPath2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingListItem)
};

#endif // TOUCHKEYS_NO_GUI
