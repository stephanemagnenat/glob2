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

#include "MultiplayersHost.h"
#include "GlobalContainer.h"
#include "GAG.h"
#include "NetDefine.h"
#include "YOG.h"
#include "Marshaling.h"
#include "Utilities.h"

MultiplayersHost::MultiplayersHost(SessionInfo *sessionInfo, bool shareOnYOG, SessionInfo *savedSessionInfo)
:MultiplayersCrossConnectable()
{
	this->sessionInfo=*sessionInfo;
	validSessionInfo=true;
	if (savedSessionInfo)
		this->savedSessionInfo=new SessionInfo(*savedSessionInfo);
	else
		this->savedSessionInfo=NULL;

	logFile=globalContainer->logFileManager.getFile("MultiplayersHost.log");
	assert(logFile);
	// net things:
	initHostGlobalState();

	socket=NULL;
	socket=SDLNet_UDP_Open(SERVER_PORT);

	serverIP.host=0;
	serverIP.port=0;

	fprintf(logFile, "Openning a socket...\n");
	if (socket)
	{
		fprintf(logFile, "Socket opened at port %d.\n", SERVER_PORT);
	}
	else
	{
		fprintf(logFile, "failed to open a socket.\n");
		return;
	}

	firstDraw=true;

	this->shareOnYOG=shareOnYOG;
	if (shareOnYOG)
	{
		fprintf(logFile, "sharing on YOG\n");
		globalContainer->yog->shareGame(sessionInfo->map.getMapName());
		globalContainer->yog->setGameSocket(socket);
	}
	
	stream=NULL;
	
	if (sessionInfo->mapGenerationDescriptor && sessionInfo->fileIsAMap)
	{
		fprintf(logFile, "MultiplayersHost() random map.\n");
	}
	else if (sessionInfo->fileIsAMap)
	{
		const char *s=sessionInfo->map.getMapFileName();
		assert(s);
		assert(s[0]);
		fprintf(logFile, "MultiplayersHost() fileName=%s.\n", s);
		stream=globalContainer->fileManager.open(s,"rb");
	}
	else
	{
		const char *s=sessionInfo->map.getGameFileName();
		assert(s);
		assert(s[0]);
		fprintf(logFile, "MultiplayersHost() fileName=%s.\n", s);
		stream=globalContainer->fileManager.open(s,"rb");
	}
	
	fileSize=0;
	if (stream)
	{
		SDL_RWseek(stream, 0, SEEK_END);
		fileSize=SDL_RWtell(stream);
	}
	
	for (int p=0; p<32; p++)
	{
		playerFileTra[p].wantsFile=false;
		playerFileTra[p].receivedFile=false;
		playerFileTra[p].packetSize=64;
		playerFileTra[p].windowSize=20;
		playerFileTra[p].unreceivedIndex=0;
		for (int i=0; i<MAX_WINDOW_SIZE; i++)
		{
			playerFileTra[p].windowstats[i]=0;
			playerFileTra[p].windowlosts[i]=0;
		}
		playerFileTra[p].totalLost=0;
		playerFileTra[p].totalSent=0;
		playerFileTra[p].onlyWaited=0;
		for (int i=0; i<NET_WINDOW_SIZE; i++)
		{
			playerFileTra[p].window[i].index=0;
			playerFileTra[p].window[i].sent=false;
			playerFileTra[p].window[i].received=false;
			playerFileTra[p].window[i].time=i%SECOND_TIMEOUT;
			playerFileTra[p].window[i].packetSize=64;
		}
	}
	
	
}

MultiplayersHost::~MultiplayersHost()
{
	
	if (shareOnYOG)
	{
		globalContainer->yog->unshareGame();
	}
	
	if (destroyNet)
	{
		assert(channel==-1);
		if (channel!=-1)
		{
			send(CLIENT_QUIT_NEW_GAME);
			SDLNet_UDP_Unbind(socket, channel);
			fprintf(logFile, "Socket unbinded.\n");
		}
		if (socket)
		{
			// We need to have the same Port openened to comunicate with all players to pass firewalls.
			// Then, "socket" is the same in all players and in "MultiplayersHost".
			// Therefore, we need to unbind players BEFORE deleting "socket".
			
			for (int p=0; p<sessionInfo.numberOfPlayer; p++)
				if (sessionInfo.players[p].socket==socket)
					sessionInfo.players[p].unbind();
			SDLNet_UDP_Close(socket);
			socket=NULL;
			fprintf(logFile, "Socket closed.\n");
		}
	}
	
	if (savedSessionInfo)
		delete savedSessionInfo;
		
	if (stream)
		SDL_RWclose(stream);
	
	if (logFile)
	{
		if (logFile!=stdout)
		{
			for (int p=0; p<32; p++)
				if (playerFileTra[p].totalSent)
				{
					fprintf(logFile, "player %d\n", p);
					int totLost=0;
					for (int i=0; i<MAX_WINDOW_SIZE; i++)
					{
						totLost+=playerFileTra[p].windowlosts[i];
						if (playerFileTra[p].windowstats[i]>0)
							fprintf(logFile, "%3d \t %3d \t %3d \t %3d\n", i, playerFileTra[p].windowstats[i], playerFileTra[p].windowlosts[i], totLost);
					}
					fprintf(logFile, "playerFileTra[p].packetSize=%d.\n", playerFileTra[p].packetSize);
					fprintf(logFile, "playerFileTra[p].totalLost=%d. (%f)\n", playerFileTra[p].totalLost, (float)playerFileTra[p].totalLost/playerFileTra[p].totalSent);
					fprintf(logFile, "playerFileTra[p].totalSent=%d.\n", playerFileTra[p].totalSent);
					fprintf(logFile, "playerFileTra[p].onlyWaited=%d.\n", playerFileTra[p].onlyWaited);
				}
		}
	}
}

int MultiplayersHost::newTeamIndice()
{
	// We put the new player in a team with the less number of player
	// and the shortest indice:
	int t=0;
	int lessPlayer=33;
	for (int ti=0; ti<sessionInfo.numberOfTeam; ti++)
	{
		int numberOfPlayer=0;
		Uint32 m=1;
		Uint32 pm=sessionInfo.team[ti].playersMask;
		for (int i=0; i<32; i++)
		{
			if (m&pm)
				numberOfPlayer++;
			m=m<<1;
		}
		if (numberOfPlayer<lessPlayer)
		{
			lessPlayer=numberOfPlayer;
			t=ti;
		}
	}
	assert(t>=0);
	assert(t<32);
	assert(t<sessionInfo.numberOfTeam);
	
	return t;
}

void MultiplayersHost::initHostGlobalState(void)
{
	for (int i=0; i<32; i++)
		crossPacketRecieved[i]=0;
	
	hostGlobalState=HGS_SHARING_SESSION_INFO;
}

