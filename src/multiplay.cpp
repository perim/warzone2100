/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * Multiplay.c
 *
 * Alex Lee, Sep97, Pumpkin Studios
 *
 * Contains the day to day networking stuff, and received message handler.
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"
#include "map.h"

#include "stats.h"									// for templates.
#include "game.h"									// for loading maps
#include "hci.h"

#include <time.h>									// for recording ping times.
#include "research.h"
#include "display3d.h"								// for changing the viewpoint
#include "console.h"								// for screen messages
#include "power.h"
#include "cmddroid.h"								//  for commanddroidupdatekills
#include "wrappers.h"								// for game over
#include "component.h"
#include "frontend.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "levels.h"
#include "selection.h"

#include "init.h"
#include "warcam.h"	// these 4 for fireworks
#include "mission.h"
#include "effects.h"
#include "lib/gamelib/gtime.h"
#include "keybind.h"

#include "lib/script/script.h"				//Because of "ScriptTabs.h"
#include "scripttabs.h"			//because of CALL_AI_MSG
#include "scriptcb.h"			//for console callback
#include "scriptfuncs.h"

#include "lib/netplay/netplay.h"								// the netplay library.
#include "multiplay.h"								// warzone net stuff.
#include "multijoin.h"								// player management stuff.
#include "multirecv.h"								// incoming messages stuff
#include "multistat.h"
#include "multigifts.h"								// gifts and alliances.
#include "multiint.h"

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// globals.
BOOL						bMultiPlayer				= false;	// true when more than 1 player.
BOOL						bMultiMessages				= false;	// == bMultiPlayer unless multimessages are disabled
BOOL						openchannels[MAX_PLAYERS]={true};
UBYTE						bDisplayMultiJoiningStatus;

MULTIPLAYERGAME				game;									//info to describe game.
MULTIPLAYERINGAME			ingame;

char						beaconReceiveMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];	//beacon msg for each player
char								playerName[MAX_PLAYERS][MAX_STR_LENGTH];	//Array to store all player names (humans and AIs)
BOOL						bPlayerReadyGUI[MAX_PLAYERS] = {false};

/////////////////////////////////////
/* multiplayer message stack stuff */
/////////////////////////////////////
#define MAX_MSG_STACK	100				// must be *at least* 64

static char msgStr[MAX_MSG_STACK][MAX_STR_LENGTH];
static SDWORD msgPlFrom[MAX_MSG_STACK];
static SDWORD msgPlTo[MAX_MSG_STACK];
static SDWORD callbackType[MAX_MSG_STACK];
static SDWORD locx[MAX_MSG_STACK];
static SDWORD locy[MAX_MSG_STACK];
static DROID *msgDroid[MAX_MSG_STACK];
static SDWORD msgStackPos = -1;				//top element pointer

// ////////////////////////////////////////////////////////////////////////////
// Remote Prototypes
extern RESEARCH*			asResearch;							//list of possible research items.
extern PLAYER_RESEARCH*		asPlayerResList[MAX_PLAYERS];
// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static BOOL recvBeacon(NETQUEUE queue);
static BOOL recvDestroyTemplate(NETQUEUE queue);
static BOOL recvResearch(NETQUEUE queue);

bool		multiplayPlayersReady		(bool bNotifyStatus);
void		startMultiplayerGame		(void);

// ////////////////////////////////////////////////////////////////////////////
// temporarily disable multiplayer mode.
void turnOffMultiMsg(BOOL bDoit)
{
	if (!bMultiPlayer)
	{
		return;
	}

	bMultiMessages = !bDoit;
	return;
}


// ////////////////////////////////////////////////////////////////////////////
// throw a party when you win!
BOOL multiplayerWinSequence(BOOL firstCall)
{
	static Position pos;
	Position pos2;
	static UDWORD last=0;
	float		rotAmount;
	STRUCTURE	*psStruct;

	if(firstCall)
	{
		pos  = cameraToHome(selectedPlayer,true);			// pan the camera to home if not already doing so
		last =0;

		// stop all research
		CancelAllResearch(selectedPlayer);

		// stop all manufacture.
		for(psStruct=apsStructLists[selectedPlayer];psStruct;psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				if (((FACTORY *)psStruct->pFunctionality)->psSubject)//check if active
				{
					cancelProduction(psStruct, ModeQueue);
				}
			}
		}
	}

	// rotate world
	if (MissionResUp && !getWarCamStatus())
	{
		rotAmount = graphicsTimeAdjustedIncrement(MAP_SPIN_RATE / 12);
		player.r.y += rotAmount;
	}

	if(last > gameTime)last= 0;
	if((gameTime-last) < 500 )							// only  if not done recently.
	{
		return true;
	}
	last = gameTime;

	if(rand()%3 == 0)
	{
		pos2=pos;
		pos2.x +=  (rand() % world_coord(8)) - world_coord(4);
		pos2.z +=  (rand() % world_coord(8)) - world_coord(4);

		if (pos2.x < 0)
			pos2.x = 128;

		if ((unsigned)pos2.x > world_coord(mapWidth))
			pos2.x = world_coord(mapWidth);

		if (pos2.z < 0)
			pos2.z = 128;

		if ((unsigned)pos2.z > world_coord(mapHeight))
			pos2.z = world_coord(mapHeight);

		addEffect(&pos2,EFFECT_FIREWORK,FIREWORK_TYPE_LAUNCHER,false,NULL,0);	// throw up some fire works.
	}

	// show the score..


	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// MultiPlayer main game loop code.
BOOL multiPlayerLoop(void)
{
	UDWORD		i;
	UBYTE		joinCount;

	sendCheck();						// send some checking info if possible

		joinCount =0;
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(isHumanPlayer(i) && ingame.JoiningInProgress[i] )
			{
				joinCount++;
			}
		}
		if(joinCount)
		{
			setWidgetsStatus(false);
			bDisplayMultiJoiningStatus = joinCount;	// someone is still joining! say So

			// deselect anything selected.
			selDroidDeselect(selectedPlayer);

			if(keyPressed(KEY_ESC) )// check for cancel
			{
				bDisplayMultiJoiningStatus = 0;
				setWidgetsStatus(true);
				setPlayerHasLost(true);
			}
		}
		else		//everyone is in the game now!
		{
			if(bDisplayMultiJoiningStatus)
			{
				bDisplayMultiJoiningStatus = 0;
				setWidgetsStatus(true);
			}
			if (!ingame.TimeEveryoneIsInGame)
			{
				ingame.TimeEveryoneIsInGame = gameTime;
				debug(LOG_NET, "I have entered the game @ %d", ingame.TimeEveryoneIsInGame );
				if (!NetPlay.isHost)
				{
					debug(LOG_NET, "=== Sending hash to host ===");
					sendDataCheck();
				}
			}
			// Only have to do this on a true MP game
			if (NetPlay.isHost && !ingame.isAllPlayersDataOK && NetPlay.bComms)
			{
				if (gameTime - ingame.TimeEveryoneIsInGame > GAME_TICKS_PER_SEC * 60)
				{
					// we waited 60 secs to make sure people didn't bypass the data integrity checks
					int index;
					for (index=0; index < MAX_PLAYERS; index++)
					{
						if (ingame.DataIntegrity[index] == false && isHumanPlayer(index) && index != NET_HOST_ONLY)
						{
							char msg[256] = {'\0'};

							sprintf(msg, _("Kicking player %s, because they tried to bypass data integrity check!"), getPlayerName(index));
							sendTextMessage(msg, true);
							addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);
							NETlogEntry(msg, SYNC_FLAG, index);

#ifndef DEBUG
							kickPlayer(index, "it is not nice to cheat!", ERROR_CHEAT);
#endif
							debug(LOG_WARNING, "Kicking Player %s (%u), they tried to bypass data integrity check!", getPlayerName(index), index);
						}
					}
					ingame.isAllPlayersDataOK = true;
				}
			}
		}

	// if player has won then process the win effects...
	if(testPlayerHasWon())
	{
		multiplayerWinSequence(false);
	}
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// quikie functions.

