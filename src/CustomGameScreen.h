/*
� � Copyright (C) 2001, 2002 Stephane Magnenat & Luc-Olivier de Charri�re
    for any question or comment contact us at nct@ysagoon.com or nuage@ysagoon.com

� � This program is free software; you can redistribute it and/or modify
� � it under the terms of the GNU General Public License as published by
� � the Free Software Foundation; either version 2 of the License, or
� � (at your option) any later version.

� � This program is distributed in the hope that it will be useful,
� � but WITHOUT ANY WARRANTY; without even the implied warranty of
� � MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. �See the
� � GNU General Public License for more details.

� � You should have received a copy of the GNU General Public License
� � along with this program; if not, write to the Free Software
� � Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA �02111-1307 �USA
*/

#ifndef __CUSTOM_GAME_SCREEN_H
#define __CUSTOM_GAME_SCREEN_H

#include "GAG.h"
#include "Session.h"
#include "GUIMapPreview.h"

class CustomGameScreen:public Screen
{
public:
	enum
	{
		OK = 1,
		CANCEL = 2
	};
	SessionInfo sessionInfo;

private:
	Button *ok, *cancel;
	List *fileList;
	MapPreview *mapPreview;
	OnOffButton *isAI[8];
	ColorButton *color[8];
	Text *isAItext[8];
	Text *mapName, *mapInfo, *mapVersion, *mapSize;
	bool validSessionInfo;

public:
	CustomGameScreen();
	virtual ~CustomGameScreen();
	void onAction(Widget *source, Action action, int par1, int par2);
	void paint(int x, int y, int w, int h);
	bool isAIactive(int i) { return isAI[i]->getState(); }
	int getSelectedColor(int i) { return color[i]->getSelectedColor(); }
};

#endif