void MultiplayersHost::reinitPlayersState()
{
	for (int j=0; j<sessionInfo.numberOfPlayer; j++)
		if (sessionInfo.players[j].netState>BasePlayer::PNS_PLAYER_SEND_PRESENCE_REQUEST)
		{
			sessionInfo.players[j].netState=BasePlayer::PNS_PLAYER_SEND_SESSION_REQUEST;
			sessionInfo.players[j].netTimeout=2*j; // we just split the sendings by 1/10 seconds.
			sessionInfo.players[j].netTimeoutSize=LONG_NETWORK_TIMEOUT;
			sessionInfo.players[j].netTOTL++;
		}
}

void MultiplayersHost::stepHostGlobalState(void)
{
	switch (hostGlobalState)
	{
	case HGS_BAD :
		fprintf(logFile, "This is a bad hostGlobalState case. Should not happend!\n");
	break;
	case HGS_SHARING_SESSION_INFO :
	{
		bool allOK=true;
		for (int i=0; i<sessionInfo.numberOfPlayer; i++)
			if (sessionInfo.players[i].type==BasePlayer::P_IP)
				if (sessionInfo.players[i].netState<BasePlayer::PNS_OK)
					allOK=false;
		

		if (allOK)
		{
			fprintf(logFile, "OK, now we are waiting for cross connections\n");
			hostGlobalState=HGS_WAITING_CROSS_CONNECTIONS;
			for (int i=0; i<sessionInfo.numberOfPlayer; i++)
				if (sessionInfo.players[i].type==BasePlayer::P_IP)
				{
					sessionInfo.players[i].netState=BasePlayer::PNS_SERVER_SEND_CROSS_CONNECTION_START;
					if (sessionInfo.players[i].netTimeout>0)
						sessionInfo.players[i].netTimeout-=sessionInfo.players[i].netTimeoutSize-i*2;
					else
						fprintf(logFile, "usefull\n");
					sessionInfo.players[i].netTOTL++;
				}
		}

	}
	break;
	case HGS_WAITING_CROSS_CONNECTIONS :
	{
		bool allPlayersCrossConnected=true;
		
		for (int j=0; j<sessionInfo.numberOfPlayer; j++)
			if (sessionInfo.players[j].type==BasePlayer::P_IP)
				if (crossPacketRecieved[j]<3)
				{
					fprintf(logFile, "player %d is not cross connected.\n", j);
					allPlayersCrossConnected=false;
					break;
				}
		
		if (allPlayersCrossConnected && (hostGlobalState>=HGS_WAITING_CROSS_CONNECTIONS))
		{
			fprintf(logFile, "Great, all players are cross connected, Game could start, except the file!.\n");
			hostGlobalState=HGS_ALL_PLAYERS_CROSS_CONNECTED;
		}
	}
	break;

	case HGS_ALL_PLAYERS_CROSS_CONNECTED :
	{
		bool allPlayersHaveFile=true;
		
		for (int j=0; j<sessionInfo.numberOfPlayer; j++)
			if (sessionInfo.players[j].type==BasePlayer::P_IP)
				if (playerFileTra[j].wantsFile)
				{
					fprintf(logFile, "player %d is still downloading game file.\n", j);
					allPlayersHaveFile=false;
					break;
				}
		
		if (allPlayersHaveFile && (hostGlobalState>=HGS_ALL_PLAYERS_CROSS_CONNECTED))
		{
			fprintf(logFile, "Great, all players have the game file too, Game could start!.\n");
			hostGlobalState=HGS_ALL_PLAYERS_CROSS_CONNECTED_AND_HAVE_FILE;
		}
	}
	break;
	
	case HGS_ALL_PLAYERS_CROSS_CONNECTED_AND_HAVE_FILE :
		
	break;

	case HGS_GAME_START_SENDED:
	{
		bool allPlayersPlaying=true;
		
		for (int j=0; j<sessionInfo.numberOfPlayer; j++)
			if (sessionInfo.players[j].type==BasePlayer::P_IP)
			{
				if (crossPacketRecieved[j]<4)
				{
					allPlayersPlaying=false;
					break;
				}
			}
		
		if (allPlayersPlaying && (hostGlobalState>=HGS_ALL_PLAYERS_CROSS_CONNECTED_AND_HAVE_FILE))
		{
			fprintf(logFile, "Great, all players have recieved start info.\n");
			hostGlobalState=HGS_PLAYING_COUNTER;
		}
	}
	break;

	case HGS_PLAYING_COUNTER :
	{

	}
	break;

	default:
	{
		fprintf(logFile, "This is a bad and unknow(%d) hostGlobalState case. Should not happend!\n",hostGlobalState);
	}
	break;

	}

}

void MultiplayersHost::kickPlayer(int p)
{
	if (sessionInfo.players[p].type==BasePlayer::P_IP)
		sessionInfo.players[p].send(SERVER_KICKED_YOU);
	removePlayer(p);
}

void MultiplayersHost::removePlayer(int p)
{
	bool wasKnownByOthers=(sessionInfo.players[p].netState>BasePlayer::PNS_PLAYER_SEND_PRESENCE_REQUEST);
	int t=sessionInfo.players[p].teamNumber;
	fprintf(logFile, "player %d quited the game, from team %d.\n", p, t);
	sessionInfo.team[t].playersMask&=~sessionInfo.players[p].numberMask;
	sessionInfo.team[t].numberOfPlayer--;

	sessionInfo.players[p].netState=BasePlayer::PNS_BAD;
	sessionInfo.players[p].type=BasePlayer::P_NONE;
	sessionInfo.players[p].netTimeout=0;
	sessionInfo.players[p].netTimeoutSize=DEFAULT_NETWORK_TIMEOUT;//Relase version
	sessionInfo.players[p].netTimeoutSize=0;// TODO : Only for debug version
	sessionInfo.players[p].netTOTL=0;

	sessionInfo.players[p].close();
	int mp=sessionInfo.numberOfPlayer-1;
	if (mp>p)
	{
		fprintf(logFile, "replace it by another player: %d\n", mp);
		int mt=sessionInfo.players[mp].teamNumber;
		sessionInfo.team[mt].playersMask&=~sessionInfo.players[mp].numberMask;
		sessionInfo.team[mt].numberOfPlayer--;

		sessionInfo.players[p]=sessionInfo.players[mp];

		sessionInfo.players[p].type=sessionInfo.players[mp].type;
		sessionInfo.players[p].netState=sessionInfo.players[mp].netState;
		sessionInfo.players[p].netTimeout=sessionInfo.players[mp].netTimeout;
		sessionInfo.players[p].netTimeoutSize=sessionInfo.players[mp].netTimeoutSize;
		sessionInfo.players[p].netTOTL=sessionInfo.players[mp].netTOTL;
		sessionInfo.players[p].numberMask=sessionInfo.players[mp].numberMask;

		//int t=(p%sessionInfo.numberOfTeam);
		int t=sessionInfo.players[mp].teamNumber;
		sessionInfo.players[p].setNumber(p);
		sessionInfo.players[p].setTeamNumber(t);
		
		// We erase replaced player:
		sessionInfo.players[mp].init();

		sessionInfo.team[t].playersMask|=sessionInfo.players[p].numberMask;
		sessionInfo.team[t].numberOfPlayer++;
	}
	sessionInfo.numberOfPlayer--;
	fprintf(logFile, "nop %d.\n", sessionInfo.numberOfPlayer);
	
	if (wasKnownByOthers)
	{
		// all other players are ignorant of the new situation:
		initHostGlobalState();
		reinitPlayersState();
	}
}

