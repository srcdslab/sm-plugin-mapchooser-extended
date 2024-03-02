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
#tryinclude <nativevotes>
#tryinclude <nominations_extended>
#tryinclude <zleader>
#define REQUIRE_PLUGIN

#define MCE_VERSION "1.11.0"

#define NV "nativevotes"
#define ZLEADER "zleader"

enum RoundCounting
{
	RoundCounting_Standard = 0,
	RoundCounting_MvM,
	RoundCounting_ArmsRace,
}

// CSGO requires two cvars to get the game type
enum
{
	GameType_Classic	= 0,
	GameType_GunGame	= 1,
	GameType_Training	= 2,
	GameType_Custom		= 3,
}

enum
{
	GunGameMode_ArmsRace	= 0,
	GunGameMode_Demolition	= 1,
	GunGameMode_DeathMatch	= 2,
}

public Plugin myinfo =
{
	name = "MapChooser Extended",
	author = "Powerlord, Zuko, BotoX, maxime1907, Rushaway and AlliedModders LLC",
	description = "Automated Map Voting with Extensions",
	version = MCE_VERSION,
	url = "https://forums.alliedmods.net/showthread.php?t=156974"
};

/* Valve ConVars */
ConVar g_Cvar_Winlimit;
ConVar g_Cvar_Maxrounds;
ConVar g_Cvar_Fraglimit;
ConVar g_Cvar_Bonusroundtime;
ConVar g_Cvar_MatchClinch;
ConVar g_Cvar_VoteNextLevel;
ConVar g_Cvar_GameType;
ConVar g_Cvar_GameMode;

/* Plugin ConVars */
ConVar g_Cvar_StartTime;
ConVar g_Cvar_StartRounds;
ConVar g_Cvar_StartFrags;
ConVar g_Cvar_ExtendTimeStep;
ConVar g_Cvar_ExtendRoundStep;
ConVar g_Cvar_ExtendFragStep;
ConVar g_Cvar_ExcludeMaps;
ConVar g_Cvar_ExcludeMapsTime;
ConVar g_Cvar_IncludeMaps;
ConVar g_Cvar_IncludeMapsReserved;
ConVar g_Cvar_NoVoteMode;
ConVar g_Cvar_Extend;
ConVar g_Cvar_DontChange;
ConVar g_Cvar_EndOfMapVote;
ConVar g_Cvar_EndOfMapInfo;
ConVar g_Cvar_VoteDuration;
ConVar g_Cvar_RandomStartTime;
ConVar g_Cvar_CountBots;

Handle g_VoteTimer = INVALID_HANDLE;
Handle g_RetryTimer = INVALID_HANDLE;
Handle g_WarningTimer = INVALID_HANDLE;

/* Data Handles */
Handle g_MapList = INVALID_HANDLE;
Handle g_NominateList = INVALID_HANDLE;
Handle g_NominateOwners = INVALID_HANDLE;
StringMap g_OldMapList;
StringMap g_TimeMapList;
Handle g_NextMapList = INVALID_HANDLE;
Handle g_VoteMenu = INVALID_HANDLE;
KeyValues g_Config;

int g_Extends;
int g_TotalRounds;
bool g_HasVoteStarted;
bool g_WaitingForVote;
bool g_MapVoteCompleted;
bool g_ChangeMapAtRoundEnd;
bool g_ChangeMapInProgress;
bool g_HasIntermissionStarted = false;
int g_mapFileSerial = -1;

int g_NominateCount = 0;
int g_NominateReservedCount = 0;
MapChange g_ChangeTime;

Handle g_NominationsResetForward = INVALID_HANDLE;
Handle g_NominationDisconnect = INVALID_HANDLE;
Handle g_MapVoteStartedForward = INVALID_HANDLE;

/* Mapchooser Extended Plugin ConVars */

ConVar g_Cvar_RunOff;
ConVar g_Cvar_RunOffPercent;
ConVar g_Cvar_BlockSlots;
ConVar g_Cvar_MaxRunOffs;
ConVar g_Cvar_StartTimePercent;
ConVar g_Cvar_StartTimePercentEnable;
ConVar g_Cvar_WarningTime;
ConVar g_Cvar_RunOffWarningTime;
ConVar g_Cvar_MenuStyle;
ConVar g_Cvar_TimerLocation;
ConVar g_Cvar_ExtendPosition;
ConVar g_Cvar_MarkCustomMaps;
ConVar g_Cvar_RandomizeNominations;
ConVar g_Cvar_HideTimer;
ConVar g_Cvar_NoVoteOption;
ConVar g_Cvar_ShufflePerClient;
ConVar g_Cvar_NoRestrictionTimeframeEnable;
ConVar g_Cvar_NoRestrictionTimeframeMinTime;
ConVar g_Cvar_NoRestrictionTimeframeMaxTime;
ConVar g_Cvar_TimerUnlockNoms;
ConVar g_Cvar_LockNominationsAtWarning;

/* Mapchooser Extended Data Handles */
Handle g_OfficialList = INVALID_HANDLE;

/* Mapchooser Extended Forwards */
Handle g_MapVoteWarningStartForward = INVALID_HANDLE;
Handle g_MapVoteWarningTickForward = INVALID_HANDLE;
Handle g_MapVoteStartForward = INVALID_HANDLE;
Handle g_MapVoteEndForward = INVALID_HANDLE;
Handle g_MapVoteRunoffStartForward = INVALID_HANDLE;

/* Mapchooser Extended Globals */
int g_RunoffCount = 0;
int g_mapOfficialFileSerial = -1;
bool g_NativeVotes = false;
char g_GameModName[64];
bool g_WarningInProgress = false;
bool g_AddNoVote = false;
bool g_SaveCDOnMapEnd = true;
bool g_ZLeader = false;

RoundCounting g_RoundCounting = RoundCounting_Standard;

/* Upper bound of how many team there could be */
#define MAXTEAMS 10
int g_winCount[MAXTEAMS];

bool g_BlockedSlots = false;
int g_ObjectiveEnt = -1;

enum TimerLocation
{
	TimerLocation_Hint = 0,
	TimerLocation_Center = 1,
	TimerLocation_Chat = 2,
}

enum WarningType
{
	WarningType_Vote,
	WarningType_Revote,
}

#define VOTE_EXTEND "##extend##"
#define VOTE_DONTCHANGE "##dontchange##"

/* Mapchooser Extended Defines */
#define LINE_ONE "##lineone##"
#define LINE_TWO "##linetwo##"
#define LINE_SPACER "##linespacer##"
#define FAILURE_TIMER_LENGTH 5