// to get droids ...
BOOL IdToDroid(UDWORD id, UDWORD player, DROID **psDroid)
{
	UDWORD i;
	DROID *d;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)			// find the droid to order form them all
		{
			d = apsDroidLists[i];
			while((d != NULL )&&(d->id !=id) )d=d->psNext;
			if(d)
			{
				*psDroid = d;
				return true;
			}
		}
		return false;
	}
	else									// find the droid, given player
	{
		if (player >= MAX_PLAYERS)
		{
			debug(LOG_FEATURE, "Feature detected");
			// feature hack, player = 9 are features
			return false;
		}
		d = apsDroidLists[player];
		while( (d != NULL ) && (d->id !=id))d=d->psNext;
		if(d)
		{
			*psDroid = d;
			return true;
		}
		return false;
	}
}

// ////////////////////////////////////////////////////////////////////////////
// find a structure
STRUCTURE *IdToStruct(UDWORD id,UDWORD player)
{
	STRUCTURE	*psStr = NULL;
	UDWORD		i;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for (psStr=apsStructLists[i];( (psStr != NULL) && (psStr->id != id)); psStr=psStr->psNext) {}
			if(psStr)
			{
				return psStr;
			}
		}
	}
	else
	{
		if (player >= MAX_PLAYERS)
		{
			debug(LOG_FEATURE, "Feature detected");
			// feature hack, player = 9 are features
			return NULL;
		}
		for (psStr=apsStructLists[player];((psStr != NULL )&&(psStr->id != id) );psStr=psStr->psNext) {}
	}
	return psStr;
}

// ////////////////////////////////////////////////////////////////////////////
// find a feature
FEATURE *IdToFeature(UDWORD id,UDWORD player)
{
	FEATURE	*psF =NULL;
	UDWORD	i;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for(psF=apsFeatureLists[i];( (psF != NULL) && (psF->id != id)); psF=psF->psNext) {}
			if(psF)
			{
				return psF;
			}
		}
	}
	else
	{
		if (player >= MAX_PLAYERS)
		{
			debug(LOG_FEATURE, "Feature detected");
			// feature hack, player = 9 are features
			return NULL;
		}
		for(psF=apsFeatureLists[player];((psF != NULL )&&(psF->id != id) );psF=psF->psNext) {}
	}
	return psF;
}

// ////////////////////////////////////////////////////////////////////////////

