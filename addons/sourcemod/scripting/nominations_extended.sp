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
#include <clientprefs>

#pragma semicolon 1
#pragma newdecls required

#undef REQUIRE_PLUGIN
#tryinclude <zleader>
#define REQUIRE_PLUGIN

#define NE_VERSION "1.12.0"

public Plugin myinfo =
{
	name = "Map Nominations Extended",
	author = "SRCDSLab Team, tilgep & koen (Based on Powerlord, AlliedModders LLC, MCU)",
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
ConVar g_Cvar_MarkCustomMaps;
ConVar g_Cvar_NominateDelay;
ConVar g_Cvar_InitialDelay;

// VIP Nomination Convars
ConVar g_Cvar_VIPTimeframe;
ConVar g_Cvar_VIPTimeframeMinTime;
ConVar g_Cvar_VIPTimeframeMaxTime;
ConVar g_Cvar_MaxBanTime;
Handle g_hDelayNominate = INVALID_HANDLE;

// Forwards
Handle g_hOnPublicMapInsert = INVALID_HANDLE;
Handle g_hOnPublicMapReplaced = INVALID_HANDLE;
Handle g_hOnAdminMapInsert = INVALID_HANDLE;
Handle g_hOnMapNominationRemove = INVALID_HANDLE;

// Clients Prefs
Handle g_hShowUnavailableMaps = INVALID_HANDLE;
Handle g_hNomBanStatus = INVALID_HANDLE; // Format("length:timeIssued")
Handle g_hAdminNomBan = INVALID_HANDLE; // Admin who gave the nomban
bool g_bShowUnavailableMaps[MAXPLAYERS + 1] = { false, ... };

#define NOMBAN_NOTBANNED -1
#define NOMBAN_PERMANENT 0
int g_iNomBanLength[MAXPLAYERS+1];
int g_iNomBanStart[MAXPLAYERS+1];
char g_sNomBanAdmin[MAXPLAYERS+1][PLATFORM_MAX_PATH];

int g_Player_NominationDelay[MAXPLAYERS+1];
int g_NominationDelay;

bool g_bLate = false;
bool g_bNEAllowed = false;		// True if Nominations is available to players.

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("clientprefs.phrases");
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

	g_Cvar_MaxBanTime = CreateConVar("sm_nominate_max_ban_time", "10080", "Maximum time a client can be nombanned in minutes (for non rcon+)", 0, true);

	RegConsoleCmd("say", Command_Say);
	RegConsoleCmd("say_team", Command_Say);

	RegConsoleCmd("sm_nom", Command_Nominate, "Nominate a map.");
	RegConsoleCmd("sm_nominate", Command_Nominate, "Nominate a map.");

	RegConsoleCmd("sm_noms", Command_NominateList, "Shows a list of currently nominated maps");
	RegConsoleCmd("sm_nomlist", Command_NominateList, "Shows a list of currently nominated maps");
	
	RegConsoleCmd("sm_unnominate", Command_UnNominate, "Removes your nomination");
	RegConsoleCmd("sm_unominate", Command_UnNominate, "Removes your nomination");
	RegConsoleCmd("sm_unnom", Command_UnNominate, "Removes your nomination");
	RegConsoleCmd("sm_unom", Command_UnNominate, "Removes your nomination");

	RegConsoleCmd("sm_nomstatus", Command_NomStatus, "Shows your current nomban status.");

	RegAdminCmd("sm_nominate_force_lock", Command_DisableNE, ADMFLAG_CONVARS, "sm_nominate_force_lock - Forces to lock nominations");
	RegAdminCmd("sm_nominate_force_unlock", Command_EnableNE, ADMFLAG_CONVARS, "sm_nominate_force_unlock - Forces to unlock nominations");

	RegAdminCmd("sm_nominate_addmap", Command_Addmap, ADMFLAG_CHANGEMAP, "sm_nominate_addmap <mapname> - Forces a map to be on the next mapvote.");
	RegAdminCmd("sm_nominate_removemap", Command_Removemap, ADMFLAG_CHANGEMAP, "sm_nominate_removemap <mapname> - Removes a map from Nominations.");

	RegAdminCmd("sm_nominate_exclude", Command_AddExclude, ADMFLAG_CHANGEMAP, "sm_nominate_exclude <mapname> [cooldown] [mode]- Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.");
	RegAdminCmd("sm_nominate_exclude_time", Command_AddExcludeTime, ADMFLAG_CHANGEMAP, "sm_nominate_exclude_time <mapname> [cooldown] [mode] - Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.");

	RegAdminCmd("sm_nomban", Command_Nomban, ADMFLAG_BAN, "Ban a client from nominating.");
	RegAdminCmd("sm_nombanlist", Command_NombanList, ADMFLAG_BAN, "View a list of nombanned clients.");
	RegAdminCmd("sm_unnomban", Command_UnNomban, ADMFLAG_BAN, "Unban a client from nominating.");
	RegAdminCmd("sm_nomunban", Command_UnNomban, ADMFLAG_BAN, "Unban a client from nominating.");
	RegAdminCmd("sm_unomban", Command_UnNomban, ADMFLAG_BAN, "Unban a client from nominating.");

	// Nominations Extended cvars
	CreateConVar("ne_version", NE_VERSION, "Nominations Extended Version", FCVAR_SPONLY|FCVAR_NOTIFY|FCVAR_DONTRECORD);

	AutoExecConfig(true, "nominations_extended");

	// Cookies support
	SetCookieMenuItem(NE_CookieHandler, 0, "Nominations Extended Settings");
	g_hShowUnavailableMaps = RegClientCookie("NE_hide_unavailable", "Hide unavailable maps from the nominations list", CookieAccess_Protected);
	g_hNomBanStatus = RegClientCookie("NE_nomban_status", "Client's nomban info (Ban lenght : Date Issued)", CookieAccess_Protected);
	g_hAdminNomBan = RegClientCookie("NE_nomban_admin", "Admin who nombanned", CookieAccess_Protected);

	if(g_bLate)
	{
		for(int i = 1; i <= MaxClients; i++)
		{
			if(IsClientInGame(i) && !IsFakeClient(i) && AreClientCookiesCached(i))
			{
				OnClientCookiesCached(i);
			}
		}
	}

	g_mapTrie = CreateTrie();

	// Timer Checker
	CreateTimer(60.0, Timer_NomBansChecker, _, TIMER_REPEAT);
}

