/*
  Copyright (C) 2007 Bradley Arsenault

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

#ifndef __NetBroadcastListener_h
#define __NetBroadcastListener_h

#include "SDL_net.h"
#include "LANGameInformation.h"
#include <vector>

///This listens for sub-net broadcasts (finding a LAN game)
class NetBroadcastListener
{
public:
	///Constructs a NetBroadcastListener, and begins listening
	NetBroadcastListener();

	~NetBroadcastListener();

	///Updates the broadcast listener
	void update();

	///Gets a list of all the LAN games
	const std::vector<LANGameInformation>& getLANGames();

	///Gets the IP address for the given lan game
	std::string getIPAddress(size_t num);
private:
	UDPsocket socket;
	std::vector<LANGameInformation> games;
	std::vector<int> timeouts;
	std::vector<IPaddress> addresses;
	Uint32 lastTime;
};

#endif