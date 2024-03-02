/**
 * vim: set ts=4 :
 * =============================================================================
 * Nominations Extended
 * Allows players to nominate maps for Mapchooser
 *
 * Nominations Extended (C)2012-2013 Powerlord (Ross Bemrose)
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


#include <sourcemod>
#include <mapchooser>
#include <mapchooser_extended>
#include <multicolors>
#include <basecomm>

#pragma semicolon 1
#pragma newdecls required

#undef REQUIRE_PLUGIN
#tryinclude <sourcecomms>
#tryinclude <zleader>
#define REQUIRE_PLUGIN

#define NE_VERSION "1.11.0"

public Plugin myinfo =
{
	name = "Map Nominations Extended",
	author = "Powerlord and AlliedModders LLC",
	description = "Provides Map Nominations",
	version = NE_VERSION,
	url = "https://forums.alliedmods.net/showthread.php?t=156974"
};

Handle g_Cvar_ExcludeOld = INVALID_HANDLE;
Handle g_Cvar_ExcludeCurrent = INVALID_HANDLE;

Handle g_MapList = INVALID_HANDLE;
Handle g_AdminMapList = INVALID_HANDLE;
int g_mapFileSerial = -1;

Menu g_MapMenu;
Menu g_AdminMapMenu;
int g_AdminMapFileSerial = -1;

#define MAPSTATUS_ENABLED (1<<0)
#define MAPSTATUS_DISABLED (1<<1)
#define MAPSTATUS_EXCLUDE_CURRENT (1<<2)
#define MAPSTATUS_EXCLUDE_PREVIOUS (1<<3)
#define MAPSTATUS_EXCLUDE_NOMINATED (1<<4)

Handle g_mapTrie;

// Nominations Extended Convars
Handle g_Cvar_MarkCustomMaps = INVALID_HANDLE;
Handle g_Cvar_NominateDelay = INVALID_HANDLE;
ConVar g_Cvar_InitialDelay;

// VIP Nomination Convars
Handle g_Cvar_VIPTimeframe = INVALID_HANDLE;
Handle g_Cvar_VIPTimeframeMinTime = INVALID_HANDLE;
Handle g_Cvar_VIPTimeframeMaxTime = INVALID_HANDLE;
Handle g_hDelayNominate = INVALID_HANDLE;

// Forwards
Handle g_hOnPublicMapInsert = INVALID_HANDLE;
Handle g_hOnPublicMapReplaced = INVALID_HANDLE;
Handle g_hOnAdminMapInsert = INVALID_HANDLE;
Handle g_hOnAdminMapRemove = INVALID_HANDLE;

int g_Player_NominationDelay[MAXPLAYERS+1];
int g_NominationDelay;

bool g_bNEAllowed = false;		// True if Nominations is available to players.

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("nominations.phrases");
	LoadTranslations("basetriggers.phrases"); // for Next Map phrase
	LoadTranslations("mapchooser_extended.phrases");

	int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	g_MapList = CreateArray(arraySize);
	g_AdminMapList = CreateArray(arraySize);

	g_Cvar_ExcludeOld = CreateConVar("sm_nominate_excludeold", "1", "Specifies if the current map should be excluded from the Nominations list", 0, true, 0.00, true, 1.0);
	g_Cvar_ExcludeCurrent = CreateConVar("sm_nominate_excludecurrent", "1", "Specifies if the MapChooser excluded maps should also be excluded from Nominations", 0, true, 0.00, true, 1.0);
	g_Cvar_InitialDelay = CreateConVar("sm_nominate_initialdelay", "60.0", "Time in seconds before first Nomination can be made", 0, true, 0.00);
	g_Cvar_NominateDelay = CreateConVar("sm_nominate_delay", "3.0", "Delay between nominations", 0, true, 0.00, true, 60.00);

	g_Cvar_VIPTimeframe = CreateConVar("sm_nominate_vip_timeframe", "1", "Specifies if the should be a timeframe where only VIPs can nominate maps", 0, true, 0.00, true, 1.0);
	g_Cvar_VIPTimeframeMinTime = CreateConVar("sm_nominate_vip_timeframe_mintime", "1800", "Start of the timeframe where only VIPs can nominate maps (Format: HHMM)", 0, true, 0000.00, true, 2359.0);
	g_Cvar_VIPTimeframeMaxTime = CreateConVar("sm_nominate_vip_timeframe_maxtime", "2200", "End of the timeframe where only VIPs can nominate maps (Format: HHMM)", 0, true, 0000.00, true, 2359.0);

	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);

	RegConsoleCmd("sm_nom", Command_Nominate);
	RegConsoleCmd("sm_nominate", Command_Nominate);
	RegConsoleCmd("sm_nomlist", Command_NominateList);

	RegAdminCmd("sm_nominate_force_lock", Command_DisableNE, ADMFLAG_CONVARS, "sm_nominate_force_lock - Forces to lock nominations");
	RegAdminCmd("sm_nominate_force_unlock", Command_EnableNE, ADMFLAG_CONVARS, "sm_nominate_force_unlock - Forces to unlock nominations");

	RegAdminCmd("sm_nominate_addmap", Command_Addmap, ADMFLAG_CHANGEMAP, "sm_nominate_addmap <mapname> - Forces a map to be on the next mapvote.");
	RegAdminCmd("sm_nominate_removemap", Command_Removemap, ADMFLAG_CHANGEMAP, "sm_nominate_removemap <mapname> - Removes a map from Nominations.");

	RegAdminCmd("sm_nominate_exclude", Command_AddExclude, ADMFLAG_CHANGEMAP, "sm_nominate_exclude <mapname> [cooldown] [mode]- Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.");
	RegAdminCmd("sm_nominate_exclude_time", Command_AddExcludeTime, ADMFLAG_CHANGEMAP, "sm_nominate_exclude_time <mapname> [cooldown] [mode] - Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.");

	// Nominations Extended cvars
	CreateConVar("ne_version", NE_VERSION, "Nominations Extended Version", FCVAR_SPONLY|FCVAR_NOTIFY|FCVAR_DONTRECORD);

	AutoExecConfig(true, "nominations_extended");

	g_mapTrie = CreateTrie();
}

public APLRes AskPluginLoad2(Handle hThis, bool bLate, char[] err, int iErrLen)
{
	RegPluginLibrary("nominations");

	CreateNative("GetNominationPool", Native_GetNominationPool);
	CreateNative("PushMapIntoNominationPool", Native_PushMapIntoNominationPool);
	CreateNative("PushMapsIntoNominationPool", Native_PushMapsIntoNominationPool);
	CreateNative("RemoveMapFromNominationPool", Native_RemoveMapFromNominationPool);
	CreateNative("RemoveMapsFromNominationPool", Native_RemoveMapsFromNominationPool);
	CreateNative("ToggleNominations", Native_ToggleNominations);

	g_hOnPublicMapInsert = CreateGlobalForward("NE_OnPublicMapInsert", ET_Ignore, Param_Cell, Param_String, Param_Cell, Param_Cell);
	g_hOnPublicMapReplaced = CreateGlobalForward("NE_OnPublicMapReplaced", ET_Ignore, Param_Cell, Param_String, Param_Cell, Param_Cell);
	g_hOnAdminMapInsert = CreateGlobalForward("NE_OnAdminMapInsert", ET_Ignore, Param_Cell, Param_String);
	g_hOnAdminMapRemove = CreateGlobalForward("NE_OnAdminMapRemove", ET_Ignore, Param_Cell, Param_String);

	return APLRes_Success;
}

public void OnAllPluginsLoaded()
{
	// This is an MCE cvar... this plugin requires MCE to be loaded.  Granted, this plugin SHOULD have an MCE dependency.
	g_Cvar_MarkCustomMaps = FindConVar("mce_markcustommaps");
}

public void OnMapEnd()
{
	g_hDelayNominate = INVALID_HANDLE;
	g_bNEAllowed = false;
}

public void OnConfigsExecuted()
{
	if(ReadMapList(g_MapList,
					g_mapFileSerial,
					"nominations",
					MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER)
		== INVALID_HANDLE)
	{
		if(g_mapFileSerial == -1)
		{
			SetFailState("Unable to create a valid map list.");
		}
	}

	if(ReadMapList(g_AdminMapList,
					g_AdminMapFileSerial,
					"sm_nominate_addmap menu",
					MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT|MAPLIST_FLAG_MAPSFOLDER)
		== INVALID_HANDLE)
	{
		if(g_AdminMapFileSerial == -1)
		{
			SetFailState("Unable to create a valid admin map list.");
		}
	}
	else
	{
		for(int i = 0; i < GetArraySize(g_MapList); i++)
		{
			static char map[PLATFORM_MAX_PATH];
			GetArrayString(g_MapList, i, map, sizeof(map));

			int Index = FindStringInArray(g_AdminMapList, map);
			if(Index != -1)
				RemoveFromArray(g_AdminMapList, Index);
		}
	}

	g_bNEAllowed = false;
	if (g_hDelayNominate != INVALID_HANDLE)
  		delete g_hDelayNominate;

	g_hDelayNominate = CreateTimer(g_Cvar_InitialDelay.FloatValue, Timer_DelayNominate, _, TIMER_FLAG_NO_MAPCHANGE);

	UpdateMapTrie();
	UpdateMapMenus();
}

void UpdateMapMenus()
{
	if(g_MapMenu != INVALID_HANDLE)
		delete g_MapMenu;

	g_MapMenu = BuildMapMenu("");

	if(g_AdminMapMenu != INVALID_HANDLE)
		delete g_AdminMapMenu;

	g_AdminMapMenu = BuildAdminMapMenu("");
}

void UpdateMapTrie()
{
	static char map[PLATFORM_MAX_PATH];
	static char currentMap[PLATFORM_MAX_PATH];
	ArrayList excludeMaps;

	if(GetConVarBool(g_Cvar_ExcludeOld))
	{
		excludeMaps = CreateArray(ByteCountToCells(PLATFORM_MAX_PATH));
		GetExcludeMapList(excludeMaps);
	}

	if(GetConVarBool(g_Cvar_ExcludeCurrent))
		GetCurrentMap(currentMap, sizeof(currentMap));

	ClearTrie(g_mapTrie);

	for(int i = 0; i < GetArraySize(g_MapList); i++)
	{
		int status = MAPSTATUS_ENABLED;

		GetArrayString(g_MapList, i, map, sizeof(map));

		if(GetConVarBool(g_Cvar_ExcludeCurrent))
		{
			if(StrEqual(map, currentMap))
				status = MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_CURRENT;
		}

		/* Dont bother with this check if the current map check passed */
		if(GetConVarBool(g_Cvar_ExcludeOld) && status == MAPSTATUS_ENABLED)
		{
			if(FindStringInArray(excludeMaps, map) != -1)
				status = MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_PREVIOUS;
		}

		SetTrieValue(g_mapTrie, map, status);
	}

	if(excludeMaps)
		delete excludeMaps;
}