public APLRes AskPluginLoad2(Handle hThis, bool bLate, char[] err, int iErrLen)
{
	g_bLate = bLate;
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
	g_hOnMapNominationRemove = CreateGlobalForward("NE_OnMapNominationRemove", ET_Ignore, Param_Cell, Param_String);

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

public void OnClientDisconnect(int client)
{
	SetClientCookies(client);
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

	g_MapMenu = BuildMapMenu("", -1);

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
			if(strcmp(map, currentMap) == 0)
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

Action Timer_NomBansChecker(Handle timer)
{
	for(int i=1; i<MaxClients+1; i++)
	{
		if(!IsClientInGame(i) || IsFakeClient(i))
			continue;
		if(g_iNomBanLength[i] == NOMBAN_PERMANENT)
			continue;
		if(g_iNomBanLength[i] == NOMBAN_NOTBANNED)
			continue;

		// Check the time of the nomban and compare it to the current time to see if it has expired
		if(g_iNomBanStart[i] + g_iNomBanLength[i] < GetTime())
			UnNomBanClient(i, 0);
	}

	return Plugin_Continue;
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
	Forward_OnMapNominationRemove(client, mapname);

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

public Action Command_Nomban(int client, int args)
{
	if(GetCmdArgs() != 2)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Nomban usage");
		return Plugin_Handled;
	}
	
	char target_argument[64];
	GetCmdArg(1, target_argument, sizeof(target_argument));
	
	int target = -1;
	if((target = FindTarget(client, target_argument, true, false)) == -1)
	{
		return Plugin_Handled;
	}

	if(IsClientNomBanned(target))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Already nombanned", target);
		return Plugin_Handled;
	}
	
	char sLen[64];
	GetCmdArg(2, sLen, sizeof(sLen));
	int length = StringToInt(sLen);
	int maxtime = g_Cvar_MaxBanTime.IntValue;
	if (maxtime < 0)
		maxtime = 10080;

	bool bPerm = CheckCommandAccess(client, "sm_nomban_perm", ADMFLAG_RCON);

	if ((length == NOMBAN_PERMANENT || length > maxtime) && bPerm || length > NOMBAN_PERMANENT && length <= maxtime)
	{
		NomBanClient(target, length*60, client);
	}
	else
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Nomban usage");
	}
		
	return Plugin_Handled;
}