public void OnPluginStart()
{
	LoadTranslations("mapchooser_extended.phrases");
	LoadTranslations("basevotes.phrases");
	LoadTranslations("common.phrases");

	int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	g_MapList = CreateArray(arraySize);
	g_NominateList = CreateArray(arraySize);
	g_NominateOwners = CreateArray(1);
	g_OldMapList = new StringMap();
	g_TimeMapList = new StringMap();
	g_NextMapList = CreateArray(arraySize);
	g_OfficialList = CreateArray(arraySize);

	GetGameFolderName(g_GameModName, sizeof(g_GameModName));

	g_Cvar_EndOfMapVote = CreateConVar("mce_endvote", "1", "Specifies if MapChooser should run an end of map vote", _, true, 0.0, true, 1.0);
	g_Cvar_EndOfMapInfo = CreateConVar("mce_endmap_info", "1", "Specifies if MapChooser should print a message with nextmap when the map end.", _, true, 0.0, true, 1.0);

	g_Cvar_StartTime = CreateConVar("mce_starttime", "10.0", "Specifies when to start the vote based on time remaining.", _, true, 1.0);
	g_Cvar_RandomStartTime = CreateConVar("mce_random_starttime", "30.0", "The max interval time to add up to the original interval time for map vote in seconds", _, true, 1.0, true, 180.0);
	g_Cvar_StartRounds = CreateConVar("mce_startround", "2.0", "Specifies when to start the vote based on rounds remaining. Use 0 on DoD:S, CS:S, and TF2 to start vote during bonus round time", _, true, 0.0);
	g_Cvar_StartFrags = CreateConVar("mce_startfrags", "5.0", "Specifies when to start the vote base on frags remaining.", _, true, 1.0);
	g_Cvar_ExtendTimeStep = CreateConVar("mce_extend_timestep", "15", "Specifies how much many more minutes each extension makes", _, true, 5.0);
	g_Cvar_ExtendRoundStep = CreateConVar("mce_extend_roundstep", "5", "Specifies how many more rounds each extension makes", _, true, 1.0);
	g_Cvar_ExtendFragStep = CreateConVar("mce_extend_fragstep", "10", "Specifies how many more frags are allowed when map is extended.", _, true, 5.0);
	g_Cvar_ExcludeMaps = CreateConVar("mce_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	g_Cvar_ExcludeMapsTime = CreateConVar("mce_exclude_time", "5h", "Specifies how long in minutes an old map is excluded from the vote.");
	g_Cvar_IncludeMaps = CreateConVar("mce_include", "5", "Specifies how many maps to include in the vote.", _, true, 2.0, true, 7.0);
	g_Cvar_IncludeMapsReserved = CreateConVar("mce_include_reserved", "2", "Specifies how many private/random maps to include in the vote.", _, true, 0.0, true, 5.0);
	g_Cvar_NoVoteMode = CreateConVar("mce_novote", "1", "Specifies whether or not MapChooser should pick a map if no votes are received.", _, true, 0.0, true, 1.0);
	g_Cvar_Extend = CreateConVar("mce_extend", "0", "Number of extensions allowed each map.", _, true, 0.0);
	g_Cvar_DontChange = CreateConVar("mce_dontchange", "1", "Specifies if a 'Don't Change' option should be added to early votes", _, true, 0.0);
	g_Cvar_VoteDuration = CreateConVar("mce_voteduration", "20", "Specifies how long the mapvote should be available for.", _, true, 5.0);
	g_Cvar_CountBots = CreateConVar("mce_count_bots", "1", "Should we count bots as players for Min/MaxPlayers ?", FCVAR_NOTIFY, true, 0.0, true, 1.0);

	// MapChooser Extended cvars
	CreateConVar("mce_version", MCE_VERSION, "MapChooser Extended Version", FCVAR_SPONLY|FCVAR_NOTIFY|FCVAR_DONTRECORD);

	g_Cvar_RunOff = CreateConVar("mce_runoff", "1", "Hold run off votes if winning choice has less than a certain percentage of votes", _, true, 0.0, true, 1.0);
	g_Cvar_RunOffPercent = CreateConVar("mce_runoffpercent", "50", "If winning choice has less than this percent of votes, hold a runoff", _, true, 0.0, true, 100.0);
	g_Cvar_BlockSlots = CreateConVar("mce_blockslots", "0", "Block slots to prevent accidental votes.  Only applies when Voice Command style menus are in use.", _, true, 0.0, true, 1.0);
	//g_Cvar_BlockSlotsCount = CreateConVar("mce_blockslots_count", "2", "Number of slots to block.", _, true, 1.0, true, 3.0);
	g_Cvar_MaxRunOffs = CreateConVar("mce_maxrunoffs", "1", "Number of run off votes allowed each map.", _, true, 0.0);
	g_Cvar_StartTimePercent = CreateConVar("mce_start_percent", "35.0", "Specifies when to start the vote based on percents.", _, true, 0.0, true, 100.0);
	g_Cvar_StartTimePercentEnable = CreateConVar("mce_start_percent_enable", "0", "Enable or Disable percentage calculations when to start vote.", _, true, 0.0, true, 1.0);
	g_Cvar_WarningTime = CreateConVar("mce_warningtime", "15.0", "Warning time in seconds.", _, true, 0.0, true, 60.0);
	g_Cvar_RunOffWarningTime = CreateConVar("mce_runoffvotewarningtime", "5.0", "Warning time for runoff vote in seconds.", _, true, 0.0, true, 30.0);
	g_Cvar_MenuStyle = CreateConVar("mce_menustyle", "0", "Menu Style.  0 is the game's default, 1 is the older Valve style that requires you to press Escape to see the menu, 2 is the newer 1-9 button Voice Command style, unavailable in some games. Ignored on TF2 if NativeVotes Plugin is loaded.", _, true, 0.0, true, 2.0);
	g_Cvar_TimerLocation = CreateConVar("mce_warningtimerlocation", "0", "Location for the warning timer text. 0 is HintBox, 1 is Center text, 2 is Chat.  Defaults to HintBox.", _, true, 0.0, true, 2.0);
	g_Cvar_MarkCustomMaps = CreateConVar("mce_markcustommaps", "1", "Mark custom maps in the vote list. 0 = Disabled, 1 = Mark with *, 2 = Mark with phrase.", _, true, 0.0, true, 2.0);
	g_Cvar_ExtendPosition = CreateConVar("mce_extendposition", "0", "Position of Extend/Don't Change options. 0 = at end, 1 = at start.", _, true, 0.0, true, 1.0);
	g_Cvar_RandomizeNominations = CreateConVar("mce_randomizeorder", "0", "Randomize map order?", _, true, 0.0, true, 1.0);
	g_Cvar_HideTimer = CreateConVar("mce_hidetimer", "0", "Hide the MapChooser Extended warning timer", _, true, 0.0, true, 1.0);
	g_Cvar_NoVoteOption = CreateConVar("mce_addnovote", "1", "Add \"No Vote\" to vote menu?", _, true, 0.0, true, 1.0);
	g_Cvar_ShufflePerClient = CreateConVar("mce_shuffle_per_client", "1", "Random shuffle map vote menu per client?", _, true, 0.0, true, 1.0);
	g_Cvar_NoRestrictionTimeframeEnable = CreateConVar("mce_no_restriction_timeframe_enable", "1", "Enable timeframe where all nomination restrictions and cooldowns are disabled?", _, true, 0.0, true, 1.0);
	g_Cvar_NoRestrictionTimeframeMinTime = CreateConVar("mce_no_restriction_timeframe_mintime", "0100", "Start of the timeframe where all nomination restrictions and cooldowns are disabled (Format: HHMM)", _, true, 0000.0, true, 2359.0);
	g_Cvar_NoRestrictionTimeframeMaxTime = CreateConVar("mce_no_restriction_timeframe_maxtime", "0700", "End of the timeframe where all nomination restrictions and cooldowns are disabled (Format: HHMM)", _, true, 0000.0, true, 2359.0);
	g_Cvar_LockNominationsAtWarning = CreateConVar("mce_locknominationswarning", "1", "Lock nominations when the warning start for vote", _, true, 0.0, true, 1.0);
	g_Cvar_TimerUnlockNoms = CreateConVar("mce_locknominations_timer", "15.0", "Unlock nominations after a vote. Time in seconds.", _, true, 0.0, true, 60.0);

	
	RegAdminCmd("sm_mapvote", Command_Mapvote, ADMFLAG_CHANGEMAP, "sm_mapvote - Forces MapChooser to attempt to run a map vote now.");
	RegAdminCmd("sm_setnextmap", Command_SetNextmap, ADMFLAG_CHANGEMAP, "sm_setnextmap <map>");

	// Mapchooser Extended Commands
	RegAdminCmd("mce_reload_maplist", Command_ReloadMaps, ADMFLAG_CHANGEMAP, "mce_reload_maplist - Reload the Official Maplist file.");

	RegConsoleCmd("sm_extends", Command_ExtendsLeft, "sm_extends - Shows how many extends are left on the current map.");
	RegConsoleCmd("sm_extendsleft", Command_ExtendsLeft, "sm_extendsleft - Shows how many extends are left on the current map.");

	g_Cvar_Winlimit = FindConVar("mp_winlimit");
	g_Cvar_Maxrounds = FindConVar("mp_maxrounds");
	g_Cvar_Fraglimit = FindConVar("mp_fraglimit");

	EngineVersion version = GetEngineVersion();

	static char mapListPath[PLATFORM_MAX_PATH];

	BuildPath(Path_SM, mapListPath, PLATFORM_MAX_PATH, "configs/mapchooser_extended/maps/%s.txt", g_GameModName);
	SetMapListCompatBind("official", mapListPath);

	switch(version)
	{
		case Engine_TF2:
		{
			g_Cvar_VoteNextLevel = FindConVar("sv_vote_issue_nextlevel_allowed");
			g_Cvar_Bonusroundtime = FindConVar("mp_bonusroundtime");
		}

		case Engine_CSGO:
		{
			g_Cvar_VoteNextLevel = FindConVar("mp_endmatch_votenextmap");
			g_Cvar_GameType = FindConVar("game_type");
			g_Cvar_GameMode = FindConVar("game_mode");
			g_Cvar_Bonusroundtime = FindConVar("mp_round_restart_delay");
		}

		case Engine_DODS:
		{
			g_Cvar_Bonusroundtime = FindConVar("dod_bonusroundtime");
		}

		case Engine_CSS:
		{
			g_Cvar_Bonusroundtime = FindConVar("mp_round_restart_delay");
		}

		default:
		{
			g_Cvar_Bonusroundtime = FindConVar("mp_bonusroundtime");
		}
	}

	if(g_Cvar_Winlimit != INVALID_HANDLE || g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		switch(version)
		{
			case Engine_TF2:
			{
				HookEvent("teamplay_win_panel", Event_TeamPlayWinPanel);
				HookEvent("teamplay_restart_round", Event_TFRestartRound);
				HookEvent("arena_win_panel", Event_TeamPlayWinPanel);
				HookEvent("pve_win_panel", Event_MvMWinPanel);
			}

			case Engine_NuclearDawn:
			{
				HookEvent("round_win", Event_RoundEnd);
			}

			case Engine_CSGO:
			{
				HookEvent("round_end", Event_RoundEnd);
				HookEvent("cs_win_panel_match", Event_WinPanel);
				HookEvent("cs_intermission", Event_Intermission);
				HookEvent("announce_phase_end", Event_PhaseEnd);
				g_Cvar_MatchClinch = FindConVar("mp_match_can_clinch");
			}

			case Engine_DODS:
			{
				HookEvent("dod_round_win", Event_RoundEnd);
			}

			default:
			{
				HookEvent("round_end", Event_RoundEnd);
				HookEvent("cs_win_panel_match", Event_WinPanel);
			}
		}
	}

	if(g_Cvar_Fraglimit != INVALID_HANDLE)
		HookEvent("player_death", Event_PlayerDeath);

	AutoExecConfig(true, "mapchooser_extended");

	//Change the mp_bonusroundtime max so that we have time to display the vote
	//If you display a vote during bonus time good defaults are 17 vote duration and 19 mp_bonustime
	if(g_Cvar_Bonusroundtime != INVALID_HANDLE)
		SetConVarBounds(g_Cvar_Bonusroundtime, ConVarBound_Upper, true, 30.0);

	g_NominationsResetForward = CreateGlobalForward("OnNominationRemoved", ET_Ignore, Param_String, Param_Cell);
	g_NominationDisconnect= CreateGlobalForward("OnNominationDisconnect", ET_Ignore, Param_String, Param_Cell);
	g_MapVoteStartedForward = CreateGlobalForward("OnMapVoteStarted", ET_Ignore);

	//MapChooser Extended Forwards
	g_MapVoteStartForward = CreateGlobalForward("OnMapVoteStart", ET_Ignore); // Deprecated
	g_MapVoteEndForward = CreateGlobalForward("OnMapVoteEnd", ET_Ignore, Param_String);
	g_MapVoteWarningStartForward = CreateGlobalForward("OnMapVoteWarningStart", ET_Ignore);
	g_MapVoteWarningTickForward = CreateGlobalForward("OnMapVoteWarningTick", ET_Ignore, Param_Cell);
	g_MapVoteRunoffStartForward = CreateGlobalForward("OnMapVoteRunnoffWarningStart", ET_Ignore);

	InternalRestoreMapCooldowns();
}

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	if(LibraryExists("mapchooser"))
	{
		strcopy(error, err_max, "MapChooser already loaded, aborting.");
		return APLRes_Failure;
	}

	RegPluginLibrary("mapchooser");

	MarkNativeAsOptional("GetEngineVersion");

	CreateNative("NominateMap", Native_NominateMap);
	CreateNative("RemoveNominationByMap", Native_RemoveNominationByMap);
	CreateNative("RemoveNominationByOwner", Native_RemoveNominationByOwner);
	CreateNative("InitiateMapChooserVote", Native_InitiateVote);
	CreateNative("CanMapChooserStartVote", Native_CanVoteStart);
	CreateNative("HasEndOfMapVoteFinished", Native_CheckVoteDone);
	CreateNative("GetExcludeMapList", Native_GetExcludeMapList);
	CreateNative("GetNominatedMapList", Native_GetNominatedMapList);
	CreateNative("EndOfMapVoteEnabled", Native_EndOfMapVoteEnabled);

	// MapChooser Extended natives
	CreateNative("IsMapOfficial", Native_IsMapOfficial);
	CreateNative("CanNominate", Native_CanNominate);
	CreateNative("ExcludeMap", Native_ExcludeMap);
	CreateNative("ExcludeMapTime", Native_ExcludeMapTime);
	CreateNative("GetMapCooldown", Native_GetMapCooldown);
	CreateNative("GetMapCooldownTime", Native_GetMapCooldownTime);
	CreateNative("GetMapMinTime", Native_GetMapMinTime);
	CreateNative("GetMapMaxTime", Native_GetMapMaxTime);
	CreateNative("GetMapMinPlayers", Native_GetMapMinPlayers);
	CreateNative("GetMapMaxPlayers", Native_GetMapMaxPlayers);
	CreateNative("GetMapTimeRestriction", Native_GetMapTimeRestriction);
	CreateNative("GetMapPlayerRestriction", Native_GetMapPlayerRestriction);
	CreateNative("GetMapGroups", Native_GetMapGroups);
	CreateNative("GetMapGroupRestriction", Native_GetMapGroupRestriction);
	CreateNative("IsClientMapAdminRestricted", Native_IsClientMapAdminRestricted);
	CreateNative("IsMapAdminRestricted", Native_IsMapAdminRestricted);
	CreateNative("IsClientMapVIPRestricted", Native_IsClientMapVIPRestricted);
	CreateNative("IsMapVIPRestricted", Native_IsMapVIPRestricted);
	CreateNative("IsClientMapLeaderRestricted", Native_IsClientMapLeaderRestricted);
	CreateNative("IsMapLeaderRestricted", Native_IsMapLeaderRestricted);
	CreateNative("GetExtendsLeft", Native_GetExtendsLeft);
	CreateNative("AreRestrictionsActive", Native_AreRestrictionsActive);
	CreateNative("SimulateMapEnd", Native_SimulateMapEnd);

	return APLRes_Success;
}

public void OnAllPluginsLoaded()
{
	g_NativeVotes = LibraryExists(NV) && NativeVotes_IsVoteTypeSupported(NativeVotesType_NextLevelMult);
	g_ZLeader = LibraryExists(ZLEADER);
}

public void OnLibraryAdded(const char[] name)
{
	if (StrEqual(name, NV) && NativeVotes_IsVoteTypeSupported(NativeVotesType_NextLevelMult))
		g_NativeVotes = true;
	if (StrEqual(name, ZLEADER))
		g_ZLeader = true;
}

public void OnLibraryRemoved(const char[] name)
{
	if (StrEqual(name, NV))
		g_NativeVotes = false;
	if (StrEqual(name, ZLEADER))
		g_ZLeader = false;
}

public void OnMapStart()
{
	static char folder[64];
	GetGameFolderName(folder, sizeof(folder));

	g_RoundCounting = RoundCounting_Standard;
	g_ObjectiveEnt = -1;

	if(strcmp(folder, "tf") == 0 && GameRules_GetProp("m_bPlayingMannVsMachine"))
	{
		g_RoundCounting = RoundCounting_MvM;
		g_ObjectiveEnt = EntIndexToEntRef(FindEntityByClassname(-1, "tf_objective_resource"));
	}
	else if(strcmp(folder, "csgo") == 0 && GetConVarInt(g_Cvar_GameType) == GameType_GunGame &&
		GetConVarInt(g_Cvar_GameMode) == GunGameMode_ArmsRace)
	{
		g_RoundCounting = RoundCounting_ArmsRace;
	}

	if(g_Config)
		delete g_Config;

	char sConfigFile[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, sConfigFile, sizeof(sConfigFile), "configs/mapchooser_extended.cfg");
	if(!FileExists(sConfigFile))
	{
		LogMessage("Could not find config: \"%s\"", sConfigFile);
		return;
	}
	LogMessage("Found config: \"%s\"", sConfigFile);

	g_Config = new KeyValues("mapchooser_extended");
	if(!g_Config.ImportFromFile(sConfigFile))
	{
		delete g_Config;
		LogMessage("ImportFromFile() failed!");
		return;
	}
	g_Config.Rewind();

	if(InternalAreRestrictionsActive())
		g_SaveCDOnMapEnd = true;
	else
		g_SaveCDOnMapEnd = false;
}

