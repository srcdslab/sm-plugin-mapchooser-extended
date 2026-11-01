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

StringMap g_mapTrie;
ArrayList g_MapList = null;
ArrayList g_AdminMapList = null;
ArrayList g_SortedMapList = null;		// g_MapList sorted alphabetically, rebuilt when g_bSortedMapListDirty is set
ArrayList g_SortedMapDisplay = null;	// Menu display text of each g_SortedMapList entry
bool g_bSortedMapListDirty = true;

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
	g_MapList = new ArrayList(arraySize);
	g_AdminMapList = new ArrayList(arraySize);
	g_SortedMapList = new ArrayList(arraySize);
	g_SortedMapDisplay = new ArrayList(arraySize);
	g_mapTrie = new StringMap();
}

stock void VerifyMapLists()
{
	if (ReadMapList(g_MapList, g_mapFileSerial, "nominations", MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER) == null)
	{
		if (g_mapFileSerial == -1)
			SetFailState("Unable to create a valid map list.");
	}

	if (ReadMapList(g_AdminMapList, g_AdminMapFileSerial, "sm_nominate_addmap menu", MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT|MAPLIST_FLAG_MAPSFOLDER) == null)
	{
		if (g_AdminMapFileSerial == -1)
			SetFailState("Unable to create a valid admin map list.");
	}
	else
	{
		for (int i = 0; i < g_MapList.Length; i++)
		{
			static char map[PLATFORM_MAX_PATH];
			g_MapList.GetString(i, map, sizeof(map));

			int Index = g_AdminMapList.FindString(map);
			if (Index != -1)
				g_AdminMapList.Erase(Index);
		}
	}
}

stock void InitTimerDelayNominate()
{
	CreateTimer(g_Cvar_InitialDelay.FloatValue, Timer_DelayNominate, _, TIMER_FLAG_NO_MAPCHANGE);
}

stock void UpdateMapTrie()
{
	bool bExcludeOld = g_Cvar_ExcludeOld.BoolValue;
	bool bExcludeCurrent = g_Cvar_ExcludeCurrent.BoolValue;

	static char map[PLATFORM_MAX_PATH];
	static char currentMap[PLATFORM_MAX_PATH];
	ArrayList excludeMaps;

	if (bExcludeOld)
	{
		excludeMaps = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
		GetExcludeMapList(excludeMaps);
	}

	if (bExcludeCurrent)
		GetCurrentMap(currentMap, sizeof(currentMap));

	g_mapTrie.Clear();

	for (int i = 0; i < g_MapList.Length; i++)
	{
		int status = MAPSTATUS_ENABLED;

		g_MapList.GetString(i, map, sizeof(map));

		if (bExcludeCurrent)
		{
			if (strcmp(map, currentMap) == 0)
				status = MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_CURRENT;
		}

		/* Dont bother with this check if the current map check passed */
		if (bExcludeOld && status == MAPSTATUS_ENABLED)
		{
			if (excludeMaps.FindString(map) != -1)
				status = MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_PREVIOUS;
		}

		g_mapTrie.SetValue(map, status);
	}

	if (excludeMaps)
		delete excludeMaps;

	// The nomination pool may have changed, rebuild the sorted menu list on next use
	g_bSortedMapListDirty = true;
}

stock void UpdateSortedMapList()
{
	if (!g_bSortedMapListDirty && g_SortedMapList.Length == g_MapList.Length)
		return;

	delete g_SortedMapList;
	g_SortedMapList = g_MapList.Clone();
	g_SortedMapList.SortCustom(AlphabeticSortCallback);
	g_SortedMapDisplay.Clear();

	static char map[PLATFORM_MAX_PATH];
	char display[PLATFORM_MAX_PATH];
	char description[PLATFORM_MAX_PATH];
	for (int i = 0; i < g_SortedMapList.Length; i++)
	{
		g_SortedMapList.GetString(i, map, sizeof(map));

		MCE_GetMapDisplayName(map, display, sizeof(display));
		if (MCE_GetMapDescription(map, description, sizeof(description)) && description[0])
			FormatEx(display, sizeof(display), "%s | %s", display[0] ? display : map, description);

		if (!display[0])
			strcopy(display, sizeof(display), map);

		g_SortedMapDisplay.PushString(display);
	}

	g_bSortedMapListDirty = false;
}

stock void MapEndCleanUp()
{
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
	if (!g_mapTrie.GetValue(map, status))
		return;

	/* Was the map disabled due to being nominated */
	if ((status & MAPSTATUS_EXCLUDE_NOMINATED) != MAPSTATUS_EXCLUDE_NOMINATED)
		return;

	g_mapTrie.SetValue(map, MAPSTATUS_ENABLED);
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
			CReplyToCommand(client, "{green}[NE]{default} %t", "Nextmap", map);
			return false;
		}
/*
		case CanNominate_No_VoteFull:
		{
			CReplyToCommand(client, "{green}[NE]{default} %t", "NE Max Nominations");
			return false;
		}
*/
	}

	return true;
}

stock int GetVIPTimeRestriction()
{
	if (!g_Cvar_VIPTimeframe.BoolValue)
		return 0;

	char sTime[8];
	FormatTime(sTime, sizeof(sTime), "%H%M");

	int CurTime = StringToInt(sTime);
	int MinTime = g_Cvar_VIPTimeframeMinTime.IntValue;
	int MaxTime = g_Cvar_VIPTimeframeMaxTime.IntValue;

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

// Parses "60" (minutes), "30m", "2h", "1h30m", "2h15m30s"... A number without unit is read as minutes.
stock int TimeStrToSeconds(const char[] str)
{
	int seconds = 0;
	int maxlen = strlen(str);
	for (int i = 0; i < maxlen;)
	{
		int val = 0;
		int read = StringToIntEx(str[i], val);
		i += read;

		switch (CharToLower(str[i]))
		{
			case 'h':
			{
				seconds += val * 3600;
				i++;
			}
			case 'm':
			{
				seconds += val * 60;
				i++;
			}
			case 's':
			{
				seconds += val;
				i++;
			}
			default:
			{
				seconds += val * 60;
				// Skip unknown characters, otherwise the loop never advances
				if (!read)
					i++;
			}
		}
	}
	return seconds;
}

stock bool IsMapRestricted(int client, char[] map)
{
	return AreRestrictionsActive() && (GetMapCooldownTime(map) > GetTime() || GetMapTimeRestriction(map) || GetMapPlayerRestriction(map) ||
	GetMapGroupRestriction(map, client) >= 0 || IsClientMapAdminRestricted(map, client) || IsClientMapVIPRestricted(map, client) || IsClientMapLeaderRestricted(map, client));
}

public int AlphabeticSortCallback(int index1, int index2, ArrayList array, ArrayList hndl)
{
	char map1[PLATFORM_MAX_PATH], map2[PLATFORM_MAX_PATH];
	view_as<ArrayList>(array).GetString(index1, map1, sizeof(map1));
	view_as<ArrayList>(array).GetString(index2, map2, sizeof(map2));

	return strcmp(map1, map2, false);
}