void MultiplayersHost::switchPlayerTeam(int p)
{
	Sint32 teamNumber=(sessionInfo.players[p].teamNumber+1)%sessionInfo.numberOfTeam;
	sessionInfo.players[p].setTeamNumber(teamNumber);
	
	reinitPlayersState();
}

void MultiplayersHost::removePlayer(char *data, int size, IPaddress ip)
{
	int i;
	for (i=0; i<sessionInfo.numberOfPlayer; i++)
		if (sessionInfo.players[i].sameip(ip))
			break;
	if (i>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%x, %d) has sended a quit game !!!\n", ip.host, ip.port);
		return;
	}
	removePlayer(i);
}

void MultiplayersHost::newPlayerPresence(char *data, int size, IPaddress ip)
{
	fprintf(logFile, "MultiplayersHost::newPlayerPresence().\n");
	if (size!=40)
	{
		fprintf(logFile, "Bad size(%d) for an Presence request from ip %s.\n", size, Utilities::stringIP(ip));
		return;
	}
	int p=sessionInfo.numberOfPlayer;
	int t=newTeamIndice();
	assert(BasePlayer::MAX_NAME_LENGTH==32);
	if (savedSessionInfo)
	{
		char playerName[BasePlayer::MAX_NAME_LENGTH];
		memcpy(playerName, (char *)(data+8), 32);
		t=savedSessionInfo->getTeamNumber(playerName, t);
	}
	
	
	playerFileTra[p].wantsFile=false;
	playerFileTra[p].receivedFile=false;
	playerFileTra[p].packetSize=64;
	playerFileTra[p].windowSize=20;
	playerFileTra[p].totalLost=0;
	playerFileTra[p].totalSent=0;
	playerFileTra[p].onlyWaited=0;
	playerFileTra[p].unreceivedIndex=0;
	for (int i=0; i<NET_WINDOW_SIZE; i++)
	{
		playerFileTra[p].window[i].index=0;
		playerFileTra[p].window[i].sent=false;
		playerFileTra[p].window[i].received=false;
		playerFileTra[p].window[i].time=i%SECOND_TIMEOUT;
		playerFileTra[p].window[i].packetSize=64;
	}
	
	sessionInfo.players[p].init();
	sessionInfo.players[p].type=BasePlayer::P_IP;
	sessionInfo.players[p].setNumber(p);
	sessionInfo.players[p].setTeamNumber(t);
	memcpy(sessionInfo.players[p].name, (char *)(data+8), 32);
	sessionInfo.players[p].setip(ip);
	sessionInfo.players[p].ipFromNAT=(bool)getSint32(data, 4);
	fprintf(logFile, "this ip(%s) has ipFromNAT=%d!\n", Utilities::stringIP(ip), sessionInfo.players[p].ipFromNAT);

	// we check if this player has already a connection:

	for (int i=0; i<p; i++)
	{
		if (sessionInfo.players[i].sameip(ip))
		{
			fprintf(logFile, "this ip(%s) is already in the player list!\n", Utilities::stringIP(ip));

			sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_SEND_PRESENCE_REQUEST;
			sessionInfo.players[i].netTimeout=0;
			sessionInfo.players[i].netTimeoutSize=LONG_NETWORK_TIMEOUT;
			sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL+1;
			return;
		}
	}

	int freeChannel=getFreeChannel();
	if (!sessionInfo.players[p].bind(socket, freeChannel))
	{
		fprintf(logFile, "this ip(%s) is not bindable\n", Utilities::stringIP(ip));
		return;
	}

	if ( sessionInfo.players[p].send(SERVER_PRESENCE) )
	{
		fprintf(logFile, "newPlayerPresence::this ip(%s) is added in player list. (player %d)\n", Utilities::stringIP(ip), p);
		sessionInfo.numberOfPlayer++;
		sessionInfo.team[sessionInfo.players[p].teamNumber].playersMask|=sessionInfo.players[p].numberMask;
		sessionInfo.team[sessionInfo.players[p].teamNumber].numberOfPlayer++;
		sessionInfo.players[p].netState=BasePlayer::PNS_PLAYER_SEND_PRESENCE_REQUEST;
		sessionInfo.players[p].netTimeout=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[p].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[p].netTOTL=DEFAULT_NETWORK_TOTL;
	}
}

void MultiplayersHost::playerWantsSession(char *data, int size, IPaddress ip)
{
	if (size!=12)
	{
		fprintf(logFile, "Bad size(%d) for an Session request from ip %s.\n", size, Utilities::stringIP(ip));
		return;
	}
	
	int p;
	for (p=0; p<sessionInfo.numberOfPlayer; p++)
		if (sessionInfo.players[p].sameip(ip))
			break;
	if (p>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sended a Session request !!!\n", Utilities::stringIP(ip));
		return;
	}
	
	bool serverIPReceived=false;
	Uint32 newHost=SDL_SwapBE32(getUint32(data, 4));
	Uint32 newPort=(Uint32)SDL_SwapBE16((Uint16)getUint32(data, 8));
	if (serverIP.host)
	{
		if (serverIP.host!=newHost)
		{
			fprintf(logFile, "Bad ip received by(%s). old=(%s) new=(%s)\n", Utilities::stringIP(ip), Utilities::stringIP(serverIP.host), Utilities::stringIP(newHost));
			return;
		}
		if (serverIP.port!=newPort)
		{
			fprintf(logFile, "Bad port received by(%s). old=(%d) new=(%d)\n", Utilities::stringIP(ip), serverIP.port, newPort);
			return;
		}
	}
	else
	{
		serverIP.host=newHost;
		serverIP.port=newPort;
		serverIPReceived=true;
		fprintf(logFile, "I recived my ip!:(%s).\n", Utilities::stringIP(serverIP));
	}

	sessionInfo.players[p].netState=BasePlayer::PNS_PLAYER_SEND_SESSION_REQUEST;
	sessionInfo.players[p].netTimeout=0;
	sessionInfo.players[p].netTimeoutSize=LONG_NETWORK_TIMEOUT;
	sessionInfo.players[p].netTOTL=DEFAULT_NETWORK_TOTL+1;

	fprintf(logFile, "this ip(%s) wantsSession (player %d)\n", Utilities::stringIP(ip), p);
	
	// all other players are ignorant of the new situation:
	initHostGlobalState();
	reinitPlayersState();
	
	if (serverIPReceived)
		sessionInfo.players[p].netTimeout=3; // =1 would be enough, if loopback where safe.
}

