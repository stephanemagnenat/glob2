/*
    Copyright (C) 2001, 2002 Stephane Magnenat & Luc-Olivier de Charriere
    for any question or comment contact us at nct@ysagoon.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef __MULTIPLAYERHOSTSCREEN_H
#define __MULTIPLAYERHOSTSCREEN_H

#include "PreparationGui.h"
#include "MultiplayersHost.h"
#include "GAG.h"

class MultiplayersHostScreen: public Screen
{
public:
	enum
	{
		START = 1,
		CANCEL = 5,

		STARTED=11
	};

private:
	bool firstDraw;

public:
	MultiplayersHostScreen(SessionInfo *sessionInfo);
	virtual ~MultiplayersHostScreen();

	void onTimer(Uint32 tick);
	void onAction(Widget *source, Action action, int par1, int par2);
	void paint(int x, int y, int w, int h);

	MultiplayersHost *multiplayersHost;
};

#endif