DROID_TEMPLATE *IdToTemplate(UDWORD tempId,UDWORD player)
{
	DROID_TEMPLATE *psTempl = NULL;
	UDWORD		i;

	// First try static templates from scripts (could potentially also happen for currently human controlled players)
	for (psTempl = apsStaticTemplates; psTempl && psTempl->multiPlayerID != tempId; psTempl = psTempl->psNext) ;
	if (psTempl) return psTempl;

	// Check if we know which player this is from, in that case, assume it is a player template
	if (player != ANYPLAYER && player < MAX_PLAYERS)
	{
		for (psTempl = apsDroidTemplates[player]; psTempl && (psTempl->multiPlayerID != tempId); psTempl = psTempl->psNext) {}		// follow templates

		return psTempl;
	}

	// We have no idea, so search through every player template
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (psTempl = apsDroidTemplates[i]; psTempl && psTempl->multiPlayerID != tempId; psTempl = psTempl->psNext) ;
		if (psTempl) return psTempl;
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
//  Returns a pointer to base object, given an id and optionally a player.
BASE_OBJECT *IdToPointer(UDWORD id,UDWORD player)
{
	DROID		*pD;
	STRUCTURE	*pS;
	FEATURE		*pF;
	// droids.
	if (IdToDroid(id,player,&pD))
	{
		return (BASE_OBJECT*)pD;
	}

	// structures
	pS = IdToStruct(id,player);
	if(pS)
	{
		return (BASE_OBJECT*)pS;
	}

	// features
	pF = IdToFeature(id,player);
	if(pF)
	{
		return (BASE_OBJECT*)pF;
	}

	return NULL;
}


// ////////////////////////////////////////////////////////////////////////////
// return a players name.
const char* getPlayerName(unsigned int player)
{
	ASSERT_OR_RETURN(NULL, player < MAX_PLAYERS , "Wrong player index: %u", player);

	if (game.type != CAMPAIGN)
	{
		if (strcmp(playerName[player], "") != 0)
		{
			return (char*)&playerName[player];
		}
	}

	if (strlen(NetPlay.players[player].name) == 0)
	{
		// make up a name for this player.
		return getPlayerColourName(player);
	}

	return NetPlay.players[player].name;
}

BOOL setPlayerName(UDWORD player, const char *sName)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Player index (%u) out of range", player);
	sstrcpy(playerName[player], sName);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// to determine human/computer players and responsibilities of each..
BOOL isHumanPlayer(UDWORD player)
{
	if (player >= MAX_PLAYERS)
	{
		return false;	// obvious, really
	}
	return NetPlay.players[player].allocated;
}

// returns player responsible for 'player'
UDWORD  whosResponsible(UDWORD player)
{
	if (isHumanPlayer(player))
	{
		return player;			// Responsible for him or her self
	}
	else if (player == selectedPlayer)
	{
		return player;			// We are responsibly for ourselves
	}
	else
	{
		return NET_HOST_ONLY;	// host responsible for all AIs
	}
}

//returns true if selected player is responsible for 'player'
BOOL myResponsibility(UDWORD player)
{
	return whosResponsible(player) == selectedPlayer;
}

//returns true if 'player' is responsible for 'playerinquestion'
BOOL responsibleFor(UDWORD player, UDWORD playerinquestion)
{
	return whosResponsible(playerinquestion) == player;
}


// ////////////////////////////////////////////////////////////////////////////
// probably temporary. Places the camera on the players 1st droid or struct.
Vector3i cameraToHome(UDWORD player,BOOL scroll)
{
	Vector3i res;
	UDWORD x,y;
	STRUCTURE	*psBuilding;

	for (psBuilding = apsStructLists[player]; psBuilding && (psBuilding->pStructureType->type != REF_HQ); psBuilding= psBuilding->psNext) {}

	if(psBuilding)
	{
		x= map_coord(psBuilding->pos.x);
		y= map_coord(psBuilding->pos.y);
	}
	else if (apsDroidLists[player])				// or first droid
	{
		 x= map_coord(apsDroidLists[player]->pos.x);
		 y=	map_coord(apsDroidLists[player]->pos.y);
	}
	else if (apsStructLists[player])							// center on first struct
	{
		x= map_coord(apsStructLists[player]->pos.x);
		y= map_coord(apsStructLists[player]->pos.y);
	}
	else														//or map center.
	{
		x= mapWidth/2;
		y= mapHeight/2;
	}


	if(scroll)
	{
		requestRadarTrack(world_coord(x), world_coord(y));
	}
	else
	{
		setViewPos(x,y,true);
	}

	res.x = world_coord(x);
	res.y = map_TileHeight(x,y);
	res.z = world_coord(y);
	return res;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Recv Messages. Get a message and dispatch to relevant function.
BOOL recvMessage(void)
{
	NETQUEUE queue;
	uint8_t type;

	while (NETrecvNet(&queue, &type) || NETrecvGame(&queue, &type))          // for all incoming messages.
	{
		bool processedMessage1 = false;
		bool processedMessage2 = false;

		if (queue.queueType == QUEUE_GAME)
		{
			syncDebug("Processing player %d, message %s", queue.index, messageTypeToString(type));
		}

		if (queue.queueType == QUEUE_GAME && myResponsibility(queue.index))
		{
			switch (type)
			{
				// TODO Remove all these cases.
				//case GAME_DROID:        //24 down, 18 to go.
				//case GAME_DROIDINFO:    // 2 down, 41 to go.
				//case GAME_DROIDDEST:    //25 down, 17 to go.
				//case GAME_DROIDMOVE:    // 1 down, 42 to go. <--- Doesn't even exist, now. Its only effect was breaking synch...
				//case GAME_GROUPORDER:   // 3 down, 40 to go.
				//case GAME_CHECK_DROID:  // 4 down, 39 to go.
				//case GAME_CHECK_STRUCT: // 5 down, 38 to go.
				//case GAME_CHECK_POWER:  // 6 down, 37 to go.
				//case NET_TEXTMSG:
				//case NET_DATA_CHECK:
				//case NET_AITEXTMSG:
				//case NET_BEACONMSG:
				//case GAME_BUILD:        //26 down, 16 to go.
				//case GAME_BUILDFINISHED://27 down, 15 to go.
				//case GAME_STRUCTDEST:   //28 down, 14 to go.
				//case GAME_SECONDARY:    // 7 down, 36 to go.
				//case GAME_SECONDARY_ALL://36 down,  6 to go. <--- Removed completely...
				//case GAME_DROIDEMBARK:  //39 down,  3 to go.
				//case GAME_DROIDDISEMBARK://40down,  2 to go.
				//case GAME_GIFT:         //32 down, 10 to go
				//case NET_SCORESUBMIT:
				//case GAME_VTOL:         //37 down,  5 to go. <--- Removed completely, the VTOLs are happy without them.
				//case GAME_LASSAT:       //38 down,  4 to go.

				case GAME_TEMPLATE:
				//case GAME_TEMPLATEDEST: //34 down,  8 to go. <--- Aaargh. This one down, but might come back as a zombie.
				//case GAME_FEATUREDEST:  //29 down, 13 to go.
				//case NET_PING:
				//case GAME_DEMOLISH:     //35 down,  7 to go.
				//case GAME_RESEARCH:     //30 down, 12 to go.
				//case NET_OPTIONS:
				//case NET_PLAYERRESPONDING:
				//case NET_COLOURREQUEST:
				//case NET_POSITIONREQUEST:
				//case NET_TEAMREQUEST:
				//case NET_READY_REQUEST:
				//case GAME_ARTIFACTS:    //23 down, 19 to go.
				//case GAME_FEATURES:     //41 down,  1 to go.
				//case GAME_ALLIANCE:     //33 down,  9 to go.
				//case NET_KICK:
				//case NET_FIREUP:
				//case GAME_RESEARCHSTATUS//31 down, 11 to go.
				//case NET_PLAYER_STATS:
				//case NET_...:           // 22 down, 20 to go.
				{
					static unsigned packets[256];
					if (++packets[type] == 1)
					{
						debug(LOG_WARNING, "Ignoring our own game queue message %s, which hasn't been converted yet", messageTypeToString(type));
					}
					NETpop(queue);
					continue;  // HACK-TODO Ignore our own queue.
				}
				default:
					break;  // Yay, we are able to process our own messages of this time.
			}
		}

		// messages only in game.
		if(!ingame.localJoiningInProgress)
		{
			processedMessage1 = true;
			switch(type)
			{
			case GAME_DROID:						// new droid of known type
				recvDroid(queue);
				break;
			case GAME_DROIDINFO:					//droid update info
				recvDroidInfo(queue);
				break;
			case GAME_DROIDDEST:					// droid destroy
				recvDestroyDroid(queue);
				break;
			case GAME_CHECK_DROID:				// droid damage and position checks
				recvDroidCheck(queue);
				break;
			case GAME_CHECK_STRUCT:				// structure damage checks.
				recvStructureCheck(queue);
				break;
			case GAME_CHECK_POWER:				// Power level syncing.
				recvPowerCheck(queue);
				break;
			case NET_TEXTMSG:					// simple text message
				recvTextMessage(queue);
				break;
			case NET_DATA_CHECK:
				recvDataCheck(queue);
				break;
			case NET_AITEXTMSG:					//multiplayer AI text message
				recvTextMessageAI(queue);
				break;
			case NET_BEACONMSG:					//beacon (blip) message
				recvBeacon(queue);
				break;
			case GAME_BUILDFINISHED:				// a building is complete
				recvBuildFinished(queue);
				break;
			case GAME_STRUCTDEST:				// structure destroy
				recvDestroyStructure(queue);
				break;
			case GAME_SECONDARY:					// set a droids secondary order level.
				recvDroidSecondary(queue);
				break;
			case GAME_DROIDEMBARK:
				recvDroidEmbark(queue);              //droid has embarked on a Transporter
				break;
			case GAME_DROIDDISEMBARK:
				recvDroidDisEmbark(queue);           //droid has disembarked from a Transporter
				break;
			case GAME_GIFT:						// an alliance gift from one player to another.
				recvGift(queue);
				break;
			case GAME_LASSAT:
				recvLasSat(queue);
				break;
			default:
				processedMessage1 = false;
				break;
			}
		}

		// messages usable all the time
		processedMessage2 = true;
		switch(type)
		{
		case GAME_TEMPLATE:					// new template
			recvTemplate(queue);
			break;
		case GAME_TEMPLATEDEST:				// template destroy
			recvDestroyTemplate(queue);
			break;
		case GAME_FEATUREDEST:				// feature destroy
			recvDestroyFeature(queue);
			break;
		case NET_PING:						// diagnostic ping msg.
			recvPing(queue);
			break;
		case GAME_DEMOLISH:					// structure demolished.
			recvDemolishFinished(queue);
			break;
		case GAME_RESEARCH:					// some research has been done.
			recvResearch(queue);
			break;
		case NET_OPTIONS:
			recvOptions(queue);
			break;
		case NET_PLAYER_DROPPED:				// remote player got disconnected
		{
			uint32_t player_id;

			NETbeginDecode(queue, NET_PLAYER_DROPPED);
			{
				NETuint32_t(&player_id);
			}
			NETend();

			debug(LOG_INFO,"** player %u has dropped!", player_id);

			MultiPlayerLeave(player_id);		// get rid of their stuff
			NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, player_id);
			break;
		}
		case NET_PLAYERRESPONDING:			// remote player is now playing
		{
			uint32_t player_id;

			resetReadyStatus(false);

			NETbeginDecode(queue, NET_PLAYERRESPONDING);
				// the player that has just responded
				NETuint32_t(&player_id);
			NETend();
			if (player_id >= MAX_PLAYERS)
			{
				debug(LOG_ERROR, "Bad NET_PLAYERRESPONDING received, ID is %d", (int)player_id);
				break;
			}
			// This player is now with us!
			ingame.JoiningInProgress[player_id] = false;
			break;
		}
		// FIXME: the next 4 cases might not belong here --check (we got two loops for this)
		case NET_COLOURREQUEST:
			recvColourRequest(queue);
			break;
		case NET_POSITIONREQUEST:
			recvPositionRequest(queue);
			break;
		case NET_TEAMREQUEST:
			recvTeamRequest(queue);
			break;
		case NET_READY_REQUEST:
			recvReadyRequest(queue);

			// if hosting try to start the game if everyone is ready
			if(NetPlay.isHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
			}
			break;
		case GAME_ARTIFACTS:
			recvMultiPlayerRandomArtifacts(queue);
			break;
		case GAME_FEATURES:
			recvMultiPlayerFeature(queue);
			break;
		case GAME_ALLIANCE:
			recvAlliance(queue, true);
			break;
		case NET_KICK:
		{
			// FIX ME: in game kick ?  Is this even possible with current code?
			uint32_t player_id;
			char reason[MAX_KICK_REASON];

			if (player_id == NET_HOST_ONLY)
			{
				debug(LOG_ERROR, "someone tried to kick the host--check your netplay logs!");
				NETend();
				break;
			}

			NETbeginDecode(queue, NET_KICK);
				NETuint32_t(&player_id);
				NETstring( reason, MAX_KICK_REASON);
			NETend();

			if (selectedPlayer == player_id)  // we've been told to leave.
			{
				debug(LOG_ERROR, "You were kicked because %s", reason);
				setPlayerHasLost(true);
			}
			else
			{
				NETplayerKicked(player_id);
			}
			break;
		}
		case GAME_RESEARCHSTATUS:
			recvResearchStatus(queue);
			break;
		case GAME_STRUCTUREINFO:
			recvStructureInfo(queue);
			break;
		case NET_PLAYER_STATS:
			recvMultiStats(queue);
			break;
		case GAME_PLAYER_LEFT:
			recvPlayerLeft(queue);
			break;
		default:
			processedMessage2 = false;
			break;
		}

		if (processedMessage1 && processedMessage2)
		{
			debug(LOG_ERROR, "Processed %s message twice!", messageTypeToString(type));
		}
		if (!processedMessage1 && !processedMessage2)
		{
			debug(LOG_ERROR, "Didn't handle %s message!", messageTypeToString(type));
		}

		NETpop(queue);
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Research Stuff. Nat games only send the result of research procedures.
BOOL SendResearch(uint8_t player, uint32_t index, bool trigger)
{
	// Send the player that is researching the topic and the topic itself
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_RESEARCH);
		NETuint8_t(&player);
		NETuint32_t(&index);
	NETend();

	return true;
}

// recv a research topic that is now complete.
static BOOL recvResearch(NETQUEUE queue)
{
	uint8_t			player;
	uint32_t		index;
	int				i;
	PLAYER_RESEARCH	*pPlayerRes;
	RESEARCH		*pResearch;

	NETbeginDecode(queue, GAME_RESEARCH);
		NETuint8_t(&player);
		NETuint32_t(&index);
	NETend();

	syncDebug("player%d, index%u", player, index);

	if (player >= MAX_PLAYERS || index >= numResearch)
	{
		debug(LOG_ERROR, "Bad GAME_RESEARCH received, player is %d, index is %u", (int)player, index);
		return false;
	}

	pPlayerRes = asPlayerResList[player] + index;
	syncDebug("research status = %d", pPlayerRes->ResearchStatus & RESBITS);

	if (!IsResearchCompleted(pPlayerRes))
	{
		MakeResearchCompleted(pPlayerRes);
		researchResult(index, player, false, NULL, true);

		// Take off the power if available
		pResearch = asResearch + index;
		usePower(player, pResearch->researchPower);
	}

	// Update allies research accordingly
	if (game.type == SKIRMISH)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (alliances[i][player] == ALLIANCE_FORMED)
			{
				pPlayerRes = asPlayerResList[i] + index;

				if (!IsResearchCompleted(pPlayerRes))
				{
					// Do the research for that player
					MakeResearchCompleted(pPlayerRes);
					researchResult(index, i, false, NULL, true);
				}
			}
		}
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// New research stuff, so you can see what others are up to!
// inform others that I'm researching this.
BOOL sendResearchStatus(STRUCTURE *psBuilding, uint32_t index, uint8_t player, BOOL bStart)
{
	if (!myResponsibility(player) || gameTime < 5)
	{
		return true;
	}

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_RESEARCHSTATUS);
		NETuint8_t(&player);
		NETbool(&bStart);

		// If we know the building researching it then send its ID
		if (psBuilding)
		{
			NETuint32_t(&psBuilding->id);
		}
		else
		{
			uint32_t zero = 0;
			NETuint32_t(&zero);
		}

		// Finally the topic in question
		NETuint32_t(&index);
	NETend();

	// Tell UI to remove from the list of available research.
	MakeResearchStartedPending(asPlayerResList[player] + index);

	return true;
}

BOOL recvResearchStatus(NETQUEUE queue)
{
	STRUCTURE			*psBuilding;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH_FACILITY	*psResFacilty;
	RESEARCH			*pResearch;
	uint8_t				player;
	BOOL				bStart;
	uint32_t			index, structRef;

	NETbeginDecode(queue, GAME_RESEARCHSTATUS);
		NETuint8_t(&player);
		NETbool(&bStart);
		NETuint32_t(&structRef);
		NETuint32_t(&index);
	NETend();

	syncDebug("player%d, bStart%d, structRef%u, index%u", player, bStart, structRef, index);

	if (player >= MAX_PLAYERS || index >= numResearch)
	{
		debug(LOG_ERROR, "Bad GAME_RESEARCHSTATUS received, player is %d, index is %u", (int)player, index);
		return false;
	}

	pPlayerRes = asPlayerResList[player] + index;

	// psBuilding may be null if finishing
	if (bStart)							// Starting research
	{
		psBuilding = IdToStruct(structRef, player);

		// Set that facility to research
		if (psBuilding && psBuilding->pFunctionality)
		{
			psResFacilty = (RESEARCH_FACILITY *) psBuilding->pFunctionality;

			psResFacilty->psSubjectPending = NULL;  // Research is no longer pending, as it's actually starting now.

			if (psResFacilty->psSubject)
			{
				cancelResearch(psBuilding, ModeImmediate);
			}

			// Set the subject up
			pResearch				= asResearch + index;
			psResFacilty->psSubject = (BASE_STATS *) pResearch;

			// If they have previously started but cancelled there is no need to accure power
			if (IsResearchCancelled(pPlayerRes))
			{
				psResFacilty->powerAccrued	= pResearch->researchPower;
			}
			else
			{
				psResFacilty->powerAccrued	= 0;
			}

			// Start the research
			MakeResearchStarted(pPlayerRes);
			psResFacilty->timeStarted		= ACTION_START_TIME;
			psResFacilty->timeStartHold		= 0;
			psResFacilty->timeToResearch	= pResearch->researchPoints / MAX(psResFacilty->researchPoints, 1);

			// A failsafe of some sort
			if (psResFacilty->timeToResearch == 0)
			{
				psResFacilty->timeToResearch = 1;
			}
		}

	}
	// Finished/cancelled research
	else
	{
		// If they completed the research, we're done
		if (IsResearchCompleted(pPlayerRes))
		{
			return true;
		}

		// If they did not say what facility it was, look it up orselves
		if (!structRef)
		{
			// Go through the structs to find the one doing this topic
			for (psBuilding = apsStructLists[player]; psBuilding; psBuilding = psBuilding->psNext)
			{
				if (psBuilding->pStructureType->type == REF_RESEARCH
				 && psBuilding->status == SS_BUILT
				 && ((RESEARCH_FACILITY *) psBuilding->pFunctionality)->psSubject
				 && ((RESEARCH_FACILITY *) psBuilding->pFunctionality)->psSubject->ref - REF_RESEARCH_START == index)
				{
					break;
				}
			}
		}
		else
		{
			psBuilding = IdToStruct(structRef, player);
		}

		// Stop the facility doing any research
		if (psBuilding)
		{
			cancelResearch(psBuilding, ModeImmediate);
		}
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Text Messaging between players. proceed string with players to send to.
// eg "123hi there" sends "hi there" to players 1,2 and 3.
BOOL sendTextMessage(const char *pStr, BOOL all)
{
	BOOL				normal = true;
	BOOL				sendto[MAX_PLAYERS];
	int					posTable[MAX_PLAYERS];
	UDWORD				i;
	char				display[MAX_CONSOLE_STRING_LENGTH];
	char				msg[MAX_CONSOLE_STRING_LENGTH];
	char*				curStr = (char*)pStr;

	if (!ingame.localOptionsReceived)
	{
		if(!bMultiPlayer)
		{
			// apparently we are not in a mp game, so dump the message to the console.
			addConsoleMessage(curStr,LEFT_JUSTIFY, SYSTEM_MESSAGE);
		}
		return true;
	}

	memset(display,0x0, sizeof(display));	//clear buffer
	memset(msg,0x0, sizeof(display));		//clear buffer
	memset(sendto,0x0, sizeof(sendto));		//clear private flag
	memset(posTable,0x0, sizeof(posTable));		//clear buffer
	sstrcpy(msg, curStr);

	if (!all)
	{
		// create a position table
		for (i = 0; i < game.maxPlayers; i++)
		{
			posTable[NetPlay.players[i].position] = i;
		}

		if (curStr[0] == '.')
		{
			curStr++;
			for (i = 0; i < game.maxPlayers; i++)
			{
				if (i != selectedPlayer && aiCheckAlliances(selectedPlayer, i))
				{
					sendto[i] = true;
				}
			}
			normal = false;
			if (!all)
			{
				sstrcpy(display, _("(allies"));
			}
		}
		for (; curStr[0] >= '0' && curStr[0] <= '7'; curStr++)		// for each 0..7 numeric char encountered
		{
			i = posTable[curStr[0]-'0'];
			if (normal)
			{
				sstrcpy(display, _("(private to "));
			}
			else
			{
				sstrcat(display, ", ");
			}
			if ((isHumanPlayer(i) || (game.type == SKIRMISH && i<game.maxPlayers && game.skDiff[i] ) ))
			{
				sstrcat(display, getPlayerName(posTable[curStr[0]-'0']));
				sendto[i] = true;
			}
			else
			{
				sstrcat(display, _("[invalid]"));
			}
			normal = false;
		}

		if (!normal)	// lets user know it is a private message
		{
			if (curStr[0] == ' ')
			{
				curStr++;
			}
			sstrcat(display, ") ");
			sstrcat(display, curStr);
		}
	}

	if (all)	//broadcast
	{
		NETbeginEncode(NETbroadcastQueue(), NET_TEXTMSG);
		NETuint32_t(&selectedPlayer);		// who this msg is from
		NETstring(msg,MAX_CONSOLE_STRING_LENGTH);	// the message to send
		NETend();
	}
	else if (normal)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (i != selectedPlayer && openchannels[i])
			{
				if (isHumanPlayer(i))
				{
					NETbeginEncode(NETnetQueue(i), NET_TEXTMSG);
					NETuint32_t(&selectedPlayer);		// who this msg is from
					NETstring(msg,MAX_CONSOLE_STRING_LENGTH);	// the message to send
					NETend();
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(msg, selectedPlayer, i);
				}
			}
		}
	}
	else	//private msg
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (sendto[i])
			{
				if (isHumanPlayer(i))
				{
					NETbeginEncode(NETnetQueue(i), NET_TEXTMSG);
					NETuint32_t(&selectedPlayer);				// who this msg is from
					NETstring(display, MAX_CONSOLE_STRING_LENGTH);	// the message to send
					NETend();
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(curStr, selectedPlayer, i);
				}
			}
		}
	}

	//This is for local display
	sstrcpy(msg, NetPlay.players[selectedPlayer].name);		// name
	sstrcat(msg, ": ");						// seperator
	sstrcat(msg, (normal?curStr:display));						// add message

	addConsoleMessage(msg, DEFAULT_JUSTIFY, selectedPlayer);	// display

	return true;
}