void MultiplayersHost::playerWantsFile(char *data, int size, IPaddress ip)
{
	if (size!=72)
	{
		fprintf(logFile, "Bad size(%d) for an File request from ip %s.\n", size, Utilities::stringIP(ip));
		return;
	}
	
	int p;
	for (p=0; p<sessionInfo.numberOfPlayer; p++)
		if (sessionInfo.players[p].sameip(ip))
			break;
	if (p>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sended a File request !!!\n", Utilities::stringIP(ip));
		return;
	}
	
	if (!playerFileTra[p].wantsFile)
	{
		if (!playerFileTra[p].receivedFile)
		{
			fprintf(logFile, "player (%s) first requests file.\n", Utilities::stringIP(ip));
			playerFileTra[p].wantsFile=true;
			for (int i=0; i<NET_WINDOW_SIZE; i++)
			{
				playerFileTra[p].window[i].index=0;
				playerFileTra[p].window[i].sent=false;
				playerFileTra[p].window[i].received=false;
				playerFileTra[p].window[i].time=i%SECOND_TIMEOUT;
				playerFileTra[p].window[i].packetSize=64;
			}

			playerFileTra[p].packetSize=64;
			playerFileTra[p].windowSize=20;
		}
		
		Uint32 unreceivedIndex=getUint32(data, 4);
		if (unreceivedIndex!=0xFFFFFFFF)
		{
			char data[12];
			data[0]=FULL_FILE_DATA;
			data[1]=0;
			data[2]=0;
			data[3]=0;
			addSint32(data, (Sint32)-1, 4);
			addUint32(data, fileSize, 8);

			bool success=sessionInfo.players[p].send(data, 12);
			assert(success);
			playerFileTra[p].totalLost++;
			playerFileTra[p].totalSent++;
			playerFileTra[p].windowstats[playerFileTra[p].windowSize]++;
		}
	}
	else
	{
		Uint32 unreceivedIndex=getUint32(data, 4);
		if (unreceivedIndex<playerFileTra[p].unreceivedIndex)
		{
			fprintf(logFile, "Bad FileRequest packet received !!!\n");
			return;
		}
		assert(unreceivedIndex>=playerFileTra[p].unreceivedIndex); //TODO: this is another security hole.
		playerFileTra[p].unreceivedIndex=unreceivedIndex;
		fprintf(logFile, "unreceivedIndex=%d\n", unreceivedIndex);
		
		if (unreceivedIndex==fileSize || unreceivedIndex==0xFFFFFFFF)
		{
			playerFileTra[p].wantsFile=false;
			playerFileTra[p].receivedFile=true;
			
			fprintf(logFile, "player %d\n", p);
			int totLost=0;
			for (int i=0; i<MAX_WINDOW_SIZE; i++)
			{
				totLost+=playerFileTra[p].windowlosts[i];
				if (playerFileTra[p].windowstats[i]>0)
					fprintf(logFile, "%3d \t %3d \t %3d \t %3d\n", i, playerFileTra[p].windowstats[i], playerFileTra[p].windowlosts[i], totLost);
			}
			fprintf(logFile, "playerFileTra[p].packetSize=%d.\n", playerFileTra[p].packetSize);
			fprintf(logFile, "playerFileTra[p].totalLost=%d. (%f)\n", playerFileTra[p].totalLost, (float)playerFileTra[p].totalLost/playerFileTra[p].totalSent);
			fprintf(logFile, "playerFileTra[p].totalSent=%d.\n", playerFileTra[p].totalSent);
			fprintf(logFile, "playerFileTra[p].onlyWaited=%d.\n", playerFileTra[p].onlyWaited);
			
			stepHostGlobalState();
		}
		else
		{
			Uint32 receivedIndex[16];
			fprintf(logFile, "receivedIndex=(");
			for (int ix=0; ix<16; ix++)
			{
				receivedIndex[ix]=getUint32(data, 8+ix*4);
				fprintf(logFile, "%d, ", receivedIndex[ix]);
				assert(receivedIndex[ix]>unreceivedIndex);
			}
			fprintf(logFile, ").\n");

			for (int i=0; i<NET_WINDOW_SIZE; i++)
			{
				Uint32 index=playerFileTra[p].window[i].index;
				if (index<unreceivedIndex && playerFileTra[p].window[i].sent && !playerFileTra[p].window[i].received)
				{
					playerFileTra[p].windowSize++;
					playerFileTra[p].window[i].received=true;
				}
				else if (index>unreceivedIndex)
					for (int ix=0; ix<16; ix++)
						if (index==receivedIndex[ix] && playerFileTra[p].window[i].sent && !playerFileTra[p].window[i].received)
						{
							playerFileTra[p].windowSize++;
							playerFileTra[p].window[i].received=true;
						}
			}
			
			for (int i=0; i<NET_WINDOW_SIZE; i++)
				if (playerFileTra[p].window[i].index==unreceivedIndex)
					assert(!playerFileTra[p].window[i].received);
			
			int totLost=0;
			int maxTotLost=playerFileTra[p].totalSent/200;
			int imax=-1;
			for (int i=0; i<MAX_WINDOW_SIZE; i++)
			{
				totLost+=playerFileTra[p].windowlosts[i];
				if (totLost>maxTotLost)
				{
					imax=i;
					break;
				}
			}
			if (imax!=-1 && imax<playerFileTra[p].windowSize)
				playerFileTra[p].windowSize=imax;
			
			if (playerFileTra[p].windowSize>=MAX_WINDOW_SIZE)
			{
				if (playerFileTra[p].packetSize>=32768)
				{
					playerFileTra[p].windowSize=MAX_WINDOW_SIZE-1;
					playerFileTra[p].packetSize=32768;
				}
				else
				{
					playerFileTra[p].windowSize/=2;
					playerFileTra[p].packetSize*=2;
					fprintf(logFile, "new packetSize=%d.\n", playerFileTra[p].packetSize);
					
					for (int i=0; i<MAX_WINDOW_SIZE; i++)
						playerFileTra[p].windowlosts[i]*=2;
				}
			}
			
			
			
			fprintf(logFile, "playerFileTra[p].windowSize=%d.\n", playerFileTra[p].windowSize);
		}
	}
	sessionInfo.players[p].netTimeout=sessionInfo.players[p].netTimeoutSize;
	sessionInfo.players[p].netTOTL=DEFAULT_NETWORK_TOTL;
}

