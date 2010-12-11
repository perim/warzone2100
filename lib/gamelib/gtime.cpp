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
 * GTime.c
 *
 * Provide a game clock that runs only when the game runs.
 *
 */

#include <time.h>

#include "lib/framework/frame.h"
#include "lib/framework/wzapp_c.h"
#include "gtime.h"
#include "src/multiplay.h"
#include "lib/netplay/netplay.h"


/* See header file for documentation */
UDWORD gameTime = 0, deltaGameTime = 0, graphicsTime = 0, deltaGraphicsTime = 0, realTime = 0, deltaRealTime = 0;
float graphicsTimeFraction = 0.0, realTimeFraction = 0.0;

/** The current clock modifier. Set to speed up the game. */
static float modifier;

/// The graphics time, the last time clock speed was set.
static UDWORD	timeOffset;

/// The real time, the last time the clock speed was set.
static UDWORD	baseTime;

/** When the game paused, so that gameTime can be adjusted when the game restarts. */
static SDWORD	pauseStart;

/** 
  * Count how many times gameTimeStop has been called without a game time start. 
  * We use this to ensure that we can properly nest stop commands. 
  **/
static UDWORD	stopCount;

static uint32_t gameQueueTime[MAX_PLAYERS];
static uint32_t gameQueueCheckTime[MAX_PLAYERS];
static uint32_t gameQueueCheckCrc[MAX_PLAYERS];
static bool     crcError = false;

static uint32_t updateReadyTime = 0;
static uint32_t updateWantedTime = 0;
static uint16_t chosenLatency = GAME_TICKS_PER_UPDATE;
static uint16_t discreteChosenLatency = GAME_TICKS_PER_UPDATE;
static uint16_t wantedLatency = GAME_TICKS_PER_UPDATE;
static uint16_t wantedLatencies[MAX_PLAYERS];

static void updateLatency(void);

/* Initialise the game clock */
void gameTimeInit(void)
{
	unsigned player;

	/* Start the timer off at 2 so that when the scripts strip the map of objects
	 * for multiPlayer they will be processed as if they died. */
	// Setting game and graphics time.
	setGameTime(2);

	// Setting real time.
	realTime = baseTime;
	deltaRealTime = 0;
	realTimeFraction = 0.f;

	modifier = 1.0f;

	stopCount = 0;

	chosenLatency = GAME_TICKS_PER_UPDATE;
	discreteChosenLatency = GAME_TICKS_PER_UPDATE;
	wantedLatency = GAME_TICKS_PER_UPDATE;
	for (player = 0; player != MAX_PLAYERS; ++player)
	{
		wantedLatencies[player] = 0;
	}

	// Don't let syncDebug from previous games cause a desynch dump at gameTime 102.
	resetSyncDebug();
}

extern void setGameTime(uint32_t newGameTime)
{
	// Setting game time.
	gameTime = newGameTime;
	setPlayerGameTime(NET_ALL_PLAYERS, newGameTime);
	deltaGameTime = 0;

	// Setting graphics time to game time.
	graphicsTime = gameTime;
	deltaGraphicsTime = gameTime;
	graphicsTimeFraction = 0.f;
	timeOffset = graphicsTime;
	baseTime = wzGetTicks();

	// Not setting real time.
}

UDWORD getModularScaledGameTime(UDWORD timePeriod, UDWORD requiredRange)
{
	return gameTime%timePeriod * requiredRange/timePeriod;
}

UDWORD getModularScaledGraphicsTime(UDWORD timePeriod, UDWORD requiredRange)
{
	return graphicsTime%timePeriod * requiredRange/timePeriod;
}

UDWORD getModularScaledRealTime(UDWORD timePeriod, UDWORD requiredRange)
{
	return realTime%timePeriod * requiredRange/timePeriod;
}