void printConsoleNameChange(const char *oldName, const char *newName)
{
	char msg[MAX_CONSOLE_STRING_LENGTH];

	// Player changed name.
	sstrcpy(msg, oldName);                               // Old name.
	sstrcat(msg, " → ");                                 // Separator
	sstrcat(msg, newName);  // New name.

	addConsoleMessage(msg, DEFAULT_JUSTIFY, selectedPlayer);  // display
}


//AI multiplayer message, send from a certain player index to another player index
BOOL sendAIMessage(char *pStr, UDWORD player, UDWORD to)
{
	UDWORD	sendPlayer;

	//check if this is one of the local players, don't need net send then
	if (to == selectedPlayer || myResponsibility(to))	//(the only) human on this machine or AI on this machine
	{
		//Just show "him" the message
		displayAIMessage(pStr, player, to);

		//Received a console message from a player callback
		//store and call later
		//-------------------------------------------------
		if (!msgStackPush(CALL_AI_MSG,player,to,pStr,-1,-1,NULL))
		{
			debug(LOG_ERROR, "sendAIMessage() - msgStackPush - stack failed");
			return false;
		}
	}
	else		//not a local player (use multiplayer mode)
	{
		if (!ingame.localOptionsReceived)
		{
			return true;
		}

		//find machine that is hosting this human or AI
		sendPlayer = whosResponsible(to);

		if (sendPlayer >= MAX_PLAYERS)
		{
			debug(LOG_ERROR, "sendAIMessage() - sendPlayer >= MAX_PLAYERS");
			return false;
		}

		if (!isHumanPlayer(sendPlayer))		//NETsend can't send to non-humans
		{
			debug(LOG_ERROR, "sendAIMessage() - player is not human.");
			return false;
		}

		//send to the player who is hosting 'to' player (might be himself if human and not AI)
		NETbeginEncode(NETnetQueue(sendPlayer), NET_AITEXTMSG);
			NETuint32_t(&player);			//save the actual sender
			//save the actual player that is to get this msg on the source machine (source can host many AIs)
			NETuint32_t(&to);				//save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
			NETstring(pStr, MAX_CONSOLE_STRING_LENGTH);		// copy message in.
		NETend();
	}

	return true;
}