void MultiplayersHost::addAI()
{
	int p=sessionInfo.numberOfPlayer;
	int t=newTeamIndice();
	if (savedSessionInfo)
		t=savedSessionInfo->getAITeamNumber(&sessionInfo, t);
	
	sessionInfo.players[p].init();
	sessionInfo.players[p].type=BasePlayer::P_AI;
	sessionInfo.players[p].setNumber(p);
	sessionInfo.players[p].setTeamNumber(t);
	sessionInfo.players[p].netState=BasePlayer::PNS_PLAYER_SEND_SESSION_REQUEST;
	strncpy(sessionInfo.players[p].name, globalContainer->texts.getString("[AI]", abs(rand())%globalContainer->texts.AI_NAME_SIZE), BasePlayer::MAX_NAME_LENGTH);
	
	sessionInfo.numberOfPlayer++;
	sessionInfo.team[sessionInfo.players[p].teamNumber].playersMask|=sessionInfo.players[p].numberMask;
	sessionInfo.team[sessionInfo.players[p].teamNumber].numberOfPlayer++;
	
	/*sessionInfo.players[p].netState=BasePlayer::PNS_AI;
	sessionInfo.players[p].netTimeout=0;
	sessionInfo.players[p].netTimeoutSize=LONG_NETWORK_TIMEOUT;
	sessionInfo.players[p].netTOTL=DEFAULT_NETWORK_TOTL+1;
	crossPacketRecieved[p]=4;*/
	
	// all other players are ignorant of the new situation:
	initHostGlobalState();
	reinitPlayersState();
}

void MultiplayersHost::confirmPlayer(char *data, int size, IPaddress ip)
{
	Sint32 rcs=getSint32(data, 4);
	Sint32 lcs=sessionInfo.checkSum();

	int i;
	for (i=0; i<sessionInfo.numberOfPlayer; i++)
		if (sessionInfo.players[i].sameip(ip))
			break;
	if (i>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sended a checksum !!!\n", Utilities::stringIP(ip));
		return;
	}

	if (rcs!=lcs)
	{
		fprintf(logFile, "this ip(%s) confirmed a wrong checksum (player %d)!\n", Utilities::stringIP(ip), i);
		fprintf(logFile, "rcs=%x, lcs=%x.\n", rcs, lcs);
		sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_SEND_SESSION_REQUEST;
		sessionInfo.players[i].netTimeout=0;
		sessionInfo.players[i].netTimeoutSize=LONG_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL+1;
		return;
	}
	else
	{
		fprintf(logFile, "this ip(%s) confirmed a good checksum (player %d)\n", Utilities::stringIP(ip), i);
		sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_SEND_CHECK_SUM;
		sessionInfo.players[i].netTimeout=0;
		sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL+1;
		fprintf(logFile, "this ip(%x) is confirmed in player list.\n", ip.host);
		return;
	}
}
void MultiplayersHost::confirmStartCrossConnection(char *data, int size, IPaddress ip)
{
	int i;
	for (i=0; i<sessionInfo.numberOfPlayer; i++)
		if (sessionInfo.players[i].sameip(ip))
			break;
	if (i>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sended a confirmStartCrossConnection !!!\n", Utilities::stringIP(ip));
		return;
	}

	if (sessionInfo.players[i].netState>=BasePlayer::PNS_SERVER_SEND_CROSS_CONNECTION_START)
	{
		sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_CONFIRMED_CROSS_CONNECTION_START;
		sessionInfo.players[i].netTimeout=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL;
		fprintf(logFile, "this ip(%s) is start cross connection confirmed..\n", Utilities::stringIP(ip));
		return;
	}
}
void MultiplayersHost::confirmStillCrossConnecting(char *data, int size, IPaddress ip)
{
	int i;
	for (i=0; i<sessionInfo.numberOfPlayer; i++)
		if (sessionInfo.players[i].sameip(ip))
			break;
	if (i>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sended a confirmStillCrossConnecting !!!\n", Utilities::stringIP(ip));
		return;
	}

	if (sessionInfo.players[i].netState>=BasePlayer::PNS_SERVER_SEND_CROSS_CONNECTION_START)
	{
		sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_CONFIRMED_CROSS_CONNECTION_START;
		sessionInfo.players[i].netTimeout=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL;
		sessionInfo.players[i].send(SERVER_CONFIRM_CLIENT_STILL_CROSS_CONNECTING);
		fprintf(logFile, "this ip(%s) is continuing cross connection confirmed..\n", Utilities::stringIP(ip));
		return;
	}
}

void MultiplayersHost::confirmCrossConnectionAchieved(char *data, int size, IPaddress ip)
{
	int i;
	for (i=0; i<sessionInfo.numberOfPlayer; i++)
		if (sessionInfo.players[i].sameip(ip))
			break;
	if (i>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sended a confirmCrossConnectionAchieved !!!\n", Utilities::stringIP(ip));
		return;
	}

	if (sessionInfo.players[i].netState>=BasePlayer::PNS_SERVER_SEND_CROSS_CONNECTION_START)
	{
		sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_FINISHED_CROSS_CONNECTION;
		sessionInfo.players[i].netTimeout=0;
		sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL;
		fprintf(logFile, "this ip(%s) is cross connection achievement confirmed..\n", Utilities::stringIP(ip));

		crossPacketRecieved[i]=3;

		// let's check if all players are cross Connected
		stepHostGlobalState();

		return;
	}
}

void MultiplayersHost::confirmPlayerStartGame(char *data, int size, IPaddress ip)
{
	if (size!=8)
	{
		fprintf(logFile, "A player (%s) has sent a bad sized confirmPlayerStartGame.\n", Utilities::stringIP(ip));
		return;
	}

	int i;
	for (i=0; i<sessionInfo.numberOfPlayer; i++)
		if (sessionInfo.players[i].sameip(ip))
			break;
	if (i>=sessionInfo.numberOfPlayer)
	{
		fprintf(logFile, "An unknow player (%s) has sent a confirmPlayerStartGame.\n", Utilities::stringIP(ip));
		return;
	}

	if (sessionInfo.players[i].netState>=BasePlayer::PNS_SERVER_SEND_START_GAME)
	{
		sessionInfo.players[i].netState=BasePlayer::PNS_PLAYER_CONFIRMED_START_GAME;
		sessionInfo.players[i].netTimeout=0;
		sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
		sessionInfo.players[i].netTOTL=DEFAULT_NETWORK_TOTL;
		int sgtc=data[4];
		if((abs(sgtc-startGameTimeCounter)<20) && (sgtc>0))
			startGameTimeCounter=(startGameTimeCounter*3+sgtc)/4;
			// ping=(startGameTimeCounter-sgtc)/2
			// startGameTimeCounter=(startGameTimeCounter+sgtc)/2 would be a full direct correction
			// but the division by 4 will gives a fair average ping between all players
		fprintf(logFile, "this ip(%s) confirmed start game within %d seconds.\n", Utilities::stringIP(ip), sgtc/20);

		crossPacketRecieved[i]=4;

		// let's check if all players are playing
		stepHostGlobalState();

		return;
	}
}