public void OnNominationRemoved(const char[] map, int owner)
{
	int status;

	/* Is the map in our list? */
	if(!GetTrieValue(g_mapTrie, map, status))
		return;

	/* Was the map disabled due to being nominated */
	if((status & MAPSTATUS_EXCLUDE_NOMINATED) != MAPSTATUS_EXCLUDE_NOMINATED)
		return;

	SetTrieValue(g_mapTrie, map, MAPSTATUS_ENABLED);
}

public Action Command_Addmap(int client, int args)
{
	if(args == 0)
	{
		AttemptAdminNominate(client);
		return Plugin_Handled;
	}

	if(args != 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_addmap <mapname>");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int status;
	if(!GetTrieValue(g_mapTrie, mapname, status))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map Not In Pool", mapname);
		AttemptAdminNominate(client, mapname);
		return Plugin_Handled;
	}

	if(!CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_CHEATS, true))
	{
		bool restrictionsActive = AreRestrictionsActive();

		if(GetTrieValue(g_mapTrie, mapname, status))
		{
			if((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
			{
				if((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
					CPrintToChat(client, "{green}[NE]{default} %t", "Can't Nominate Current Map");

				if(restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
				{
					int Cooldown = GetMapCooldown(mapname);
					CPrintToChat(client, "{green}[NE]{default} %t (%d)", "Map in Exclude List", Cooldown);
				}

				if((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
					CPrintToChat(client, "{green}[NE]{default} %t", "Map Already Nominated");

				return Plugin_Handled;
			}
		}

		int Cooldown = GetMapCooldownTime(mapname);
		if(restrictionsActive && Cooldown > GetTime())
		{
			int Seconds = Cooldown - GetTime();
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Cooldown Time Error", Seconds / 3600, (Seconds % 3600) / 60);

			return Plugin_Handled;
		}

		int TimeRestriction = GetMapTimeRestriction(mapname);
		if(restrictionsActive && TimeRestriction)
		{
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Time Error", TimeRestriction / 60, TimeRestriction % 60);

			return Plugin_Handled;
		}

		int PlayerRestriction = GetMapPlayerRestriction(mapname);
		if(restrictionsActive && PlayerRestriction)
		{
			if(PlayerRestriction < 0)
				CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MinPlayers Error", PlayerRestriction * -1);
			else
				CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MaxPlayers Error", PlayerRestriction);

			return Plugin_Handled;
		}

		int GroupRestriction = GetMapGroupRestriction(mapname);
		if(restrictionsActive && GroupRestriction >= 0)
		{
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Group Error", GroupRestriction);
			return Plugin_Handled;
		}
	}

	NominateResult result = NominateMap(mapname, true, 0);

	if(result > Nominate_InvalidMap)
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map was not found", mapname);
		return Plugin_Handled;
	}

	if(result > Nominate_Replaced)
	{
		/* We assume already in vote is the casue because the maplist does a Map Validity check and we forced, so it can't be full */
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map Already In Vote", mapname);

		return Plugin_Handled;
	}

	/* Map was nominated! - Disable the menu item and update the trie */
	SetTrieValue(g_mapTrie, mapname, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);

	CReplyToCommand(client, "{green}[NE]{default} %t", "Map Inserted", mapname);
	CPrintToChatAll("{green}[NE]{default} %N has inserted %s into nominations", client, mapname);

	LogAction(client, -1, "\"%L\" has inserted map \"%s\".", client, mapname);
	Forward_OnAdminMapInsert(client, mapname);
	return Plugin_Handled;
}

public Action Command_Removemap(int client, int args)
{
	if(args == 0 && client > 0)
	{
		AttemptAdminRemoveMap(client);
		return Plugin_Handled;
	}

	if(args != 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_removemap <mapname>");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int status;
	if(!GetTrieValue(g_mapTrie, mapname, status))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map was not found", mapname);
		AttemptAdminRemoveMap(client, mapname);
		return Plugin_Handled;
	}

	if(!RemoveNominationByMap(mapname))
	{
		CReplyToCommand(client, "{green}[NE]{default} This map isn't nominated.", mapname);

		return Plugin_Handled;
	}

	CReplyToCommand(client, "{green}[NE]{default} Map '%s' removed from the nominations list.", mapname);
	LogAction(client, -1, "\"%L\" has removed map \"%s\" from nominations.", client, mapname);
	Forward_OnAdminMapRemove(client, mapname);

	CPrintToChatAll("{green}[NE]{default} %N has removed %s from nominations", client, mapname);

	return Plugin_Handled;
}

public Action Command_AddExclude(int client, int args)
{
	if(args < 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_exclude <mapname> [cooldown] [mode]");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int cooldown = 0;
	int mode = 0;
	if(args >= 2)
	{
		static char buffer[8];
		GetCmdArg(2, buffer, sizeof(buffer));
		cooldown = StringToInt(buffer);
	}
	if(args >= 3)
	{
		static char buffer[8];
		GetCmdArg(3, buffer, sizeof(buffer));
		mode = StringToInt(buffer);
	}

	int status;
	if(!GetTrieValue(g_mapTrie, mapname, status))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map was not found", mapname);
		return Plugin_Handled;
	}

	CShowActivity(client, "Excluded map \"%s\" from nomination", mapname);
	LogAction(client, -1, "\"%L\" excluded map \"%s\" from nomination", client, mapname);

	SetTrieValue(g_mapTrie, mapname, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_PREVIOUS);

	// native call to mapchooser_extended
	ExcludeMap(mapname, cooldown, mode);

	return Plugin_Handled;
}

public Action Command_AddExcludeTime(int client, int args)
{
	if(args < 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_exclude_time <mapname> [cooldown] [mode]");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int cooldown = 0;
	int mode = 0;
	if(args >= 2)
	{
		static char buffer[16];
		GetCmdArg(2, buffer, sizeof(buffer));
		cooldown = TimeStrToSeconds(buffer);
	}
	if(args >= 3)
	{
		static char buffer[8];
		GetCmdArg(3, buffer, sizeof(buffer));
		mode = StringToInt(buffer);
	}

	int status;
	if(!GetTrieValue(g_mapTrie, mapname, status))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map was not found", mapname);
		return Plugin_Handled;
	}

	CShowActivity(client, "ExcludedTime map \"%s\" from nomination", mapname);
	LogAction(client, -1, "\"%L\" excludedTime map \"%s\" from nomination", client, mapname);

	// native call to mapchooser_extended
	ExcludeMapTime(mapname, cooldown, mode);

	return Plugin_Handled;
}

public Action Timer_DelayNominate(Handle timer)
{
	if (!g_bNEAllowed)
		CPrintToChatAll("{green}[NE]{default} Map nominations are available now!");

	g_bNEAllowed = true;
	g_NominationDelay = 0;

	return Plugin_Stop;
}

public Action Command_DisableNE(int client, int args)
{
	if (!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations are already restricted.");
		return Plugin_Handled;
	}

	g_bNEAllowed = false;
	CPrintToChatAll("{green}[NE]{default} Map nominations are restricted.");
	return Plugin_Handled;
}

public Action Command_EnableNE(int client, int args)
{
	if (g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations are already available.");
		return Plugin_Handled;
	}

	g_bNEAllowed = true;
	g_NominationDelay = 0;
	CPrintToChatAll("{green}[NE]{default} Map nominations are available now!");
	return Plugin_Handled;
}

public Action Command_Say(int client, int args)
{
	if(!client)
		return Plugin_Continue;

	static char text[192];
	if(!GetCmdArgString(text, sizeof(text)))
		return Plugin_Continue;

	int startidx = 0;
	if(text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}

	ReplySource old = SetCmdReplySource(SM_REPLY_TO_CHAT);

	if(strcmp(text[startidx], "nominate", false) == 0)
	{
		if(IsNominateAllowed(client))
		{
			if(g_NominationDelay > GetTime())
				CReplyToCommand(client, "{green}[NE]{default} Nominations will be unlocked in %d seconds.", g_NominationDelay - GetTime());
			if(!g_bNEAllowed)
			{
				CReplyToCommand(client, "{green}[NE]{default} Map nominations are currently locked.");
				return Plugin_Handled;
			}
			else
				AttemptNominate(client);
		}
	}

	SetCmdReplySource(old);

	return Plugin_Continue;
}

public Action Command_Nominate(int client, int args)
{
	if(!client || !IsNominateAllowed(client))
		return Plugin_Handled;

	if(g_NominationDelay > GetTime())
	{
		CPrintToChat(client, "{green}[NE]{default} Nominations will be unlocked in %d seconds.", g_NominationDelay - GetTime());
		return Plugin_Handled;
	}

	if(!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map Nominations are currently locked.");
		return Plugin_Handled;
	}
	
	if(args == 0)
	{
		AttemptNominate(client);
		return Plugin_Handled;
	}

	if(g_Player_NominationDelay[client] > GetTime())
	{
		CPrintToChat(client, "{green}[NE]{default} Please wait %d seconds before you can nominate again", g_Player_NominationDelay[client] - GetTime());
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));


	int status;
	if(!GetTrieValue(g_mapTrie, mapname, status))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Not In Pool", mapname);
		AttemptNominate(client, mapname);
		return Plugin_Handled;
	}

	bool restrictionsActive = AreRestrictionsActive();

	if((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
	{
		if((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
			CPrintToChat(client, "{green}[NE]{default} %t", "Can't Nominate Current Map");

		if(restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
		{
			int Cooldown = GetMapCooldown(mapname);
			CPrintToChat(client, "{green}[NE]{default} %t (%d)", "Map in Exclude List", Cooldown);
		}

		if((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Already Nominated");

		return Plugin_Handled;
	}

	int Cooldown = GetMapCooldownTime(mapname);
	if(restrictionsActive && Cooldown > GetTime())
	{
		int Seconds = Cooldown - GetTime();
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Cooldown Time Error", Seconds / 3600, (Seconds % 3600) / 60);

		return Plugin_Handled;
	}

	bool adminRestriction = IsClientMapAdminRestricted(mapname, client);
	if(restrictionsActive && adminRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Admin Error");

		return Plugin_Handled;
	}

	bool VIPRestriction = IsClientMapVIPRestricted(mapname, client);
	if(restrictionsActive && VIPRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate VIP Error");

		return Plugin_Handled;
	}

	#if defined _zleader_included
	bool LeaderRestriction = IsClientMapLeaderRestricted(mapname, client);
	if(restrictionsActive && LeaderRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Leader Error");

		return Plugin_Handled;
	}
	#endif

	int TimeRestriction = GetMapTimeRestriction(mapname);
	if(restrictionsActive && TimeRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Time Error", TimeRestriction / 60, TimeRestriction % 60);

		return Plugin_Handled;
	}

	int PlayerRestriction = GetMapPlayerRestriction(mapname);
	if(restrictionsActive && PlayerRestriction)
	{
		if(PlayerRestriction < 0)
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MinPlayers Error", PlayerRestriction * -1);
		else
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MaxPlayers Error", PlayerRestriction);

		return Plugin_Handled;
	}

	int GroupRestriction = GetMapGroupRestriction(mapname, client);
	if(restrictionsActive && GroupRestriction >= 0)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Group Error", GroupRestriction);
		return Plugin_Handled;
	}

	NominateResult result = NominateMap(mapname, false, client);

	if(result > Nominate_Replaced)
	{
		if(result == Nominate_AlreadyInVote)
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Already In Vote", mapname);
		else if(result == Nominate_VoteFull)
			CPrintToChat(client, "{green}[NE]{default} %t", "Max Nominations");

		return Plugin_Handled;
	}

	/* Map was nominated! - Disable the menu item and update the trie */
	SetTrieValue(g_mapTrie, mapname, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);
	g_Player_NominationDelay[client] = GetTime() + GetConVarInt(g_Cvar_NominateDelay);

	static char name[MAX_NAME_LENGTH];
	GetClientName(client, name, sizeof(name));

	if(result == Nominate_Added)
	{
		CPrintToChatAll("{green}[NE]{default} %t", "Map Nominated", name, mapname);
		LogMessage("%L has nominated %s", client, mapname);
		Forward_OnPublicMapInsert(client, mapname, IsMapVIPRestricted(mapname), IsMapLeaderRestricted(mapname));
	}
	else if(result == Nominate_Replaced)
	{
		CPrintToChatAll("{green}[NE]{default} %t", "Map Nomination Changed", name, mapname);
		LogMessage("%L has changed their nomination to %s", client, mapname);
		Forward_OnPublicMapReplaced(client, mapname, IsMapVIPRestricted(mapname), IsMapLeaderRestricted(mapname));
	}

	return Plugin_Continue;
}

public Action Command_NominateList(int client, int args)
{
	if (client == 0)
	{
		int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
		ArrayList MapList = CreateArray(arraySize);
		GetNominatedMapList(MapList);

		char aBuf[2048];
		StrCat(aBuf, sizeof(aBuf), "{green}[NE]{default} Nominated Maps:");
		static char map[PLATFORM_MAX_PATH];
		for(int i = 0; i < GetArraySize(MapList); i++)
		{
			StrCat(aBuf, sizeof(aBuf), "\n");
			GetArrayString(MapList, i, map, sizeof(map));
			StrCat(aBuf, sizeof(aBuf), map);
		}

		CReplyToCommand(client, aBuf);
		delete MapList;
		return Plugin_Handled;
	}

	Menu NominateListMenu = CreateMenu(Handler_NominateListMenu, MENU_ACTIONS_DEFAULT|MenuAction_DisplayItem);

	if(!PopulateNominateListMenu(NominateListMenu, client))
	{
		CReplyToCommand(client, "{green}[NE]{default} No maps have been nominated.");
		return Plugin_Handled;
	}

	SetMenuTitle(NominateListMenu, "Nominated Maps", client);
	DisplayMenu(NominateListMenu, client, MENU_TIME_FOREVER);

	return Plugin_Handled;
}

public int Handler_NominateListMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
}

Action AttemptNominate(int client, const char[] filter = "")
{
	#if defined _sourcecomms_included
		if (client)
		{
			int IsGagged = SourceComms_GetClientGagType(client);
			if(IsGagged > 0)
			{
				CReplyToCommand(client, "{green}[NE]{default} You are not allowed to nominate maps while you are gagged.");
				return Plugin_Handled;
			}
		}
	#else
		if (BaseComm_IsClientGagged(client))
		{
			CReplyToCommand(client, "{green}[NE]{default} You are not allowed to nominate maps while you are gagged.");
			return Plugin_Handled;
		}
	#endif

	if (!client)
	{
		ReplyToCommand(client, "[SM] Cannot use this command from server console.");
		return Plugin_Handled;
	}
	
	if(!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations is currently locked.");
		return Plugin_Handled;
	}

	Menu menu = g_MapMenu;
	if(filter[0])
		menu = BuildMapMenu(filter);

	SetMenuTitle(menu, "%T", "Nominate Title", client);
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
	return Plugin_Handled;
}

Action AttemptAdminNominate(int client, const char[] filter = "")
{
	#if defined _sourcecomms_included
		if (client)
		{
			int IsGagged = SourceComms_GetClientGagType(client);
			if(IsGagged > 0)
			{
				CReplyToCommand(client, "{green}[NE]{default} You are not allowed to nominate maps while you are gagged.");
				return Plugin_Handled;
			}
		}
	#else
		if (BaseComm_IsClientGagged(client))
		{
			CReplyToCommand(client, "{green}[NE]{default} You are not allowed to nominate maps while you are gagged.");
			return Plugin_Handled;
		}
	#endif

	if(!client)
		return Plugin_Handled;

	if(!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations is currently locked.");
		return Plugin_Handled;
	}

	Menu menu = g_AdminMapMenu;
	if(filter[0])
		menu = BuildAdminMapMenu(filter);

	SetMenuTitle(menu, "%T", "Nominate Title", client);
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
	return Plugin_Handled;
}

void AttemptAdminRemoveMap(int client, const char[] filter = "")
{
	if(!client)
		return;

	Menu AdminRemoveMapMenu = CreateMenu(Handler_AdminRemoveMapMenu, MENU_ACTIONS_DEFAULT|MenuAction_DisplayItem);

	if(!PopulateNominateListMenu(AdminRemoveMapMenu, client, filter))
	{
		CReplyToCommand(client, "{green}[NE]{default} No maps have been nominated.");
		return;
	}

	SetMenuTitle(AdminRemoveMapMenu, "Remove nomination", client);
	DisplayMenu(AdminRemoveMapMenu, client, MENU_TIME_FOREVER);

}

bool PopulateNominateListMenu(Menu menu, int client, const char[] filter = "")
{
	int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	ArrayList MapList = CreateArray(arraySize);
	ArrayList OwnerList = CreateArray();

	GetNominatedMapList(MapList, OwnerList);
	if(!GetArraySize(MapList))
	{
		delete MapList;
		delete OwnerList;
		return false;
	}

	bool restrictionsActive = AreRestrictionsActive();

	static char map[PLATFORM_MAX_PATH];
	static char display[PLATFORM_MAX_PATH];
	for(int i = 0; i < GetArraySize(MapList); i++)
	{
		GetArrayString(MapList, i, map, sizeof(map));

		if(!filter[0] || StrContains(map, filter, false) != -1)
		{
			strcopy(display, sizeof(display), map);

			bool adminRestriction = IsClientMapAdminRestricted(map);
			if((adminRestriction) && restrictionsActive)
				Format(display, sizeof(display), "%s (%T)", display, "Admin Nomination", client);

			bool VIPRestriction = IsClientMapVIPRestricted(map);
			if((VIPRestriction) && restrictionsActive)
				Format(display, sizeof(display), "%s (%T)", display, "VIP Nomination", client);

			#if defined _zleader_included
			bool LeaderRestriction = IsClientMapLeaderRestricted(map);
			if((LeaderRestriction) && restrictionsActive)
				Format(display, sizeof(display), "%s (%T)", display, "Leader Nomination", client);
			#endif

			int owner = GetArrayCell(OwnerList, i);
			if(!owner)
				Format(display, sizeof(display), "%s (%T)", display, "Nominated by Admin", client);
			else
				Format(display, sizeof(display), "%s (%N)", display, owner);

			AddMenuItem(menu, map, display);
		}
	}

	delete MapList;
	delete OwnerList;
	return true;
}

Menu BuildMapMenu(const char[] filter)
{
	Menu menu = CreateMenu(Handler_MapSelectMenu, MENU_ACTIONS_DEFAULT|MenuAction_DrawItem|MenuAction_DisplayItem);

	static char map[PLATFORM_MAX_PATH];

	for(int i = 0; i < GetArraySize(g_MapList); i++)
	{
		GetArrayString(g_MapList, i, map, sizeof(map));

		if(!filter[0] || StrContains(map, filter, false) != -1)
		{
			AddMenuItem(menu, map, map);
		}
	}

	SetMenuExitButton(menu, true);

	return menu;
}

Menu BuildAdminMapMenu(const char[] filter)
{
	Menu menu = CreateMenu(Handler_AdminMapSelectMenu, MENU_ACTIONS_DEFAULT|MenuAction_DrawItem|MenuAction_DisplayItem);

	static char map[PLATFORM_MAX_PATH];

	for(int i = 0; i < GetArraySize(g_AdminMapList); i++)
	{
		GetArrayString(g_AdminMapList, i, map, sizeof(map));

		if(!filter[0] || StrContains(map, filter, false) != -1)
			AddMenuItem(menu, map, map);
	}

	if(filter[0])
	{
		// Search normal maps aswell if filter is specified
		for(int i = 0; i < GetArraySize(g_MapList); i++)
		{
			GetArrayString(g_MapList, i, map, sizeof(map));

			if(!filter[0] || StrContains(map, filter, false) != -1)
				AddMenuItem(menu, map, map);
		}
	}

	SetMenuExitButton(menu, true);

	return menu;
}

public int Handler_MapSelectMenu(Menu menu, MenuAction action, int param1, int param2)
{
	bool restrictionsActive = AreRestrictionsActive();

	switch(action)
	{
		case MenuAction_End:
		{
			if(menu != g_MapMenu)
				delete menu;
		}
		case MenuAction_Select:
		{
			if(!g_bNEAllowed)
			{
				CPrintToChat(param1, "{green}[NE]{default} Map Nominations is currently locked.");
				return 0;
			}
	
			if(g_Player_NominationDelay[param1] > GetTime())
			{
				CPrintToChat(param1, "{green}[NE]{default} Please wait %d seconds before you can nominate again", g_Player_NominationDelay[param1] - GetTime());
				DisplayMenuAtItem(menu, param1, GetMenuSelectionPosition(), MENU_TIME_FOREVER);
				return 0;
			}

			static char map[PLATFORM_MAX_PATH];
			char name[MAX_NAME_LENGTH];
			GetMenuItem(menu, param2, map, sizeof(map));

			GetClientName(param1, name, MAX_NAME_LENGTH);

			if(restrictionsActive && (
				GetMapCooldownTime(map) > GetTime() ||
				GetMapTimeRestriction(map) ||
				GetMapPlayerRestriction(map) ||
				GetMapGroupRestriction(map, param1) >= 0 ||
				IsClientMapAdminRestricted(map, param1) ||
				IsClientMapVIPRestricted(map, param1)
				#if defined _zleader_included
				|| IsClientMapLeaderRestricted(map, param1)
				#endif
				))
			{
				CPrintToChat(param1, "{green}[NE]{default} You can't nominate this map right now.");
				return 0;
			}

			NominateResult result = NominateMap(map, false, param1);

			/* Don't need to check for InvalidMap because the menu did that already */
			if(result == Nominate_AlreadyInVote)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "Map Already Nominated");
				return 0;
			}
			else if(result == Nominate_VoteFull)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "Max Nominations");
				return 0;
			}

			/* Map was nominated! - Disable the menu item and update the trie */
			SetTrieValue(g_mapTrie, map, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);
			g_Player_NominationDelay[param1] = GetTime() + GetConVarInt(g_Cvar_NominateDelay);

			if(result == Nominate_Added)
			{
				CPrintToChatAll("{green}[NE]{default} %t", "Map Nominated", name, map);
				LogMessage("%L has nominated %s", param1, map);
				Forward_OnPublicMapInsert(param1, map, IsMapVIPRestricted(map), IsMapLeaderRestricted(map));
			}
			else if(result == Nominate_Replaced)
			{
				CPrintToChatAll("{green}[NE]{default} %t", "Map Nomination Changed", name, map);
				LogMessage("%L has changed their nomination to %s", param1, map);
				Forward_OnPublicMapReplaced(param1, map, IsMapVIPRestricted(map), IsMapLeaderRestricted(map));
			}
		}

		case MenuAction_DrawItem:
		{
			static char map[PLATFORM_MAX_PATH];
			GetMenuItem(menu, param2, map, sizeof(map));

			int status;
			if(GetTrieValue(g_mapTrie, map, status))
			{
				if((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
				{
					if((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
					{
						return ITEMDRAW_DISABLED;
					}

					if(restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
					{
						return ITEMDRAW_DISABLED;
					}

					if((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
					{
						return ITEMDRAW_DISABLED;
					}
				}
			}

			if(restrictionsActive && (
				GetMapCooldownTime(map) > GetTime() ||
				GetMapTimeRestriction(map) ||
				GetMapPlayerRestriction(map) ||
				GetMapGroupRestriction(map, param1) >= 0 ||
				IsClientMapAdminRestricted(map, param1) ||
				IsClientMapVIPRestricted(map, param1)
				#if defined _zleader_included
				|| IsClientMapLeaderRestricted(map, param1)
				#endif
				))
			{
				return ITEMDRAW_DISABLED;
			}

			return ITEMDRAW_DEFAULT;
		}

		case MenuAction_DisplayItem:
		{
			static char map[PLATFORM_MAX_PATH];
			GetMenuItem(menu, param2, map, sizeof(map));

			int mark = GetConVarInt(g_Cvar_MarkCustomMaps);
			bool official;

			static char buffer[100];
			static char display[150];

			if(mark)
				official = IsMapOfficial(map);

			if(mark && !official)
			{
				switch(mark)
				{
					case 1:
					{
						Format(buffer, sizeof(buffer), "%T", "Custom Marked", param1, map);
					}

					case 2:
					{
						Format(buffer, sizeof(buffer), "%T", "Custom", param1, map);
					}
				}
			}
			else
				strcopy(buffer, sizeof(buffer), map);

			bool adminRestriction = IsClientMapAdminRestricted(map);
			if(restrictionsActive && adminRestriction)
			{
				Format(buffer, sizeof(buffer), "%s (%T)", buffer, "Admin Restriction", param1);
			}

			bool VIPRestriction = IsClientMapVIPRestricted(map);
			if(restrictionsActive && VIPRestriction)
			{
				Format(buffer, sizeof(buffer), "%s (%T)", buffer, "VIP Restriction", param1);
			}

			#if defined _zleader_included
			bool LeaderRestriction = IsClientMapLeaderRestricted(map);
			if(restrictionsActive && LeaderRestriction)
			{
				Format(buffer, sizeof(buffer), "%s (%T)", buffer, "Leader Restriction", param1);
			}
			#endif

			int status;
			if(GetTrieValue(g_mapTrie, map, status))
			{
				if((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
				{
					if((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
					{
						Format(display, sizeof(display), "%s (%T)", buffer, "Current Map", param1);
						return RedrawMenuItem(display);
					}

					if(restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
					{
						int Cooldown = GetMapCooldown(map);
						Format(display, sizeof(display), "%s (%T %d)", buffer, "Recently Played", param1, Cooldown);
						return RedrawMenuItem(display);
					}

					if((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
					{
						Format(display, sizeof(display), "%s (%T)", buffer, "Nominated", param1);
						return RedrawMenuItem(display);
					}
				}
			}

			int Cooldown = GetMapCooldownTime(map);
			if(restrictionsActive && Cooldown > GetTime())
			{
				int Seconds = Cooldown - GetTime();
				char time[16];
				CustomFormatTime(Seconds, time, sizeof(time));
				Format(display, sizeof(display), "%s (%T %s)", buffer, "Recently Played", param1, time);
				return RedrawMenuItem(display);
			}

			int TimeRestriction = GetMapTimeRestriction(map);
			if(restrictionsActive && TimeRestriction)
			{
				Format(display, sizeof(display), "%s (%T)", buffer, "Map Time Restriction", param1, "+", TimeRestriction / 60, TimeRestriction % 60);
				return RedrawMenuItem(display);
			}

			int PlayerRestriction = GetMapPlayerRestriction(map);
			if(restrictionsActive && PlayerRestriction)
			{
				if(PlayerRestriction < 0)
					Format(display, sizeof(display), "%s (%T)", buffer, "Map Player Restriction", param1, "+", PlayerRestriction * -1);
				else
					Format(display, sizeof(display), "%s (%T)", buffer, "Map Player Restriction", param1, "-", PlayerRestriction);

				return RedrawMenuItem(display);
			}

			int GroupRestriction = GetMapGroupRestriction(map, param1);
			if(restrictionsActive && GroupRestriction >= 0)
			{
				Format(display, sizeof(display), "%s (%T)", buffer, "Map Group Restriction", param1, GroupRestriction);
				return RedrawMenuItem(display);
			}

			if(restrictionsActive && adminRestriction)
			{
				return RedrawMenuItem(buffer);
			}

			if(restrictionsActive && VIPRestriction)
			{
				return RedrawMenuItem(buffer);
			}

			#if defined _zleader_included
			if(restrictionsActive && LeaderRestriction)
			{
				return RedrawMenuItem(buffer);
			}
			#endif

			if(mark && !official)
				return RedrawMenuItem(buffer);

			return 0;
		}
	}

	return 0;
}

stock bool IsNominateAllowed(int client)
{
	#if defined _sourcecomms_included
		if (client)
		{
			int IsGagged = SourceComms_GetClientGagType(client);
			if(IsGagged > 0)
			{
				CReplyToCommand(client, "{green}[NE]{default} You are not allowed to nominate maps while you are gagged.");
				return false;
			}
		}
	#else
		if (BaseComm_IsClientGagged(client))
		{
			CReplyToCommand(client, "{green}[NE]{default} You are not allowed to nominate maps while you are gagged.");
			return false;
		}
	#endif

	if (!CheckCommandAccess(client, "sm_tag", ADMFLAG_CUSTOM1))
	{
		int VIPTimeRestriction = GetVIPTimeRestriction();
		if((VIPTimeRestriction) && AreRestrictionsActive())
		{
			CReplyToCommand(client, "{green}[NE]{default} During peak hours only VIPs are allowed to nominate maps. Wait for %d hours and %d minutes or buy VIP to nominate maps again.", VIPTimeRestriction / 60, VIPTimeRestriction % 60);
			return false;
		}
	}

	CanNominateResult result = CanNominate();

	switch(result)
	{
		case CanNominate_No_VoteInProgress:
		{
			CReplyToCommand(client, "{green}[NE]{default} %t", "Nextmap Voting Started");
			return false;
		}

		case CanNominate_No_VoteComplete:
		{
			char map[PLATFORM_MAX_PATH];
			GetNextMap(map, sizeof(map));
			CReplyToCommand(client, "{green}[NE]{default} %t", "Next Map", map);
			return false;
		}
/*
		case CanNominate_No_VoteFull:
		{
			CReplyToCommand(client, "{green}[NE]{default} %t", "Max Nominations");
			return false;
		}
*/
	}

	return true;
}

public int Handler_AdminMapSelectMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
		{
			if(menu != g_AdminMapMenu)
				delete menu;
		}
		case MenuAction_Select:
		{
			static char map[PLATFORM_MAX_PATH];
			GetMenuItem(menu, param2, map, sizeof(map));

			if(!CheckCommandAccess(param1, "sm_nominate_ignore", ADMFLAG_CHEATS, true))
			{
				if(AreRestrictionsActive() && (
					GetMapCooldownTime(map) > GetTime() ||
					GetMapTimeRestriction(map) ||
					GetMapPlayerRestriction(map) ||
					GetMapGroupRestriction(map, param1) >= 0 ||
					IsClientMapAdminRestricted(map, param1) ||
					IsClientMapVIPRestricted(map, param1)
					#if defined _zleader_included
					|| IsClientMapLeaderRestricted(map, param1)
					#endif
					))
				{
					CPrintToChat(param1, "{green}[NE]{default} You can't nominate this map right now.");
					return 0;
				}
			}

			NominateResult result = NominateMap(map, true, 0);

			if(result > Nominate_Replaced)
			{
				/* We assume already in vote is the casue because the maplist does a Map Validity check and we forced, so it can't be full */
				CPrintToChat(param1, "{green}[NE]{default} %t", "Map Already In Vote", map);
				return 0;
			}

			/* Map was nominated! - Disable the menu item and update the trie */
			SetTrieValue(g_mapTrie, map, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);

			CPrintToChat(param1, "{green}[NE]{default} %t", "Map Inserted", map);
			CPrintToChatAll("{green}[NE]{default} %N has inserted %s into nominations", param1, map);

			LogAction(param1, -1, "[NE] \"%L\" has inserted map \"%s\".", param1, map);
			Forward_OnAdminMapInsert(param1, map);
		}

		case MenuAction_DrawItem:
		{
			if(!CheckCommandAccess(param1, "sm_nominate_ignore", ADMFLAG_CHEATS, true))
			{
				return Handler_MapSelectMenu(menu, action, param1, param2);
			}

			return ITEMDRAW_DEFAULT;
		}

		case MenuAction_DisplayItem:
		{
			return Handler_MapSelectMenu(menu, action, param1, param2);
		}
	}

	return 0;
}

public int Handler_AdminRemoveMapMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
		{
			delete menu;
		}
		case MenuAction_Select:
		{
			static char map[PLATFORM_MAX_PATH];
			GetMenuItem(menu, param2, map, sizeof(map));

			if(!RemoveNominationByMap(map))
			{
				CReplyToCommand(param1, "{green}[NE]{default} This map isn't nominated.", map);
				return 0;
			}

			CReplyToCommand(param1, "{green}[NE]{default} Map '%s' removed from the nominations list.", map);
			CPrintToChatAll("{green}[NE]{default} %N has removed %s from nominations", param1, map);

			LogAction(param1, -1, "\"%L\" has removed map \"%s\" from nominations.", param1, map);
			Forward_OnAdminMapRemove(param1, map);
		}
	}

	return 0;
}

public int Native_GetNominationPool(Handle plugin, int numArgs)
{
	SetNativeCellRef(1, g_MapList);

	return 0;
}

public int Native_PushMapIntoNominationPool(Handle plugin, int numArgs)
{
	char map[PLATFORM_MAX_PATH];

	GetNativeString(1, map, PLATFORM_MAX_PATH);

	ShiftArrayUp(g_MapList, 0);
	SetArrayString(g_MapList, 0, map);

	UpdateMapTrie();
	UpdateMapMenus();

	return 0;
}

public int Native_PushMapsIntoNominationPool(Handle plugin, int numArgs)
{
	ArrayList maps = GetNativeCell(1);

	for (int i = 0; i < maps.Length; i++)
	{
		char map[PLATFORM_MAX_PATH];
		maps.GetString(i, map, PLATFORM_MAX_PATH);

		if (FindStringInArray(g_MapList, map) == -1)
		{
			ShiftArrayUp(g_MapList, 0);
			SetArrayString(g_MapList, 0, map);
		}
	}

	delete maps;

	UpdateMapTrie();
	UpdateMapMenus();

	return 0;
}

public int Native_RemoveMapFromNominationPool(Handle plugin, int numArgs)
{
	char map[PLATFORM_MAX_PATH];

	GetNativeString(1, map, PLATFORM_MAX_PATH);

	int idx;

	if ((idx = FindStringInArray(g_MapList, map)) != -1)
		RemoveFromArray(g_MapList, idx);

	UpdateMapTrie();
	UpdateMapMenus();

	return 0;
}

public int Native_RemoveMapsFromNominationPool(Handle plugin, int numArgs)
{
	ArrayList maps = GetNativeCell(1);

	for (int i = 0; i < maps.Length; i++)
	{
		char map[PLATFORM_MAX_PATH];
		maps.GetString(i, map, PLATFORM_MAX_PATH);

		int idx = -1;

		if ((idx = FindStringInArray(g_MapList, map)) != -1)
			RemoveFromArray(g_MapList, idx);
	}

	delete maps;

	UpdateMapTrie();
	UpdateMapMenus();

	return 0;
}

public int Native_ToggleNominations(Handle plugin, int numArgs)
{
	bool toggle = GetNativeCell(1);

	if(toggle)
		g_bNEAllowed = false;
	else
		g_bNEAllowed = true;
		
	return 1;
}

stock int GetVIPTimeRestriction()
{
	if (!GetConVarBool(g_Cvar_VIPTimeframe))
		return 0;

	char sTime[8];
	FormatTime(sTime, sizeof(sTime), "%H%M");

	int CurTime = StringToInt(sTime);
	int MinTime = GetConVarInt(g_Cvar_VIPTimeframeMinTime);
	int MaxTime = GetConVarInt(g_Cvar_VIPTimeframeMaxTime);

	//Wrap around.
	CurTime = (CurTime <= MinTime) ? CurTime + 2400 : CurTime;
	MaxTime = (MaxTime <= MinTime) ? MaxTime + 2400 : MaxTime;

	if ((MinTime <= CurTime <= MaxTime))
	{
		//Wrap around.
		MinTime = (MinTime <= CurTime) ? MinTime + 2400 : MinTime;
		MinTime = (MinTime <= MaxTime) ? MinTime + 2400 : MinTime;

		// Convert our 'time' to minutes.
		CurTime = ((CurTime / 100) * 60) + (CurTime % 100);
		MinTime = ((MinTime / 100) * 60) + (MinTime % 100);
		MaxTime = ((MaxTime / 100) * 60) + (MaxTime % 100);

		return MaxTime - CurTime;
	}

	return 0;
}

stock void CustomFormatTime(int seconds, char[] buffer, int maxlen)
{
	if(seconds <= 60)
		Format(buffer, maxlen, "%ds", seconds);
	else if(seconds <= 3600)
		Format(buffer, maxlen, "%dm", seconds / 60);
	else if(seconds < 10*3600)
		Format(buffer, maxlen, "%dh%dm", seconds / 3600, (seconds % 3600) / 60);
	else
		Format(buffer, maxlen, "%dh", seconds / 3600);
}

stock int TimeStrToSeconds(const char[] str)
{
	int seconds = 0;
	int maxlen = strlen(str);
	for(int i = 0; i < maxlen;)
	{
		int val = 0;
		i += StringToIntEx(str[i], val);
		if(str[i] == 'h')
		{
			val *= 60;
			i++;
		}
		seconds += val * 60;
	}
	return seconds;
}

stock void Forward_OnPublicMapInsert(int client, char[] mapname, bool IsVIP, bool IsLeader)
{
	Call_StartForward(g_hOnPublicMapInsert);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_PushCell(IsVIP);
	Call_PushCell(IsLeader);
	Call_Finish();
}

stock void Forward_OnPublicMapReplaced(int client, char[] mapname, bool IsVIP, bool IsLeader)
{
	Call_StartForward(g_hOnPublicMapReplaced);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_PushCell(IsVIP);
	Call_PushCell(IsLeader);
	Call_Finish();
}

stock void Forward_OnAdminMapInsert(int client, char[] mapname)
{
	Call_StartForward(g_hOnAdminMapInsert);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_Finish();
}

stock void Forward_OnAdminMapRemove(int client, char[] mapname)
{
	Call_StartForward(g_hOnAdminMapRemove);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_Finish();
}
