var lastHitTime = 0;
var numArtifact = 0;
var scav1groupid, scav2groupid, scav3groupid, scav4groupid, seen1id, seenbase2id, seenbase3id, seenbase4id;
var scavstarted = false;
var artifactid;
var cheatmode = false;
var seenbase2 = false;
var seenbase3 = false;

function gameLost()
{
	gameOverMessage(false);
}

function eventCheatMode(entered)
{
	cheatmode = entered; // remember this setting
}

// proceed to next level
function gameWon()
{
	loadLevel("CAM_1B");
}

// listen to chat messages from player
function eventChat(from, to, message)
{
	if (message == "let me win" && cheatmode)
	{
		enableResearch("R-Wpn-MG-Damage01");
		enableResearch("R-Sys-Engineering01");
		enableResearch("R-Defense-Tower01");
		enableResearch("R-Wpn-Flamer01Mk1");
		var artifact = label("artifact");
		if (artifact)
		{
			removeObject(artifact);
		}
		queue(gameWon);
	}
}

// used to be that it went away when droids reached 4928, 6592 with radius 5 tiles
// but we have no nice way of doing that yet...
function removefirstobjective()
{
	hackRemoveMessage("C1A_OBJ1", PROX_MSG, 0);
}

// things that need checking every second
function tick()
{
	// check if we should launch the scavs yet
	if (!scavstarted)
	{
		var droids = enumArea("launchScavAttack");
		for (var i = 0; i < droids.length; i++)
		{
			if (droids[i].player == 0) // there is a human droid in this area
			{
				debug("-- LAUNCHING SCAV ATTACK");
				var spos = label("scav1soundpos");
				playSound("pcv375.ogg", spos.x, spos.y, 0);
				playSound("pcv456.ogg");
				hackAddMessage("MB1A_MSG", MISS_MSG, 0, true);
				hackAddMessage("C1A_OBJ1", PROX_MSG, 0, false);
				scavstarted = true;
				queue("removefirstobjective", 12000); // go away after a short while
				break;
			}
		}
		// send scavengers on war path if triggered above
		var startpos = label("playerBase");
		// FIXME, really should be only inside area 4416, 6336, 5440, 7104 that we send from
		for (var i = 0; scavstarted && i < droids.length; i++)
		{
			if ((droids[i].player == 7 || droids[i].player == 6) && droids[i].type == DROID)
			{
debug("  sending " + droids[i].name + " to attack player");
				orderDroidLoc(droids[i], DORDER_SCOUT, startpos.x, startpos.y);
			}
else debug("  NOT sending " +droids[i].name+ " to attack", droids[i].player, droids[i].type);
		}
	}
	// check if game is lost
	var factories = countStruct("A0LightFactory") + countStruct("A0CyborgFactory");
	var droids = countDroid(DROID_CONSTRUCT);
	if (droids == 0 && factories == 0)
	{
		queue("gameLost", 2000); // wait 2 secs before throwing the game
	}
	// check if game is won
	var hostiles = countStruct("A0BaBaFactory", 6) + countStruct("A0BaBaFactory", 7)
	               + countDroid(DROID_ANY, 6) + countDroid(DROID_ANY, 7);
	if (!hostiles && numArtifact > 4)
	{
		queue("gameWon", 4000); // wait 4 secs before giving it
	}
}

function showbase2()
{
	if (!seenbase2) // extra safety, world goes boom if message added twice
	{
debug("adding C1A_BASE1");
		seenbase2 = true;
		hackAddMessage("C1A_BASE1", PROX_MSG, 0, false);
		var spos = label("scav2soundpos");
		playSound("pcv374.ogg", spos.x, spos.y, 0);
	}
}

function showbase3()
{
	if (!seenbase3) // extra safety, world goes boom if message added twice
	{
		seenbase3 = true;
		hackAddMessage("C1A_BASE2", PROX_MSG, 0, false);
		var spos = label("scav3soundpos");
		playSound("pcv374.ogg", spos.x, spos.y, 0);
	}
}

function eventObjectSeen(viewer, seen)
{
	if (seen.id == seen1id)
	{
		hackAddMessage("C1A_BASE0", PROX_MSG, 0, false);
	}
	else if (seen.id == seenbase2id && !seenbase2)
	{
debug("seen base 2");
		queue("showbase2", 1000);
	}
	else if (seen.id == seenbase3id && !seenbase3)
	{
		queue("showbase3", 1000);
	}
	else if (seen.id == seenbase4id)
	{
debug("seen base 4");
		hackAddMessage("C1A_BASE3", PROX_MSG, 0, false);
		var spos = label("retreat4");
		playSound("pcv374.ogg", spos.x, spos.y, 0);
	}
}