/* Call this each loop to update the game timer */
void gameTimeUpdate()
{
	uint32_t currTime = wzGetTicks();

	if (currTime < baseTime)
	{
		// Warzone 2100, the first relativistic computer game!
		// Exhibit A: Time travel
		// force a rebase
		debug(LOG_WARNING, "Time travel is occurring! Clock went back in time a bit from %d to %d!\n", baseTime, currTime);
		baseTime = currTime;
		timeOffset = graphicsTime;
	}

	// Do not update the game time if gameTimeStop has been called
	if (stopCount == 0)
	{
		bool mayUpdate = true;

		// Calculate the new game time
		uint32_t scaledCurrTime = (currTime - baseTime)*modifier + timeOffset;
		if (scaledCurrTime < graphicsTime)  // Make sure the clock doesn't step back at all.
		{
			debug(LOG_WARNING, "Rescaled clock went back in time a bit from %d to %d!\n", graphicsTime, scaledCurrTime);
			scaledCurrTime = graphicsTime;
			baseTime = currTime;
			timeOffset = graphicsTime;
		}

		if (updateWantedTime == 0 && scaledCurrTime >= gameTime)
		{
			updateWantedTime = currTime;  // This is the time that we wanted to tick.
		}

		if (scaledCurrTime >= gameTime && !checkPlayerGameTime(NET_ALL_PLAYERS))
		{
			unsigned player;

			// Pause time, since we are waiting GAME_GAME_TIME from other players.
			scaledCurrTime = graphicsTime;
			baseTime = currTime;
			timeOffset = graphicsTime;

			debug(LOG_SYNC, "Waiting for other players. gameTime = %u, player times are {%u, %u, %u, %u, %u, %u, %u, %u}", gameTime, gameQueueTime[0], gameQueueTime[1], gameQueueTime[2], gameQueueTime[3], gameQueueTime[4], gameQueueTime[5], gameQueueTime[6], gameQueueTime[7]);
			mayUpdate = false;

			for (player = 0; player < MAX_PLAYERS; ++player)
			{
				if (!checkPlayerGameTime(player))
				{
					NETsetPlayerConnectionStatus(CONNECTIONSTATUS_WAITING_FOR_PLAYER, player);
					break;  // GAME_GAME_TIME is processed serially, so don't know if waiting for more players.
				}
			}
		}

		// Calculate the time for this frame
		deltaGraphicsTime = scaledCurrTime - graphicsTime;

		// Adjust deltas.
		if (scaledCurrTime >= gameTime && mayUpdate)
		{
			if (scaledCurrTime > gameTime + GAME_TICKS_PER_UPDATE)
			{
				// Game isn't updating fast enough...
				uint32_t slideBack = deltaGraphicsTime - GAME_TICKS_PER_UPDATE;
				baseTime += slideBack / modifier;  // adjust the addition to base time
				deltaGraphicsTime -= slideBack;
			}

			deltaGameTime = GAME_TICKS_PER_UPDATE;

			updateLatency();

			if (crcError)
			{
				debug(LOG_ERROR, "Synch error, gameTimes were: {%10u, %10u, %10u, %10u, %10u, %10u, %10u, %10u}", gameQueueCheckTime[0], gameQueueCheckTime[1], gameQueueCheckTime[2], gameQueueCheckTime[3], gameQueueCheckTime[4], gameQueueCheckTime[5], gameQueueCheckTime[6], gameQueueCheckTime[7]);
				debug(LOG_ERROR, "Synch error, CRCs were:      {0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X}", gameQueueCheckCrc[0], gameQueueCheckCrc[1], gameQueueCheckCrc[2], gameQueueCheckCrc[3], gameQueueCheckCrc[4], gameQueueCheckCrc[5], gameQueueCheckCrc[6], gameQueueCheckCrc[7]);
				crcError = false;
			}
		}
		else
		{
			deltaGameTime = 0;
		}

		// Store the game and graphics times
		gameTime     += deltaGameTime;
		graphicsTime += deltaGraphicsTime;
	}
	else
	{
		// The game is paused, so the change in time is zero.
		deltaGameTime = 0;
		deltaGraphicsTime = 0;
	}

	// Pre-calculate fraction used in timeAdjustedIncrement
	graphicsTimeFraction = (float)deltaGraphicsTime / (float)GAME_TICKS_PER_SEC;

	ASSERT(graphicsTime <= gameTime, "Trying to see the future.");
}

