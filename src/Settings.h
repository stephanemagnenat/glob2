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

#ifndef __SETTINGS_H
#define __SETTINGS_H

#include "Header.h"
#include <string>
#include <map>
#include "IntBuildingType.h"

class Settings
{
public:
	std::string username;
	std::string password;
	int screenWidth;
	int screenHeight;
	Uint32 screenFlags;
	Uint32 optionFlags;
	Uint32 defaultLanguage;
	Uint32 musicVolume;
	int mute;
	
	bool rememberUnit;
	
	///Levels are from 0 to 5, where even numbers are building
	///under construction and odd ones are completed buildings.
	int defaultUnitsAssigned[IntBuildingType::NB_BUILDING][6];

	int cloudPatchSize;//the bigger the faster the uglier
	int cloudMaxAlpha;//the higher the nicer the clouds the harder the units are visible
	int cloudMaxSpeed;
	int cloudWindStability;//how much will the wind change
	int cloudStability;//how much will the clouds change shape
	int cloudSize;//the bigger the better they look with big Patches. The smaller the better they look with smaller patches
	int cloudHeight;//(cloud - ground) / (eyes - ground)

	std::map<std::string, std::string> editor_keyboard_shortcuts;

	int tempUnit;
	int tempUnitFuture;

public:
	void restoreDefaultShortcuts();
	void load(const char *filename="preferences.txt");
	void save(const char *filename="preferences.txt");
	Settings();
};

#endif
