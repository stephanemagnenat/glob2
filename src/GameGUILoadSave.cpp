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

#include "GameGUILoadSave.h"
#include "GlobalContainer.h"
#include "Utilities.h"

//! Load/Save screen
InGameLoadSaveScreen::InGameLoadSaveScreen(const char *directory, const char *extension, bool isLoad, const char *defaultFileName)
:InGameScreen(300, 275)
{
	this->isLoad=isLoad;
	this->extension=Utilities::concat(".", extension);
	firstPaint=true;

	fileList=new List(10, 35, 280, 150, globalContainer->standardFont);

	if (globalContainer->fileManager.initDirectoryListing(directory, extension))
	{
		const char *fileName;
		while ((fileName=globalContainer->fileManager.getNextDirectoryEntry())!=NULL)
		{
			char *mapTempName=Utilities::dencat(fileName,this->extension);
			fileList->addText(mapTempName);
			delete[] mapTempName;
		}
		fileList->sort();
	}
	addWidget(fileList);

	if (!defaultFileName)
		defaultFileName="";
	fileNameEntry=new TextInput(10, 195, 280, 25, globalContainer->standardFont, defaultFileName, true);
	addWidget(fileNameEntry);

	addWidget(new TextButton(10, 230, 135, 35, NULL, -1, -1, globalContainer->menuFont, globalContainer->texts.getString("[ok]"), OK, 13));
	addWidget(new TextButton(155, 230, 135, 35, NULL, -1, -1, globalContainer->menuFont, globalContainer->texts.getString("[cancel]"), CANCEL, 27));

	fileName=Utilities::concat(fileNameEntry->getText(), this->extension);
	//printf("defaultFileName=(%s), fileName=(%s).\n", defaultFileName, fileName);
}

InGameLoadSaveScreen::~InGameLoadSaveScreen()
{
	delete[] fileName;
	delete[] extension;
}

void InGameLoadSaveScreen::onAction(Widget *source, Action action, int par1, int par2)
{
	if ((action==BUTTON_RELEASED) || (action==BUTTON_SHORTCUT))
	{
		if (par1==OK)
		{
			char *mapName=Utilities::dencat(fileName, extension);
			if (mapName[0])
				endValue=OK;
			delete[] mapName;
		}
		else
			endValue=par1;
	}
	else if (action==LIST_ELEMENT_SELECTED)
	{
		const char *s=fileList->getText(par1);
		assert(fileName);
		delete[] fileName;
		fileName=Utilities::concat(s, extension);
		fileNameEntry->setText(s);
	}
	else if (action==TEXT_MODIFFIED)
	{
		assert(fileName);
		delete[] fileName;
		fileName=Utilities::concat(fileNameEntry->getText(), extension);
	}
}

void InGameLoadSaveScreen::onSDLEvent(SDL_Event *event)
{

}

void InGameLoadSaveScreen::paint(int x, int y, int w, int h)
{
	InGameScreen::paint(x, y, w, h);
	if (firstPaint)
	{
		if (isLoad)
			gfxCtx->drawString(10, 7, globalContainer->menuFont, globalContainer->texts.getString("[load game]"));
		else
			gfxCtx->drawString(10, 7, globalContainer->menuFont, globalContainer->texts.getString("[save game]"));
		firstPaint=false;
	}
}