void gameTimeUpdateEnd()
{
	deltaGameTime = 0;
}

void realTimeUpdate(void)
{
	uint32_t currTime = wzGetTicks();

	// now update realTime which does not pause
	// Store the real time
	deltaRealTime = currTime - realTime;
	realTime += deltaRealTime;

	deltaRealTime = MIN(deltaRealTime, GTIME_MAXFRAME);  // Don't scroll across the map suddenly, if computer freezes for a moment.

	// Pre-calculate fraction used in timeAdjustedIncrement
	realTimeFraction = (float)deltaRealTime / (float)GAME_TICKS_PER_SEC;
}

// reset the game time modifiers
void gameTimeResetMod(void)
{
	timeOffset = graphicsTime;
	baseTime = wzGetTicks();

	modifier = 1.0f;
}

// set the time modifier
void gameTimeSetMod(float mod)
{
	gameTimeResetMod();
	modifier = mod;
}

// get the current time modifier
void gameTimeGetMod(float *pMod)
{
	*pMod = modifier;
}

BOOL gameTimeIsStopped(void)
{
	return (stopCount != 0);
}

/* Call this to stop the game timer */
void gameTimeStop(void)
{
	if (stopCount == 0)
	{
		pauseStart = wzGetTicks();
		debug( LOG_NEVER, "Clock paused at %d\n", pauseStart);
	}
	stopCount += 1;
}

/* Call this to restart the game timer after a call to gameTimeStop */
void gameTimeStart(void)
{
	if (stopCount == 1)
	{
		// shift the base time to now
		timeOffset = gameTime;
		baseTime = wzGetTicks();
	}

	if (stopCount > 0)
	{
		stopCount --;
	}
}

/* Call this to reset the game timer */
void gameTimeReset(UDWORD time)
{
	// reset the game timers
	setGameTime(time);
	gameTimeResetMod();
	realTime = wzGetTicks();
	deltaRealTime = 0;
}

void	getTimeComponents(UDWORD time, UDWORD *hours, UDWORD *minutes, UDWORD *seconds)
{
	UDWORD	h, m, s;
	UDWORD	ticks_per_hour, ticks_per_minute;

	/* Ticks in a minute */
	ticks_per_minute = GAME_TICKS_PER_SEC * 60;

	/* Ticks in an hour */
	ticks_per_hour = ticks_per_minute * 60;

	h = time / ticks_per_hour;
	m = (time - (h * ticks_per_hour)) / ticks_per_minute;
	s = (time - ((h * ticks_per_hour) + (m * ticks_per_minute))) / GAME_TICKS_PER_SEC;

	*hours = h;
	*minutes = m;
	*seconds = s;
}