public void OnConfigsExecuted()
{
	if(ReadMapList(g_MapList,
					 g_mapFileSerial,
					 "mapchooser",
					 MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER)
		!= INVALID_HANDLE)

	{
		if(g_mapFileSerial == -1)
			LogError("Unable to create a valid map list.");
	}

	// Disable the next level vote in TF2 and CS:GO
	// In TF2, this has two effects: 1. Stop the next level vote (which overlaps rtv functionality).
	// 2. Stop the built-in end level vote.  This is the only thing that happens in CS:GO
	if(g_Cvar_VoteNextLevel != INVALID_HANDLE)
		SetConVarBool(g_Cvar_VoteNextLevel, false);

	SetupTimeleftTimer();

	g_TotalRounds = 0;

	g_Extends = 0;

	g_MapVoteCompleted = false;

	g_NominateCount = 0;
	g_NominateReservedCount = 0;
	ClearArray(g_NominateList);
	ClearArray(g_NominateOwners);

	for(int i = 0; i < MAXTEAMS; i++)
		g_winCount[i] = 0;

	/* Check if mapchooser will attempt to start mapvote during bonus round time */
	if((g_Cvar_Bonusroundtime != INVALID_HANDLE) && !GetConVarInt(g_Cvar_StartRounds))
	{
		if(!GetConVarInt(g_Cvar_StartTime) && GetConVarFloat(g_Cvar_Bonusroundtime) <= GetConVarFloat(g_Cvar_VoteDuration))
			LogError("Warning - Bonus Round Time shorter than Vote Time. Votes during bonus round may not have time to complete");
	}

	InitializeOfficialMapList();
}

public void OnMapEnd()
{
	g_HasVoteStarted = false;
	g_WaitingForVote = false;
	g_ChangeMapAtRoundEnd = false;
	g_ChangeMapInProgress = false;
	g_HasIntermissionStarted = false;

	g_VoteTimer = INVALID_HANDLE;
	g_RetryTimer = INVALID_HANDLE;
	g_WarningTimer = INVALID_HANDLE;
	g_RunoffCount = 0;

	static char map[PLATFORM_MAX_PATH];
	int Cooldown;

	if(g_SaveCDOnMapEnd)
	{
		GetCurrentMap(map, PLATFORM_MAX_PATH);
		Cooldown = InternalGetMapCooldown(map);
		g_OldMapList.SetValue(map, Cooldown, true);

		Cooldown = GetTime() + InternalGetMapCooldownTime(map) - RoundToFloor(GetGameTime());
		g_TimeMapList.SetValue(map, Cooldown, true);
	}

	StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
	for(int i = 0; i < OldMapListSnapshot.Length; i++)
	{
		OldMapListSnapshot.GetKey(i, map, sizeof(map));
		g_OldMapList.GetValue(map, Cooldown);

		Cooldown--;
		if(Cooldown > 0)
			g_OldMapList.SetValue(map, Cooldown, true);
		else
			g_OldMapList.Remove(map);
	}
	delete OldMapListSnapshot;

	StringMapSnapshot TimeMapListSnapshot = g_TimeMapList.Snapshot();
	for(int i = 0; i < TimeMapListSnapshot.Length; i++)
	{
		TimeMapListSnapshot.GetKey(i, map, sizeof(map));
		g_TimeMapList.GetValue(map, Cooldown);

		if(Cooldown < GetTime())
			g_TimeMapList.Remove(map);
	}
	delete OldMapListSnapshot;

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
	int index = FindValueInArray(g_NominateOwners, client);

	if(index == -1)
		return;

	char oldmap[PLATFORM_MAX_PATH];
	GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);
	Forward_OnNominationRemoved(oldmap, GetArrayCell(g_NominateOwners, index));
	Forward_OnNominationDisconnect(oldmap, GetArrayCell(g_NominateOwners, index));

	RemoveFromArray(g_NominateOwners, index);
	RemoveFromArray(g_NominateList, index);
	g_NominateCount--;
}

public Action Command_SetNextmap(int client, int args)
{
	if(args < 1)
	{
		CReplyToCommand(client, "{green}[MCE]{default} Usage: {lightgreen}sm_setnextmap <map>");
		return Plugin_Handled;
	}

	static char map[PLATFORM_MAX_PATH];
	GetCmdArg(1, map, PLATFORM_MAX_PATH);

	if(!IsMapValid(map))
	{
		CReplyToCommand(client, "{green}[MCE]{default} %t", "Map was not found", map);
		return Plugin_Handled;
	}

	CShowActivity(client, "%t", "Changed Next Map", map);
	LogAction(client, -1, "[MCE] \"%L\" changed nextmap to \"%s\"", client, map);

	SetNextMap(map);
	g_MapVoteCompleted = true;

	return Plugin_Handled;
}

public Action Command_ReloadMaps(int client, int args)
{
	InitializeOfficialMapList();
	return Plugin_Handled;
}

public Action Command_ExtendsLeft(int client, int args)
{
	CReplyToCommand(client, "{green}[MCE]{default} Available Extends:{green} %d", GetConVarInt(g_Cvar_Extend) - g_Extends);
	return Plugin_Handled;
}

public void OnMapTimeLeftChanged()
{
	if(GetArraySize(g_MapList))
		SetupTimeleftTimer();
}

void SetupTimeleftTimer()
{
	int time;
	if(GetMapTimeLeft(time) && time > 0)
	{
		int startTime;
		if(GetConVarBool(g_Cvar_StartTimePercentEnable))
		{
			int timeLimit;
			if(GetMapTimeLimit(timeLimit) && timeLimit > 0)
			{
				startTime = GetConVarInt(g_Cvar_StartTimePercent) * (timeLimit * 60) / 100;
			}
		}
		else
			startTime = GetConVarInt(g_Cvar_StartTime) * 60;

		if(time - startTime < 0 && GetConVarBool(g_Cvar_EndOfMapVote) && !g_MapVoteCompleted && !g_HasVoteStarted)
		{
			SetupWarningTimer(WarningType_Vote);
		}
		else
		{
			if(g_WarningTimer == INVALID_HANDLE)
			{
				if(g_VoteTimer != INVALID_HANDLE)
				{
					KillTimer(g_VoteTimer);
					g_VoteTimer = INVALID_HANDLE;
				}

				//g_VoteTimer = CreateTimer(float(time - startTime), Timer_StartMapVoteTimer_StartMapVote, _, TIMER_FLAG_NO_MAPCHANGE);
				float interval = float((time - startTime));
				float random = GetRandomFloat(interval, (interval + g_Cvar_RandomStartTime.FloatValue));
				
				g_VoteTimer = CreateTimer(random, Timer_StartWarningTimer, _, TIMER_FLAG_NO_MAPCHANGE);
			}
		}
	}
}