public Action Command_UnNomban(int client, int args)
{
	if(GetCmdArgs() < 1)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "UnNomban usage");
		return Plugin_Handled;
	}
	
	char target_argument[64];
	GetCmdArg(1, target_argument, sizeof(target_argument));
	
	int target = -1;
	if((target = FindTarget(client, target_argument, true, false)) == -1)
		return Plugin_Handled;

	if(!IsClientNomBanned(target))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Already not nombanned", target);
		return Plugin_Handled;
	}
	
	UnNomBanClient(target, client);
		
	return Plugin_Handled;
}

public Action Command_NombanList(int client, int args)
{
	if(client == 0)
	{
		PrintToServer("--------------------");
		PrintToServer("Nomban List");
		PrintToServer("--------------------");

		for(int i = 1; i <= MaxClients; i++)
		{
			if(!IsClientNomBanned(i))
				continue;

			int uid = GetClientUserId(i);

			PrintToServer("[#%d] %N", uid, i);
		}
		PrintToServer("--------------------");
	}
	else
	{
		PrepareNombanListMenu(client);
	}

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

public Action Command_UnNominate(int client, int args)
{
	if(!client)
		return Plugin_Handled;

	char map[PLATFORM_MAX_PATH];
	if(!g_bNEAllowed || !GetNominationByOwner(client, map) || !RemoveNominationByOwner(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Unable To Unnominate");
		return Plugin_Handled;
	}

	CPrintToChatAll("{green}[NE]{default} %t", "Map Unnominated", client, map);
	LogAction(client, -1, "\"%L\" has removed his nomination \"%s\".", client, map);

	return Plugin_Handled;
}

public Action Command_NomStatus(int client, int args)
{
    if(client == 0)
    {
        CPrintToChat(client, "{green}[NE]{default} %t", "Can only use command in game");
        return Plugin_Handled;
    }

    int target = client;

    if(args > 0)
    {
        char targ[64];
        GetCmdArg(1, targ, sizeof(targ));
        target = FindTarget(client, targ, true, false);
    }

    if(!AreClientCookiesCached(target))
    {
        CPrintToChat(client, "{green}[NE]{default} %t", "Cookies are not cached");
        return Plugin_Handled;
    }

    PrepareNomStatusMenu(client, target);

    return Plugin_Handled;
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
		CPrintToChat(client, "{green}[NE]{default} No maps have been nominated.");
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
		case MenuAction_Select:
		{
			char sMap[PLATFORM_MAX_PATH], sParam[PLATFORM_MAX_PATH];
			GetMenuItem(menu, param2, sParam, sizeof(sParam));

			// Nominations are locked, so we block the unnomination
			if (!g_bNEAllowed)
				return 0;

			if (GetNominationByOwner(param1, sMap) && strcmp(sMap, sParam, false) == 0)
			{
				char sName[MAX_NAME_LENGTH];
				GetClientName(param1, sName, sizeof(sName));

				RemoveNominationByOwner(param1);
				CPrintToChatAll("{green}[NE]{default} %t", "Map Unnominated", sName, sMap);
				LogAction(param1, -1, "\"%L\" has removed his nomination \"%s\".", param1, sMap);
			}
		}
	}

	return 0;
}

Action AttemptNominate(int client, const char[] filter = "")
{
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

	if(IsClientNomBanned(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Cannot nominate - Nombanned");
		return Plugin_Handled;
	}

	Menu menu = g_MapMenu;
	menu = BuildMapMenu(filter[0] ? filter : "", filter[0] ? -1 : client);

	SetMenuTitle(menu, "%T", "Nominate Title", client);
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
	return Plugin_Handled;
}

Action AttemptAdminNominate(int client, const char[] filter = "")
{
	if(!client)
		return Plugin_Handled;

	if(!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations is currently locked.");
		return Plugin_Handled;
	}

	if(IsClientNomBanned(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Cannot nominate - Nombanned");
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

			char sBuffer[64];
			if (owner == client)
				Format(sBuffer, sizeof(sBuffer), "%T", "Unnominate", client);
			else
				Format(sBuffer, sizeof(sBuffer), "%N", owner);

			if(!owner)
				Format(display, sizeof(display), "%s (%T)", display, "Nominated by Admin", client);
			else
				Format(display, sizeof(display), "%s (%s)", display, sBuffer);

			AddMenuItem(menu, map, display);
		}
	}

	delete MapList;
	delete OwnerList;
	return true;
}

Menu BuildMapMenu(const char[] filter, int client = -1)
{
	Menu menu = CreateMenu(Handler_MapSelectMenu, MENU_ACTIONS_DEFAULT|MenuAction_DrawItem|MenuAction_DisplayItem);

	static char map[PLATFORM_MAX_PATH];
	bool bValid = client > 0 && client <= MaxClients && IsClientInGame(client);
	bool bCached = bValid && AreClientCookiesCached(client);

	// We use \n to create a space between the nomination and the rest of the menu

	if (bCached && !filter[0])
	{
		char buffer[128];
		Format(buffer, sizeof(buffer), "%T: %T\n ", "Nominations Show Unavailable", client, g_bShowUnavailableMaps[client] ? "Yes" : "No", client);
		menu.AddItem("show_unavailable", buffer);
	}

	if(bValid)
	{
		char sNominated[PLATFORM_MAX_PATH];
		if(GetNominationByOwner(client, sNominated))
		{
			Format(sNominated, sizeof(sNominated), "%s (%T)\n ", sNominated, "Unnominate", client);
			menu.AddItem("show_nominated", sNominated);
		}
	}

	for(int i = 0; i < GetArraySize(g_MapList); i++)
	{
		GetArrayString(g_MapList, i, map, sizeof(map));

		if(!filter[0] || StrContains(map, filter, false) != -1)
		{
			// If client does not have cookies cached or choose see unavailable maps: Show all maps
			if(!bCached || bCached && g_bShowUnavailableMaps[client])
			{
				AddMenuItem(menu, map, map);
			}
			// Cookies are cached and client choose to Hide unavailable maps
			if(bCached && !g_bShowUnavailableMaps[client] && 
				AreRestrictionsActive() &&
				GetMapCooldown(map) == 0 &&
				GetMapCooldownTime(map) < GetTime() &&
				GetMapTimeRestriction(map) == 0 &&
				GetMapPlayerRestriction(map) == 0 &&
				GetMapGroupRestriction(map, client) < 0)
			{
				AddMenuItem(menu, map, map);
			}
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

			if (strcmp(map, "show_unavailable", false) == 0)
			{
				g_bShowUnavailableMaps[param1] = !g_bShowUnavailableMaps[param1];
				AttemptNominate(param1, "");
				return 0;
			}

			GetClientName(param1, name, MAX_NAME_LENGTH);

			if (strcmp(map, "show_nominated", false) == 0)
			{
				char sNominated[PLATFORM_MAX_PATH];
				if (GetNominationByOwner(param1, sNominated))
				{
					RemoveNominationByOwner(param1);
					CPrintToChatAll("{green}[NE]{default} %t", "Map Unnominated", name, sNominated);
					LogAction(param1, -1, "\"%L\" has removed his nomination \"%s\".", param1, sNominated);
					Forward_OnMapNominationRemove(param1, sNominated);
					return 0;
				}
			}

			if(IsMapRestricted(param1, map))
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

			if(IsMapRestricted(param1, map))
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
	if(IsClientNomBanned(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Cannot nominate - Nombanned", client);
		return false;
	}

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
				if(IsMapRestricted(param1, map))
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
			Forward_OnMapNominationRemove(param1, map);
		}
	}

	return 0;
}

/* COOKIES SUPPORT */
public void OnClientCookiesCached(int client)
{
	ReadClientCookies(client);
}

public void ReadClientCookies(int client) {
	char buffer[128];
	GetClientCookie(client, g_hShowUnavailableMaps, buffer, sizeof(buffer));
	// If no cookies was found (null), set the value to false as initial value
	// Client can choose to enable the settings from the settings or nominate menu
	// Otherwise, apply the value from the cookie
	g_bShowUnavailableMaps[client] = (buffer[0] != '\0') ? view_as<bool>(StringToInt(buffer)) : false;

	GetClientCookie(client, g_hNomBanStatus, buffer, sizeof(buffer));
	if(strcmp(buffer, "") == 0)
	{
		g_iNomBanStart[client] = NOMBAN_NOTBANNED;
		g_iNomBanLength[client] = NOMBAN_NOTBANNED;
		return;
	}

	PrintToChatAll("Debug cookie: %s - strlen %d", buffer, strlen(buffer));

	char sBuffer[2][64];
	ExplodeString(buffer, ":", sBuffer, sizeof(sBuffer), sizeof(sBuffer[]), true);
	g_iNomBanLength[client] = StringToInt(sBuffer[0]);
	g_iNomBanStart[client] = StringToInt(sBuffer[1]);

	PrintToChatAll("Debug cookie: %d - %d", g_iNomBanLength[client], g_iNomBanStart[client]);

	if(IsClientNomBanned(client))
		RemoveNominationByOwner(client);
}

public void SetClientCookies(int client)
{
	if (!AreClientCookiesCached(client) || IsFakeClient(client))
		return;

	char sValue[8];
	Format(sValue, sizeof(sValue), "%i", g_bShowUnavailableMaps[client]);
	SetClientCookie(client, g_hShowUnavailableMaps, sValue);
	SaveClientNombanStatus(client);
}

public void NE_CookieHandler(int client, CookieMenuAction action, any info, char[] buffer, int maxlen)
{
	switch (action)
	{
		case CookieMenuAction_SelectOption:
		{
			NE_ClientSettings_Menu(client);
		}
	}
}

public void NE_ClientSettings_Menu(int client)
{
	Menu menu = new Menu(NE_ClientSettingsHandler, MENU_ACTIONS_ALL);
	menu.SetTitle("%T %T", "Nominate Title", client, "Client Settings", client);

	bool bCached = AreClientCookiesCached(client);
	char buffer[128];
	if (bCached)
		Format(buffer, sizeof(buffer), "%T: %T", "Nominations Show Unavailable", client, g_bShowUnavailableMaps[client] ? "Yes" : "No", client);
	else
		Format(buffer, sizeof(buffer), "%T", "Can not Load Cookies", client);
	
	menu.AddItem("ShowMap", buffer, bCached ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(client, MENU_TIME_FOREVER);
}

public int NE_ClientSettingsHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_Select:
		{
			char info[64];
			menu.GetItem(param2, info, sizeof(info));
			if (strcmp(info, "ShowMap", false) == 0)
				g_bShowUnavailableMaps[param1] = !g_bShowUnavailableMaps[param1];

			NE_ClientSettings_Menu(param1);
		}
		case MenuAction_Cancel:
		{
			ShowCookieMenu(param1);
		}
		case MenuAction_End:
		{
			delete menu;
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

stock bool IsMapRestricted(int client, char[] map)
{
	return AreRestrictionsActive() && (GetMapCooldownTime(map) > GetTime() || GetMapTimeRestriction(map) || GetMapPlayerRestriction(map) ||
	GetMapGroupRestriction(map, client) >= 0 || IsClientMapAdminRestricted(map, client) || IsClientMapVIPRestricted(map, client) || IsClientMapLeaderRestricted(map, client));
}

public void PrepareNomStatusMenu(int client, int target)
{
	Menu menu = CreateMenu(NomStatusMenu_Handler);
	menu.SetTitle("%T", "NomStatus Menu Title", client, target);

	char buffer[PLATFORM_MAX_PATH];
	if(!IsClientNomBanned(target))
	{
		Format(buffer, sizeof(buffer), "%T", "Menu - Not Nombanned", client);
		menu.AddItem("b", buffer, ITEMDRAW_DISABLED);
		menu.Display(client, MENU_TIME_FOREVER);
		return;
	}

	if(g_iNomBanLength[target] == NOMBAN_PERMANENT)
	{
		Format(buffer, sizeof(buffer), "%T", "Menu - Nomban Duration", client, "Permanent");
		menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);
	}
	else
	{
		int seconds = g_iNomBanLength[target];
		int minutes = seconds / 60;
		int hours = minutes / 60;
		int days = hours / 24;

		char time[64];

		if(days > 0)
			Format(time, sizeof(time), "%d days, %d hours, %d minutes", days, hours%24, minutes%60);
		else if(hours > 0)
			Format(time, sizeof(time), "%d hours %d minutes", hours, minutes%60);
		else if(minutes > 0)
			Format(time, sizeof(time), "%d minutes", minutes);
		else
			Format(time, sizeof(time), "%d seconds", seconds);

		Format(buffer, sizeof(buffer), "%t", "Menu - Nomban Duration", time);
		menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);

		menu.AddItem(" ", " ", ITEMDRAW_SPACER);

		int end = g_iNomBanStart[target] + g_iNomBanLength[target];
		FormatTime(time, sizeof(time), "%c", end);
		Format(buffer, sizeof(buffer), "%T", "Menu - Nomban end", client, time);
		menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);

		int timeleftS = end - GetTime();
		int timeleftM = timeleftS / 60;
		int timeleftH = timeleftM / 60;
		int timeleftD = timeleftH / 24;

		if(timeleftD > 0)
			Format(time, sizeof(time), "%d days, %d hours, %d minutes", timeleftD, timeleftH%24, timeleftM%60);
		else if(timeleftH > 0)
			Format(time, sizeof(time), "%d hours, %d minutes, %d seconds", timeleftH, timeleftM%60, timeleftS%60);
		else if(timeleftM > 0)
			Format(time, sizeof(time), "%d minutes, %d seconds", timeleftM, timeleftS%60);
		else
			Format(time, sizeof(time), "%d seconds", timeleftS);

		Format(buffer, sizeof(buffer), "%T", "Menu - Nomban timeleft", client, time);
		menu.AddItem("timeleft", buffer, ITEMDRAW_DISABLED);

		menu.AddItem(" ", " ", ITEMDRAW_SPACER);
	}

	Format(buffer, sizeof(buffer), "%T", "Menu - Nomban Admin", client, g_sNomBanAdmin[client]);
	menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);

	menu.Display(client, MENU_TIME_FOREVER);
}

public int NomStatusMenu_Handler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
			delete menu;
	}
	return 0;
}