void MultiplayersHost::broadcastRequest(char *data, int size, IPaddress ip)
{
	if (size!=4)
	{
		fprintf(logFile, "broad:Bad size(%d) for a broadcast request from ip %s.\n", size, Utilities::stringIP(ip));
		return;
	}

	//int channel=getFreeChannel();
	//channel=SDLNet_UDP_Bind(socket, channel, &ip);
	//if (channel!=-1)
	{
		UDPpacket *packet=SDLNet_AllocPacket(68);
		
		if (packet==NULL)
		{
			fprintf(logFile, "broad:can't alocate packet!\n");
			return;
		}
		
		if (ip.host==0)
		{
			fprintf(logFile, "broad:can't have a null ip.host\n");
			return;
		}
		
		char data[68];
		if (shareOnYOG)
			data[0]=BROADCAST_RESPONSE_YOG;
		else
			data[0]=BROADCAST_RESPONSE_LAN;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		memset(&data[4], 0, 32);
		strncpy(&data[4], sessionInfo.map.getMapName(), 32);
		memset(&data[36], 0, 32);
		strncpy(&data[36], globalContainer->userName, 32);
		
		//fprintf(logFile, "MultiplayersHost sending1 (%d, %d, %d, %d).\n", data[4], data[5], data[6], data[7]);
		//fprintf(logFile, "MultiplayersHost sending2 (%s).\n", sessionInfo.map.getMapName());
		//fprintf(logFile, "MultiplayersHost sendingB (%s).\n", &data[4]);
		packet->len=68;
		memcpy((char *)packet->data, data, 68);
		
		bool sucess;
		
		packet->address=ip;
		//packet->channel=channel;
		packet->channel=-1;
		
		sucess=SDLNet_UDP_Send(socket, channel, packet)==1;
		// Notice that we can choose between giving a "channel", or the ip.
		// Here we do both. Then "channel" could be -1.
		// This is interesting because getFreeChannel() may return -1.
		// We have no real use of "channel".
		if (sucess)
			fprintf(logFile, "broad:sucedded to response. shareOnYOG=(%d)\n", shareOnYOG);
		
		
		SDLNet_FreePacket(packet);
		
		fprintf(logFile, "broad:Unbinding (socket=%x)(channel=%d).\n", (int)socket, channel);
		SDLNet_UDP_Unbind(socket, channel);
		channel=-1;
	}
	//else
	//{
	//	fprintf(logFile, "broad:can't bind (socket=%x).\n", (int)socket);
	//}
}

void MultiplayersHost::treatData(char *data, int size, IPaddress ip)
{
	fprintf(logFile, "MultiplayersHost::treatData (%d)\n", data[0]);
	if ((data[1]!=0)||(data[2]!=0)||(data[3]!=0))
	{
		printf("Bad packet received (%d,%d,%d,%d)!\n", data[0], data[1], data[2], data[3]);
		return;
	}
	if (hostGlobalState<HGS_GAME_START_SENDED)
	{
		switch (data[0])
		{
		case BROADCAST_REQUEST:
			broadcastRequest(data, size, ip);
		break;
		
		case NEW_PLAYER_WANTS_PRESENCE:
			newPlayerPresence(data, size, ip);
		break;
		
		case NEW_PLAYER_WANTS_SESSION_INFO:
			playerWantsSession(data, size, ip);
		break;
		
		case NEW_PLAYER_WANTS_FILE:
			playerWantsFile(data, size, ip);
		break;
		
		case NEW_PLAYER_SEND_CHECKSUM_CONFIRMATION:
			confirmPlayer(data, size, ip);
		break;

		case CLIENT_QUIT_NEW_GAME:
			removePlayer(data, size, ip);
		break;

		case PLAYERS_CONFIRM_START_CROSS_CONNECTIONS:
			confirmStartCrossConnection(data, size, ip);
		break;

		case PLAYERS_STILL_CROSS_CONNECTING:
			confirmStillCrossConnecting(data, size, ip);
		break;

		case PLAYERS_CROSS_CONNECTIONS_ACHIEVED:
			confirmCrossConnectionAchieved(data, size, ip);
		break;

		default:
			fprintf(logFile, "Unknow kind of packet(%d) recieved by ip(%s).\n", data[0], Utilities::stringIP(ip));
		};
	}
	else
	{
		switch (data[0])
		{
		case PLAYER_CONFIRM_GAME_BEGINNING :
			confirmPlayerStartGame(data, size, ip);
		break;

		default:
			fprintf(logFile, "Unknow kind of packet(%d) recieved by ip(%s).\n", data[0], Utilities::stringIP(ip));
		};
	}
}

void MultiplayersHost::onTimer(Uint32 tick)
{
	// call yog step
	if (shareOnYOG)
		globalContainer->yog->step(); // YOG cares about firewall and NAT
	
	if (hostGlobalState>=HGS_GAME_START_SENDED)
	{
		if (--startGameTimeCounter<0)
		{
			send(SERVER_ASK_FOR_GAME_BEGINNING, startGameTimeCounter);
			fprintf(logFile, "Lets quit this screen and start game!\n");
			if (hostGlobalState<=HGS_GAME_START_SENDED)
			{
				// done in game: drop player.
			}
		}
		else if (startGameTimeCounter%20==0)
		{
			send(SERVER_ASK_FOR_GAME_BEGINNING, startGameTimeCounter);
		}
	}
	else
		sendingTime();

	if (socket)
	{
		UDPpacket *packet=NULL;
		packet=SDLNet_AllocPacket(MAX_PACKET_SIZE);
		assert(packet);

		while (SDLNet_UDP_Recv(socket, packet)==1)
		{
			fprintf(logFile, "Packet received.\n");
			//fprintf(logFile, "packet=%d\n", (int)packet);
			//fprintf(logFile, "packet->channel=%d\n", packet->channel);
			//fprintf(logFile, "packet->len=%d\n", packet->len);
			//fprintf(logFile, "packet->maxlen=%d\n", packet->maxlen);
			//fprintf(logFile, "packet->status=%d\n", packet->status);
			//fprintf(logFile, "packet->address=%x,%d\n", packet->address.host, packet->address.port);

			//fprintf(logFile, "packet->data=%s\n", packet->data);

			treatData((char *)(packet->data), packet->len, packet->address);

			//paintSessionInfo(hostGlobalState);
			//addUpdateRect();
		}

		SDLNet_FreePacket(packet);
	}
}

bool MultiplayersHost::send(const int v)
{
	//fprintf(logFile, "Sending packet to all players (%d).\n", v);
	char data[4];
	data[0]=v;
	data[1]=0;
	data[2]=0;
	data[3]=0;
	for (int i=0; i<sessionInfo.numberOfPlayer; i++)
		sessionInfo.players[i].send(data, 4);

	return true;
}
bool MultiplayersHost::send(const int u, const int v)
{
	//fprintf(logFile, "Sending packet to all players (%d;%d).\n", u, v);
	char data[8];
	data[0]=u;
	data[1]=0;
	data[2]=0;
	data[3]=0;
	data[4]=v;
	data[5]=0;
	data[6]=0;
	data[7]=0;
	for (int i=0; i<sessionInfo.numberOfPlayer; i++)
		sessionInfo.players[i].send(data, 8);

	return true;
}