public Action Timer_StartWarningTimer(Handle timer)
{
	g_VoteTimer = INVALID_HANDLE;

	if(!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
		SetupWarningTimer(WarningType_Vote);
	return Plugin_Continue;
}

public Action Timer_StartMapVote(Handle timer, Handle data)
{
	static int timePassed;

	// This is still necessary because InitiateVote still calls this directly via the retry timer
	if(!GetArraySize(g_MapList) || !GetConVarBool(g_Cvar_EndOfMapVote) || g_MapVoteCompleted || g_HasVoteStarted)
	{
		g_WarningTimer = INVALID_HANDLE;
		return Plugin_Stop;
	}

	ResetPack(data);
	int warningMaxTime = ReadPackCell(data);
	int warningTimeRemaining = warningMaxTime - timePassed;

	char warningPhrase[32];
	ReadPackString(data, warningPhrase, sizeof(warningPhrase));

	// Tick timer for external plugins
	Call_StartForward(g_MapVoteWarningTickForward);
	Call_PushCell(warningTimeRemaining);
	Call_Finish();

	if(timePassed == 0 || !GetConVarBool(g_Cvar_HideTimer))
	{
		TimerLocation timerLocation = view_as<TimerLocation>(GetConVarInt(g_Cvar_TimerLocation));

		switch(timerLocation)
		{
			case TimerLocation_Center:
			{
				PrintCenterTextAll("%t", warningPhrase, warningTimeRemaining);
			}

			case TimerLocation_Chat:
			{
				PrintToChatAll("%t", warningPhrase, warningTimeRemaining);
			}

			default:
			{
				PrintHintTextToAll("%t", warningPhrase, warningTimeRemaining);
			}
		}
	}

	if(timePassed++ >= warningMaxTime)
	{
		if(timer == g_RetryTimer)
		{
			g_WaitingForVote = false;
			g_RetryTimer = INVALID_HANDLE;
		}
		else
			g_WarningTimer = INVALID_HANDLE;

		timePassed = 0;
		MapChange mapChange = view_as<MapChange>(ReadPackCell(data));
		Handle hndl = view_as<Handle>(ReadPackCell(data));

		InitiateVote(mapChange, hndl);

		return Plugin_Stop;
	}

	return Plugin_Continue;
}

public void Event_TFRestartRound(Handle event, const char[] name, bool dontBroadcast)
{
	/* Game got restarted - reset our round count tracking */
	g_TotalRounds = 0;
}

public void Event_TeamPlayWinPanel(Handle event, const char[] name, bool dontBroadcast)
{
	if(g_ChangeMapAtRoundEnd)
	{
		g_ChangeMapAtRoundEnd = false;
		CreateTimer(2.0, Timer_ChangeMap, INVALID_HANDLE, TIMER_FLAG_NO_MAPCHANGE);
		g_ChangeMapInProgress = true;
	}

	int bluescore = GetEventInt(event, "blue_score");
	int redscore = GetEventInt(event, "red_score");

	if(GetEventInt(event, "round_complete") == 1 || StrEqual(name, "arena_win_panel"))
	{
		g_TotalRounds++;

		if(!GetArraySize(g_MapList) || g_HasVoteStarted || g_MapVoteCompleted || !GetConVarBool(g_Cvar_EndOfMapVote))
			return;

		CheckMaxRounds(g_TotalRounds);

		switch(GetEventInt(event, "winning_team"))
		{
			case 3:
			{
				CheckWinLimit(bluescore);
			}
			case 2:
			{
				CheckWinLimit(redscore);
			}
			//We need to do nothing on winning_team == 0 this indicates stalemate.
			default:
			{
				return;
			}
		}
	}
}

public void Event_MvMWinPanel(Handle event, const char[] name, bool dontBroadcast)
{
	if(GetEventInt(event, "winning_team") == 2)
	{
		int objectiveEnt = EntRefToEntIndex(g_ObjectiveEnt);
		if(objectiveEnt != INVALID_ENT_REFERENCE)
		{
			g_TotalRounds = GetEntProp(g_ObjectiveEnt, Prop_Send, "m_nMannVsMachineWaveCount");
			CheckMaxRounds(g_TotalRounds);
		}
	}
}

public void Event_Intermission(Handle event, const char[] name, bool dontBroadcast)
{
	g_HasIntermissionStarted = true;
}

public void Event_PhaseEnd(Handle event, const char[] name, bool dontBroadcast)
{
	/* announce_phase_end fires for both half time and the end of the map, but intermission fires first for end of the map. */
	if(g_HasIntermissionStarted)
		return;

	/* No intermission yet, so this must be half time. Swap the score counters. */
	int t_score = g_winCount[2];
	g_winCount[2] =  g_winCount[3];
	g_winCount[3] = t_score;
}

public void Event_WeaponRank(Handle event, const char[] name, bool dontBroadcast)
{
	int rank = GetEventInt(event, "weaponrank");
	if(rank > g_TotalRounds)
	{
		g_TotalRounds = rank;
		CheckMaxRounds(g_TotalRounds);
	}
}

public void Event_WinPanel(Handle event, const char[] name, bool dontBroadcast)
{
	if (g_Cvar_EndOfMapInfo.IntValue != 1)
		return;

	char nextMap[64];
	if(!GetNextMap(nextMap, sizeof(nextMap)))
		return;

	Handle g_hHud = CreateHudSynchronizer();

	if (g_hHud != INVALID_HANDLE)
	{
		SetHudTextParams(-1.0, 0.89, 7.0, 255, 71, 1, 1, 1, 4.0, 0.6, 0.6);
		for(int i = 1; i <= MaxClients; i++)
		{
			if(!IsClientInGame(i) || IsFakeClient(i))
				continue;

			ClearSyncHud(i, g_hHud);
			ShowSyncHudText(i, g_hHud, "Next Map: %s", nextMap);
		}
	}

	CPrintToChatAll("{lightgreen}Next Map: {green}%s", nextMap);	
	CPrintToChatAll("{lightgreen}Next Map: {green}%s", nextMap);	
	CPrintToChatAll("{lightgreen}Next Map: {green}%s", nextMap);

	delete g_hHud;
}

/* You ask, why don't you just use team_score event? And I answer... Because CSS doesn't. */
public void Event_RoundEnd(Handle event, const char[] name, bool dontBroadcast)
{
	if(g_RoundCounting == RoundCounting_ArmsRace)
		return;

	if(g_ChangeMapAtRoundEnd)
	{
		g_ChangeMapAtRoundEnd = false;
		CreateTimer(2.0, Timer_ChangeMap, INVALID_HANDLE, TIMER_FLAG_NO_MAPCHANGE);
		g_ChangeMapInProgress = true;
	}

	int winner;
	if(strcmp(name, "round_win") == 0 || strcmp(name, "dod_round_win") == 0)
		winner = GetEventInt(event, "team"); // Nuclear Dawn & DoD:S
	else
		winner = GetEventInt(event, "winner");

	if(winner == 0 || winner == 1 || !GetConVarBool(g_Cvar_EndOfMapVote))
		return;

	if(winner >= MAXTEAMS)
		SetFailState("Mod exceed maximum team count - Please file a bug report.");

	g_TotalRounds++;

	g_winCount[winner]++;

	if(!GetArraySize(g_MapList) || g_HasVoteStarted || g_MapVoteCompleted)
	{
		return;
	}

	CheckWinLimit(g_winCount[winner]);
	CheckMaxRounds(g_TotalRounds);
}

public void CheckWinLimit(int winner_score)
{
	if(g_Cvar_Winlimit != INVALID_HANDLE)
	{
		int winlimit = GetConVarInt(g_Cvar_Winlimit);
		if(winlimit)
		{
			if(winner_score >= (winlimit - GetConVarInt(g_Cvar_StartRounds)))
			{
				if(!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
				{
					SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
					//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
				}
			}
		}
	}

	if(g_Cvar_MatchClinch != INVALID_HANDLE && g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		bool clinch = GetConVarBool(g_Cvar_MatchClinch);

		if(clinch)
		{
			int maxrounds = GetConVarInt(g_Cvar_Maxrounds);
			int winlimit = RoundFloat(maxrounds / 2.0);

			if(winner_score == winlimit - 1)
			{
				if(!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
				{
					SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
					//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
				}
			}
		}
	}
}

public void CheckMaxRounds(int roundcount)
{
	int maxrounds = 0;

	if(g_RoundCounting == RoundCounting_ArmsRace)
		maxrounds = GameRules_GetProp("m_iNumGunGameProgressiveWeaponsCT");
	else if(g_RoundCounting == RoundCounting_MvM)
		maxrounds = GetEntProp(g_ObjectiveEnt, Prop_Send, "m_nMannVsMachineMaxWaveCount");
	else if(g_Cvar_Maxrounds != INVALID_HANDLE)
		maxrounds = GetConVarInt(g_Cvar_Maxrounds);
	else
		return;

	if(maxrounds)
	{
		if(roundcount >= (maxrounds - GetConVarInt(g_Cvar_StartRounds)))
		{
			if(!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
			{
				SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
				//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
			}
		}
	}
}

public void Event_PlayerDeath(Handle event, const char[] name, bool dontBroadcast)
{
	if(!GetArraySize(g_MapList) || g_Cvar_Fraglimit == INVALID_HANDLE || g_HasVoteStarted)
		return;

	if(!GetConVarInt(g_Cvar_Fraglimit) || !GetConVarBool(g_Cvar_EndOfMapVote))
		return;

	if(g_MapVoteCompleted)
		return;

	int fragger = GetClientOfUserId(GetEventInt(event, "attacker"));

	if(!fragger)
		return;

	if(GetClientFrags(fragger) >= (GetConVarInt(g_Cvar_Fraglimit) - GetConVarInt(g_Cvar_StartFrags)))
	{
		if(!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
		{
			SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
			//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
		}
	}
}

public Action Command_Mapvote(int client, int args)
{
	CShowActivity2(client, "{green}[MCE]{olive} ", "{default}%t", "Initiated Vote Map");
	LogAction(client, -1, "[MCE] \"%L\" Initiated a map vote.", client);

	SetupWarningTimer(WarningType_Vote, MapChange_MapEnd, INVALID_HANDLE, true);

	//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);

	return Plugin_Handled;
}

/**
 * Starts a new map vote
 *
 * @param when			When the resulting map change should occur.
 * @param inputlist		Optional list of maps to use for the vote, otherwise an internal list of nominations + random maps will be used.
 */
void InitiateVote(MapChange when, Handle inputlist=INVALID_HANDLE)
{
	g_WaitingForVote = true;
	g_WarningInProgress = false;
	int MenuRandomShuffleStart = 0;
	int MenuRandomShuffleStop = 0;

	// Check if a vote is in progress first
	if(IsVoteInProgress() || (g_NativeVotes && NativeVotes_IsVoteInProgress()))
	{
		// Can't start a vote, try again in 5 seconds.
		//g_RetryTimer = CreateTimer(5.0, Timer_StartMapVote, _, TIMER_FLAG_NO_MAPCHANGE);

		CPrintToChatAll("{green}[MCE]{default} %t", "Cannot Start Vote", FAILURE_TIMER_LENGTH);
		Handle data;
		g_RetryTimer = CreateDataTimer(1.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);

		/* Mapchooser Extended */
		WritePackCell(data, FAILURE_TIMER_LENGTH);

		if(GetConVarBool(g_Cvar_RunOff) && g_RunoffCount > 0)
			WritePackString(data, "Revote Warning");
		else
			WritePackString(data, "Vote Warning");
		/* End Mapchooser Extended */

		WritePackCell(data, view_as<int>(when));
		WritePackCell(data, view_as<int>(inputlist));
		ResetPack(data);
		return;
	}

	/* If the main map vote has completed (and chosen result) and its currently changing (not a delayed change) we block further attempts */
	if(g_MapVoteCompleted && g_ChangeMapInProgress)
		return;

	CheckMapRestrictions(true, true);
	CreateNextVote();

	g_ChangeTime = when;

	g_WaitingForVote = false;

	g_HasVoteStarted = true;

	if (g_NativeVotes)
	{
		g_VoteMenu = NativeVotes_Create(Handler_MapVoteMenu, NativeVotesType_NextLevelMult, MenuAction_End | MenuAction_VoteCancel | MenuAction_Display | MenuAction_DisplayItem);
		NativeVotes_SetResultCallback(g_VoteMenu, Handler_NativeVoteFinished);
	}
	else
	{
		Handle menuStyle = GetMenuStyleHandle(MenuStyle:GetConVarInt(g_Cvar_MenuStyle));
		if (menuStyle != INVALID_HANDLE)
		{
			g_VoteMenu = CreateMenuEx(menuStyle, Handler_MapVoteMenu, MenuAction_End | MenuAction_Display | MenuAction_DisplayItem | MenuAction_VoteCancel);
		}
		else
		{
			// You chose... poorly
			g_VoteMenu = CreateMenu(Handler_MapVoteMenu, MenuAction_End | MenuAction_Display | MenuAction_DisplayItem | MenuAction_VoteCancel);
		}

		g_AddNoVote = GetConVarBool(g_Cvar_NoVoteOption);

		// Block Vote Slots
		if(GetConVarBool(g_Cvar_BlockSlots))
		{
			Handle radioStyle = GetMenuStyleHandle(MenuStyle_Radio);

			if(GetMenuStyle(g_VoteMenu) == radioStyle)
			{
				g_BlockedSlots = true;
				AddMenuItem(g_VoteMenu, LINE_ONE, "Choose something...", ITEMDRAW_DISABLED);
				AddMenuItem(g_VoteMenu, LINE_TWO, "...will ya?", ITEMDRAW_DISABLED);
				MenuRandomShuffleStart += 2;

				if(!g_AddNoVote) {
					AddMenuItem(g_VoteMenu, LINE_SPACER, "", ITEMDRAW_SPACER);
					MenuRandomShuffleStart++;
				}
			}
			else
				g_BlockedSlots = false;
		}

		if(g_AddNoVote)
			SetMenuOptionFlags(g_VoteMenu, MENUFLAG_BUTTON_NOVOTE);

		SetMenuTitle(g_VoteMenu, "Vote Nextmap");
		SetVoteResultCallback(g_VoteMenu, Handler_MapVoteFinished);
	}

	/* Call OnMapVoteStarted() Forward */
	//	Call_StartForward(g_MapVoteStartedForward);
	//	Call_Finish();

	/**
	 * TODO: Make a proper decision on when to clear the nominations list.
	 * Currently it clears when used, and stays if an external list is provided.
	 * Is this the right thing to do? External lists will probably come from places
	 * like sm_mapvote from the adminmenu in the future.
	 */

	static char map[PLATFORM_MAX_PATH];

	/* No input given - User our internal nominations and maplist */
	if(inputlist == INVALID_HANDLE)
	{
		int allMapsSize = (PLATFORM_MAX_PATH * 3);
		char[] allMapsBuffer = new char[allMapsSize];
		
		Handle randomizeList = INVALID_HANDLE;
		if(GetConVarBool(g_Cvar_RandomizeNominations))
			randomizeList = CloneArray(g_NominateList);

		int nominateCount = GetArraySize(g_NominateList);

		int voteSize = GetVoteSize(2);

		// The if and else if could be combined, but it looks extremely messy
		// This is a hack to lower the vote menu size by 1 when Don't Change or Extend Map should appear
		if (g_NativeVotes)
		{
			if ((when == MapChange_Instant || when == MapChange_RoundEnd) && GetConVarBool(g_Cvar_DontChange))
			{
				voteSize--;
			}
			else if (GetConVarBool(g_Cvar_Extend) && g_Extends < GetConVarInt(g_Cvar_Extend))
			{
				voteSize--;
			}
		}

		/* Smaller of the two - It should be impossible for nominations to exceed the size though (cvar changed mid-map?) */
		int nominationsToAdd = nominateCount >= voteSize ? voteSize : nominateCount;

		bool extendFirst = GetConVarBool(g_Cvar_ExtendPosition);

		if(extendFirst) {
			AddExtendToMenu(g_VoteMenu, when);
			MenuRandomShuffleStart++;
		}

		for(int i = 0; i < nominationsToAdd; i++)
		{
			GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);

			if(randomizeList == INVALID_HANDLE)
			{
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}

			RemoveStringFromArray(g_NextMapList, map);

			/* Notify Nominations that this map is now free */
			Forward_OnNominationRemoved(map, GetArrayCell(g_NominateOwners, i));
		}

		/* Clear out the rest of the nominations array */
		for(int i = nominationsToAdd; i < nominateCount; i++)
		{
			GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);
			/* These maps shouldn't be excluded from the vote as they weren't really nominated at all */

			/* Notify Nominations that this map is now free */
			Forward_OnNominationRemoved(map, GetArrayCell(g_NominateOwners, i));
		}

		/* There should currently be 'nominationsToAdd' unique maps in the vote */

		int i = nominationsToAdd;
		int count = 0;
		int availableMaps = GetArraySize(g_NextMapList);

		if(i < voteSize && availableMaps == 0)
		{
			if(i == 0)
			{
				LogError("No maps available for vote.");
				return;
			}
			else
			{
				LogMessage("Not enough maps to fill map list, reducing map count. Adjust mce_include and mce_exclude to avoid this warning.");
				voteSize = i;
			}
		}

		while(i < voteSize)
		{
			GetArrayString(g_NextMapList, count, map, PLATFORM_MAX_PATH);
			count++;

			if(randomizeList == INVALID_HANDLE)
			{
				/* Insert the map and increment our count */
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}
			else
				PushArrayString(randomizeList, map);
			i++;

			//Run out of maps, this will have to do.
			if(count >= availableMaps)
				break;
		}

		if(randomizeList != INVALID_HANDLE)
		{
			// Fisher-Yates Shuffle
			for(int j = GetArraySize(randomizeList) - 1; j >= 1; j--)
			{
				int k = GetRandomInt(0, j);
				SwapArrayItems(randomizeList, j, k);
			}

			for(int j = 0; j < GetArraySize(randomizeList); j++)
			{
				GetArrayString(randomizeList, j, map, PLATFORM_MAX_PATH);
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}

			delete randomizeList;
			randomizeList = INVALID_HANDLE;
		}

		/* Wipe out our nominations list - Nominations have already been informed of this */
		g_NominateCount = 0;
		g_NominateReservedCount = 0;
		ClearArray(g_NominateOwners);
		ClearArray(g_NominateList);

		if(!extendFirst) {
			if (GetConVarInt(g_Cvar_Extend) - g_Extends > 0)
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, "Extend");
			AddExtendToMenu(g_VoteMenu, when);
			MenuRandomShuffleStop++;
		}
		
		if(allMapsBuffer[0])
			LogAction(-1, -1, "[MCE] Maps List: %s", allMapsBuffer);
	}
	else //We were given a list of maps to start the vote with
	{
		int size = GetArraySize(inputlist);

		int allMapsSize = (PLATFORM_MAX_PATH * 3);
		char[] allMapsBuffer = new char[allMapsSize];
		
		for(int i = 0; i < size; i++)
		{
			GetArrayString(inputlist, i, map, PLATFORM_MAX_PATH);

			if(IsMapValid(map))
			{
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}
			// New in Mapchooser Extended
			else if(StrEqual(map, VOTE_DONTCHANGE))
			{
				if (g_NativeVotes)
				{
					NativeVotes_AddItem(g_VoteMenu, VOTE_DONTCHANGE, "Don't Change");
				}
				else
				{
					AddMenuItem(g_VoteMenu, VOTE_DONTCHANGE, "Don't Change");
				}
			}
			else if(StrEqual(map, VOTE_EXTEND))
			{
				if (GetConVarInt(g_Cvar_Extend) - g_Extends > 0)
					FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, "Extend");

				if (g_NativeVotes)
				{
					NativeVotes_AddItem(g_VoteMenu, VOTE_EXTEND, "Extend Map");
				}
				else
				{
					AddMenuItem(g_VoteMenu, VOTE_EXTEND, "Extend Map");
				}
			}
		}
		delete inputlist;

		if(allMapsBuffer[0])
			LogAction(-1, -1, "[MCE] Maps List: %s", allMapsBuffer);
	}

	int voteDuration = GetConVarInt(g_Cvar_VoteDuration);

	if (g_NativeVotes)
	{
		NativeVotes_DisplayToAll(g_VoteMenu, voteDuration);
	}
	else
	{
		//SetMenuExitButton(g_VoteMenu, false);

		if(GetVoteSize(2) <= GetMaxPageItems(GetMenuStyle(g_VoteMenu)))
		{
			//This is necessary to get items 9 and 0 as usable voting items
			SetMenuPagination(g_VoteMenu, MENU_NO_PAGINATION);
		}

		if(GetConVarInt(g_Cvar_ShufflePerClient))
			MenuShufflePerClient(g_VoteMenu, MenuRandomShuffleStart, GetMenuItemCount(g_VoteMenu) - MenuRandomShuffleStop);

		VoteMenuToAll(g_VoteMenu, voteDuration);
	}

	/* Call OnMapVoteStarted() Forward */
	Call_StartForward(g_MapVoteStartForward); // Deprecated
	Call_Finish();

	Call_StartForward(g_MapVoteStartedForward);
	Call_Finish();

	CPrintToChatAll("{green}[MCE]{default} %t", "Nextmap Voting Started");
}

public void Handler_VoteFinishedGeneric(Handle menu,
										int num_votes,
										int num_clients,
										const int[][] client_info,
										int num_items,
										const int[][] item_info)
{
	static char map[PLATFORM_MAX_PATH];
	GetMapItem(menu, item_info[0][VOTEINFO_ITEM_INDEX], map, PLATFORM_MAX_PATH);

	Call_StartForward(g_MapVoteEndForward);
	Call_PushString(map);
	Call_Finish();

	if(strcmp(map, VOTE_EXTEND, false) == 0)
	{
		g_Extends++;

		int time;
		if(GetMapTimeLimit(time))
		{
			if(time > 0)
				ExtendMapTimeLimit(GetConVarInt(g_Cvar_ExtendTimeStep)*60);
		}

		if(g_Cvar_Winlimit != INVALID_HANDLE)
		{
			int winlimit = GetConVarInt(g_Cvar_Winlimit);
			if(winlimit)
				SetConVarInt(g_Cvar_Winlimit, winlimit + GetConVarInt(g_Cvar_ExtendRoundStep));
		}

		if(g_Cvar_Maxrounds != INVALID_HANDLE)
		{
			int maxrounds = GetConVarInt(g_Cvar_Maxrounds);
			if(maxrounds)
				SetConVarInt(g_Cvar_Maxrounds, maxrounds + GetConVarInt(g_Cvar_ExtendRoundStep));
		}

		if(g_Cvar_Fraglimit != INVALID_HANDLE)
		{
			int fraglimit = GetConVarInt(g_Cvar_Fraglimit);
			if(fraglimit)
				SetConVarInt(g_Cvar_Fraglimit, fraglimit + GetConVarInt(g_Cvar_ExtendFragStep));
		}

		if (g_NativeVotes)
		{
			NativeVotes_DisplayPassEx(menu, NativeVotesPass_Extend);
		}

		CPrintToChatAll("{green}[MCE]{default} %t", "Current Map Extended", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);
		LogAction(-1, -1, "[MCE] Voting for next map has finished. \nThe current map has been extended. (Received \"%d\"\%% of %d votes) \nAvailable Extends: %d", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes, GetConVarInt(g_Cvar_Extend) - g_Extends);
		CPrintToChatAll("{green}[MCE]{default} Available Extends:{green} %d", GetConVarInt(g_Cvar_Extend) - g_Extends);
		if(g_Cvar_LockNominationsAtWarning.IntValue > 0)
		{
			CreateTimer(GetConVarFloat(g_Cvar_TimerUnlockNoms), UnlockNominations, _, TIMER_FLAG_NO_MAPCHANGE);
		}

		// We extended, so we'll have to vote again.
		g_RunoffCount = 0;
		g_HasVoteStarted = false;
		SetupTimeleftTimer();

	}
	else if(strcmp(map, VOTE_DONTCHANGE, false) == 0)
	{
		if (g_NativeVotes)
		{
			NativeVotes_DisplayPassEx(menu, NativeVotesPass_Extend);
		}

		CPrintToChatAll("{green}[MCE]{default} %t", "Current Map Stays", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);
		LogAction(-1, -1, "[MCE] Current map continues! The Vote has spoken! (Received \"%d\"\%% of %d votes)", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);

		if(g_Cvar_LockNominationsAtWarning.IntValue > 0)
		{
			CreateTimer(GetConVarFloat(g_Cvar_TimerUnlockNoms), UnlockNominations, _, TIMER_FLAG_NO_MAPCHANGE);
		}

		g_RunoffCount = 0;
		g_HasVoteStarted = false;
		SetupTimeleftTimer();
	}
	else
	{
		if(g_ChangeTime == MapChange_MapEnd)
		{
			SetNextMap(map);
			if (g_NativeVotes)
			{
				NativeVotes_DisplayPass(menu, map);
			}
		}
		else if(g_ChangeTime == MapChange_Instant)
		{
			Handle data;
			CreateDataTimer(4.0, Timer_ChangeMap, data);
			WritePackString(data, map);
			g_ChangeMapInProgress = false;
			if (g_NativeVotes)
			{
				NativeVotes_DisplayPassEx(menu, NativeVotesPass_ChgLevel, map);
			}
		}
		else // MapChange_RoundEnd
		{
			SetNextMap(map);
			g_ChangeMapAtRoundEnd = true;

			if (g_NativeVotes)
			{
				NativeVotes_DisplayPass(menu, map);
			}
		}

		g_HasVoteStarted = false;
		g_MapVoteCompleted = true;

		CPrintToChatAll("{green}[MCE]{default} %t", "Nextmap Voting Finished", map, RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);
		LogAction(-1, -1, "[MCE] Voting for next map has finished. \nNextmap: %s. (Received \"%d\"\%% of %d votes)", map, RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);
	}
}

public void Handler_MapVoteFinished(Handle menu,
									int num_votes,
									int num_clients,
									const int[][] client_info,
									int num_items,
									const int[][] item_info)
{
	// Implement revote logic - Only run this` block if revotes are enabled and this isn't the last revote
	if(GetConVarBool(g_Cvar_RunOff) && num_items > 1 && g_RunoffCount < GetConVarInt(g_Cvar_MaxRunOffs))
	{
		g_RunoffCount++;
		int highest_votes = item_info[0][VOTEINFO_ITEM_VOTES];
		int required_percent = GetConVarInt(g_Cvar_RunOffPercent);
		int required_votes = RoundToCeil(float(num_votes) * float(required_percent) / 100.0);

		if(highest_votes == item_info[1][VOTEINFO_ITEM_VOTES])
		{
			g_HasVoteStarted = false;

			//Revote is needed
			ArrayList mapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH + 1));

			for(int i = 0; i < num_items; i++)
			{
				if(item_info[i][VOTEINFO_ITEM_VOTES] == highest_votes)
				{
					static char map[PLATFORM_MAX_PATH];

					GetMapItem(menu, item_info[i][VOTEINFO_ITEM_INDEX], map, PLATFORM_MAX_PATH);
					PushArrayString(mapList, map);
				}
				else
					break;
			}

			if (g_NativeVotes)
			{
				NativeVotes_DisplayFail(menu, NativeVotesFail_NotEnoughVotes);
			}

			LogAction(-1, -1, "[MCE] The top maps had the same number of votes. A revote is needed!");
			CPrintToChatAll("{green}[MCE]{default} %t", "Tie Vote", GetArraySize(mapList));
			SetupWarningTimer(WarningType_Revote, view_as<MapChange>(g_ChangeTime), mapList);
			return;
		}
		else if(highest_votes < required_votes)
		{
			g_HasVoteStarted = false;

			//Revote is needed
			ArrayList mapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH + 1));

			static char map1[PLATFORM_MAX_PATH];
			GetMapItem(menu, item_info[0][VOTEINFO_ITEM_INDEX], map1, PLATFORM_MAX_PATH);

			PushArrayString(mapList, map1);

			// We allow more than two maps for a revote if they are tied
			for(int i = 1; i < num_items; i++)
			{
				if(GetArraySize(mapList) < 2 || item_info[i][VOTEINFO_ITEM_VOTES] == item_info[i - 1][VOTEINFO_ITEM_VOTES])
				{
					static char map[PLATFORM_MAX_PATH];
					GetMapItem(menu, item_info[i][VOTEINFO_ITEM_INDEX], map, PLATFORM_MAX_PATH);
					mapList.PushString(map);
				}
				else
					break;
			}

			if (g_NativeVotes)
			{
				NativeVotes_DisplayFail(menu, NativeVotesFail_NotEnoughVotes);
			}

			LogAction(-1, -1, "[MCE] No map has received more than \"%d\"\%% of the vote.\nA revote is needed!", required_percent);
			CPrintToChatAll("{green}[MCE]{default} %t", "Revote Is Needed", required_percent);
			SetupWarningTimer(WarningType_Revote, view_as<MapChange>(g_ChangeTime), mapList);
			return;
		}
	}

	// No revote needed, continue as normal.
	Handler_VoteFinishedGeneric(menu, num_votes, num_clients, client_info, num_items, item_info);
}

