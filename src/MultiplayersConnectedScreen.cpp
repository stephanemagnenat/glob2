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

#include "MultiplayersConnectScreen.h"
#include "GlobalContainer.h"
#include "GAG.h"

//MultiplayersConnectScreen pannel part !!


MultiplayersConnectScreen::MultiplayersConnectScreen(MultiplayersJoin *multiplayersJoin)
{
	this->multiplayersJoin=multiplayersJoin;

	addWidget(new TextButton(360, 350, 200, 40, NULL, -1, -1, globalContainer->menuFont, globalContainer->texts.getString("[disconnect]"), DISCONNECT));

	globalContainer->gfx->setClipRect();
}

MultiplayersConnectScreen::~MultiplayersConnectScreen()
{

}

void MultiplayersConnectScreen::paint(int x, int y, int w, int h)
{
	gfxCtx->drawFilledRect(x, y, w, h, 0, 0, 0);

	multiplayersJoin->sessionInfo.draw(gfxCtx);
	
	addUpdateRect();
}


void MultiplayersConnectScreen::onTimer(Uint32 tick)
{
	multiplayersJoin->onTimer(tick);
	
	if (multiplayersJoin->waitingState<MultiplayersJoin::WS_WAITING_FOR_SESSION_INFO)
	{
		printf("MultiplayersConnectScreen:DISCONNECT!\n");
		endExecute(DISCONNECT);
	}
	
	if ((tick % 10)==0)
		dispatchPaint(gfxCtx);
	
	if (multiplayersJoin->waitingState==MultiplayersJoin::WS_SERVER_START_GAME)
	{
		if (multiplayersJoin->startGameTimeCounter<0)
		{
			printf("MultiplayersConnectScreen::STARTED!\n");
			endExecute(STARTED);
		}
	}
}

void MultiplayersJoinScreen::onSDLEvent(SDL_Event *event)
{

}


void MultiplayersJoinScreen::onAction(Widget *source, Action action, int par1, int par2)
{
	if (action==BUTTON_RELEASED)
	{
		if (par1==DISCONNECT)
			endExecute(DISCONNECT);
		else
			assert(false);
	}
}