void MultiplayersHost::sendingTime()
{
	bool update=false;
	if (hostGlobalState<HGS_GAME_START_SENDED)
	{
		for (int i=0; i<sessionInfo.numberOfPlayer; i++)
			if (sessionInfo.players[i].netState==BasePlayer::PNS_BAD)
			{
				removePlayer(i);
				update=true;
			}

		if (update)
		{
			// all other players are ignorant of the new situation:
			initHostGlobalState();
			reinitPlayersState();
		}
	}
	
	// We send the file if necessary
	for (int p=0; p<32; p++)
	{
		BasePlayer &player=sessionInfo.players[p];
		if (playerFileTra[p].wantsFile)
		{
			bool onlyWait=true;
			
			int unreceived=0;
			Uint32 lastReceivedIndex=0; //TODO: optimisable!
			for (int i=0; i<NET_WINDOW_SIZE; i++)
				if (playerFileTra[p].window[i].sent)
				{
					Uint32 index=playerFileTra[p].window[i].index;
					if (playerFileTra[p].window[i].received)
					{
						if (index>lastReceivedIndex)
							lastReceivedIndex=index;
					}
					else
						unreceived++;
				}
			
			
			int toSend;
			if (playerFileTra[p].windowSize<=unreceived)
				toSend=0;
			else
			{
				toSend=playerFileTra[p].windowSize/20;
				if (toSend<1)
					toSend=1;
			}
			//if (toSend)
			fprintf(logFile, "unreceived=%d, windowSize=%d, toSend=%d.\n", unreceived, playerFileTra[p].windowSize, toSend);
			fprintf(logFile, "lastReceivedIndex=%d\n", lastReceivedIndex);
			
			for (int i=0; i<NET_WINDOW_SIZE; i++)
				if (playerFileTra[p].window[i].sent && !playerFileTra[p].window[i].received)
				{
					if (playerFileTra[p].window[i].index<lastReceivedIndex)
						playerFileTra[p].window[i].time+=2;//gaps are more probably lost
					else
						playerFileTra[p].window[i].time++;
				}
			
			Uint32 smallestIndexTimeout=0xFFFFFFFF;
			int wisit=-1;
			for (int i=0; i<NET_WINDOW_SIZE; i++)
				if (playerFileTra[p].window[i].sent && !playerFileTra[p].window[i].received && playerFileTra[p].window[i].time>LONG_NETWORK_TIMEOUT)
				{
					Uint32 index=playerFileTra[p].window[i].index;
					if (index<smallestIndexTimeout)
					{
						smallestIndexTimeout=index;
						wisit=i;
					}
				}
			if (wisit>=0)
			{
				int i=wisit;
				playerFileTra[p].window[i].time=0;

				toSend--;
				int sendingIndex=playerFileTra[p].window[i].index;
				// We have to resend this packet
				fprintf(logFile, "resending index=%d.\n", sendingIndex);

				int size=playerFileTra[p].window[i].packetSize;
				if (sendingIndex+size>(int)fileSize)
					size=fileSize-sendingIndex;
				assert(size>=0);
				if (size>=0)
				{
					char *data=(char *)malloc(size+12);
					assert(data);
					data[0]=FULL_FILE_DATA;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					addSint32(data, (Sint32)i, 4);
					addUint32(data, sendingIndex, 8);
					SDL_RWseek(stream, sendingIndex, SEEK_SET);
					SDL_RWread(stream, data+12, size, 1);

					bool success=player.send(data, size+12);
					assert(success);
					playerFileTra[p].totalLost++;
					playerFileTra[p].totalSent++;
					playerFileTra[p].windowstats[playerFileTra[p].windowSize]++;
					onlyWait=false;

					free(data);
				}
				
				int windowSize=playerFileTra[p].windowSize;
				playerFileTra[p].windowlosts[windowSize]++;
				windowSize=(windowSize*90)/100;
				windowSize--;
				if (windowSize<20)
				{
					windowSize=20;
					if (playerFileTra[p].packetSize>32)
					{
						playerFileTra[p].packetSize/=2;
						for (int i=0; i<MAX_WINDOW_SIZE; i++)
							playerFileTra[p].windowlosts[i]/=2;
						fprintf(logFile, "new packetSize=%d.\n", playerFileTra[p].packetSize);
					}
				}
				playerFileTra[p].windowSize=windowSize;
				fprintf(logFile, "windowSize=%d\n", windowSize);
			}
			
			if (toSend)
				fprintf(logFile, "toSend=%d\n", toSend);
			
			for (int t=0; t<toSend ; t++)
			{
				Uint32 sendingIndex=0;
				bool endOfFile=false;
				for (int i=0; i<NET_WINDOW_SIZE; i++)
					if (playerFileTra[p].window[i].sent)
					{
						Uint32 index=playerFileTra[p].window[i].index;
						Uint32 packetSize=playerFileTra[p].window[i].packetSize;
						Uint32 tot=index+packetSize;
						if (packetSize==0)
						{
							endOfFile=true;
							break;
						}
						else if (tot>sendingIndex)
							sendingIndex=tot;
						
					}
				if (endOfFile)
					break;
				int size=playerFileTra[p].packetSize;
				
				if (sendingIndex+size>fileSize)
					size=fileSize-sendingIndex;
				assert(size>=0);
				
				if (size>=0)
				{
					fprintf(logFile, "t=%d, sendingIndex=%d, size=%d\n", t, sendingIndex, size);
					
					char *data=(char *)malloc(size+12);
					assert(data);
					data[0]=FULL_FILE_DATA;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					addUint32(data, sendingIndex, 8);
					SDL_RWseek(stream, sendingIndex, SEEK_SET);
					SDL_RWread(stream, data+12, size, 1);

					// We look for a free window index:
					Uint32 smallestIndex=0xFFFFFFFF;
					int wi=-1;
					//first, we look for a free window slot:
					for (int i=0; i<NET_WINDOW_SIZE; i++)
						if (!playerFileTra[p].window[i].sent)
						{
							Uint32 index=playerFileTra[p].window[i].index;
							if (index<smallestIndex)
							{
								smallestIndex=index;
								wi=i;
							}
						}
					if (wi==-1) //Second, we need to free a widndow slot: we take the oldest sent:
						for (int i=0; i<NET_WINDOW_SIZE; i++)
							if (playerFileTra[p].window[i].sent && playerFileTra[p].window[i].received)
							{
								Uint32 index=playerFileTra[p].window[i].index;
								if (index<smallestIndex)
								{
									smallestIndex=index;
									wi=i;
								}
							}
							
					assert(wi!=-1);

					if (wi!=-1)
					{
						addSint32(data, (Sint32)wi, 4);

						playerFileTra[p].window[wi].index=sendingIndex;
						playerFileTra[p].window[wi].sent=true;
						playerFileTra[p].window[wi].received=false;
						playerFileTra[p].window[wi].time=0;
						playerFileTra[p].window[wi].packetSize=size;

						bool success=player.send(data, size+12);
						assert(success);
						playerFileTra[p].totalSent++;
						playerFileTra[p].windowstats[playerFileTra[p].windowSize]++;
						onlyWait=false;
					}

					free(data);
				}
			}
			
			if (onlyWait)
				playerFileTra[p].onlyWaited++;
		}
	}
	
	
	for (int i=0; i<sessionInfo.numberOfPlayer; i++)
	{
		if ((sessionInfo.players[i].type==BasePlayer::P_IP)&&(--sessionInfo.players[i].netTimeout<0))
		{
			update=true;
			sessionInfo.players[i].netTimeout+=sessionInfo.players[i].netTimeoutSize;

			assert(sessionInfo.players[i].netTimeoutSize);

			if (--sessionInfo.players[i].netTOTL<0)
			{
				if (hostGlobalState>=HGS_GAME_START_SENDED)
				{
					// we only drop the players, because other player are already playing.
					// will be done in the game!
				}
				else
				{
					sessionInfo.players[i].netState=BasePlayer::PNS_BAD;
					fprintf(logFile, "Last timeout for player %d has been spent.\n", i);
				}
			}

			switch (sessionInfo.players[i].netState)
			{
			case BasePlayer::PNS_BAD :
			{
				// we remove player out of this loop, to avoid mess.
			}
			break;

			case BasePlayer::PNS_PLAYER_SEND_PRESENCE_REQUEST :
			{
				fprintf(logFile, "Lets send the presence to player %d.\n", i);
				sessionInfo.players[i].send(SERVER_PRESENCE);
			}
			break;

			case BasePlayer::PNS_PLAYER_SEND_SESSION_REQUEST :
			{
				fprintf(logFile, "Lets send the session info to player %d.\n", i);

				char *data=NULL;
				int size=sessionInfo.getDataLength();

				data=(char *)malloc(size+8);
				assert(data);

				data[0]=DATA_SESSION_INFO;
				data[1]=0;
				data[2]=0;
				data[3]=0;
				addSint32(data, i, 4);

				memcpy(data+8, sessionInfo.getData(), size);

				sessionInfo.players[i].send(data, size+8);
				
				free(data);
			}
			break;


			case BasePlayer::PNS_PLAYER_SEND_CHECK_SUM :
			{
				fprintf(logFile, "Lets send the confiramtion for checksum to player %d.\n", i);
				char data[8];
				data[0]=SERVER_SEND_CHECKSUM_RECEPTION;
				data[1]=0;
				data[2]=0;
				data[3]=0;
				addSint32(data, sessionInfo.checkSum(), 4);
				sessionInfo.players[i].send(data, 8);

				// Now that's not our problem if this packet don't sucess.
				// In such a case, the client will reply.
				sessionInfo.players[i].netTimeout=0;
				sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
				sessionInfo.players[i].netState=BasePlayer::PNS_OK;

				// Lets check if all players has the sessionInfo:
				stepHostGlobalState();

				fprintf(logFile, "player %d is know ok. (%d)\n", i, sessionInfo.players[i].netState);
			}
			break;


			case BasePlayer::PNS_OK :
			{
				if (hostGlobalState>=HGS_WAITING_CROSS_CONNECTIONS)
				{

					sessionInfo.players[i].netState=BasePlayer::PNS_SERVER_SEND_CROSS_CONNECTION_START;
					sessionInfo.players[i].netTimeout=0;
					sessionInfo.players[i].netTimeoutSize=SHORT_NETWORK_TIMEOUT;
					sessionInfo.players[i].netTOTL++;
				}
				else
					fprintf(logFile, "Player %d is all right, TOTL %d.\n", i, sessionInfo.players[i].netTOTL);
				// players keeps ok.
			}
			break;

			case BasePlayer::PNS_SERVER_SEND_CROSS_CONNECTION_START :
			{
				fprintf(logFile, "We have to inform player %d to start cross connection.\n", i);
				sessionInfo.players[i].send(PLAYERS_CAN_START_CROSS_CONNECTIONS);
			}
			break;

			case BasePlayer::PNS_PLAYER_CONFIRMED_CROSS_CONNECTION_START :
			{
				fprintf(logFile, "Player %d is cross connecting, TOTL %d.\n", i, sessionInfo.players[i].netTOTL);
				sessionInfo.players[i].send(PLAYERS_CAN_START_CROSS_CONNECTIONS);
			}
			break;

			case BasePlayer::PNS_PLAYER_FINISHED_CROSS_CONNECTION :
			{
				fprintf(logFile, "We have to inform player %d that we recieved his crossConnection confirmation.\n", i);
				sessionInfo.players[i].send(SERVER_HEARD_CROSS_CONNECTION_CONFIRMATION);

				sessionInfo.players[i].netState=BasePlayer::PNS_CROSS_CONNECTED;
			}
			break;

			case BasePlayer::PNS_CROSS_CONNECTED :
			{
				fprintf(logFile, "Player %d is cross connected ! Yahoo !, TOTL %d.\n", i, sessionInfo.players[i].netTOTL);
			}
			break;

			case BasePlayer::PNS_SERVER_SEND_START_GAME :
			{
				fprintf(logFile, "We send start game to player %d, TOTL %d.\n", i, sessionInfo.players[i].netTOTL);
				sessionInfo.players[i].send(SERVER_ASK_FOR_GAME_BEGINNING);
			}
			break;

			case BasePlayer::PNS_PLAYER_CONFIRMED_START_GAME :
			{
				// here we could tell other players
				fprintf(logFile, "Player %d plays, TOTL %d.\n", i, sessionInfo.players[i].netTOTL);
			}
			break;

			default:
			{
				fprintf(logFile, "Buggy state for player %d.\n", i);
			}

			}

		}
	}
	
}