public void PrepareNombanListMenu(int client)
{
	Menu menu = CreateMenu(Nombanlist_Handler);

	char info[64], display[64];
	int total = 0;

	for(int i = 1; i <= MaxClients; i++)
	{
		if(!IsClientNomBanned(i))
			continue;

		int uid = GetClientUserId(i);

		Format(info, sizeof(info), "%d", uid);
		Format(display, sizeof(display), "[#%d] %N", uid, i);
		menu.AddItem(info, display);

		total++;
	}

	if(total == 0)
	{
		Format(display, sizeof(display), "%T", "No clients nombanned", client);
		menu.AddItem("nothing", display, ITEMDRAW_DISABLED);
	}
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int Nombanlist_Handler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			char buffer[64];
			GetMenuItem(menu, param2, buffer, sizeof(buffer));
			
			int uid = StringToInt(buffer);
			int client = GetClientOfUserId(uid);

			if(client == 0)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "NombanList Client Not valid");
				PrepareNombanListMenu(param1);
			}
			else
			{
				PrepareNomStatusMenu(param1, client);
			}
		}
		case MenuAction_End:
			delete menu;
	}

	return 0;
}

public void NomBanClient(int target, int duration, int admin)
{
	char buffer[64];

	if(admin == 0)
		Format(g_sNomBanAdmin[target], sizeof(g_sNomBanAdmin[]), "[Console]");
	else
	{
		GetClientAuthId(admin, AuthId_Steam2, buffer, sizeof(buffer), false);
		Format(g_sNomBanAdmin[target], sizeof(g_sNomBanAdmin[]), "%N (%s)", admin, buffer);
	}

	// Format = length:timeIssued
	int issued = GetTime();

	g_iNomBanLength[target] = duration;
	g_iNomBanStart[target] = issued;

	// Store to cookies
	SaveClientNombanStatus(target);
	
	if(RemoveNominationByOwner(target))
		CPrintToChat(target, "{green}[NE]{default} %t", "Nomination removed on nomban");

	if(duration == NOMBAN_PERMANENT)
	{
		CShowActivity2(admin, "{green}[NE] {olive}","{default}%t", "Nombanned permanent", target);
		LogAction(admin, target, "%L nombanned %L permanently.", admin, target);
		return;
	}

	CShowActivity2(admin, "{green}[NE] {olive}","{default}%t", "Nombanned", target, duration/60);
	LogAction(admin, target, "%L nombanned %L for %d minutes.", admin, target, duration/60);
}

