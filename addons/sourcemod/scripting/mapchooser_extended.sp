/**
 * vim: set ts=4 :
 * =============================================================================
 * MapChooser Extended
 * Creates a map vote at appropriate times, setting sm_nextmap to the winning
 * vote.  Includes extra options not present in the SourceMod MapChooser
 *
 * MapChooser Extended (C)2011-2013 Powerlord (Ross Bemrose)
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

//#define DEBUG

#if defined DEBUG
	#define assert(%1) if (!(%1)) ThrowError("Debug Assertion Failed");
	#define assert_msg(%1,%2) if (!(%1)) ThrowError(%2);
#else
	#define assert(%1)
	#define assert_msg(%1,%2)
#endif

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <mapchooser>
#include <mapchooser_extended>
#include <nextmap>
#include <sdktools>
#include <multicolors>

#undef REQUIRE_PLUGIN
#tryinclude <DynamicChannels>
#tryinclude <nominations_extended>
#tryinclude <zleader>
#define REQUIRE_PLUGIN

#define MCE_VERSION "1.12.6"

#define ZLEADER "zleader"
#define DYNCHANNELS "DynamicChannels"

#include "mce/globals_variables.inc"
#include "mce/cvars.inc"
#include "mce/forwards.inc"
#include "mce/functions.inc"
#include "mce/commands.inc"
#include "mce/events.inc"
#include "mce/internal_functions.inc"
#include "mce/natives.inc"
#include "mce/menus.inc"

public Plugin myinfo =
{
	name = "MapChooser Extended",
	author = "Powerlord, Zuko, BotoX, maxime1907, Rushaway and AlliedModders LLC",
	description = "Automated Map Voting with Extensions",
	version = MCE_VERSION,
	url = "https://forums.alliedmods.net/showthread.php?t=156974"
};

public void OnPluginStart()
{
	LoadTranslations("mapchooser_extended.phrases");
	LoadTranslations("basevotes.phrases");
	LoadTranslations("common.phrases");

	EngineVersion version = GetEngineVersion();

	InitializeMapLists();
	CvarsInit();
	CvarsEngineInit(version);
	CommandsInit();
	EventsInit(version);
	ConfigureBonusRoundTime();
	InternalRestoreMapCooldowns();
}

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	if (LibraryExists("mapchooser"))
	{
		strcopy(error, err_max, "MapChooser already loaded, aborting.");
		return APLRes_Failure;
	}

	RegPluginLibrary("mapchooser");

	// Initializes all main natives and forwards.
	API_ForwardsInit();
	API_NativesInit();

	return APLRes_Success;
}

public void OnAllPluginsLoaded()
{
	g_ZLeader = LibraryExists(ZLEADER);
	g_DynamicChannels = LibraryExists(DYNCHANNELS);
}

public void OnLibraryAdded(const char[] name)
{
	if (strcmp(name, ZLEADER) == 0)
		g_ZLeader = true;
	if (strcmp(name, DYNCHANNELS) == 0)
		g_DynamicChannels = true;
}

public void OnLibraryRemoved(const char[] name)
{
	if (strcmp(name, ZLEADER) == 0)
		g_ZLeader = false;
	if (strcmp(name, DYNCHANNELS) == 0)
		g_DynamicChannels = false;
}

public void OnMapStart()
{
	InitializeGameModeSettings();
	InitializeGroupSettings();
}

public void OnConfigsExecuted()
{
	InitializeMapVoteSettings();
	InitializeOfficialMapList();
}

public void OnMapEnd()
{
	CleanOnMapEnd();
	InternalStoreMapCooldowns();
}

public void OnClientPutInServer(int client)
{
	CheckMapRestrictions(false, true);
}

public void OnClientDisconnect_Post(int client)
{
	CheckMapRestrictions(false, true);
}

public void OnClientDisconnect(int client)
{
	NominationsOnClientDisconnect(client);
}

public void OnMapTimeLeftChanged()
{
	if (GetArraySize(g_MapList))
		SetupTimeleftTimer();
}