//
// At this time, we do NOT support messages for beacons
//
BOOL sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char* pStr)
{
	int sendPlayer;
	//debug(LOG_WZ, "sendBeacon: '%s'",pStr);

	//find machine that is hosting this human or AI
	sendPlayer = whosResponsible(forPlayer);

	if(sendPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "sendAIMessage() - whosResponsible() failed.");
		return false;
	}

	// I assume this is correct, looks like it sends it to ONLY that person, and the routine
	// kf_AddHelpBlip() iterates for each player it needs.
	NETbeginEncode(NETnetQueue(sendPlayer), NET_BEACONMSG);    // send to the player who is hosting 'to' player (might be himself if human and not AI)
		NETint32_t(&sender);                                // save the actual sender

		// save the actual player that is to get this msg on the source machine (source can host many AIs)
		NETint32_t(&forPlayer);                             // save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
		NETint32_t(&locX);                                  // save location
		NETint32_t(&locY);

		// const_cast: need to cast away constness because of the const-incorrectness of NETstring (const-incorrect when sending/encoding a packet)
		NETstring((char*)pStr, MAX_CONSOLE_STRING_LENGTH);  // copy message in.
	NETend();

	return true;
}

void displayAIMessage(char *pStr, SDWORD from, SDWORD to)
{
	char				tmp[255];

	if (isHumanPlayer(to))		//display text only if receiver is the (human) host machine itself
	{
		strcpy(tmp, getPlayerName(from));
		strcat(tmp, ": ");											// seperator
		strcat(tmp, pStr);											// add message

		addConsoleMessage(tmp, DEFAULT_JUSTIFY, from);
	}
}