public void UnNomBanClient(int client, int admin)
{
	g_iNomBanLength[client] = NOMBAN_NOTBANNED;
	g_iNomBanStart[client] = NOMBAN_NOTBANNED;
	g_sNomBanAdmin[client] = "";
	
	SaveClientNombanStatus(client);

	CShowActivity2(admin, "{green}[NE] {olive}", "%t", "UnNombanned", client);
	LogAction(admin, client, "%L unnombanned %L", admin, client);

	Menu menu = CreateMenu(NomStatusMenu_Handler);
	menu.SetTitle("%T", "NomStatus Menu Title", client, client);
	menu.AddItem("s", " ", ITEMDRAW_SPACER);

	char sTitle[64];
	Format(sTitle, sizeof(sTitle), "%T", "Menu Got UnNombanned", client);
	menu.AddItem("b", sTitle, ITEMDRAW_DISABLED);

	menu.Display(client, 15);
}

stock void SaveClientNombanStatus(int client)
{
	if(g_iNomBanLength[client] != NOMBAN_NOTBANNED)
	{
		char buffer[128];
		Format(buffer, sizeof(buffer), "%d:%d", g_iNomBanLength[client], g_iNomBanStart[client]);
		SetClientCookie(client, g_hNomBanStatus, buffer);
	}
	else
	{
		SetClientCookie(client, g_hNomBanStatus, "");
	}

	SetClientCookie(client, g_hAdminNomBan, g_sNomBanAdmin[client]);
}

stock bool IsClientNomBanned(int client)
{
	if(!IsClientInGame(client) || !AreClientCookiesCached(client) || g_iNomBanLength[client] == NOMBAN_NOTBANNED)
		return false;

	if(g_iNomBanLength[client] == NOMBAN_PERMANENT || GetTime() < g_iNomBanStart[client] + g_iNomBanLength[client])
		return true;

	return false;
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

stock void Forward_OnMapNominationRemove(int client, char[] mapname)
{
	Call_StartForward(g_hOnMapNominationRemove);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_Finish();
}