public int Handler_MapVoteMenu(Handle menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
		{
			g_VoteMenu = INVALID_HANDLE;
			if (g_NativeVotes)
			{
				NativeVotes_Close(menu);
			}
			else
			{
				delete menu;
			}
		}

		case MenuAction_Display:
		{
			// NativeVotes uses the standard TF2/CSGO vote screen
			if (!g_NativeVotes)
			{
				static char buffer[255];
				Format(buffer, sizeof(buffer), "%T", "Vote Nextmap", param1);
				Handle panel = view_as<Handle>(param2);
				SetPanelTitle(panel, buffer);
				//DrawPanelText(panel, "Warning: The Position of the Maps are different for each Player.");
			}
		}

		case MenuAction_DisplayItem:
		{
			char map[PLATFORM_MAX_PATH];
			char buffer[255];
			int mark = GetConVarInt(g_Cvar_MarkCustomMaps);

			if (g_NativeVotes)
			{
				NativeVotes_GetItem(menu, param2, map, PLATFORM_MAX_PATH);
			}
			else
			{
				GetMenuItem(menu, param2, map, PLATFORM_MAX_PATH, _, _, _, param1);
			}

			if(StrEqual(map, VOTE_EXTEND, false))
			{
				Format(buffer, sizeof(buffer), "%T", "Extend Map", param1);
			}
			else if(StrEqual(map, VOTE_DONTCHANGE, false))
			{
				Format(buffer, sizeof(buffer), "%T", "Dont Change", param1);
			}
			// Mapchooser Extended
			else if(StrEqual(map, LINE_ONE, false))
			{
				Format(buffer, sizeof(buffer),"%T", "Line One", param1);
			}
			else if(StrEqual(map, LINE_TWO, false))
			{
				Format(buffer, sizeof(buffer),"%T", "Line Two", param1);
			}
			// Note that the first part is to discard the spacer line
			else if(!StrEqual(map, LINE_SPACER, false))
			{
				if(mark == 1 && !InternalIsMapOfficial(map))
				{
					Format(buffer, sizeof(buffer), "%T", "Custom Marked", param1, map);
				}
				else if(mark == 2 && !InternalIsMapOfficial(map))
				{
					Format(buffer, sizeof(buffer), "%T", "Custom", param1, map);
				}
				else if(InternalGetMapVIPRestriction(map))
				{
					Format(buffer, sizeof(buffer), "%s (%T)", map, "VIP Nomination", param1);
				}
				else if(InternalGetMapLeaderRestriction(map))
				{
					Format(buffer, sizeof(buffer), "%s (%T)", map, "Leader Nomination", param1);
				}
			}

			if(buffer[0] != '\0')
			{
				if (g_NativeVotes)
				{
					NativeVotes_RedrawVoteItem(buffer);
					return _:Plugin_Continue;
				}
				else
				{
					return RedrawMenuItem(buffer);
				}
			}
			// End Mapchooser Extended
		}

		case MenuAction_VoteCancel:
		{
			// If we receive 0 votes, pick at random.
			if(param1 == VoteCancel_NoVotes && GetConVarBool(g_Cvar_NoVoteMode))
			{
				int count;
				if (g_NativeVotes)
				{
					count = NativeVotes_GetItemCount(menu);
				}
				else
				{
					count = GetMenuItemCount(menu);
				}

				int item;
				static char map[PLATFORM_MAX_PATH];

				do
				{
					int startInt = 0;
					if (!g_NativeVotes && g_BlockedSlots)
					{
						if(g_AddNoVote)
						{
							startInt = 2;
						}
						else
						{
							startInt = 3;
						}
					}
					item = GetRandomInt(startInt, count - 1);
					if (g_NativeVotes)
					{
						NativeVotes_GetItem(menu, item, map, PLATFORM_MAX_PATH);
					}
					else
					{
						GetMenuItem(menu, item, map, PLATFORM_MAX_PATH, _, _, _, param1);
					}
				}
				while(strcmp(map, VOTE_EXTEND, false) == 0);

				SetNextMap(map);
				LogAction(-1, -1, "[MCE] No votes has been receive. Pickup a random map. Nextmap is : %s", map);
				g_MapVoteCompleted = true;
				if (g_NativeVotes)
				{
					NativeVotes_DisplayPass(menu, map);
				}
			}
			else if (g_NativeVotes)
			{
				NativeVotesFailType reason = NativeVotesFail_Generic;
				if (param1 == VoteCancel_NoVotes)
				{
					reason = NativeVotesFail_NotEnoughVotes;
				}
				
				NativeVotes_DisplayFail(menu, reason);
			}
			else
				LogAction(-1, -1, "[MCE] No votes has been receive.");

			g_HasVoteStarted = false;
		}
	}

	return 0;
}