function eventGroupLoss(obj, groupid, newsize)
{
	var leftovers;
	if (groupid == scav1groupid && newsize == 0)
	{
		// eliminated scav base 1
		leftovers = enumArea("scavbase1area");
		hackRemoveMessage("C1A_BASE0", PROX_MSG, 0);
		var spos = label("scav1soundpos");
		playSound("pcv391.ogg", spos.x, spos.y, 0);
	}
	else if (groupid == scav2groupid && newsize == 0)
	{
		// eliminated scav base 2
		debug("SCAV BASE 2 ELIMINATED!!");
		leftovers = enumArea("scavbase2area");
		hackRemoveMessage("C1A_BASE1", PROX_MSG, 0);
		var spos = label("scav2soundpos");
		playSound("pcv392.ogg", spos.x, spos.y, 0);
		if (!seenbase3)		// show next base, if not discovered yet
		{
			queue("showbase3", 2000);
		}
	}
	else if (groupid == scav3groupid && newsize == 0)
	{
		// eliminated scav base 3
		debug("SCAV BASE 3 ELIMINATED!!");
		leftovers = enumArea("scavbase3area");
		hackRemoveMessage("C1A_BASE2", PROX_MSG, 0);
		var spos = label("scav3soundpos");
		playSound("pcv392.ogg", spos.x, spos.y, 0);
	}
	else if (groupid == scav4groupid && newsize == 0)
	{
		// eliminated scav base 4
		debug("SCAV BASE 4 ELIMINATED!!");
		leftovers = enumArea("scavbase4area");
		hackRemoveMessage("C1A_BASE3", PROX_MSG, 0);
		var spos = label("retreat4");
		playSound("pcv392.ogg", spos.x, spos.y, 0);
	}
	// if scav group gone, nuke any leftovers, such as scav walls
	for (var i = 0; leftovers && i < leftovers.length; i++)
	{
		if ((leftovers[i].player == 6 || leftovers[i].player == 7) && leftovers[i].type == STRUCTURE)
		{
			removeObject(leftovers[i], true); // remove with special effect
		}
	}
}

function eventGameInit()
{
	var startpos = label("startPosition");
	var lz = label("landingZone");

	scav1groupid = label("scavgroup1").id;
	scav2groupid = label("scavgroup2").id;
	scav3groupid = label("scavgroup3").id;
	scav4groupid = label("scavgroup4").id;
	seen1id = label("seen1").id;
	seenbase2id = label("seenbase2").id;
	seenbase3id = label("seenbase3").id;
	seenbase4id = label("seenbase4").id;

	centreView(startpos.x, startpos.y);
	setNoGoArea(lz.x, lz.y, lz.x2, lz.y2, 0);
	setPower(1300);

	// allow to build stuff
	setStructureLimits("A0PowerGenerator", 5, 0);
	setStructureLimits("A0ResourceExtractor", 200, 0);
	setStructureLimits("A0ResearchFacility", 5, 0);
	setStructureLimits("A0LightFactory", 5, 0);
	setStructureLimits("A0CommandCentre", 1, 0);
	enableStructure("A0CommandCentre", 0);
	enableStructure("A0PowerGenerator", 0);
	enableStructure("A0ResourceExtractor", 0);
	enableStructure("A0ResearchFacility", 0);
	enableStructure("A0LightFactory", 0);

	makeComponentAvailable("MG1Mk1", me);	// needs to be done this way so doesn't enable rest of tree!
	completeResearch("R-Vehicle-Body01", me);
	completeResearch("R-Sys-Spade1Mk1", me);
	completeResearch("R-Vehicle-Prop-Wheels", me);

	// give player briefing
	hackAddMessage("CMB1_MSG", CAMP_MSG, 0, false);

	setReinforcementTime(-1);
	setMissionTime(-1); // was 36000

	var art4pos = label("artifact4pos");
	var artifact4 = addFeature("Crate", art4pos.x, art4pos.y);
	addLabel(artifact4, "artifact");
	numArtifact = 1;

	//groupAddArea(scavGroup, enemy1, 4416, 6336, 5440, 7104);
	//setGroupRetreatPoint(scavGroup, 4416, 5440);	//retreat to crossroads
	//setGroupRetreatForce(scavGroup, 99);			//set morale to 1%
	//setGroupRetreatLeadership(scavGroup, 10);

	setTimer("tick", 1000);
}

// Called when a human droid moves close to a crate. Remove it, then
// add another, unless we're done with pickup service for this level.
function eventPickup(feature, droid)
{
	if (feature.stattype != ARTIFACT)
	{
		return; // not interested!
	}
	playSound("pcv352.ogg", feature.x, feature.y, feature.z);
	removeObject(feature); // artifacts are not self-removing...
	if (numArtifact == 1) // first artifact
	{
		enableResearch("R-Wpn-MG-Damage01");
		var art1pos = label("artifact1pos");
		var artifact1 = addFeature("Crate", art1pos.x, art1pos.y);
		addLabel(artifact1, "artifact");
		numArtifact++;
	}
	else if (numArtifact == 2) // second artifact
	{
		enableResearch("R-Wpn-Flamer01Mk1");
		var art3pos = label("artifact3pos");
		var artifact3 = addFeature("Crate", art3pos.x, art3pos.y);
		addLabel(artifact3, "artifact");
		numArtifact++;
	}
	else if (numArtifact == 3) // third artifact
	{
		enableResearch("R-Defense-Tower01");
		var art2pos = label("artifact2pos");
		var artifact2 = addFeature("Crate", art2pos.x, art2pos.y);
		addLabel(artifact2, "artifact");
		numArtifact++;
	}
	else if (numArtifact == 4) // final artifact
	{
		enableResearch("R-Sys-Engineering01");
		numArtifact++;
	}
}

// /////////////////////////////////////////////////////////////////
// WARNING MESSAGES
// Base Under Attack
function eventAttacked(victimObj, attackerObj)
{
	if (gameTime > lastHitTime + 5000)
	{
		lastHitTime = gameTime;
		if (victimObj.type == STRUCTURE)
		{
			playSound("pcv337.ogg", victimObj.x, victimObj.y, victimObj.z);	// show position if still alive
		}
		else
		{
			playSound("pcv399.ogg", victimObj.x, victimObj.y, victimObj.z);
		}
	}
}