static void updateLatency()
{
	uint16_t maxWantedLatency = 0;
	unsigned player;
	uint16_t prevDiscreteChosenLatency = discreteChosenLatency;

	// Find out what latency has been agreed on, next.
	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		if (!NetPlay.players[player].kick)  // .kick: Don't wait for dropped players.
		{
			//minWantedLatency = MIN(minWantedLatency, wantedLatencies[player]);  // Minimum, so the clients don't increase the latency to try to make one slow computer run faster.
			maxWantedLatency = MAX(maxWantedLatency, wantedLatencies[player]);  // Maximum, since the host experiences lower latency than everyone else.
		}
	}
	// Adjust the agreed latency. (Can maximum decrease by 15ms or increase by 30ms per update.
	chosenLatency = chosenLatency + clip(maxWantedLatency - chosenLatency, -15, 30);
	// Round the chosen latency to an integer number of updates, up to 10.
	discreteChosenLatency = clip((chosenLatency + GAME_TICKS_PER_UPDATE/2)/GAME_TICKS_PER_UPDATE*GAME_TICKS_PER_UPDATE, GAME_TICKS_PER_UPDATE, GAME_TICKS_PER_UPDATE*GAME_UPDATES_PER_SEC);
	if (prevDiscreteChosenLatency != discreteChosenLatency)
	{
		debug(LOG_SYNC, "Adjusting latency %d -> %d", prevDiscreteChosenLatency, discreteChosenLatency);
	}

	// We want the chosen latency to increase by how much our update was delayed waiting for others, or to decrease by how long after we got the messages from others that it was time to tick. Plus a tiny 10ms buffer.
	// We will send this number to others.
	wantedLatency = clip((int)(discreteChosenLatency + updateReadyTime - updateWantedTime + 10), 0, UINT16_MAX);

	// Reset the times, ready to be set again.
	updateReadyTime = 0;
	updateWantedTime = 0;
}

void sendPlayerGameTime()
{
	unsigned player;
	uint32_t latencyTicks = discreteChosenLatency / GAME_TICKS_PER_UPDATE;
	uint32_t checkTime = gameTime;
	uint32_t checkCrc = nextDebugSync();

	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		if (!myResponsibility(player) && whosResponsible(player) != realSelectedPlayer)
		{
			continue;
		}

		NETbeginEncode(NETgameQueue(player), GAME_GAME_TIME);
			NETuint32_t(&latencyTicks);
			NETuint32_t(&checkTime);
			NETuint32_tLarge(&checkCrc);
			NETuint16_t(&wantedLatency);
		NETend();
	}
}

void recvPlayerGameTime(NETQUEUE queue)
{
	uint32_t latencyTicks = 0;
	uint32_t checkTime = 0;
	uint32_t checkCrc = 0;

	NETbeginDecode(queue, GAME_GAME_TIME);
		NETuint32_t(&latencyTicks);
		NETuint32_t(&checkTime);
		NETuint32_tLarge(&checkCrc);
		NETuint16_t(&wantedLatencies[queue.index]);
	NETend();

	gameQueueTime[queue.index] = checkTime + latencyTicks * GAME_TICKS_PER_UPDATE;  // gameTime when future messages shall be processed.

	gameQueueCheckTime[queue.index] = checkTime;
	gameQueueCheckCrc[queue.index] = checkCrc;
	if (!checkDebugSync(checkTime, checkCrc))
	{
		crcError = true;
		if (NetPlay.players[queue.index].allocated)
		{
			NETsetPlayerConnectionStatus(CONNECTIONSTATUS_DESYNC, queue.index);
		}
	}

	if (updateReadyTime == 0 && checkPlayerGameTime(NET_ALL_PLAYERS))
	{
		updateReadyTime = wzGetTicks();  // This is the time we were able to tick.
	}
}

bool checkPlayerGameTime(unsigned player)
{
	unsigned begin = player, end = player + 1;
	if (player == NET_ALL_PLAYERS)
	{
		begin = 0;
		end = MAX_PLAYERS;
	}

	for (player = begin; player < end; ++player)
	{
		if (gameTime > gameQueueTime[player] && !NetPlay.players[player].kick)  // .kick: Don't wait for dropped players.
		{
			return false;  // Still waiting for this player.
		}
	}

	return true;  // Have GAME_GAME_TIME from all players.
}

void setPlayerGameTime(unsigned player, uint32_t time)
{
	if (player == NET_ALL_PLAYERS)
	{
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			gameQueueTime[player] = time;
		}
	}
	else
	{
		gameQueueTime[player] = time;
	}
}

bool isInSync(void)
{
	return !crcError;
}