public Action Timer_ChangeMap(Handle hTimer, Handle dp)
{
	g_ChangeMapInProgress = false;

	char map[PLATFORM_MAX_PATH];

	if(dp == INVALID_HANDLE)
	{
		if(!GetNextMap(map, PLATFORM_MAX_PATH))
		{
			//No passed map and no set nextmap. fail!
			return Plugin_Stop;
		}
	}
	else
	{
		ResetPack(dp);
		ReadPackString(dp, map, PLATFORM_MAX_PATH);
	}

	ForceChangeLevel(map, "Map Vote");

	return Plugin_Stop;
}

bool RemoveStringFromArray(Handle array, char[] str)
{
	int index = FindStringInArray(array, str);
	if(index != -1)
	{
		RemoveFromArray(array, index);
		return true;
	}

	return false;
}

public Action UnlockNominations(Handle timer)
{
#if defined _nominations_extended_included_
	ToggleNominations(false);
#endif
	return Plugin_Continue;
}

void LockNominations()
{
#if defined _nominations_extended_included_
	ToggleNominations(true);
#endif
}

void CreateNextVote()
{
	assert(g_NextMapList)
	ClearArray(g_NextMapList);

	static char map[PLATFORM_MAX_PATH];
	Handle tempMaps = CloneArray(g_MapList);

	GetCurrentMap(map, PLATFORM_MAX_PATH);
	RemoveStringFromArray(tempMaps, map);

	if(GetArraySize(tempMaps) > GetConVarInt(g_Cvar_ExcludeMaps) && InternalAreRestrictionsActive())
	{
		StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
		for(int i = 0; i < OldMapListSnapshot.Length; i++)
		{
			OldMapListSnapshot.GetKey(i, map, sizeof(map));
			RemoveStringFromArray(tempMaps, map);
		}
		delete OldMapListSnapshot;
	}

	if(InternalAreRestrictionsActive())
	{
		StringMapSnapshot TimeMapListSnapshot = g_TimeMapList.Snapshot();
		for(int i = 0; i < TimeMapListSnapshot.Length; i++)
		{
			TimeMapListSnapshot.GetKey(i, map, sizeof(map));
			int Cooldown;
			g_TimeMapList.GetValue(map, Cooldown);

			if(Cooldown > GetTime())
				RemoveStringFromArray(tempMaps, map);
		}
		delete TimeMapListSnapshot;
	}

	int voteSize = GetVoteSize(2);
	int limit = (voteSize < GetArraySize(tempMaps) ? voteSize : GetArraySize(tempMaps));

	// group -> number of maps nominated from group
	StringMap groupmap = new StringMap();
	char groupstr[8];

	// populate groupmap with maps from nomination list
	static char map_[PLATFORM_MAX_PATH];
	int groups_[32];
	for(int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		GetArrayString(g_NominateList, i, map_, PLATFORM_MAX_PATH);
		int groupsfound = InternalGetMapGroups(map_, groups_, sizeof(groups_));
		for(int group = 0; group < groupsfound; group++)
		{
			IntToString(group, groupstr, sizeof(groupstr));
			int groupcur = 0;
			groupmap.GetValue(groupstr, groupcur);
			groupcur++;
			groupmap.SetValue(groupstr, groupcur, true);
		}
	}

	// find random maps which honor all restrictions
	for(int i = 0; i < limit; i++)
	{
		int b;
		for(int j = 0; j < 1000; j++)
		{
			b = GetRandomInt(0, GetArraySize(tempMaps) - 1);
			GetArrayString(tempMaps, b, map, PLATFORM_MAX_PATH);

			if(!InternalAreRestrictionsActive())
				break;

			if(InternalGetMapVIPRestriction(map))
				continue;

			if(InternalGetMapLeaderRestriction(map))
				continue;

			if(InternalGetMapTimeRestriction(map) != 0)
				continue;

			if(InternalGetMapPlayerRestriction(map) != 0)
				continue;

			bool okay = true;

			int groups[32];
			int groupsfound = InternalGetMapGroups(map, groups, sizeof(groups));
			for(int group = 0; group < groupsfound; group++)
			{
				IntToString(group, groupstr, sizeof(groupstr));

				int groupmax = InternalGetGroupMax(groups[group]);
				if(groupmax >= 0)
				{
					int groupcur = 0;
					groupmap.GetValue(groupstr, groupcur);

					if(groupcur >= groupmax)
					{
						okay = false;
						break;
					}

					groupcur++;
					groupmap.SetValue(groupstr, groupcur, true);
				}
			}

			if(okay)
				break;
		}
		PushArrayString(g_NextMapList, map);
		RemoveFromArray(tempMaps, b);
	}

	delete groupmap;
	delete tempMaps;
}

bool CanVoteStart()
{
	if(g_WaitingForVote || g_HasVoteStarted)
		return false;

	return true;
}

NominateResult InternalNominateMap(char[] map, bool force, int owner)
{
	if(!IsMapValid(map))
	{
		return Nominate_InvalidMap;
	}

	/* Map already in the vote */
	if(FindStringInArray(g_NominateList, map) != -1)
	{
		return Nominate_AlreadyInVote;
	}

	int index;

	/* Look to replace an existing nomination by this client - Nominations made with owner = 0 aren't replaced */
	if(owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);
		Forward_OnNominationRemoved(oldmap, owner);

		SetArrayString(g_NominateList, index, map);
		return Nominate_Replaced;
	}

	/* Too many nominated maps. */
	if(g_NominateCount >= GetVoteSize(0) && !force)
	{
		return Nominate_VoteFull;
	}

	PushArrayString(g_NominateList, map);
	PushArrayCell(g_NominateOwners, owner);
	if(owner == 0 && g_NominateReservedCount < GetVoteSize(1))
		g_NominateReservedCount++;
	else
		g_NominateCount++;

	while(GetArraySize(g_NominateList) > GetVoteSize(2))
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, 0, oldmap, PLATFORM_MAX_PATH);
		int owner_ = GetArrayCell(g_NominateOwners, 0);

		Forward_OnNominationRemoved(oldmap, owner_);

		RemoveFromArray(g_NominateList, 0);
		RemoveFromArray(g_NominateOwners, 0);
		if(owner_ == 0)
			g_NominateReservedCount--;
		else
			g_NominateCount--;
	}

	return Nominate_Added;
}

/* Add natives to allow nominate and initiate vote to be call */

/* native  bool NominateMap(const char[] map, bool force, &NominateError:error); */
public int Native_NominateMap(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return view_as<int>(InternalNominateMap(map, GetNativeCell(2), GetNativeCell(3)));
}

bool InternalRemoveNominationByMap(char[] map)
{
	for(int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, i, oldmap, PLATFORM_MAX_PATH);

		if(strcmp(map, oldmap, false) == 0)
		{
			Forward_OnNominationRemoved(oldmap, GetArrayCell(g_NominateOwners, i));

			int owner = GetArrayCell(g_NominateOwners, i);
			if(owner)
				g_NominateCount--;
			else
				g_NominateReservedCount--;

			RemoveFromArray(g_NominateList, i);
			RemoveFromArray(g_NominateOwners, i);

			return true;
		}
	}

	return false;
}

/* native  bool RemoveNominationByMap(const char[] map); */
public int Native_RemoveNominationByMap(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return view_as<int>(InternalRemoveNominationByMap(map));
}

bool InternalRemoveNominationByOwner(int owner)
{
	int index;

	if(owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);

		Forward_OnNominationRemoved(oldmap, owner);

		RemoveFromArray(g_NominateList, index);
		RemoveFromArray(g_NominateOwners, index);
		g_NominateCount--;

		return true;
	}

	return false;
}

/* native  bool RemoveNominationByOwner(owner); */
public int Native_RemoveNominationByOwner(Handle plugin, int numParams)
{
	return view_as<int>(InternalRemoveNominationByOwner(GetNativeCell(1)));
}

/* native InitiateMapChooserVote(); */
public int Native_InitiateVote(Handle plugin, int numParams)
{
	MapChange when = view_as<MapChange>(GetNativeCell(1));
	Handle inputarray = view_as<Handle>(GetNativeCell(2));

	LogAction(-1, -1, "[MCE] Starting map vote because players RTV.");

	SetupWarningTimer(WarningType_Vote, when, inputarray);
	//InitiateVote(when, inputarray);
	return 0;
}

public int Native_CanVoteStart(Handle plugin, int numParams)
{
	return CanVoteStart();
}

public int Native_CheckVoteDone(Handle plugin, int numParams)
{
	return g_MapVoteCompleted;
}

public int Native_EndOfMapVoteEnabled(Handle plugin, int numParams)
{
	return GetConVarBool(g_Cvar_EndOfMapVote);
}

public int Native_GetExcludeMapList(Handle plugin, int numParams)
{
	Handle array = view_as<Handle>(GetNativeCell(1));
	if(array == INVALID_HANDLE)
		return 0;

	static char map[PLATFORM_MAX_PATH];
	StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
	for(int i = 0; i < OldMapListSnapshot.Length; i++)
	{
		OldMapListSnapshot.GetKey(i, map, sizeof(map));
		PushArrayString(array, map);
	}
	delete OldMapListSnapshot;
	return 0;
}

