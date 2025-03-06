/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/functions.inc
*  Description:   Handles NE functions
* 
* ============================================================================
*/

Handle g_mapTrie;
Handle g_MapList = INVALID_HANDLE;
Handle g_AdminMapList = INVALID_HANDLE;
Handle g_hDelayNominate = INVALID_HANDLE;

Menu g_MapMenu;
Menu g_AdminMapMenu;

#define MAPSTATUS_ENABLED (1<<0)
#define MAPSTATUS_DISABLED (1<<1)
#define MAPSTATUS_EXCLUDE_CURRENT (1<<2)
#define MAPSTATUS_EXCLUDE_PREVIOUS (1<<3)
#define MAPSTATUS_EXCLUDE_NOMINATED (1<<4)

int g_NominationDelay;
int g_mapFileSerial = -1;
int g_AdminMapFileSerial = -1;
int g_Player_NominationDelay[MAXPLAYERS+1];

bool g_bLate = false;
bool g_bNextMapSet = false;
bool g_bNEAllowed = false;		// True if Nominations is available to players.

stock void InitializeMapLists()
{
	int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	g_MapList = CreateArray(arraySize);
	g_AdminMapList = CreateArray(arraySize);
	g_mapTrie = CreateTrie();
}

stock void VerifyMapLists()
{
	if (ReadMapList(g_MapList, g_mapFileSerial, "nominations", MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER) == INVALID_HANDLE)
	{
		if (g_mapFileSerial == -1)
			SetFailState("Unable to create a valid map list.");
	}

	if (ReadMapList(g_AdminMapList, g_AdminMapFileSerial, "sm_nominate_addmap menu", MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT|MAPLIST_FLAG_MAPSFOLDER) == INVALID_HANDLE)
	{
		if (g_AdminMapFileSerial == -1)
			SetFailState("Unable to create a valid admin map list.");
	}
	else
	{
		for (int i = 0; i < GetArraySize(g_MapList); i++)
		{
			static char map[PLATFORM_MAX_PATH];
			GetArrayString(g_MapList, i, map, sizeof(map));

			int Index = FindStringInArray(g_AdminMapList, map);
			if (Index != -1)
				RemoveFromArray(g_AdminMapList, Index);
		}
	}
}

stock void InitTimerDelayNominate()
{
	if (g_hDelayNominate != INVALID_HANDLE)
		delete g_hDelayNominate;

	g_hDelayNominate = CreateTimer(g_Cvar_InitialDelay.FloatValue, Timer_DelayNominate, _, TIMER_FLAG_NO_MAPCHANGE);
}

stock void UpdateMapMenus()
{
	if (g_MapMenu != INVALID_HANDLE)
		delete g_MapMenu;

	g_MapMenu = BuildMapMenu("", -1);

	if (g_AdminMapMenu != INVALID_HANDLE)
		delete g_AdminMapMenu;

	g_AdminMapMenu = BuildAdminMapMenu("");
}

stock void UpdateMapTrie()
{
	bool bExcludeOld = GetConVarBool(g_Cvar_ExcludeOld);
	bool bExcludeCurrent = GetConVarBool(g_Cvar_ExcludeCurrent);

	static char map[PLATFORM_MAX_PATH];
	static char currentMap[PLATFORM_MAX_PATH];
	ArrayList excludeMaps;

	if (bExcludeOld)
	{
		excludeMaps = CreateArray(ByteCountToCells(PLATFORM_MAX_PATH));
		GetExcludeMapList(excludeMaps);
	}

	if (bExcludeCurrent)
		GetCurrentMap(currentMap, sizeof(currentMap));

	ClearTrie(g_mapTrie);

	for (int i = 0; i < GetArraySize(g_MapList); i++)
	{
		int status = MAPSTATUS_ENABLED;

		GetArrayString(g_MapList, i, map, sizeof(map));

		if (bExcludeCurrent)
		{
			if (strcmp(map, currentMap) == 0)
				status = MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_CURRENT;
		}

		/* Dont bother with this check if the current map check passed */
		if (bExcludeOld && status == MAPSTATUS_ENABLED)
		{
			if (FindStringInArray(excludeMaps, map) != -1)
				status = MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_PREVIOUS;
		}

		SetTrieValue(g_mapTrie, map, status);
	}

	if (excludeMaps)
		delete excludeMaps;
}

stock void MapEndCleanUp()
{
	g_hDelayNominate = INVALID_HANDLE;
	g_bNEAllowed = false;
	g_bNextMapSet = false;
}

public Action Timer_DelayNominate(Handle timer)
{
	if (IsNextMapAlreadySet())
		return Plugin_Stop;

	if (!g_bNEAllowed)
		CPrintToChatAll("{green}[NE]{default} %t", "Nominations Unlocked");

	g_bNEAllowed = true;
	g_NominationDelay = 0;

	return Plugin_Stop;
}

// This is SM Forward (file: mapchooser.inc)
public void OnNominationRemoved(const char[] map, int owner)
{
	int status;

	/* Is the map in our list? */
	if (!GetTrieValue(g_mapTrie, map, status))
		return;

	/* Was the map disabled due to being nominated */
	if ((status & MAPSTATUS_EXCLUDE_NOMINATED) != MAPSTATUS_EXCLUDE_NOMINATED)
		return;

	SetTrieValue(g_mapTrie, map, MAPSTATUS_ENABLED);
}

stock bool IsNominateAllowed(int client)
{
	if (IsClientNomBanned(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Cannot nominate - Nombanned", client);
		return false;
	}

	if (!CheckCommandAccess(client, "sm_tag", ADMFLAG_CUSTOM1))
	{
		int VIPTimeRestriction = GetVIPTimeRestriction();
		if ((VIPTimeRestriction) && AreRestrictionsActive())
		{
			CReplyToCommand(client, "{green}[NE]{default} During peak hours only VIPs are allowed to nominate maps. Wait for %d hours and %d minutes or buy VIP to nominate maps again.", VIPTimeRestriction / 60, VIPTimeRestriction % 60);
			return false;
		}
	}

	CanNominateResult result = CanNominate();

	switch (result)
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

public void OnSetNextMap(const char[] map)
{
	g_bNextMapSet = true;
}

stock bool IsNextMapAlreadySet()
{
	if (g_bNextMapSet)
		return true;

	return false;
}

stock void CustomFormatTime(int seconds, char[] buffer, int maxlen)
{
	if (seconds <= 60)
		Format(buffer, maxlen, "%ds", seconds);
	else if (seconds <= 3600)
		Format(buffer, maxlen, "%dm", seconds / 60);
	else if (seconds < 10*3600)
		Format(buffer, maxlen, "%dh%dm", seconds / 3600, (seconds % 3600) / 60);
	else
		Format(buffer, maxlen, "%dh", seconds / 3600);
}

stock int TimeStrToSeconds(const char[] str)
{
	int seconds = 0;
	int maxlen = strlen(str);
	for (int i = 0; i < maxlen;)
	{
		int val = 0;
		i += StringToIntEx(str[i], val);
		if (str[i] == 'h')
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

public int AlphabeticSortCallback(int index1, int index2, Handle array, Handle hndl)
{
	char map1[PLATFORM_MAX_PATH], map2[PLATFORM_MAX_PATH];
	GetArrayString(view_as<ArrayList>(array), index1, map1, sizeof(map1));
	GetArrayString(view_as<ArrayList>(array), index2, map2, sizeof(map2));
	
	return strcmp(map1, map2, false);
}