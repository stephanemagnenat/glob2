/*
  Copyright (C) 2001-2004 Stephane Magnenat & Luc-Olivier de Charrière
  for any question or comment contact us at <stephane at magnenat dot net> or <NuageBleu at gmail dot com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef __CUSTOM_GAME_SCREEN_H
#define __CUSTOM_GAME_SCREEN_H

#include "AI.h"
#include "ChooseMapScreen.h"
#include <GUIBase.h>
using namespace GAGGUI;

namespace GAGGUI
{
	class Button;
	class TextButton;
	class OnOffButton;
	class ColorButton;
	class MultiTextButton;
	class Text;
}
class Glob2FileList;
class MapPreview;

const int NumberOfPlayerSelectors=16;

//! This screen is used to setup a custom game. AI can be set. Map choosing functionnalities are inherited from ChooseMapScreen
class CustomGameScreen : public ChooseMapScreen
{
private:
	//! AI enable/disable buttons
	OnOffButton *isAI[NumberOfPlayerSelectors];
	//! Team color buttons
	ColorButton *color[NumberOfPlayerSelectors];
	//! Text shown when entry is disabled
	Text *closedText[NumberOfPlayerSelectors];
	//! Multi-text button containing names of availables AIs
	MultiTextButton *aiSelector[NumberOfPlayerSelectors];

public:
	//! Constructor, builds a ChooseMapScreen for maps
	CustomGameScreen();
	//! Destructor
	virtual ~CustomGameScreen();
	virtual void onAction(Widget *source, Action action, int par1, int par2);
	virtual void validMapSelectedhandler(void);
	//! Returns true if AI i is enabled
	bool isAIactive(int i);
	//! Returns the implementation of AI i. If AI is disabled, result is undefined
	AI::ImplementitionID getAiImplementation(int i);
	//! Returns the color of AI i. If AI is disabled, result is undefined
	int getSelectedColor(int i);
};

#endif
