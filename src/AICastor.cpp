/*
  Copyright (C) 2001-2004 Stephane Magnenat & Luc-Olivier de Charrière
  for any question or comment contact us at nct@ysagoon.com or nuage@ysagoon.com

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

#include "AICastor.h"
#include "Game.h"
#include "GlobalContainer.h"
#include "LogFileManager.h"
#include "Order.h"
#include "Player.h"
#include "Unit.h"
#include "Utilities.h"


#define AI_FILE_MIN_VERSION 1
#define AI_FILE_VERSION 2

inline static void dxdyfromDirection(int direction, int *dx, int *dy)
{
	const int tab[9][2]={	{ -1, -1},
							{ 0, -1},
							{ 1, -1},
							{ 1, 0},
							{ 1, 1},
							{ 0, 1},
							{ -1, 1},
							{ -1, 0},
							{ 0, 0} };
	assert(direction>=0);
	assert(direction<=8);
	*dx=tab[direction][0];
	*dy=tab[direction][1];
}

void AICastor::firstInit()
{
	obstacleUnitMap=NULL;
	obstacleBuildingMap=NULL;
	spaceForBuildingMap=NULL;
	buildingNeighbourMap=NULL;
	twoSpaceNeighbourMap=NULL;
	
	workPowerMap=NULL;
	workRangeMap=NULL;
	workAbilityMap=NULL;
	hydratationMap=NULL;
	wheatGrowthMap=NULL;
	
	goodBuildingMap=NULL;
	
	ressourcesCluster=NULL;
}

AICastor::AICastor(Player *player)
{
	logFile=globalContainer->logFileManager->getFile("AICastor.log");
	
	firstInit();
	init(player);
}

AICastor::AICastor(SDL_RWops *stream, Player *player, Sint32 versionMinor)
{
	logFile=globalContainer->logFileManager->getFile("AICastor.log");
	
	firstInit();
	bool goodLoad=load(stream, player, versionMinor);
	assert(goodLoad);
}

void AICastor::init(Player *player)
{
	assert(player);
	
	// Logical :
	timer=0;
	canSwim=false;
	needPool=false;
	computeNeedPoolTimer=0;
	controlSwarmsTimer=0;
	expandFoodTimer=0;
	hydratationMapComputed=false;
	
	for (std::list<Project *>::iterator pi=projects.begin(); pi!=projects.end(); pi++)
		delete *pi;
	projects.clear();

	// Structural:
	this->player=player;
	this->team=player->team;
	this->game=player->game;
	this->map=player->map;

	assert(this->team);
	assert(this->game);
	assert(this->map);
	
	size_t size=map->w*map->h;
	assert(size>0);
	
	if (obstacleUnitMap!=NULL)
		delete[] obstacleUnitMap;
	obstacleUnitMap=new Uint8[size];
	
	if (obstacleBuildingMap!=NULL)
		delete[] obstacleBuildingMap;
	obstacleBuildingMap=new Uint8[size];
	
	if (spaceForBuildingMap!=NULL)
		delete[] spaceForBuildingMap;
	spaceForBuildingMap=new Uint8[size];
	
	if (buildingNeighbourMap!=NULL)
		delete[] buildingNeighbourMap;
	buildingNeighbourMap=new Uint8[size];
	
	if (twoSpaceNeighbourMap!=NULL)
		delete[] twoSpaceNeighbourMap;
	twoSpaceNeighbourMap=new Uint8[size];
	
	
	if (workPowerMap!=NULL)
		delete[] workPowerMap;
	workPowerMap=new Uint8[size];
	
	if (workRangeMap!=NULL)
		delete[] workRangeMap;
	workRangeMap=new Uint8[size];
	
	if (workAbilityMap!=NULL)
		delete[] workAbilityMap;
	workAbilityMap=new Uint8[size];
	
	if (hydratationMap!=NULL)
		delete[] hydratationMap;
	hydratationMap=new Uint8[size];
	
	if (wheatGrowthMap!=NULL)
		delete[] wheatGrowthMap;
	wheatGrowthMap=new Uint8[size];
	
	if (goodBuildingMap!=NULL)
		delete[] goodBuildingMap;
	goodBuildingMap=new Uint8[size];
	
	
	if (ressourcesCluster!=NULL)
		delete[] ressourcesCluster;
	ressourcesCluster=new Uint16[size];
}

AICastor::~AICastor()
{
	if (obstacleUnitMap!=NULL)
		delete[] obstacleUnitMap;
	
	if (obstacleBuildingMap!=NULL)
		delete[] obstacleBuildingMap;
	
	if (spaceForBuildingMap!=NULL)
		delete[] spaceForBuildingMap;
	
	if (buildingNeighbourMap!=NULL)
		delete[] buildingNeighbourMap;
	
	if (twoSpaceNeighbourMap!=NULL)
		delete[] twoSpaceNeighbourMap;
	
	
	if (workPowerMap!=NULL)
		delete[] workPowerMap;
	
	if (workRangeMap!=NULL)
		delete[] workRangeMap;
	
	if (workAbilityMap!=NULL)
		delete[] workAbilityMap;
	
	if (hydratationMap!=NULL)
		delete[] hydratationMap;
	
	if (wheatGrowthMap!=NULL)
		delete[] wheatGrowthMap;
	
	if (goodBuildingMap!=NULL)
		delete[] goodBuildingMap;
	
	
	if (ressourcesCluster!=NULL)
		delete[] ressourcesCluster;
}

bool AICastor::load(SDL_RWops *stream, Player *player, Sint32 versionMinor)
{
	printf("AICastor::load\n");
	init(player);
	assert(game);
	
	if (versionMinor<29)
	{
		//TODO:init
		return true;
	}
	
	Sint32 aiFileVersion=SDL_ReadBE32(stream);
	if (aiFileVersion<AI_FILE_MIN_VERSION)
		return false;
	if (aiFileVersion>=1)
		timer=SDL_ReadBE32(stream);
	else
		timer=0;
	
	return true;
}

void AICastor::save(SDL_RWops *stream)
{
	SDL_WriteBE32(stream, AI_FILE_VERSION);
	SDL_WriteBE32(stream, timer);
}

Order *AICastor::getOrder(void)
{
	timer++;
	
	if (!hydratationMapComputed)
		computeHydratationMap();
	
	//printf("getOrder(), %d projects\n", projects.size());
	for (std::list<Project *>::iterator pi=projects.begin(); pi!=projects.end(); pi++)
		if ((*pi)->finished)
		{
			printf("deleting project %s\n", (*pi)->debugName);
			delete *pi;
			pi=projects.erase(pi);
		}
	bool blocking=false;
	bool critical=false;
	for (std::list<Project *>::iterator pi=projects.begin(); pi!=projects.end(); pi++)
	{
		if ((*pi)->blocking)
			blocking=true;
		if ((*pi)->critical)
			critical=true;
	}
	if (!blocking)// No blocking project, we can start a new one:
		addProjects();
	
	if (timer>controlSwarmsTimer)
	{
		controlSwarmsTimer=timer+256; // each 10s
		Order *order=controlSwarms();
		if (order)
			return order;
	}
	
	for (std::list<Project *>::iterator pi=projects.begin(); pi!=projects.end(); pi++)
	{
		Order *order=continueProject(*pi);
		if (order)
			return order;
	}
	
	if (!critical && timer>expandFoodTimer)
	{
		expandFoodTimer=timer+128; // each 5s
		Order *order=expandFood();
		if (order)
			return order;
	}
	
	return new NullOrder();
}

Order *AICastor::controlSwarms(void)
{
	int unitSum[NB_UNIT_TYPE];
	for (int i=0; i<NB_UNIT_TYPE; i++)
		unitSum[i]=0;
	Unit **myUnits=team->myUnits;
	for (int i=0; i<1024; i++)
	{
		Unit *u=myUnits[i];
		if (u)
			unitSum[u->typeNum]++;
	}
	int foodSum=0;
	Building **myBuildings=team->myBuildings;
	for (int i=0; i<1024; i++)
	{
		Building *b=myBuildings[i];
		if (b && b->type->canFeedUnit)
			foodSum+=b->type->maxUnitInside;
	}
	
	int unitSumAll=unitSum[0]+unitSum[1]+unitSum[2];
	printf("unitSum=[%d, %d, %d], unitSumAll=%d, foodSum=%d\n", unitSum[0], unitSum[1], unitSum[2], unitSumAll, foodSum);
	
	if (unitSumAll>=foodSum)
	{
		// Stop making any units!
		Building **myBuildings=team->myBuildings;
		for (int bi=0; bi<1024; bi++)
		{
			Building *b=myBuildings[bi];
			if (b && b->type->unitProductionTime)
				for (int ri=0; ri<NB_UNIT_TYPE; ri++)
					if (b->ratio[ri]!=0)
					{
						for (int ri=0; ri<NB_UNIT_TYPE; ri++)
						{
							b->ratio[ri]=0;
							b->ratioLocal[ri]=0;
						}
						b->update();
						return new OrderModifySwarm(b->gid, b->ratioLocal);
					}
		}
		
		return NULL;
	}
	
	size_t size=map->w*map->h;
	int discovered=0;
	int seeable=0;
	Uint32 *mapDiscovered=map->mapDiscovered;
	Uint32 *fogOfWar=map->fogOfWar;
	Uint32 me=team->me;
	for (size_t i=0; i<size; i++)
	{
		if (mapDiscovered[i]&me!=0)
			discovered++;
		if (fogOfWar[i]&me!=0)
			seeable++;
	}
	Sint32 explorerGoal;
	if (unitSum[WORKER]<4)
		explorerGoal=0;
	else if (unitSum[EXPLORER]==0)
		explorerGoal=1;
	else if ((unitSum[EXPLORER]<<4)<unitSum[WORKER])
		explorerGoal=1;
	else
		explorerGoal=0;
	Sint32 workerGoal=3;
	
	printf("discovered=%d, seeable=%d, size=%d, explorerGoal=%d\n", discovered, seeable, size, explorerGoal);
	
	for (int bi=0; bi<1024; bi++)
	{
		Building *b=myBuildings[bi];
		if (b && b->type->unitProductionTime)
		{
			if (b->ratio[EXPLORER]!=explorerGoal)
			{
				b->ratio[EXPLORER]=explorerGoal;
				b->ratioLocal[EXPLORER]=explorerGoal;
				b->ratio[WORKER]=workerGoal;
				b->ratioLocal[WORKER]=workerGoal;
				b->update();
				return new OrderModifySwarm(b->gid, b->ratioLocal);
			}
		}
	}
	
	for (int bi=0; bi<1024; bi++)
	{
		Building *b=myBuildings[bi];
		if (b && b->type->unitProductionTime)
		{
			if (b->ratio[WORKER]!=workerGoal)
			{
				b->ratio[WORKER]=workerGoal;
				b->ratioLocal[WORKER]=workerGoal;
				b->update();
				return new OrderModifySwarm(b->gid, b->ratioLocal);
			}
		}
	}
	
	
	return NULL;
}

Order *AICastor::expandFood(void)
{
	Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::FOOD_BUILDING, 0, true);
	int bw=globalContainer->buildingsTypes.get(typeNum)->width;
	int bh=globalContainer->buildingsTypes.get(typeNum)->height;
	assert(bw==bh);
	
	computeCanSwim();
	computeObstacleBuildingMap();
	computeSpaceForBuildingMap(bw);
	computeBuildingNeighbourMap(bw, bh);
	computeWheatGrowthMap(bw, bh);
	computeObstacleUnitMap();
	computeWorkPowerMap();
	computeWorkRangeMap();
	computeWorkAbilityMap();
	
	return findGoodBuilding(typeNum, true, false);
}

void AICastor::addProjects(void)
{
	Sint32 poolSiteTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::SWIMSPEED_BUILDING, 0, true);
	Sint32 attackSiteTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::ATTACK_BUILDING, 0, true);
	Sint32 speedSiteTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::WALKSPEED_BUILDING, 0, true);
	
	int canFeedUnit=0;
	int swarms=0;
	int pool=0;
	int poolSite=0;
	int attaque=0;
	int attaqueSite=0;
	int speed=0;
	int speedSite=0;
	
	Building **myBuildings=team->myBuildings;
	for (int i=0; i<1024; i++)
	{
		Building *b=myBuildings[i];
		if (b)
		{
			//printf("unitProductionTime[%d]=%d\n", i, b->type->unitProductionTime);
			if (b->type->canFeedUnit)
				canFeedUnit++;
			if (b->type->unitProductionTime)
				swarms++;
			
			if (b->type->upgrade[SWIM])
				pool++;
			if (b->typeNum==poolSiteTypeNum)
				poolSite++;
			
			if (b->type->upgrade[ATTACK_SPEED])
				attaque++;
			if (b->typeNum==attackSiteTypeNum)
				attaqueSite++;
			
			if (b->type->upgrade[WALK])
				speed++;
			if (b->typeNum==speedSiteTypeNum)
				speedSite++;
		}
	}
	
	//printf(" canFeedUnit=%d, swarms=%d, pool=%d+%d, attaque=%d+%d, speed=%d+%d\n",
	//	canFeedUnit, swarms, pool, poolSite, attaque, attaqueSite, speed, speedSite);
	
	if (canFeedUnit==0)
	{
		Project *project=new Project(BuildingType::FOOD_BUILDING, "food");
		projects.push_back(project);
		
		project->critical=true;
		project->food=true;
		
		project->mainWorkers=3;
		project->foodWorkers=1;
		project->otherWorkers=0;
		
		project->multipleStart=true;
		project->waitFinished=true;
		project->finalWorkers=1;
		
		return;
	}
	if (swarms==0)
	{
		Project *project=new Project(BuildingType::SWARM_BUILDING, "swarm");
		projects.push_back(project);
		
		project->critical=true;
		project->food=true;
		
		project->mainWorkers=20;
		project->foodWorkers=1;
		project->otherWorkers=0;
		
		project->multipleStart=false;
		project->waitFinished=true;
		project->finalWorkers=2;
		
		return;
	}
	if (pool+poolSite==0)
	{
		if (timer>computeNeedPoolTimer)
		{
			computeNeedPoolTimer=timer+1024;// every 41s
			computeNeedPool();
		}
		if (needPool)
		{
			Project *project=new Project(BuildingType::SWIMSPEED_BUILDING, 1, "pool");
			projects.push_back(project);
			project->critical=true;
			return;
		}
	}
	if (attaque+attaqueSite==0)
	{
		Project *project=new Project(BuildingType::ATTACK_BUILDING, 1, "attack");
		projects.push_back(project);
		project->critical=true;
		return;
	}
	if (speed+speedSite==0)
	{
		Project *project=new Project(BuildingType::WALKSPEED_BUILDING, 1, "speed");
		projects.push_back(project);
		project->critical=true;
		return;
	}
	// all critical projects succeded.
}

Order *AICastor::continueProject(Project *project)
{
	// Phase alpha will make a new Food Building at any price.
	//printf("(%s)(stn=%d, f=%d, w=[%d, %d, %d], ms=%d, wf=%d), sp=%d\n",
	//	project->debugName,
	//	project->shortTypeNum, project->food,
	//	project->mainWorkers, project->foodWorkers, project->otherWorkers,
	//	project->multipleStart, project->waitFinished, project->subPhase);
	
	if (timer<project->timer+32)
		return NULL;
	
	if (project->subPhase==0)
	{
		// boot phase
		project->subPhase=2;
		printf("%s (boot), switching to subphase 2.\n", project->debugName);
	}
	else if (project->subPhase==1)
	{
		// find any good building place
		
		Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, true);
		int bw=globalContainer->buildingsTypes.get(typeNum)->width;
		int bh=globalContainer->buildingsTypes.get(typeNum)->height;
		assert(bw==bh);
		
		computeCanSwim();
		computeObstacleBuildingMap();
		computeSpaceForBuildingMap(bw);
		computeBuildingNeighbourMap(bw, bh);
		computeWheatGrowthMap(bw, bh);
		computeObstacleUnitMap();
		computeWorkPowerMap();
		computeWorkRangeMap();
		computeWorkAbilityMap();
		
		Order *gfbm=findGoodBuilding(typeNum, project->food, true);
		project->timer=timer;
		if (gfbm)
		{
			project->subPhase=2;
			printf("%s (building site placed), switching to next subphase 2.\n", project->debugName);
			return gfbm;
		}
	}
	else if (project->subPhase==2)
	{
		// Checking of any real building site:
		
		Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, false);
		Sint32 siteTypeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, true);
		int count=0;
		int siteCount=0;
		
		Building **myBuildings=team->myBuildings;
		for (int i=0; i<1024; i++)
		{
			Building *b=myBuildings[i];
			if (b)
			{
				if (b->typeNum==typeNum)
					count++;
				if (b->typeNum==siteTypeNum)
					siteCount++;
			}
		}
		
		if (count>0)
		{
			project->subPhase=6;
			printf("%s (building finished), switching to subphase 6.\n", project->debugName);
		}
		else if (siteCount==0)
		{
			project->subPhase=1;
			printf("%s (no real building site found), switching back to subphase 1.\n", project->debugName);
		}
		else
		{
			project->subPhase=3;
			printf("%s (real building site found), switching to next subphase 3.\n", project->debugName);
			if (!project->waitFinished)
			{
				project->blocking=false;
				project->critical=false;
				printf("%s (deblocking)\n", project->debugName);
			}
		}
	}
	else if (project->subPhase==3)
	{
		// balance workers:
		int isFree=getFreeWorkers();
		Sint32 mainWorkers=project->mainWorkers;
		if (isFree<=3)
		{
			if (mainWorkers>3)
				mainWorkers=3;
		}
		else if (project->mainWorkers>isFree)
			mainWorkers=isFree;
		
		Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, false);
		Sint32 siteTypeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, true);
		int count=0;
		
		Sint32 swarmTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::SWARM_BUILDING, 0, false);
		Sint32 swarmSiteTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::SWARM_BUILDING, 0, true);
		Sint32 foodTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::FOOD_BUILDING, 0, false);
		Sint32 foodSiteTypeNum=globalContainer->buildingsTypes.getTypeNum(BuildingType::FOOD_BUILDING, 0, true);
		
		Building **myBuildings=team->myBuildings;
		for (int i=0; i<1024; i++)
		{
			Building *b=myBuildings[i];
			if (b)
			{
				if (b->typeNum==typeNum)
					count++;
				if (b->typeNum==siteTypeNum)
				{
					// main building
					if (mainWorkers>=0 && b->maxUnitWorking!=mainWorkers)
					{
						b->maxUnitWorking=mainWorkers;
						b->maxUnitWorkingLocal=mainWorkers;
						b->update();
						project->timer=timer;
						return new OrderModifyBuilding(b->gid, mainWorkers);
					}
				}
				else if (b->typeNum==swarmTypeNum
					|| b->typeNum==swarmSiteTypeNum
					|| b->typeNum==foodTypeNum
					|| b->typeNum==foodSiteTypeNum)
				{
					// food buildings
					if (project->foodWorkers>=0 && b->maxUnitWorking!=project->foodWorkers)
					{
						b->maxUnitWorking=project->foodWorkers;
						b->maxUnitWorkingLocal=project->foodWorkers;
						b->update();
						project->timer=timer;
						return new OrderModifyBuilding(b->gid, project->foodWorkers);
					}
				}
				else if (b->type->maxUnitWorking!=0)
				{
					// others buildings:
					if (project->otherWorkers>=0 && b->maxUnitWorking!=project->otherWorkers)
					{
						b->maxUnitWorking=project->otherWorkers;
						b->maxUnitWorkingLocal=project->otherWorkers;
						b->update();
						project->timer=timer;
						return new OrderModifyBuilding(b->gid, project->otherWorkers);
					}
				}
			}
		}
		
		if (count==0)
		{
			if (project->multipleStart)
			{
				project->subPhase=4;
				printf("%s (all max-unit-working set), switching to next subphase 4.\n", project->debugName);
			}
			else
			{
				project->subPhase=5;
				printf("%s (all max-unit-working set), switching to next subphase 5.\n", project->debugName);
			}
		}
		else
		{
			project->subPhase=6;
			printf("%s (building finished), switching to subphase 6.\n", project->debugName);
		}
	}
	else if (project->subPhase==4)
	{
		// If enough workers are free, we start another building.
		int isFree=getFreeWorkers();
		
		if (isFree>0)
		{
			Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, true);
			int bw=globalContainer->buildingsTypes.get(typeNum)->width;
			int bh=globalContainer->buildingsTypes.get(typeNum)->height;
			assert(bw==bh);
			
			computeCanSwim();
			computeObstacleBuildingMap();
			computeSpaceForBuildingMap(bw);
			computeBuildingNeighbourMap(bw, bh);
			computeWheatGrowthMap(bw, bh);
			computeObstacleUnitMap();
			computeWorkPowerMap();
			computeWorkRangeMap();
			computeWorkAbilityMap();
			
			Order *gfbm=findGoodBuilding(typeNum, project->food, false);
			project->timer=timer;
			if (gfbm)
			{
				project->subPhase=3;
				printf("%s (enough free workers and site placed), switching back to subphase 3.\n", project->debugName);
				return gfbm;
			}
		}
		
		project->subPhase=5;
		printf("%s (no more free workers), switching to next subphase 5.\n", project->debugName);
	}
	else if (project->subPhase==5)
	{
		// We simply wait for the food building to be finished,
		// and add free workers if aviable and project.waitFinished:
		
		int isFree=getFreeWorkers();
		
		Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, false);
		Sint32 siteTypeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, true);
		int count=0;
		int siteCount=0;
		
		Building **myBuildings=team->myBuildings;
		for (int i=0; i<1024; i++)
		{
			Building *b=myBuildings[i];
			if (b)
			{
				if (b->typeNum==typeNum)
					count++;
				else if (b->typeNum==siteTypeNum)
				{
					siteCount++;
					if (project->waitFinished && isFree>0 && b->maxUnitWorking<project->mainWorkers)
					{
						printf("%s (incrementing workers) isFree=%d, current=%d\n",
							project->debugName, isFree, b->maxUnitWorking);
						b->maxUnitWorking++;
						b->maxUnitWorkingLocal=b->maxUnitWorking;
						b->update();
						project->timer=timer;
						return new OrderModifyBuilding(b->gid, b->maxUnitWorking);
					}
				}
			}
		}
		if (count>0)
		{
			project->subPhase=6;
			printf("%s (building finished), switching to next subphase 6.\n", project->debugName);
		}
		else if (siteCount==0)
		{
			project->subPhase=2;
			printf("%s (building site destroyed), switching back to subphase 2.\n", project->debugName);
		}
	}
	else if (project->subPhase==6)
	{
		// balance final workers:
		
		if (project->blocking)
		{
			project->blocking=false;
			project->critical=false;
			printf("%s (deblocking)\n", project->debugName);
		}
		
		Sint32 typeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, false);
		Sint32 siteTypeNum=globalContainer->buildingsTypes.getTypeNum(project->shortTypeNum, 0, true);
		int siteCount=0;
		
		int isFree=getFreeWorkers();
		Sint32 finalWorkers=project->finalWorkers;
		if (isFree<=3)
		{
			if (finalWorkers>3)
				finalWorkers=3;
		}
		else if (finalWorkers>isFree)
			finalWorkers=isFree;
		
		Building **myBuildings=team->myBuildings;
		for (int i=0; i<1024; i++)
		{
			Building *b=myBuildings[i];
			if (b)
			{
				if (b->typeNum==siteTypeNum)
					siteCount++;
				else if (project->finalWorkers>=0 && b->typeNum==typeNum && b->maxUnitWorking!=finalWorkers)
				{
					assert(b->type->maxUnitWorking!=0);
					b->maxUnitWorking=finalWorkers;
					b->maxUnitWorkingLocal=finalWorkers;
					b->update();
					project->timer=timer;
					return new OrderModifyBuilding(b->gid, finalWorkers);
				}
			}
		}
		
		if (siteCount==0)
		{
			project->finished=true;
			printf("%s succeded. (all finalWorkers set)\n", project->debugName);
		}
	}
	else
		assert(false);
	
	return NULL;
}

int AICastor::getFreeWorkers()
{
	int sum=0;
	Unit **myUnits=team->myUnits;
	for (int i=0; i<1024; i++)
	{
		Unit *u=myUnits[i];
		if (u && u->medical==Unit::MED_FREE && u->activity==Unit::ACT_RANDOM && u->typeNum==WORKER && !u->subscribed)
			sum++;
	}
	return sum;
}

void AICastor::computeCanSwim()
{
	//printf("computeCanSwim()...\n");
	// If our population has more healthy-working-units able to swimm than healthy-working-units
	// unable to swimm then we choose to be able to go trough water:
	Unit **myUnits=team->myUnits;
	int sumCanSwim=0;
	int sumCantSwim=0;
	for (int i=0; i<1024; i++)
	{
		Unit *u=myUnits[i];
		if (u && u->typeNum==WORKER && u->medical==0)
		{
			if (u->performance[SWIM]>0)
				sumCanSwim++;
			else
				sumCantSwim++;
		}
	}
	
	canSwim=(sumCanSwim>sumCantSwim);
	//printf("...computeCanSwim() done\n");
}

void AICastor::computeNeedPool()
{
	int w=map->w;
	int h=map->h;
	size_t size=w*h;
	
	canSwim=false;
	computeObstacleUnitMap();
	computeWorkRangeMap();
	
	Sint32 baseCount=0;
	for (size_t i=0; i<size; i++)
		if (workRangeMap[i]!=0)
			baseCount++;
	
	canSwim=true;
	computeObstacleUnitMap();
	computeWorkRangeMap();
	
	Sint32 extendedCount=0;
	for (size_t i=0; i<size; i++)
		if (workRangeMap[i]!=0)
			extendedCount++;
	
	needPool=((baseCount<<4)>(7*extendedCount));
	printf("needPool=%d\n", needPool);
	
	computeCanSwim();
}

void AICastor::computeObstacleUnitMap()
{
	//printf("computeObstacleUnitMap()...\n");
	int w=map->w;
	int h=map->h;
	//int wMask=map->wMask;
	//int hMask=map->hMask;
	//size_t size=w*h;
	Case *cases=map->cases;
	Uint32 teamMask=team->me;
		
	for (int y=0; y<h; y++)
	{
		int wy=w*y;
		for (int x=0; x<w; x++)
		{
			int wyx=wy+x;
			Case c=cases[wyx];
			if (c.building==NOGBID)
			{
				if (c.ressource.type!=NO_RES_TYPE)
					obstacleUnitMap[wyx]=0;
				else if (c.forbidden&teamMask)
					obstacleUnitMap[wyx]=0;
				else if (!canSwim && (c.terrain>=256) && (c.terrain<256+16)) // !canSwim && isWatter ?
					obstacleUnitMap[wyx]=0;
				else
					obstacleUnitMap[wyx]=1;
			}
			else
				obstacleUnitMap[wyx]=0;
		}
	}
	//printf("...computeObstacleUnitMap() done\n");
}


void AICastor::computeObstacleBuildingMap()
{
	//printf("computeObstacleBuildingMap()...\n");
	int w=map->w;
	int h=map->h;
	//int wMask=map->wMask;
	//int hMask=map->hMask;
	//int hDec=map->hDec;
	//int wDec=map->wDec;
	//size_t size=w*h;
	Case *cases=map->cases;
	
	for (int y=0; y<h; y++)
	{
		int wy=w*y;
		for (int x=0; x<w; x++)
		{
			int wyx=wy+x;
			Case c=cases[wyx];
			if (c.building==NOGBID)
			{
				if (c.terrain>=16) // if (!isGrass)
					obstacleBuildingMap[wyx]=0;
				else if (c.ressource.type!=NO_RES_TYPE)
					obstacleBuildingMap[wyx]=0;
				else
					obstacleBuildingMap[wyx]=1;
			}
			else
				obstacleBuildingMap[wyx]=0;
		}
	}
	//printf("...computeObstacleBuildingMap() done\n");
}

void AICastor::computeSpaceForBuildingMap(int max)
{
	//printf("computeSpaceForBuildingMap()...\n");
	int w=map->w;
	int h=map->h;
	int wMask=map->wMask;
	//int hMask=map->hMask;
	//int hDec=map->hDec;
	//int wDec=map->wDec;
	size_t size=w*h;
	
	memcpy(spaceForBuildingMap, obstacleBuildingMap, size);
	
	for (int i=1; i<max; i++)
	{
		for (int y=0; y<h; y++)
		{
			int wy0=w*y;
			int wy1=w*((y+1)&wMask);
			
			for (int x=0; x<w; x++)
			{
				int wyx[4];
				wyx[0]=wy0+x+0;
				wyx[1]=wy0+x+1;
				wyx[2]=wy1+x+0;
				wyx[3]=wy1+x+1;
				Uint8 obs[4];
				for (int i=0; i<4; i++)
					obs[i]=spaceForBuildingMap[wyx[i]];
				Uint8 min=255;
				for (int i=0; i<4; i++)
					if (min>obs[i])
						min=obs[i];
				if (min!=0)
					spaceForBuildingMap[wyx[0]]=min+1;
			}
		}
	}
	//printf("...computeSpaceForBuildingMap() done\n");
}


void AICastor::computeBuildingNeighbourMap(int dw, int dh)
{
	int w=map->w;
	int h=map->h;
	int wMask=map->wMask;
	int hMask=map->hMask;
	//int hDec=map->hDec;
	int wDec=map->wDec;
	size_t size=w*h;
	Uint8 *gradient=buildingNeighbourMap;
	Case *cases=map->cases;
	
	Uint8 *wheatGradient=map->ressourcesGradient[team->teamNumber][CORN][canSwim];
	memset(gradient, 0, size);
	
	Building **myBuildings=team->myBuildings;
	for (int i=0; i<1024; i++)
	{
		Building *b=myBuildings[i];
		if (b && !b->type->isVirtual)
		{
			int bx=b->posX;
			int by=b->posY;
			int bw=b->type->width;
			int bh=b->type->height;
			
			// we skip building with already a neighbour:
			bool neighbour=false;
			bool wheat=false;
			for (int xi=bx-1; xi<=bx+bw; xi++)
			{
				int index;
				index=(xi&wMask)+(((by-1 )&hMask)<<wDec);
				if (cases[index].building!=NOGBID)
					neighbour=true;
				if (wheatGradient[index]==255)
					wheat=true;
				index=(xi&wMask)+(((by+bh)&hMask)<<wDec);
				if (cases[index].building!=NOGBID)
					neighbour=true;
				if (wheatGradient[index]==255)
					wheat=true;
			}
			if (!neighbour)
				for (int yi=by-1; yi<=by+bh; yi++)
				{
					int index;
					index=((bx-1 )&wMask)+((yi&hMask)<<wDec);
					if (cases[index].building!=NOGBID)
						neighbour=true;
					if (wheatGradient[index]==255)
						wheat=true;
					index=((bx+bw)&wMask)+((yi&hMask)<<wDec);
					if (cases[index].building!=NOGBID)
						neighbour=true;
					if (wheatGradient[index]==255)
						wheat=true;
				}
			
			Uint8 dirty;
			if (neighbour || !wheat)
				dirty=1;
			else
				dirty=0;
			
			// dirty at a range of 1 space case, without corners;
			for (int xi=bx-dw+1; xi<bx+bw; xi++)
			{
				gradient[(xi&wMask)+(((by-dh-1)&hMask)<<wDec)]|=1;
				gradient[(xi&wMask)+(((by+bh+1)&hMask)<<wDec)]|=1;
			}
			for (int yi=by-dh+1; yi<by+bh; yi++)
			{
				gradient[((bx-dw-1)&wMask)+((yi&hMask)<<wDec)]|=1;
				gradient[((bx+bw+1)&wMask)+((yi&hMask)<<wDec)]|=1;
			}
			{
				// the same with inner inner corners:
				gradient[((bx-dw)&wMask)+(((by-dh)&hMask)<<wDec)]|=1;
				gradient[((bx-dw)&wMask)+(((by+bh)&hMask)<<wDec)]|=1;
				gradient[((bx+bw)&wMask)+(((by-dh)&hMask)<<wDec)]|=1;
				gradient[((bx+bw)&wMask)+(((by+bh)&hMask)<<wDec)]|=1;
			}
			
			// At a range of 0 space case (neignbours), without corners,
			// we increment (bit 1 to 3), and dirty bit 0 in case:
			for (int xi=bx-dw+1; xi<bx+bw; xi++)
			{
				Uint8 *p;
				p=&gradient[(xi&wMask)+(((by-dh)&hMask)<<wDec)];
				*p=((*p+2)|dirty)&(~16);
				p=&gradient[(xi&wMask)+(((by+bh)&hMask)<<wDec)];
				*p=((*p+2)|dirty)&(~16);
			}
			for (int yi=by-dh+1; yi<by+bh; yi++)
			{
				Uint8 *p;
				p=&gradient[((bx-dw)&wMask)+((yi&hMask)<<wDec)];
				*p=((*p+2)|dirty)&(~16);
				p=&gradient[((bx+bw)&wMask)+((yi&hMask)<<wDec)];
				*p=((*p+2)|dirty)&(~16);
			}
			
			// At a range of 2 space case, without corners,
			// we increment (from bit 5):
			Uint8 inc;
			if (neighbour)
				inc=32;
			else
				inc=64;
			for (int xi=bx-dw; xi<bx+bw+1; xi++)
			{
				Uint8 *p;
				p=&gradient[(xi&wMask)+(((by-dh-2)&hMask)<<wDec)];
				(*p)+=inc;
				p=&gradient[(xi&wMask)+(((by+bh+2)&hMask)<<wDec)];
				(*p)+=inc;
			}
			for (int yi=by-dh; yi<by+bh+1; yi++)
			{
				Uint8 *p;
				p=&gradient[((bx-dw-2)&wMask)+((yi&hMask)<<wDec)];
				(*p)+=inc;
				p=&gradient[((bx+bw+2)&wMask)+((yi&hMask)<<wDec)];
				(*p)+=inc;
			}
			{
				// the same with inner inner corners:
				Uint8 *p;
				p=&gradient[((bx-dw-1)&wMask)+(((by-dh-1)&hMask)<<wDec)];
				(*p)+=inc;
				p=&gradient[((bx-dw-1)&wMask)+(((by+bh+1)&hMask)<<wDec)];
				(*p)+=inc;
				p=&gradient[((bx+bw+1)&wMask)+(((by-dh-1)&hMask)<<wDec)];
				(*p)+=inc;
				p=&gradient[((bx+bw+1)&wMask)+(((by+bh+1)&hMask)<<wDec)];
				(*p)+=inc;
			}
		}
	}
}

void AICastor::computeTwoSpaceNeighbourMap()
{
}


void AICastor::computeWorkPowerMap()
{
	int w=map->w;
	int h=map->h;
	int wMask=map->wMask;
	int hMask=map->hMask;
	//int hDec=map->hDec;
	int wDec=map->wDec;
	size_t size=w*h;
	Uint8 *gradient=workPowerMap;
	Uint8 maxRange=64;
	if (maxRange>w/2)
		maxRange=w/2;
	if (maxRange>h/2)
		maxRange=h/2;
	
	memset(gradient, 0, size);
	
	Unit **myUnits=team->myUnits;
	for (int i=0; i<1024; i++)
	{
		Unit *u=myUnits[i];
		if (u && u->typeNum==WORKER && u->medical==0 && u->activity!=Unit::ACT_UPGRADING)
		{
			int range=((u->hungry-u->trigHungry)>>1)/u->race->unitTypes[0][0].hungryness;
			if (range<0)
				continue;
			//printf(" range=%d\n", range);
			if (range>maxRange)
				range=maxRange;
			int ux=u->posX;
			int uy=u->posY;
			static const int reducer=3;
			{
				Uint8 *gp=&gradient[(ux&wMask)+((uy&hMask)<<wDec)];
				Uint16 sum=*gp+(range>>reducer);
				if (sum>255)
					sum=255;
				*gp=sum;
			}
			for (int r=1; r<range; r++)
			{
				for (int dx=-r; dx<=r; dx++)
				{
					Uint8 *gp=&gradient[((ux+dx)&wMask)+(((uy -r)&hMask)<<wDec)];
					Uint16 sum=*gp+((range-r)>>reducer);
					if (sum>255)
						sum=255;
					*gp=sum;
				}
				for (int dx=-r; dx<=r; dx++)
				{
					Uint8 *gp=&gradient[((ux+dx)&wMask)+(((uy +r)&hMask)<<wDec)];
					Uint16 sum=*gp+((range-r)>>reducer);
					if (sum>255)
						sum=255;
					*gp=sum;
				}
				for (int dy=(1-r); dy<r; dy++)
				{
					Uint8 *gp=&gradient[((ux -r)&wMask)+(((uy+dy)&hMask)<<wDec)];
					Uint16 sum=*gp+((range-r)>>reducer);
					if (sum>255)
						sum=255;
					*gp=sum;
				}
				for (int dy=(1-r); dy<r; dy++)
				{
					Uint8 *gp=&gradient[((ux +r)&wMask)+(((uy+dy)&hMask)<<wDec)];
					Uint16 sum=*gp+((range-r)>>reducer);
					if (sum>255)
						sum=255;
					*gp=sum;
				}
			}
		}
	}
}


void AICastor::computeWorkRangeMap()
{
	int w=map->w;
	int h=map->h;
	int wMask=map->wMask;
	int hMask=map->hMask;
	//int hDec=map->hDec;
	int wDec=map->wDec;
	size_t size=w*h;
	Uint8 *gradient=workRangeMap;
	
	memcpy(gradient, obstacleUnitMap, size);
	
	Unit **myUnits=team->myUnits;
	for (int i=0; i<1024; i++)
	{
		Unit *u=myUnits[i];
		if (u && u->typeNum==WORKER && u->medical==0 && u->activity!=Unit::ACT_UPGRADING)
		{
			int range=((u->hungry-u->trigHungry)>>1)/u->race->unitTypes[0][0].hungryness;
			if (range<0)
				continue;
			//printf(" range=%d\n", range);
			if (range>255)
				range=255;
			int index=(u->posX&wMask)+((u->posY&hMask)<<wDec);
			gradient[index]=(Uint8)range;
		}
	}
	
	map->updateGlobalGradient(gradient);
}


void AICastor::computeWorkAbilityMap()
{
	int w=map->w;
	int h=map->h;
	//int wMask=map->wMask;
	//int hMask=map->hMask;
	//int hDec=map->hDec;
	//int wDec=map->wDec;
	size_t size=w*h;
	
	for (size_t i=0; i<size; i++)
	{
		Uint8 workPower=workPowerMap[i];
		Uint8 workRange=workRangeMap[i];
		
		Uint32 workAbility=((workPower*workRange)>>5);
		if (workAbility>255)
			workAbility=255;
		
		workAbilityMap[i]=(Uint8)workAbility;
	}
}

void AICastor::computeHydratationMap()
{
	printf("computeHydratationMap()...\n");
	int w=map->w;
	int h=map->w;
	//int wMask=map->wMask;
	//int hMask=map->hMask;
	size_t size=w*h;
	
	memset(hydratationMap, 0, size);
	
	Case *cases=map->cases;
	for (size_t i=0; i<size; i++)
	{
		Uint16 t=cases[i].terrain;
		if ((t>=256) && (t<256+16)) // if WATER
			hydratationMap[i]=16;
	}
	
	updateGlobalGradientNoObstacle(hydratationMap);
	hydratationMapComputed=true;
	printf("...computeHydratationMap() done\n");
}

void AICastor::computeWheatGrowthMap(int dw, int dh)
{
	int w=map->w;
	int h=map->w;
	int wMask=map->wMask;
	int hMask=map->hMask;
	//int hDec=map->hDec;
	int wDec=map->wDec;
	size_t size=w*h;
	
	memcpy(wheatGrowthMap, obstacleBuildingMap, size);
	Uint8 *wheatGradient=map->ressourcesGradient[team->teamNumber][CORN][canSwim];
	
	for (size_t i=0; i<size; i++)
	{
		Uint8 wheat=wheatGradient[i]; // [0..255]
		if (wheat>=254)
			wheatGrowthMap[i]=1+hydratationMap[i];
	}
	
	map->updateGlobalGradient(wheatGrowthMap);
	
	for (int y=0; y<h; y++)
		for (int x=0; x<w; x++)
		{
			Uint8 best=1;
			for (int dy=0; dy<dh; dy++)
			{
				int wyd=(((y+dy)&hMask)<<wDec);
				for (int dx=0; dx<dw; dx++)
				{
					int wyxd=wyd+((x+dx)&wMask);
					Uint8 wheatGrowth=wheatGrowthMap[wyxd];
					if (best<wheatGrowth)
						best=wheatGrowth;
				}
			}
			wheatGrowthMap[(y<<wDec)+x]=best-1;
		}
}

Order *AICastor::findGoodBuilding(Sint32 typeNum, bool food, bool critical)
{
	int w=map->w;
	int h=map->w;
	int bw=globalContainer->buildingsTypes.get(typeNum)->width;
	int bh=globalContainer->buildingsTypes.get(typeNum)->height;
	assert(bw==bh);
	//int hDec=map->hDec;
	int wDec=map->wDec;
	//int wMask=map->wMask;
	//int hMask=map->hMask;
	size_t size=w*h;
	Uint32 *mapDiscovered=map->mapDiscovered;
	Uint32 me=team->me;
	printf("findGoodBuilding(%d, %d, %d)\n", typeNum, food, critical);
	
	// first, we auto calibrate minWork:
	Uint32 minWork;
	if (critical)
		minWork=1;
	else
	{
		Uint8 bestWorkScore=2;
		for (size_t i=0; i<size; i++)
		{
			if ((mapDiscovered[i]&me)==0)
				continue;
			Uint8 work=workAbilityMap[i];
			if (bestWorkScore<work)
				bestWorkScore=work;
		}
		minWork=bestWorkScore/2;
		if (minWork>30)
			minWork=30;
		printf(" bestWorkScore=%d, minWork=%d\n", bestWorkScore, minWork);
	}
	
	// second, we auto calibrate wheatLimit:
	Uint16 wheatLimit;
	if (food)
	{
		if (critical)
			wheatLimit=2; // TODO: test a very bad map.
		else
		{
			Uint8 maxWheat=0;
			for (size_t i=0; i<size; i++)
			{
				Uint8 work=workAbilityMap[i];
				if (work<minWork)
					continue;
				Uint8 wheat=wheatGrowthMap[i];
				if (maxWheat<wheat)
					maxWheat=wheat;
			}
			wheatLimit=maxWheat/2;
			//if (wheatLimit>7)
			//	wheatLimit=7;
			printf(" maxWheat=%d, wheatLimit=%d\n", maxWheat, wheatLimit);
		}
	}
	else
	{
		Uint8 minWheat=255;
		for (size_t i=0; i<size; i++)
		{
			if ((mapDiscovered[i]&me)==0)
				continue;
			Uint8 work=workAbilityMap[i];
			if (work<minWork)
				continue;
			Uint8 wheat=wheatGrowthMap[i];
			if (minWheat>wheat)
				minWheat=wheat;
		}
		if (critical)
			wheatLimit=minWheat+7;
		else
			wheatLimit=minWheat+4;
		printf(" minWheat=%d, wheatLimit=%d\n", minWheat, wheatLimit);
	}
	
	// third, we find the best place possible:
	size_t bestIndex;
	Sint32 bestScore=0;
	minWork=(minWork<<2);
	wheatLimit=(wheatLimit<<2);
	printf(" minWork=%d, wheatLimit=%d\n", minWork, wheatLimit);
	memset(goodBuildingMap, 0, size);
	for (size_t i=0; i<size; i++)
	{
		size_t corner0=i;
		size_t corner1=(i+bw-1)&(size-1);
		size_t corner2=(i+(bw<<wDec))&(size-1);
		size_t corner3=(i+(bw<<wDec)+bw-1)&(size-1);
		
		if (critical
			&& (mapDiscovered[corner0]&me)==0
			&& (mapDiscovered[corner1]&me)==0
			&& (mapDiscovered[corner2]&me)==0
			&& (mapDiscovered[corner3]&me)==0)
			continue;
		
		Uint8 space=spaceForBuildingMap[corner0];
		if (space<bw)
			continue;
		//goodBuildingMap[i]=1;
		
		Uint32 work=workAbilityMap[corner0]+workAbilityMap[corner1]+workAbilityMap[corner2]+workAbilityMap[corner3];
		if (work<minWork)
			continue;
		//goodBuildingMap[i]=2;
		
		Uint16 wheat=wheatGrowthMap[corner0]+wheatGrowthMap[corner1]+wheatGrowthMap[corner2]+wheatGrowthMap[corner3];
		if (food)
		{
			if (wheat<wheatLimit)
				continue;
		}
		else if (wheat>wheatLimit)
			continue;
		//goodBuildingMap[i]=3;
		
		Uint8 neighbour=buildingNeighbourMap[i];
		Uint8 directNeighboursCount=(neighbour>>1)&7;
		Uint8 farNeighboursCount=(neighbour>>5)&7;
		if ((neighbour&1)||(directNeighboursCount>1))
			continue;
		//goodBuildingMap[i]=4;
		
		Sint32 score;
		if (food)
			score=(((Sint32)wheat<<8)+(Sint32)work)*(8+(directNeighboursCount<<2)+farNeighboursCount);
		else
			score=(5120+(Sint32)work-((Sint32)wheat<<8))*(8+(directNeighboursCount<<2)+farNeighboursCount);
		
		if ((score>>9)>=255)
			goodBuildingMap[i]=255;
		else
			goodBuildingMap[i]=(score>>9);
		
		if (bestScore<score)
		{
			bestScore=score;
			bestIndex=i;
		}
	}
	
	if (bestScore>0)
	{
		printf(" found a cool place, score=%d, wheat=%d, work=%d\n", bestScore, wheatGrowthMap[bestIndex], workAbilityMap[bestIndex]);
		Sint32 x=(bestIndex&map->wMask);
		Sint32 y=((bestIndex>>map->wDec)&map->hMask);
		return new OrderCreate(team->teamNumber, x, y, typeNum);
	}
	
	return NULL;
}

void AICastor::computeRessourcesCluster()
{
	printf("computeRessourcesCluster()\n");
	int w=map->w;
	int h=map->w;
	int wMask=map->wMask;
	int hMask=map->hMask;
	size_t size=w*h;
	
	memset(ressourcesCluster, 0, size*2);
	
	//int i=0;
	Uint8 old=0xFF;
	Uint16 id=0;
	bool usedid[65536];
	memset(usedid, 0, 65536*sizeof(bool));
	for (int y=0; y<h; y++)
	{
		for (int x=0; x<w; x++)
		{
			Case *c=map->cases+w*(y&hMask)+(x&wMask); // case
			Ressource r=c->ressource; // ressource
			Uint8 rt=r.type; // ressources type
			
			int rci=x+y*w; // ressource cluster index
			Uint16 *rcp=&ressourcesCluster[rci]; // ressource cluster pointer
			Uint16 rc=*rcp; // ressource cluster
			
			if (rt==0xFF)
			{
				*rcp=0;
				old=0xFF;
			}
			else
			{
				printf("ressource rt=%d, at (%d, %d)\n", rt, x, y);
				if (rt!=old)
				{
					printf(" rt!=old\n");
					id=1;
					while (usedid[id])
						id++;
					if (id)
						usedid[id]=true;
					old=rt;
					printf("  id=%d\n", id);
				}
				if (rc!=id)
				{
					if (rc==0)
					{
						*rcp=id;
						printf(" wrote.\n");
					}
					else
					{
						Uint16 oldid=id;
						usedid[oldid]=false;
						id=rc; // newid
						printf(" cleaning oldid=%d to id=%d.\n", oldid, id);
						// We have to correct last ressourcesCluster values:
						*rcp=id;
						while (*rcp==oldid)
						{
							*rcp=id;
							rcp--;
						}
					}
				}
			}
		}
		memcpy(ressourcesCluster+((y+1)&hMask)*w, ressourcesCluster+y*w, w*2);
	}
	
	int used=0;
	for (int id=1; id<65536; id++)
		if (usedid[id])
			used++;
	printf("computeRessourcesCluster(), used=%d\n", used);
}

void AICastor::updateGlobalGradientNoObstacle(Uint8 *gradient)
{
	//In this algotithm, "l" stands for one case at Left, "r" for one case at Right, "u" for Up, and "d" for Down.
	// Warning, this is *nearly* a copy-past, 4 times, once for each direction.
	int w=map->w;
	int h=map->h;
	int hMask=map->hMask;
	int wMask=map->wMask;
	//int hDec=map->hDec;
	int wDec=map->wDec;
	
	for (int yi=0; yi<h; yi++)
	{
		int wy=((yi&hMask)<<wDec);
		int wyu=(((yi-1)&hMask)<<wDec);
		for (int xi=yi; xi<(yi+w); xi++)
		{
			int x=xi&wMask;
			Uint8 max=gradient[wy+x];
			if (max!=16)
			{
				int xl=(x-1)&wMask;
				int xr=(x+1)&wMask;

				Uint8 side[4];
				side[0]=gradient[wyu+xl];
				side[1]=gradient[wyu+x ];
				side[2]=gradient[wyu+xr];
				side[3]=gradient[wy +xl];

				for (int i=0; i<4; i++)
					if (side[i]>max)
						max=side[i];
				if (max==0)
					gradient[wy+x]=0;
				else
					gradient[wy+x]=max-1;
			}
		}
	}

	for (int y=hMask; y>=0; y--)
	{
		int wy=(y<<wDec);
		int wyd=(((y+1)&hMask)<<wDec);
		for (int xi=y; xi<(y+w); xi++)
		{
			int x=xi&wMask;
			Uint8 max=gradient[wy+x];
			if (max!=16)
			{
				int xl=(x-1)&wMask;
				int xr=(x+1)&wMask;

				Uint8 side[4];
				side[0]=gradient[wyd+xr];
				side[1]=gradient[wyd+x ];
				side[2]=gradient[wyd+xl];
				side[3]=gradient[wy +xl];

				for (int i=0; i<4; i++)
					if (side[i]>max)
						max=side[i];
				if (max==0)
					gradient[wy+x]=0;
				else
					gradient[wy+x]=max-1;
			}
		}
	}

	for (int x=0; x<w; x++)
	{
		int xl=(x-1)&wMask;
		for (int yi=x; yi<(x+h); yi++)
		{
			int wy=((yi&hMask)<<wDec);
			int wyu=(((yi-1)&hMask)<<wDec);
			int wyd=(((yi+1)&hMask)<<wDec);
			Uint8 max=gradient[wy+x];
			if (max!=16)
			{
				Uint8 side[4];
				side[0]=gradient[wyu+xl];
				side[1]=gradient[wyd+xl];
				side[2]=gradient[wy +xl];
				side[3]=gradient[wyu+x ];

				for (int i=0; i<4; i++)
					if (side[i]>max)
						max=side[i];
				if (max==0)
					gradient[wy+x]=0;
				else
					gradient[wy+x]=max-1;
			}
		}
	}

	for (int x=wMask; x>=0; x--)
	{
		int xr=(x+1)&wMask;
		for (int yi=x; yi<(x+h); yi++)
		{
			int wy=((yi&hMask)<<wDec);
			int wyu=(((yi-1)&hMask)<<wDec);
			int wyd=(((yi+1)&hMask)<<wDec);
			Uint8 max=gradient[wy+x];
			if (max!=16)
			{
				Uint8 side[4];
				side[0]=gradient[wyu+xr];
				side[1]=gradient[wy +xr];
				side[2]=gradient[wyd+xr];
				side[3]=gradient[wyu+x ];

				for (int i=0; i<4; i++)
					if (side[i]>max)
						max=side[i];
				if (max==0)
					gradient[wy+x]=0;
				else
					gradient[wy+x]=max-1;
			}
		}
	}
}