// Write a message to the console.
BOOL recvTextMessage(NETQUEUE queue)
{
	UDWORD	playerIndex;
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	char newmsg[MAX_CONSOLE_STRING_LENGTH];

	memset(msg, 0x0, sizeof(msg));
	memset(newmsg, 0x0, sizeof(newmsg));

	NETbeginDecode(queue, NET_TEXTMSG);
		// Who this msg is from
		NETuint32_t(&playerIndex);
		// The message to send
		NETstring(newmsg, MAX_CONSOLE_STRING_LENGTH);
	NETend();

	if (whosResponsible(playerIndex) != queue.index)
	{
		playerIndex = queue.index;  // Fix corrupted playerIndex.
	}

	if (playerIndex >= MAX_PLAYERS)
	{
		return false;
	}

	sstrcpy(msg, NetPlay.players[playerIndex].name);
	// Seperator
	sstrcat(msg, ": ");
	// Add message
	sstrcat(msg, newmsg);

	addConsoleMessage(msg, DEFAULT_JUSTIFY, playerIndex);

	// Multiplayer message callback
	// Received a console message from a player, save
	MultiMsgPlayerFrom = playerIndex;
	MultiMsgPlayerTo = selectedPlayer;

	sstrcpy(MultiplayMsg, newmsg);
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_AI_MSG);

	// make some noise!
	if (titleMode == MULTIOPTION || titleMode == MULTILIMIT)
	{
		audio_PlayTrack(FE_AUDIO_MESSAGEEND);
	}
	else if (!ingame.localJoiningInProgress)
	{
		audio_PlayTrack(ID_SOUND_MESSAGEEND);
	}

	return true;
}