public int Native_GetNominatedMapList(Handle plugin, int numParams)
{
	Handle maparray = view_as<Handle>(GetNativeCell(1));
	Handle ownerarray = view_as<Handle>(GetNativeCell(2));

	if(maparray == INVALID_HANDLE)
		return 0;

	static char map[PLATFORM_MAX_PATH];

	for(int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);
		PushArrayString(maparray, map);

		// If the optional parameter for an owner list was passed, then we need to fill that out as well
		if(ownerarray != INVALID_HANDLE)
		{
			int index = GetArrayCell(g_NominateOwners, i);
			PushArrayCell(ownerarray, index);
		}
	}
	return 0;
}

// Functions new to Mapchooser Extended
stock void SetupWarningTimer(WarningType type, MapChange when=MapChange_MapEnd, Handle mapList=INVALID_HANDLE, bool force=false)
{
	if(!GetArraySize(g_MapList) || g_ChangeMapInProgress || g_HasVoteStarted || (!force && ((when == MapChange_MapEnd && !GetConVarBool(g_Cvar_EndOfMapVote)) || g_MapVoteCompleted)))
		return;

	bool interrupted = false;
	if(g_WarningInProgress && g_WarningTimer != INVALID_HANDLE)
	{
		interrupted = true;
		KillTimer(g_WarningTimer);
	}

	g_WarningInProgress = true;

	if(g_Cvar_LockNominationsAtWarning.IntValue > 0)
		LockNominations();

	Handle forwardVote;
	Handle cvarTime;
	static char translationKey[64];

	switch(type)
	{
		case WarningType_Vote:
		{
			forwardVote = g_MapVoteWarningStartForward;
			cvarTime = g_Cvar_WarningTime;
			strcopy(translationKey, sizeof(translationKey), "Vote Warning");
			LogAction(-1, -1, "[MCE] Voting for next map has started.");
		}

		case WarningType_Revote:
		{
			forwardVote = g_MapVoteRunoffStartForward;
			cvarTime = g_Cvar_RunOffWarningTime;
			strcopy(translationKey, sizeof(translationKey), "Revote Warning");
			LogAction(-1, -1, "[MCE] Revote for next map has started.");
		}
	}

	if(!interrupted)
	{
		Call_StartForward(forwardVote);
		Call_Finish();
	}

	Handle data;
	g_WarningTimer = CreateDataTimer(1.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);
	WritePackCell(data, GetConVarInt(cvarTime));
	WritePackString(data, translationKey);
	WritePackCell(data, view_as<int>(when));
	WritePackCell(data, view_as<int>(mapList));
	ResetPack(data);
}

stock void InitializeOfficialMapList()
{
	// If this fails, we want it to have an empty adt_array
	if(ReadMapList(g_OfficialList,
		g_mapOfficialFileSerial,
		"official",
		MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT)
		!= INVALID_HANDLE)
	{
		LogMessage("Loaded map list for %s.", g_GameModName);
	}
	// Check if the map list was ever loaded
	else if(g_mapOfficialFileSerial == -1)
	{
		LogMessage("No official map list found for %s. Consider submitting one!", g_GameModName);
	}
}

stock bool IsMapEndVoteAllowed()
{
	if(!GetConVarBool(g_Cvar_EndOfMapVote) || g_MapVoteCompleted || g_HasVoteStarted)
		return false;
	return true;
}

public int Native_IsMapOfficial(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalIsMapOfficial(map);
}

bool InternalIsMapOfficial(const char[] mapname)
{
	int officialMapIndex = FindStringInArray(g_OfficialList, mapname);
	return (officialMapIndex > -1);
}

public int Native_IsWarningTimer(Handle plugin, int numParams)
{
	return g_WarningInProgress;
}

public int Native_CanNominate(Handle plugin, int numParams)
{
	if(g_HasVoteStarted)
	{
		return view_as<int>(CanNominate_No_VoteInProgress);
	}

	if(g_MapVoteCompleted)
	{
		return view_as<int>(CanNominate_No_VoteComplete);
	}

	if(g_NominateCount >= GetVoteSize())
	{
		return view_as<int>(CanNominate_No_VoteFull);
	}

	return view_as<int>(CanNominate_Yes);
}

public int Native_ExcludeMap(Handle plugin, int numParams)
{
	if(!InternalAreRestrictionsActive())
		return true;

	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown;
	int Mode = GetNativeCell(3);

	if(Mode == 0)
	{
		Cooldown = InternalGetMapCooldown(map);
	}
	else if(Mode == 1)
	{
		Cooldown = GetNativeCell(2);
	}
	else if(Mode == 2)
	{
		g_OldMapList.GetValue(map, Cooldown);
		int NewCooldown = GetNativeCell(2);
		if(NewCooldown > Cooldown)
			Cooldown = NewCooldown;
	}

	g_OldMapList.SetValue(map, Cooldown, true);
	InternalStoreMapCooldowns();

	return true;
}

public int Native_ExcludeMapTime(Handle plugin, int numParams)
{
	if(!InternalAreRestrictionsActive())
		return true;

	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown;
	int Mode = GetNativeCell(3);

	if(Mode == 0)
	{
		Cooldown = InternalGetMapCooldownTime(map);
	}
	else if(Mode == 1)
	{
		Cooldown = GetNativeCell(2);
	}
	else if(Mode == 2)
	{
		g_TimeMapList.GetValue(map, Cooldown);
		int NewCooldown = GetTime() + GetNativeCell(2);
		if(NewCooldown > Cooldown)
			Cooldown = GetNativeCell(2);
	}

	Cooldown += GetTime();
	g_TimeMapList.SetValue(map, Cooldown, true);
	InternalStoreMapCooldowns();

	return true;
}

public int Native_GetMapCooldown(Handle plugin, int numParams)
{
	if(!InternalAreRestrictionsActive())
		return 0;

	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown = 0;
	g_OldMapList.GetValue(map, Cooldown);

	return Cooldown;
}

public int Native_GetMapCooldownTime(Handle plugin, int numParams)
{
	if(!InternalAreRestrictionsActive())
		return 0;

	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown = 0;
	g_TimeMapList.GetValue(map, Cooldown);

	return Cooldown;
}

public int Native_GetMapMinTime(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMinTime(map);
}

public int Native_GetMapMaxTime(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMaxTime(map);
}

public int Native_GetMapMinPlayers(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMinPlayers(map);
}

public int Native_GetMapMaxPlayers(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMaxPlayers(map);
}

public int Native_GetMapTimeRestriction(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapTimeRestriction(map);
}

public int Native_GetMapPlayerRestriction(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapPlayerRestriction(map);
}

public int Native_GetMapGroups(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);
	int size = GetNativeCell(3);

	if(len <= 0)
		return -999;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int[] groups = new int[size];
	int found = InternalGetMapGroups(map, groups, size);
	if(found >= 0)
		SetNativeArray(2, groups, size);
	return found;
}

public int Native_GetMapGroupRestriction(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return -999;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int groups[32];
	int groupsfound = InternalGetMapGroups(map, groups, sizeof(groups));

	for(int group = 0; group < groupsfound; group ++)
	{
		int groupcur = 0;
		int groupmax = InternalGetGroupMax(groups[group]);

		if(groupmax >= 0)
		{
			static char map_[PLATFORM_MAX_PATH];
			int groups_[32];
			for(int i = 0; i < GetArraySize(g_NominateList); i++)
			{
				GetArrayString(g_NominateList, i, map_, PLATFORM_MAX_PATH);
				int tmp = InternalGetMapGroups(map_, groups_, sizeof(groups_));
				if(FindIntInArray(groups_, tmp, groups[group]) != -1)
					groupcur++;
			}

			if(groupcur >= groupmax)
			{
				// Check if client has nominated a map in the same group and can change their nomination
				bool okay = false;
				if(client >= 1 && client <= MaxClients)
				{
					int index = FindValueInArray(g_NominateOwners, client);
					if(index != -1)
					{
						static char oldmap[PLATFORM_MAX_PATH];
						GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);
						static int oldgroups[32];
						int tmp = InternalGetMapGroups(oldmap, oldgroups, sizeof(oldgroups));
						if(FindIntInArray(groups_, tmp, groups[group]) != -1)
							okay = true;
					}
				}

				if(!okay)
					return groupmax;
			}
		}
	}

	return -1;
}

public int Native_IsClientMapAdminRestricted(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	// Check if client should bypass admin restrictions
	if(client >= 1 && client <= MaxClients)
	{
		// Client has bypass flag, dont return admin restrictions
		if(CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC))
			return false;

		// Client has ban flag, dont return admin restrictions
		if(CheckCommandAccess(client, "sm_nominate_ban", ADMFLAG_BAN))
			return false;
	}

	return InternalGetMapAdminRestriction(map);
}

public int Native_IsMapAdminRestricted(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapAdminRestriction(map);
}

public int Native_IsClientMapVIPRestricted(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	// Check if client should bypass vip restrictions
	if(client >= 1 && client <= MaxClients)
	{
		// Client has bypass flag, dont return vip restrictions
		if(CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC))
			return false;

		// Client has vip flag, dont return vip restrictions
		if(CheckCommandAccess(client, "sm_nominate_vip", ADMFLAG_CUSTOM1))
			return false;
	}

	return InternalGetMapVIPRestriction(map);
}

public int Native_IsMapVIPRestricted(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapVIPRestriction(map);
}

public int Native_IsClientMapLeaderRestricted(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	// Check if client should bypass leader restrictions
	if(client >= 1 && client <= MaxClients)
	{
#if defined _zleader_included
		// Client has leader.ini access, dont return leader restrictions
		if(g_ZLeader && ZL_IsPossibleLeader(client))
			return false;
#else
		return false;
#endif
	}

	return InternalGetMapLeaderRestriction(map);
}

public int Native_IsMapLeaderRestricted(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if(len <= 0) return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapLeaderRestriction(map);
}

public int Native_GetExtendsLeft(Handle plugin, int numParams)
{
	return GetConVarInt(g_Cvar_Extend) - g_Extends;
}

public int Native_AreRestrictionsActive(Handle plugin, int numParams)
{
	return InternalAreRestrictionsActive();
}

public int Native_SimulateMapEnd(Handle plugin, int numParams)
{
	OnMapEnd();
	return 0;
}

stock void AddMapItem(const char[] map)
{
	if (g_NativeVotes)
	{
		NativeVotes_AddItem(g_VoteMenu, map, map);
	}
	else
	{
		AddMenuItem(g_VoteMenu, map, map);
	}
}

stock void GetMapItem(Handle menu, int position, char[] map, int mapLen)
{
	if (g_NativeVotes)
	{
		NativeVotes_GetItem(menu, position, map, mapLen);
	}
	else
	{
		GetMenuItem(menu, position, map, mapLen, _, _, _, -1);
	}
}

stock void AddExtendToMenu(Handle menu, MapChange when)
{
	/* Do we add any special items? */
	// Moved for Mapchooser Extended

	if((when == MapChange_Instant || when == MapChange_RoundEnd) && GetConVarBool(g_Cvar_DontChange))
	{
		if (g_NativeVotes)
		{
			// Built-in votes don't have "Don't Change", send Extend instead
			NativeVotes_AddItem(menu, VOTE_DONTCHANGE, "Don't Change");
		}
		else
		{
			AddMenuItem(menu, VOTE_DONTCHANGE, "Don't Change");
		}
	}
	else if(GetConVarBool(g_Cvar_Extend) && g_Extends < GetConVarInt(g_Cvar_Extend))
	{
		if (g_NativeVotes)
		{
			NativeVotes_AddItem(menu, VOTE_EXTEND, "Extend Map");
		}
		else
		{
			AddMenuItem(menu, VOTE_EXTEND, "Extend Map");
		}
	}
}

// 0 = IncludeMaps, 1 = Reserved, 2 = IncludeMaps+Reserved
stock int GetVoteSize(int what = 0)
{
	int voteSize = GetConVarInt(g_Cvar_IncludeMaps);
	int includeMapsReserved = GetConVarInt(g_Cvar_IncludeMapsReserved);

	if (what == 1)
		voteSize = includeMapsReserved;
	else if (what == 2)
		voteSize = voteSize + includeMapsReserved;

	// New in 1.9.5 to let us bump up the included maps count
	if (g_NativeVotes)
	{
		int max = NativeVotes_GetMaxItems();

		if (max < voteSize)
		{
			voteSize = max;
		}
	}

	return voteSize;
}

