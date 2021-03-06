/*
  Copyright (C) 2008 Bradley Arsenault

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


#include "MapScript.h"
#include <assert.h>
#include <iostream>

#include "Stream.h"

MapScript::MapScript(GameGUI* gui):
	usl(gui)
{
	mode = USL;
}



void MapScript::encodeData(GAGCore::OutputStream* stream) const
{
	stream->writeEnterSection("MapScript");
	stream->writeText(script, "script");
	stream->writeUint8(static_cast<Uint8>(mode), "mode");
	usl.encodeData(stream);
	stream->writeLeaveSection();
}



void MapScript::decodeData(GAGCore::InputStream* stream, Uint32 versionMinor)
{
	stream->readEnterSection("MapScript");
	script = stream->readText("script");
	mode = static_cast<MapScriptMode>(stream->readUint8("mode"));
	usl.compileCode(script);
	usl.decodeData(stream, versionMinor);
	stream->readLeaveSection();
}



const std::string& MapScript::getMapScript() const
{
	return script;
}



void MapScript::setMapScript(const std::string& newScript)
{
	script = newScript;
}


MapScript::MapScriptMode MapScript::getMapScriptMode() const
{
	return mode;
}

	
void MapScript::setMapScriptMode(MapScript::MapScriptMode newMode)
{
	mode = newMode;
}



bool MapScript::compileCode()
{
	if(mode == USL)
	{
		return usl.compileCode(script);
	}
	else
	{
		std::cerr << "mode unknown." << std::endl;
		assert(false);
	}
}


bool MapScript::testCompileCode(const std::string& testScript)
{
	if(mode == USL)
	{
		return usl.compileCode(testScript);
	}
	else
	{
		std::cerr << "mode unknown." << std::endl;
		assert(false);
	}
}


const MapScriptError& MapScript::getError() const
{
	return usl.getError();
}

void MapScript::syncStep(GameGUI *gui)
{
	usl.syncStep(gui);
}