//AI multiplayer message - received message from AI (from scripts)
BOOL recvTextMessageAI(NETQUEUE queue)
{
	UDWORD	sender, receiver;
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	char	newmsg[MAX_CONSOLE_STRING_LENGTH];

	NETbeginDecode(queue, NET_AITEXTMSG);
		NETuint32_t(&sender);			//in-game player index ('normal' one)
		NETuint32_t(&receiver);			//in-game player index
		NETstring(newmsg,MAX_CONSOLE_STRING_LENGTH);
	NETend();

	if (whosResponsible(sender) != queue.index)
	{
		sender = queue.index;  // Fix corrupted sender.
	}

	//sstrcpy(msg, getPlayerName(sender));  // name
	//sstrcat(msg, " : ");                  // seperator
	sstrcpy(msg, newmsg);

	//Display the message and make the script callback
	displayAIMessage(msg, sender, receiver);

	//Received a console message from a player callback
	//store and call later
	//-------------------------------------------------
	if(!msgStackPush(CALL_AI_MSG,sender,receiver,msg,-1,-1,NULL))
	{
		debug(LOG_ERROR, "recvTextMessageAI() - msgStackPush - stack failed");
		return false;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Templates

// send a newly created template to other players
BOOL sendTemplate(DROID_TEMPLATE *pTempl)
{
	int i;
	uint8_t player = selectedPlayer;

	ASSERT(pTempl != NULL, "sendTemplate: Old Pumpkin bug");
	if (!pTempl) return true; /* hack */

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_TEMPLATE);
		NETuint8_t(&player);
		NETuint32_t(&pTempl->ref);
		NETstring(pTempl->aName, sizeof(pTempl->aName));
		NETuint8_t(&pTempl->NameVersion);

		for (i = 0; i < ARRAY_SIZE(pTempl->asParts); ++i)
		{
			// signed, but sent as a bunch of bits...
			NETint32_t(&pTempl->asParts[i]);
		}

		NETuint32_t(&pTempl->buildPoints);
		NETuint32_t(&pTempl->powerPoints);
		NETuint32_t(&pTempl->storeCount);
		NETuint32_t(&pTempl->numWeaps);

		for (i = 0; i < DROID_MAXWEAPS; i++)
		{
			NETuint32_t(&pTempl->asWeaps[i]);
		}

		NETenum(&pTempl->droidType);
		NETuint32_t(&pTempl->multiPlayerID);

	return NETend();
}

// receive a template created by another player
BOOL recvTemplate(NETQUEUE queue)
{
	uint8_t			player;
	DROID_TEMPLATE	*psTempl;
	DROID_TEMPLATE	t, *pT = &t;
	int				i;

	NETbeginDecode(queue, GAME_TEMPLATE);
		NETuint8_t(&player);
		ASSERT(player < MAX_PLAYERS, "recvtemplate: invalid player size: %d", player);

		NETuint32_t(&pT->ref);
		NETstring(pT->aName, sizeof(pT->aName));
		NETuint8_t(&pT->NameVersion);

		for (i = 0; i < ARRAY_SIZE(pT->asParts); ++i)
		{
			// signed, but sent as a bunch of bits...
			NETint32_t(&pT->asParts[i]);
		}

		NETuint32_t(&pT->buildPoints);
		NETuint32_t(&pT->powerPoints);
		NETuint32_t(&pT->storeCount);
		NETuint32_t(&pT->numWeaps);

		for (i = 0; i < DROID_MAXWEAPS; i++)
		{
			NETuint32_t(&pT->asWeaps[i]);
		}

		NETenum(&pT->droidType);
		NETuint32_t(&pT->multiPlayerID);
	NETend();

	t.prefab = false;
	t.psNext = NULL;
	t.pName = NULL;

	psTempl = IdToTemplate(t.multiPlayerID,player);

	// Already exists
	if (psTempl)
	{
		t.psNext = psTempl->psNext;
		memcpy(psTempl, &t, sizeof(DROID_TEMPLATE));
		debug(LOG_SYNC, "Updating MP template %d", (int)t.multiPlayerID);
	}
	else
	{
		addTemplate(player,&t);
		apsDroidTemplates[player]->ref = REF_TEMPLATE_START;
		debug(LOG_SYNC, "Creating MP template %d", (int)t.multiPlayerID);
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// inform others that you no longer have a template

BOOL SendDestroyTemplate(DROID_TEMPLATE *t)
{
	uint8_t player = selectedPlayer;

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_TEMPLATEDEST);
		NETuint8_t(&player);
		NETuint32_t(&t->multiPlayerID);
	NETend();

	return true;
}

// acknowledge another player no longer has a template
static BOOL recvDestroyTemplate(NETQUEUE queue)
{
	uint8_t			player;
	uint32_t		templateID;
	DROID_TEMPLATE	*psTempl, *psTempPrev = NULL;

	NETbeginDecode(queue, GAME_TEMPLATEDEST);
		NETuint8_t(&player);
		NETuint32_t(&templateID);
	NETend();

	// Find the template in the list
	for (psTempl = apsDroidTemplates[player]; psTempl; psTempl = psTempl->psNext)
	{
		if (psTempl->multiPlayerID == templateID)
		{
			break;
		}

		psTempPrev = psTempl;
	}

	// If we found it then delete it
	if (psTempl)
	{
		// Update the linked list
		if (psTempPrev)
		{
			psTempPrev->psNext = psTempl->psNext;
		}
		else
		{
			apsDroidTemplates[player] = psTempl->psNext;
		}

		// Delete the template.
		//before deleting the template, need to make sure not being used in production
		deleteTemplateFromProduction(psTempl, player, ModeImmediate);
		free(psTempl);
	}
	else
	{
		DROID_TEMPLATE aaargh;
		aaargh.multiPlayerID = templateID;
		deleteTemplateFromProduction(&aaargh, player, ModeImmediate);
		debug(LOG_ERROR, "TODO: Rewrite the whole interface, so it's possible to change the code without spaghetti dependencies causing problems everywhere, and without resorting to ugly hacks.");
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Features

// send a destruct feature message.
BOOL SendDestroyFeature(FEATURE *pF)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_FEATUREDEST);
		NETuint32_t(&pF->id);
	return NETend();
}

// process a destroy feature msg.
BOOL recvDestroyFeature(NETQUEUE queue)
{
	FEATURE *pF;
	uint32_t	id;

	NETbeginDecode(queue, GAME_FEATUREDEST);
		NETuint32_t(&id);
	NETend();

	pF = IdToFeature(id,ANYPLAYER);
	if (pF == NULL)
	{
		debug(LOG_FEATURE, "feature id %d not found (probably already destroyed)", id);
		return false;
	}

	debug(LOG_FEATURE, "p%d feature id %d destroyed (%s)", pF->player, pF->id, pF->psStats->pName);
	// Remove the feature locally
	turnOffMultiMsg(true);
	removeFeature(pF);
	turnOffMultiMsg(false);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Network File packet processor.
BOOL recvMapFileRequested(NETQUEUE queue)
{
	char mapStr[256],mapName[256],fixedname[256];
	uint32_t player;

	PHYSFS_sint64 fileSize_64;
	PHYSFS_file	*pFileHandle;

	// We are not the host, so we don't care. (in fact, this would be a error)
	if(!NetPlay.isHost)
	{
		return true;
	}

	//	Check to see who wants the file
	NETbeginDecode(queue, NET_FILE_REQUESTED);
	NETuint32_t(&player);
	NETend();

	if (!NetPlay.players[player].wzFile.isSending)
	{
		NetPlay.players[player].needFile = true;
		NetPlay.players[player].wzFile.isCancelled = false;
		NetPlay.players[player].wzFile.isSending = true;

		memset(mapStr,0,256);
		memset(mapName,0,256);
		memset(fixedname,0,256);

		addConsoleMessage("Map was requested: SENDING MAP!",DEFAULT_JUSTIFY, SYSTEM_MESSAGE);

		sstrcpy(mapName, game.map);
		if (	strstr(mapName,"-T1") != 0
			|| strstr(mapName,"-T2") != 0
			|| strstr(mapName,"-T3") != 0)
		{
		// chop off the -T1 *only when needed!*
		mapName[strlen(game.map)-3] = 0;		// chop off the -T1 etc..
		}
		// chop off the sk- if required.
		if(strncmp(mapName,"Sk-",3) == 0)
		{
			sstrcpy(mapStr, &(mapName[3]));
			sstrcpy(mapName, mapStr);
		}

		snprintf(mapStr, sizeof(mapStr), "%dc-%s.wz", game.maxPlayers, mapName);
		snprintf(fixedname, sizeof(fixedname), "maps/%s", mapStr);		//We know maps are in /maps dir...now. fix for linux -Q
		sstrcpy(mapStr, fixedname);
		debug(LOG_NET, "Map was requested. Looking for %s", mapStr);

		// Checking to see if file is available...
		pFileHandle = PHYSFS_openRead(mapStr);
		if (pFileHandle == NULL)
		{
			debug(LOG_ERROR, "Failed to open %s for reading: %s", mapStr, PHYSFS_getLastError());
			debug(LOG_FATAL, "You have a map (%s) that can't be located.\n\nMake sure it is in the correct directory and or format! (No map packs!)", mapStr);
			// NOTE: if we get here, then the game is basically over, The host can't send the file for whatever reason...
			// Which also means, that we can't continue.
			debug(LOG_NET, "***Host has a file issue, and is being forced to quit!***");
			NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
			NETend();
			abort();
	}

		// get the file's size.
		fileSize_64 = PHYSFS_fileLength(pFileHandle);
		debug(LOG_NET, "File is valid, sending [directory: %s] %s to client %u", PHYSFS_getRealDir(mapStr), mapStr, player);

		NetPlay.players[player].wzFile.pFileHandle = pFileHandle;
		NetPlay.players[player].wzFile.fileSize_32 = (int32_t) fileSize_64;		//we don't support 64bit int nettypes.
		NetPlay.players[player].wzFile.currPos = 0;

		NETsendFile(mapStr, player);
	}
	return true;
}

// continue sending the map
void sendMap(void)
{
	int i = 0;
	UBYTE done;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (NetPlay.players[i].wzFile.isSending)
	{
			done = NETsendFile(game.map, i);
			if (done == 100)
			{
		addConsoleMessage("MAP SENT!",DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
				debug(LOG_NET, "=== File has been sent to player %d ===", i);
				NetPlay.players[i].wzFile.isSending = false;
				NetPlay.players[i].needFile = false;
	}
		}
	}
}

// Another player is broadcasting a map, recv a chunk. Returns false if not yet done.
BOOL recvMapFileData(NETQUEUE queue)
{
	mapDownloadProgress = NETrecvFile(queue);
	if (mapDownloadProgress == 100)
	{
		addConsoleMessage("MAP DOWNLOADED!",DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		sendTextMessage("MAP DOWNLOADED",true);					//send
		debug(LOG_NET, "=== File has been received. ===");

		// clear out the old level list.
		levShutDown();
		levInitialise();
		rebuildSearchPath(mod_multiplay, true);	// MUST rebuild search path for the new maps we just got!
		if (!buildMapList())
		{
			return false;
		}
		return true;
	}

	return false;
}


//------------------------------------------------------------------------------------------------//

/* multiplayer message stack */
void msgStackReset(void)
{
	msgStackPos = -1;		//Beginning of the stack
}

UDWORD msgStackPush(SDWORD CBtype, SDWORD plFrom, SDWORD plTo, const char *tStr, SDWORD x, SDWORD y, DROID *psDroid)
{
	debug(LOG_WZ, "msgStackPush: pushing message type %d to pos %d", CBtype, msgStackPos + 1);

	if (msgStackPos >= MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPush() - stack full");
		return false;
	}

	//make point to the last valid element
	msgStackPos++;

	//remember values
	msgPlFrom[msgStackPos] = plFrom;
	msgPlTo[msgStackPos] = plTo;

	callbackType[msgStackPos] = CBtype;
	locx[msgStackPos] = x;
	locy[msgStackPos] = y;

	strcpy(msgStr[msgStackPos], tStr);

	msgDroid[msgStackPos] = psDroid;

	return true;
}

BOOL isMsgStackEmpty(void)
{
	if(msgStackPos <= (-1)) return true;
	return false;
}

BOOL msgStackGetFrom(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetFrom: msgStackPos < 0");
		return false;
	}

	*psVal = msgPlFrom[0];

	return true;
}

BOOL msgStackGetTo(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetTo: msgStackPos < 0");
		return false;
	}

	*psVal = msgPlTo[0];

	return true;
}

static BOOL msgStackGetCallbackType(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetCallbackType: msgStackPos < 0");
		return false;
	}

	*psVal = callbackType[0];

	return true;
}

static BOOL msgStackGetXY(SDWORD  *psValx, SDWORD  *psValy)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetXY: msgStackPos < 0");
		return false;
	}

	*psValx = locx[0];
	*psValy = locy[0];

	return true;
}