stock int InternalGetMapCooldown(const char[] map)
{
	int Cooldown = g_Cvar_ExcludeMaps.IntValue;

	if(g_Config && g_Config.JumpToKey(map))
	{
		Cooldown = g_Config.GetNum("Cooldown", Cooldown);
		g_Config.Rewind();
	}

	return Cooldown;
}

stock int InternalGetMapCooldownTime(const char[] map)
{
	char time[16];
	g_Cvar_ExcludeMapsTime.GetString(time, sizeof(time));
	int Cooldown = TimeStrToSeconds(time);

	if(g_Config && g_Config.JumpToKey(map))
	{
		g_Config.GetString("CooldownTime", time, sizeof(time), "");
		if(time[0])
			Cooldown = TimeStrToSeconds(time);

		g_Config.Rewind();
	}

	return Cooldown;
}

void CheckMapRestrictions(bool time = false, bool players = false)
{
	if(!InternalAreRestrictionsActive())
		return;

	static char map[PLATFORM_MAX_PATH];
	for(int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		int client = GetArrayCell(g_NominateOwners, i);
		if(!client)
			continue;

		if(CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC, true))
			continue;

		bool remove;
		GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);

		if (time)
		{
			int TimeRestriction = InternalGetMapTimeRestriction(map);
			if(TimeRestriction)
			{
				remove = true;

				CPrintToChat(client, "{green}[MCE]{default} %t", "Nomination Removed Time Error", map);
			}
		}

		if (players)
		{
			int PlayerRestriction = InternalGetMapPlayerRestriction(map);
			if(PlayerRestriction)
			{
				remove = true;

				if(PlayerRestriction < 0)
					CPrintToChat(client, "{green}[MCE]{default} %t", "Nomination Removed MinPlayers Error", map);
				else
					CPrintToChat(client, "{green}[MCE]{default} %t", "Nomination Removed MaxPlayers Error", map);
			}
		}

		if (remove)
		{
			Forward_OnNominationRemoved(map, GetArrayCell(g_NominateOwners, i));

			RemoveFromArray(g_NominateList, i);
			RemoveFromArray(g_NominateOwners, i);
			g_NominateCount--;
		}
	}
}

stock int InternalGetMapMinTime(const char[] map)
{
	int MinTime = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		MinTime = g_Config.GetNum("MinTime", MinTime);
		g_Config.Rewind();
	}

	return MinTime;
}

stock int InternalGetMapMaxTime(const char[] map)
{
	int MaxTime = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		MaxTime = g_Config.GetNum("MaxTime", MaxTime);
		g_Config.Rewind();
	}

	return MaxTime;
}

stock int InternalGetMapMinPlayers(const char[] map)
{
	int MinPlayers = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		MinPlayers = g_Config.GetNum("MinPlayers", MinPlayers);
		g_Config.Rewind();
	}

	return MinPlayers;
}

stock int InternalGetMapMaxPlayers(const char[] map)
{
	int MaxPlayers = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		MaxPlayers = g_Config.GetNum("MaxPlayers", MaxPlayers);
		g_Config.Rewind();
	}

	return MaxPlayers;
}

stock int InternalGetMapGroups(const char[] map, int[] groups, int size)
{
	int found = 0;
	if(g_Config && g_Config.JumpToKey("_groups"))
	{
		if(!g_Config.GotoFirstSubKey(false))
		{
			g_Config.Rewind();
			return -999;
		}

		do
		{
			char groupstr[8];
			g_Config.GetSectionName(groupstr, sizeof(groupstr));
			int group = StringToInt(groupstr);
			if(g_Config.JumpToKey(map, false))
			{
				groups[found++] = group;
				if(found >= size)
				{
					g_Config.Rewind();
					return found;
				}
				g_Config.GoBack();
			}
		} while(g_Config.GotoNextKey());

		g_Config.Rewind();
	}

	return found;
}

stock int InternalGetGroupMax(int group)
{
	char groupstr[8];
	IntToString(group, groupstr, sizeof(groupstr));
	if(g_Config && g_Config.JumpToKey("_groups"))
	{
		if(g_Config.JumpToKey(groupstr, false))
		{
			int max = g_Config.GetNum("_max", -1);
			g_Config.Rewind();
			return max;
		}

		g_Config.Rewind();
	}

	return -1;
}

// 0 = Okay
// >0 = Minutes till Okay
stock int InternalGetMapTimeRestriction(const char[] map)
{
	char sTime[8];
	FormatTime(sTime, sizeof(sTime), "%H%M");

	int CurTime = StringToInt(sTime);
	int MinTime = InternalGetMapMinTime(map);
	int MaxTime = InternalGetMapMaxTime(map);

	//Wrap around.
	CurTime = (CurTime <= MinTime) ? CurTime + 2400 : CurTime;
	MaxTime = (MaxTime <= MinTime) ? MaxTime + 2400 : MaxTime;

	if (!(MinTime <= CurTime <= MaxTime))
	{
		//Wrap around.
		MinTime = (MinTime <= CurTime) ? MinTime + 2400 : MinTime;
		MinTime = (MinTime <= MaxTime) ? MinTime + 2400 : MinTime;

		// Convert our 'time' to minutes.
		CurTime = (RoundToFloor(float(CurTime / 100)) * 60) + (CurTime % 100);
		MinTime = (RoundToFloor(float(MinTime / 100)) * 60) + (MinTime % 100);
		MaxTime = (RoundToFloor(float(MaxTime / 100)) * 60) + (MaxTime % 100);

		return MinTime - CurTime;
	}

	return 0;
}

// <0 = Less than MinPlayers
// 0 = Okay
// >0 = More than MaxPlayers
stock int InternalGetMapPlayerRestriction(const char[] map)
{
	int NumPlayers = GetClientCountEx(g_Cvar_CountBots.BoolValue);
	int MinPlayers = InternalGetMapMinPlayers(map);
	int MaxPlayers = InternalGetMapMaxPlayers(map);

	if(MinPlayers && NumPlayers < MinPlayers)
		return NumPlayers - MinPlayers;

	if(MaxPlayers && NumPlayers > MaxPlayers)
		return NumPlayers - MaxPlayers;

	return 0;
}

stock int GetClientCountEx(bool countBots)
{
	int iRealClients = 0;
	int iFakeClients = 0;

	for (int player = 1; player <= MaxClients; player++)
	{
		if (IsClientConnected(player))
		{
			if (IsFakeClient(player))
				iFakeClients++;
			else
				iRealClients++;
		}
	}
	return countBots ? iFakeClients + iRealClients : iRealClients;
}

stock bool InternalAreRestrictionsActive()
{
	if (!GetConVarBool(g_Cvar_NoRestrictionTimeframeEnable))
		return true;

	char sTime[8];
	FormatTime(sTime, sizeof(sTime), "%H%M");

	int CurTime = StringToInt(sTime);
	int MinTime = GetConVarInt(g_Cvar_NoRestrictionTimeframeMinTime);
	int MaxTime = GetConVarInt(g_Cvar_NoRestrictionTimeframeMaxTime);

	//Wrap around.
	CurTime = (CurTime <= MinTime) ? CurTime + 2400 : CurTime;
	MaxTime = (MaxTime <= MinTime) ? MaxTime + 2400 : MaxTime;

	if ((MinTime <= CurTime <= MaxTime))
	{
		return false;
	}

	return true;
}

stock int FindIntInArray(int[] array, int size, int value)
{
	for(int i = 0; i < size; i++)
	{
		if(array[i] == value)
			return i;
	}

	return -1;
}

stock bool InternalGetMapAdminRestriction(const char[] map)
{
	int iAdmin = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		iAdmin = g_Config.GetNum("Admin", iAdmin);
		g_Config.Rewind();
	}

	return view_as<bool>(iAdmin);
}

stock bool InternalGetMapVIPRestriction(const char[] map)
{
	int VIP = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		VIP = g_Config.GetNum("VIP", VIP);
		g_Config.Rewind();
	}

	return view_as<bool>(VIP);
}

stock bool InternalGetMapLeaderRestriction(const char[] map)
{
	int Leader = 0;

	if(g_Config && g_Config.JumpToKey(map))
	{
		Leader = g_Config.GetNum("Leader", Leader);
		g_Config.Rewind();
	}

	return view_as<bool>(Leader);
}

stock void InternalRestoreMapCooldowns()
{
	char sCooldownFile[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, sCooldownFile, sizeof(sCooldownFile), "configs/mapchooser_extended/cooldowns.cfg");

	if(!FileExists(sCooldownFile))
	{
		LogMessage("Could not find cooldown file: \"%s\"", sCooldownFile);
		return;
	}

	KeyValues Cooldowns = new KeyValues("mapchooser_extended");

	if(!Cooldowns.ImportFromFile(sCooldownFile))
	{
		LogMessage("Unable to load cooldown file: \"%s\"", sCooldownFile);
		delete Cooldowns;
		return;
	}

	if(!Cooldowns.GotoFirstSubKey(true))
	{
		LogMessage("Unable to goto first sub key: \"%s\"", sCooldownFile);
		delete Cooldowns;
		return;
	}

	int Cooldown;
	char map[PLATFORM_MAX_PATH];

	do
	{
		if(!Cooldowns.GetSectionName(map, sizeof(map)))
		{
			LogMessage("Unable to get section name: \"%s\"", sCooldownFile);
			delete Cooldowns;
			return;
		}

		if((Cooldown = Cooldowns.GetNum("Cooldown", -1)) > 0)
		{
			LogMessage("Restored cooldown: %s -> %d", map, Cooldown);
			g_OldMapList.SetValue(map, Cooldown, true);
		}

		if((Cooldown = Cooldowns.GetNum("CooldownTime", -1)) > 0)
		{
			LogMessage("Restored time cooldown: %s -> %d", map, Cooldown);
			g_TimeMapList.SetValue(map, Cooldown, true);
		}
	} while(Cooldowns.GotoNextKey(true));

	delete Cooldowns;
}

stock void InternalStoreMapCooldowns()
{
	char sCooldownFile[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, sCooldownFile, sizeof(sCooldownFile), "configs/mapchooser_extended/cooldowns.cfg");

	if(!FileExists(sCooldownFile))
	{
		LogMessage("Could not find cooldown file: \"%s\"", sCooldownFile);
		return;
	}

	KeyValues Cooldowns = new KeyValues("mapchooser_extended");

	int Cooldown;
	char map[PLATFORM_MAX_PATH];

	StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
	for(int i = 0; i < OldMapListSnapshot.Length; i++)
	{
		OldMapListSnapshot.GetKey(i, map, sizeof(map));
		g_OldMapList.GetValue(map, Cooldown);

		if (!Cooldowns.JumpToKey(map, true))
		{
			LogMessage("Unable to create/find key: %s", map);
			delete OldMapListSnapshot;
			delete Cooldowns;
			return;
		}

		Cooldowns.SetNum("Cooldown", Cooldown);
		Cooldowns.Rewind();
	}
	delete OldMapListSnapshot;

	StringMapSnapshot TimeMapListSnapshot = g_TimeMapList.Snapshot();
	for(int i = 0; i < TimeMapListSnapshot.Length; i++)
	{
		TimeMapListSnapshot.GetKey(i, map, sizeof(map));
		g_TimeMapList.GetValue(map, Cooldown);

		if (!Cooldowns.JumpToKey(map, true))
		{
			LogMessage("Unable to create/find key: %s", map);
			delete TimeMapListSnapshot;
			delete Cooldowns;
			return;
		}

		Cooldowns.SetNum("CooldownTime", Cooldown);
		Cooldowns.Rewind();
	}
	delete TimeMapListSnapshot;

	if(!Cooldowns.ExportToFile(sCooldownFile))
	{
		LogMessage("Unable to export cooldown file: \"%s\"", sCooldownFile);
		delete Cooldowns;
		return;
	}

	delete Cooldowns;
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

stock void Forward_OnNominationRemoved(char[] oldmap, int owner)
{
	Call_StartForward(g_NominationsResetForward);
	Call_PushString(oldmap);
	Call_PushCell(owner);
	Call_Finish();
}

stock void Forward_OnNominationDisconnect(char[] oldmap, int owner)
{
	Call_StartForward(g_NominationDisconnect);
	Call_PushString(oldmap);
	Call_PushCell(owner);
	Call_Finish();
}