void MultiplayersHost::stopHosting(void)
{
	fprintf(logFile, "Every player has one chance to get the server-quit packet.\n");
	send(SERVER_QUIT_NEW_GAME);
	
	if (shareOnYOG)
	{
		globalContainer->yog->unshareGame();
	}
}

void MultiplayersHost::startGame(void)
{
	if(hostGlobalState>=HGS_ALL_PLAYERS_CROSS_CONNECTED_AND_HAVE_FILE)
	{
		fprintf(logFile, "Lets tell all players to start game.\n");
		startGameTimeCounter=SECOND_TIMEOUT*SECONDS_BEFORE_START_GAME;
		{
			for (int i=0; i<sessionInfo.numberOfPlayer; i++)
			{
				sessionInfo.players[i].netState=BasePlayer::PNS_SERVER_SEND_START_GAME;
				sessionInfo.players[i].send(SERVER_ASK_FOR_GAME_BEGINNING, startGameTimeCounter);
			}
		}
		hostGlobalState=HGS_GAME_START_SENDED;

		// let's check if all players are playing
		stepHostGlobalState();
	}
	else
		fprintf(logFile, "can't start now. hostGlobalState=(%d)<(%d)\n", hostGlobalState, HGS_ALL_PLAYERS_CROSS_CONNECTED_AND_HAVE_FILE);

}