BOOL msgStackGetMsg(char  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetMsg: msgStackPos < 0");
		return false;
	}

	strcpy(psVal, msgStr[0]);
	//*psVal = msgPlTo[msgStackPos];

	return true;
}

static BOOL msgStackSort(void)
{
	SDWORD i;

	//go through all-1 elements (bottom-top)
	for(i=0;i<msgStackPos;i++)
	{
		msgPlFrom[i] = msgPlFrom[i+1];
		msgPlTo[i] = msgPlTo[i+1];

		callbackType[i] = callbackType[i+1];
		locx[i] = locx[i+1];
		locy[i] = locy[i+1];

		strcpy(msgStr[i], msgStr[i+1]);
	}

	//erase top element
	msgPlFrom[msgStackPos] = -2;
	msgPlTo[msgStackPos] = -2;

	callbackType[msgStackPos] = -2;
	locx[msgStackPos] = -2;
	locy[msgStackPos] = -2;

	strcpy(msgStr[msgStackPos], "ERROR char!!!!!!!!");

	msgStackPos--;		//since removed the top element

	return true;
}

BOOL msgStackPop(void)
{
	debug(LOG_WZ, "msgStackPop: stack size %d", msgStackPos);

	if(msgStackPos < 0 || msgStackPos > MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPop: wrong msgStackPos index: %d", msgStackPos);
		return false;
	}

	return msgStackSort();		//move all elements 1 pos lower
}

BOOL msgStackGetDroid(DROID **ppsDroid)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetDroid: msgStackPos < 0");
		return false;
	}

	*ppsDroid = msgDroid[0];

	return true;
}

SDWORD msgStackGetCount(void)
{
	return msgStackPos + 1;
}

BOOL msgStackFireTop(void)
{
	SDWORD		_callbackType;
	char		msg[255];

	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackFireTop: msgStackPos < 0");
		return false;
	}

	if(!msgStackGetCallbackType(&_callbackType))
		return false;

	switch(_callbackType)
	{
		case CALL_VIDEO_QUIT:
			debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_VIDEO_QUIT");
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
			break;

		case CALL_DORDER_STOP:
			ASSERT(false,"CALL_DORDER_STOP is currently disabled");

			debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_DORDER_STOP");

			if(!msgStackGetDroid(&psScrCBOrderDroid))
				return false;

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DORDER_STOP);
			break;

		case CALL_BEACON:

			if(!msgStackGetXY(&beaconX, &beaconY))
				return false;

			if(!msgStackGetFrom(&MultiMsgPlayerFrom))
				return false;

			if(!msgStackGetTo(&MultiMsgPlayerTo))
				return false;

			if(!msgStackGetMsg(msg))
				return false;

			strcpy(MultiplayMsg, msg);

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BEACON);
			break;

		case CALL_AI_MSG:
			if(!msgStackGetFrom(&MultiMsgPlayerFrom))
				return false;

			if(!msgStackGetTo(&MultiMsgPlayerTo))
				return false;

			if(!msgStackGetMsg(msg))
				return false;

			strcpy(MultiplayMsg, msg);

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_AI_MSG);
			break;

		default:
			debug(LOG_ERROR, "msgStackFireTop: unknown callback type");
			return false;
			break;
	}

	if(!msgStackPop())
		return false;

	return true;
}

static BOOL recvBeacon(NETQUEUE queue)
{
	int32_t sender, receiver,locX, locY;
	char    msg[MAX_CONSOLE_STRING_LENGTH];

	NETbeginDecode(queue, NET_BEACONMSG);
	    NETint32_t(&sender);            // the actual sender
	    NETint32_t(&receiver);          // the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
	    NETint32_t(&locX);
	    NETint32_t(&locY);
	    NETstring(msg, sizeof(msg));    // Receive the actual message
	NETend();

	debug(LOG_WZ, "Received beacon for player: %d, from: %d",receiver, sender);

	sstrcat(msg, NetPlay.players[sender].name);    // name
	sstrcpy(beaconReceiveMsg[sender], msg);

	return addBeaconBlip(locX, locY, receiver, sender, beaconReceiveMsg[sender]);
}

const char* getPlayerColourName(unsigned int player)
{
	static const char* playerColors[] =
	{
		N_("Green"),
		N_("Orange"),
		N_("Grey"),
		N_("Black"),
		N_("Red"),
		N_("Blue"),
		N_("Pink"),
		N_("Cyan")
	};

	ASSERT(player < ARRAY_SIZE(playerColors), "player number (%d) exceeds maximum (%lu)", player, (unsigned long) ARRAY_SIZE(playerColors));

	if (player >= ARRAY_SIZE(playerColors))
	{
		return "";
	}

	return gettext(playerColors[getPlayerColour(player)]);
}

/* Reset ready status for all players */
void resetReadyStatus(bool bSendOptions)
{
	unsigned int player;

	for(player = 0; player < MAX_PLAYERS; player++)
	{
		bPlayerReadyGUI[player] = false;
	}

	// notify all clients if needed
	if(bSendOptions)
	{
		sendOptions();
	}
	netPlayersUpdated = true;
}
