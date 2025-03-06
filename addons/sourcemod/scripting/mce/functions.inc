/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/functions.inc
*  Description:   Handles MCE functions
* 
* ============================================================================
*/

stock void InitializeMapLists()
{
	int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	g_MapList = CreateArray(arraySize);
	g_NominateList = CreateArray(arraySize);
	g_NominateOwners = CreateArray(1);
	g_OldMapList = new StringMap();
	g_TimeMapList = new StringMap();
	g_NextMapList = CreateArray(arraySize);
	g_OfficialList = CreateArray(arraySize);

	GetGameFolderName(g_sGameModName, sizeof(g_sGameModName));

	static char mapListPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, mapListPath, PLATFORM_MAX_PATH, "configs/mapchooser_extended/maps/%s.txt", g_sGameModName);
	SetMapListCompatBind("official", mapListPath);
}

stock void InitializeGameModeSettings()
{
	static char folder[64];
	// TODO: This is already checked in plugin start, DRY
	GetGameFolderName(folder, sizeof(folder));

	g_RoundCounting = RoundCounting_Standard;
	g_ObjectiveEnt = -1;

	if (strcmp(folder, "tf") == 0 && GameRules_GetProp("m_bPlayingMannVsMachine"))
	{
		g_RoundCounting = RoundCounting_MvM;
		g_ObjectiveEnt = EntIndexToEntRef(FindEntityByClassname(-1, "tf_objective_resource"));
	}
	else if (strcmp(folder, "csgo") == 0 && GetConVarInt(g_Cvar_GameType) == GameType_GunGame &&
		GetConVarInt(g_Cvar_GameMode) == GunGameMode_ArmsRace)
	{
		g_RoundCounting = RoundCounting_ArmsRace;
	}

	if (g_Config)
		delete g_Config;

	char sConfigFile[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, sConfigFile, sizeof(sConfigFile), "configs/mapchooser_extended.cfg");
	if (!FileExists(sConfigFile))
	{
		LogMessage("Could not find config: \"%s\"", sConfigFile);
		return;
	}
	LogMessage("Found config: \"%s\"", sConfigFile);

	g_Config = new KeyValues("mapchooser_extended");
	if (!g_Config.ImportFromFile(sConfigFile))
	{
		delete g_Config;
		LogMessage("ImportFromFile() failed!");
		return;
	}
	g_Config.Rewind();

	if (InternalAreRestrictionsActive())
		g_SaveCDOnMapEnd = true;
	else
		g_SaveCDOnMapEnd = false;
}

stock void InitializeGroupSettings()
{
	if (!g_Config)
		return;

	char sCurrentMap[PLATFORM_MAX_PATH];
	GetCurrentMap(sCurrentMap, sizeof(sCurrentMap));

	int groups[32];
	int iDefaultGrpCooldown = -1;
	int iTotalGrpFound = InternalGetMapGroups(sCurrentMap, groups, sizeof(groups));

	bool bValidCooldownFound = false;
	bool bCooldownMatching = true;

	for (int i = 0; i < iTotalGrpFound; ++i)
	{
		int groupID = groups[i];
		int iGrpCooldown = InternalGetGroupCooldown(groupID);

		if (iGrpCooldown >= 0)
		{
			if (!bValidCooldownFound) // First valid cooldown found
			{
				iDefaultGrpCooldown = iGrpCooldown;
				bValidCooldownFound = true;
			}
			else if (iDefaultGrpCooldown != iGrpCooldown) // Found a different cooldown value in another group
			{
				bCooldownMatching = false;
				LogMessage("Map %s is in multiple groups with different cooldowns. No shared cooldown will be applied.", sCurrentMap);
				return;
			}
		}
	}

	if (bValidCooldownFound && bCooldownMatching)
	{
		LogMessage("Map %s is in multiple groups with matching cooldowns. Applying shared cooldown to all maps in these groups.", sCurrentMap);
		// Apply the cooldown to all maps in groups with a matching cooldown
		for (int i = 0; i < iTotalGrpFound; ++i)
		{
			int groupID = groups[i];
			if (InternalGetGroupCooldown(groupID) == -1) // No cooldown configured for this group
				continue;

			InternalSetGroupCooldownToMaps(groupID, iDefaultGrpCooldown);
			LogMessage("Applied shared cooldown to maps in group \"%d\".", groupID);
		}
	}
}

stock void InitializeMapVoteSettings()
{
	if (ReadMapList(g_MapList, g_mapFileSerial, "mapchooser", MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER) != INVALID_HANDLE)
	{
		if (g_mapFileSerial == -1)
			LogError("Unable to create a valid map list.");
	}

	// Disable the next level vote in TF2 and CS:GO
	// In TF2, this has two effects: 1. Stop the next level vote (which overlaps rtv functionality).
	// 2. Stop the built-in end level vote.  This is the only thing that happens in CS:GO
	if (g_Cvar_VoteNextLevel != INVALID_HANDLE)
		SetConVarBool(g_Cvar_VoteNextLevel, false);

	SetupTimeleftTimer();

	g_MapVoteCompleted = false;

	g_TotalRounds = 0;
	g_Extends = 0;
	g_NominateCount = 0;
	g_NominateReservedCount = 0;

	ClearArray(g_NominateList);
	ClearArray(g_NominateOwners);

	for (int i = 0; i < MAXTEAMS; i++)
		g_winCount[i] = 0;

	/* Check if mapchooser will attempt to start mapvote during bonus round time */
	if ((g_Cvar_Bonusroundtime != INVALID_HANDLE) && !g_iStartRounds)
	{
		if (!g_iStartTime && GetConVarFloat(g_Cvar_Bonusroundtime) <= view_as<float>(g_iVoteDuration))
			LogError("Warning - Bonus Round Time shorter than Vote Time. Votes during bonus round may not have time to complete");
	}
}

stock void ConfigureBonusRoundTime()
{
	// Change the mp_bonusroundtime max so that we have time to display the vote
	// If you display a vote during bonus time good defaults are 17 vote duration and 19 mp_bonustime
	if (g_Cvar_Bonusroundtime != INVALID_HANDLE)
		SetConVarBounds(g_Cvar_Bonusroundtime, ConVarBound_Upper, true, 30.0);
}

stock void CleanOnMapEnd()
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
	int iCurrentTime = GetTime();
	int Cooldown;

	if (g_SaveCDOnMapEnd)
	{
		GetCurrentMap(map, PLATFORM_MAX_PATH);
		Cooldown = InternalGetMapCooldown(map);
		g_OldMapList.SetValue(map, Cooldown, true);

		Cooldown = iCurrentTime + InternalGetMapCooldownTime(map) - RoundToFloor(GetGameTime());
		g_TimeMapList.SetValue(map, Cooldown, true);
	}

	StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
	for (int i = 0; i < OldMapListSnapshot.Length; i++)
	{
		OldMapListSnapshot.GetKey(i, map, sizeof(map));
		g_OldMapList.GetValue(map, Cooldown);

		Cooldown--;
		if (Cooldown > 0)
			g_OldMapList.SetValue(map, Cooldown, true);
		else
			g_OldMapList.Remove(map);
	}
	delete OldMapListSnapshot;

	StringMapSnapshot TimeMapListSnapshot = g_TimeMapList.Snapshot();
	for (int i = 0; i < TimeMapListSnapshot.Length; i++)
	{
		TimeMapListSnapshot.GetKey(i, map, sizeof(map));
		g_TimeMapList.GetValue(map, Cooldown);

		if (Cooldown < iCurrentTime)
			g_TimeMapList.Remove(map);
	}
	delete OldMapListSnapshot;
}

stock void NominationsOnClientDisconnect(client)
{
	int index = FindValueInArray(g_NominateOwners, client);

	if (index == -1)
		return;

	char oldmap[PLATFORM_MAX_PATH];
	GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);
	Forward_OnNominationRemoved(oldmap, GetArrayCell(g_NominateOwners, index));
	Forward_OnNominationDisconnect(oldmap, GetArrayCell(g_NominateOwners, index));

	RemoveFromArray(g_NominateOwners, index);
	RemoveFromArray(g_NominateList, index);
	ClearStoredNomination(client);
	g_NominateCount--;
}

void SetupTimeleftTimer()
{
	int time;
	if (GetMapTimeLeft(time) && time > 0)
	{
		int startTime;
		if (g_bEnableStartPercent)
		{
			int timeLimit;
			if (GetMapTimeLimit(timeLimit) && timeLimit > 0)
			{
				startTime = g_iStartTimePercent * (timeLimit * 60) / 100;
			}
		}
		else
			startTime = g_iStartTime * 60;

		if (time - startTime < 0 && g_bEndOfMapVote && !g_MapVoteCompleted && !g_HasVoteStarted)
		{
			SetupWarningTimer(WarningType_Vote);
		}
		else
		{
			if (g_WarningTimer == INVALID_HANDLE)
			{
				if (g_VoteTimer != INVALID_HANDLE)
				{
					KillTimer(g_VoteTimer);
					g_VoteTimer = INVALID_HANDLE;
				}

				//g_VoteTimer = CreateTimer(float(time - startTime), Timer_StartMapVoteTimer_StartMapVote, _, TIMER_FLAG_NO_MAPCHANGE);
				float interval = float((time - startTime));
				float random = GetRandomFloat(interval, (interval + g_fRandomStartTime));
				
				g_VoteTimer = CreateTimer(random, Timer_StartWarningTimer, _, TIMER_FLAG_NO_MAPCHANGE);
			}
		}
	}
}

public Action Timer_StartWarningTimer(Handle timer)
{
	g_VoteTimer = INVALID_HANDLE;

	if (!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
		SetupWarningTimer(WarningType_Vote);
	return Plugin_Continue;
}

public Action Timer_StartMapVote(Handle timer, Handle data)
{
	static int timePassed;

	// This is still necessary because InitiateVote still calls this directly via the retry timer
	if (!GetArraySize(g_MapList) || !g_bEndOfMapVote || g_MapVoteCompleted || g_HasVoteStarted)
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

	if (timePassed == 0 || !g_bHideTimer)
	{
		TimerLocation timerLocation = view_as<TimerLocation>(g_iTimerLocation);

		switch (timerLocation)
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

	if (timePassed++ >= warningMaxTime)
	{
		if (timer == g_RetryTimer)
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

public void CheckWinLimit(int winner_score)
{
	if (g_Cvar_Winlimit != INVALID_HANDLE)
	{
		int winlimit = GetConVarInt(g_Cvar_Winlimit);
		if (winlimit)
		{
			if (winner_score >= (winlimit - g_iStartRounds))
			{
				if (!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
				{
					SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
					//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
				}
			}
		}
	}

	if (g_Cvar_MatchClinch != INVALID_HANDLE && g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		bool clinch = GetConVarBool(g_Cvar_MatchClinch);

		if (clinch)
		{
			int maxrounds = GetConVarInt(g_Cvar_Maxrounds);
			int winlimit = RoundFloat(maxrounds / 2.0);

			if (winner_score == winlimit - 1)
			{
				if (!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
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

	if (g_RoundCounting == RoundCounting_ArmsRace)
		maxrounds = GameRules_GetProp("m_iNumGunGameProgressiveWeaponsCT");
	else if (g_RoundCounting == RoundCounting_MvM)
		maxrounds = GetEntProp(g_ObjectiveEnt, Prop_Send, "m_nMannVsMachineMaxWaveCount");
	else if (g_Cvar_Maxrounds != INVALID_HANDLE)
		maxrounds = GetConVarInt(g_Cvar_Maxrounds);
	else
		return;

	if (maxrounds)
	{
		if (roundcount >= (maxrounds - g_iStartRounds))
		{
			if (!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
			{
				SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
				//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
			}
		}
	}
}

public Action Timer_ChangeMap(Handle hTimer, Handle dp)
{
	g_ChangeMapInProgress = false;

	char map[PLATFORM_MAX_PATH];

	if (dp == INVALID_HANDLE)
	{
		if (!GetNextMap(map, PLATFORM_MAX_PATH))
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
	if (index != -1)
	{
		RemoveFromArray(array, index);
		return true;
	}

	return false;
}

public Action UnlockNominations(Handle timer)
{
#if defined _nominations_extended_included_
	// TODO: Verify is native is available
	ToggleNominations(false);
#endif
	return Plugin_Continue;
}

void LockNominations()
{
#if defined _nominations_extended_included_
	// TODO: Verify is native is available
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

	if (GetArraySize(tempMaps) > g_iExcludeMaps && InternalAreRestrictionsActive())
	{
		StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
		for (int i = 0; i < OldMapListSnapshot.Length; i++)
		{
			OldMapListSnapshot.GetKey(i, map, sizeof(map));
			RemoveStringFromArray(tempMaps, map);
		}
		delete OldMapListSnapshot;
	}

	if (InternalAreRestrictionsActive())
	{
		int iCurrentTime = GetTime();
		StringMapSnapshot TimeMapListSnapshot = g_TimeMapList.Snapshot();
		for (int i = 0; i < TimeMapListSnapshot.Length; i++)
		{
			TimeMapListSnapshot.GetKey(i, map, sizeof(map));
			int Cooldown;
			g_TimeMapList.GetValue(map, Cooldown);

			if (Cooldown > iCurrentTime)
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
	for (int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		GetArrayString(g_NominateList, i, map_, PLATFORM_MAX_PATH);
		int groupsfound = InternalGetMapGroups(map_, groups_, sizeof(groups_));
		for (int group = 0; group < groupsfound; group++)
		{
			IntToString(group, groupstr, sizeof(groupstr));
			int groupcur = 0;
			groupmap.GetValue(groupstr, groupcur);
			groupcur++;
			groupmap.SetValue(groupstr, groupcur, true);
		}
	}

	// find random maps which honor all restrictions
	for (int i = 0; i < limit; i++)
	{
		int b;
		for (int j = 0; j < 1000; j++)
		{
			b = GetRandomInt(0, GetArraySize(tempMaps) - 1);
			GetArrayString(tempMaps, b, map, PLATFORM_MAX_PATH);

			if (!InternalAreRestrictionsActive())
				break;

			if (InternalGetMapAdminRestriction(map))
				continue;

			if (InternalGetMapVIPRestriction(map))
				continue;

			if (InternalGetMapLeaderRestriction(map))
				continue;

			if (InternalGetMapTimeRestriction(map) != 0)
				continue;

			if (InternalGetMapPlayerRestriction(map) != 0)
				continue;

			bool okay = true;

			int groups[32];
			int groupsfound = InternalGetMapGroups(map, groups, sizeof(groups));
			for (int group = 0; group < groupsfound; group++)
			{
				IntToString(group, groupstr, sizeof(groupstr));

				int groupmax = InternalGetGroupMax(groups[group]);
				if (groupmax >= 0)
				{
					int groupcur = 0;
					groupmap.GetValue(groupstr, groupcur);

					if (groupcur >= groupmax)
					{
						okay = false;
						break;
					}

					groupcur++;
					groupmap.SetValue(groupstr, groupcur, true);
				}
			}

			if (okay)
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
	if (g_WaitingForVote || g_HasVoteStarted)
		return false;

	return true;
}

// Functions new to Mapchooser Extended
stock void SetupWarningTimer(WarningType type, MapChange when=MapChange_MapEnd, Handle mapList=INVALID_HANDLE, bool force=false)
{
	if (!GetArraySize(g_MapList) || g_ChangeMapInProgress || g_HasVoteStarted || (!force && ((when == MapChange_MapEnd && !g_bEndOfMapVote) || g_MapVoteCompleted)))
		return;

	bool interrupted = false;
	if (g_WarningInProgress && g_WarningTimer != INVALID_HANDLE)
	{
		interrupted = true;
		KillTimer(g_WarningTimer);
	}

	g_WarningInProgress = true;

	if (g_bLockNominationsAtWarning)
		LockNominations();

	int cvarTime;
	static char translationKey[64];

	switch (type)
	{
		case WarningType_Vote:
		{
			cvarTime = g_iWarningTime;
			strcopy(translationKey, sizeof(translationKey), "Vote Warning");
			LogAction(-1, -1, "[MCE] Voting for next map has started.");
		}

		case WarningType_Revote:
		{
			cvarTime = g_iRunOffWarningTime;
			strcopy(translationKey, sizeof(translationKey), "Revote Warning");
			LogAction(-1, -1, "[MCE] Revote for next map has started.");
		}
	}

	if (!interrupted)
	{
		Call_StartForward(type == WarningType_Vote ? g_MapVoteWarningStartForward : g_MapVoteRunoffStartForward);
		Call_Finish();
	}

	Handle data;
	g_WarningTimer = CreateDataTimer(1.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);
	WritePackCell(data, view_as<int>(cvarTime));
	WritePackString(data, translationKey);
	WritePackCell(data, view_as<int>(when));
	WritePackCell(data, view_as<int>(mapList));
	ResetPack(data);
}

stock void InitializeOfficialMapList()
{
	// If this fails, we want it to have an empty adt_array
	if (ReadMapList(g_OfficialList,
		g_mapOfficialFileSerial,
		"official",
		MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT)
		!= INVALID_HANDLE)
	{
		LogMessage("Loaded map list for %s.", g_sGameModName);
	}
	// Check if the map list was ever loaded
	else if (g_mapOfficialFileSerial == -1)
	{
		LogMessage("No official map list found for %s. Consider submitting one!", g_sGameModName);
	}
}

stock void InitializeMapTimeLimit()
{
	if (g_Cvar_TimeLimit != INVALID_HANDLE)
	{
		char sCurrentMap[PLATFORM_MAX_PATH];
		GetCurrentMap(sCurrentMap, sizeof(sCurrentMap));
		int iMapTimeLimit = InternalGetMapTimeLimit(sCurrentMap);
		SetConVarInt(g_Cvar_TimeLimit, iMapTimeLimit);
		LogMessage("Setting timelimit to %d for map %s", iMapTimeLimit, sCurrentMap);
	}
}

stock bool IsMapEndVoteAllowed()
{
	if (!g_bEndOfMapVote || g_MapVoteCompleted || g_HasVoteStarted)
		return false;
	return true;
}

stock void AddMapItem(const char[] map)
{
	AddMenuItem(g_VoteMenu, map, map);
}

stock void GetMapItem(Handle menu, int position, char[] map, int mapLen)
{
	GetMenuItem(menu, position, map, mapLen, _, _, _, -1);
}

stock void AddExtendToMenu(Handle menu, MapChange when)
{
	/* Do we add any special items? */
	// Moved for Mapchooser Extended

	char map[PLATFORM_MAX_PATH];
	GetCurrentMap(map, sizeof(map));

	if ((when == MapChange_Instant || when == MapChange_RoundEnd) && g_bDontChange)
	{
		AddMenuItem(menu, VOTE_DONTCHANGE, "Don't Change");
	}
	else if (view_as<bool>(g_iMaxExtends) && g_Extends < InternalGetMapMaxExtends(map))
	{
		AddMenuItem(menu, VOTE_EXTEND, "Extend Map");
	}
}

// 0 = IncludeMaps, 1 = Reserved, 2 = IncludeMaps+Reserved
stock int GetVoteSize(int what = 0)
{
	int voteSize = g_iIncludeMaps;
	int includeMapsReserved = g_iIncludeMapsReserved;

	if (what == 1)
		voteSize = includeMapsReserved;
	else if (what == 2)
		voteSize = voteSize + includeMapsReserved;

	return voteSize;
}

void CheckMapRestrictions(bool time = false, bool players = false)
{
	if (!InternalAreRestrictionsActive())
		return;

	static char map[PLATFORM_MAX_PATH];
	for (int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		int client = GetArrayCell(g_NominateOwners, i);
		if (!client)
			continue;

		if (CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC, true))
			continue;

		bool remove;
		GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);

		if (time)
		{
			int TimeRestriction = InternalGetMapTimeRestriction(map);
			if (TimeRestriction)
			{
				remove = true;

				CPrintToChat(client, "{green}[MCE]{default} %t", "Nomination Removed Time Error", map);
			}
		}

		if (players)
		{
			int PlayerRestriction = InternalGetMapPlayerRestriction(map);
			if (PlayerRestriction)
			{
				remove = true;

				if (PlayerRestriction < 0)
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
			ClearStoredNomination(client);
			g_NominateCount--;
		}
	}
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

stock int FindIntInArray(int[] array, int size, int value)
{
	for (int i = 0; i < size; i++)
	{
		if (array[i] == value)
			return i;
	}

	return -1;
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

stock ClearStoredNomination(int client)
{
	g_sNominations[client][0] = '\0';
}

stock void AlphabeticSortMapLists(Handle mapList, Handle ownerList)
{
	int size = GetArraySize(mapList);
	
	if (size <= 1)
		return;
	
	for (int i = 0; i < size - 1; i++)
	{
		for (int j = 0; j < size - i - 1; j++)
		{
			char map1[PLATFORM_MAX_PATH], map2[PLATFORM_MAX_PATH];
			GetArrayString(mapList, j, map1, sizeof(map1));
			GetArrayString(mapList, j+1, map2, sizeof(map2));
			
			if (strcmp(map1, map2, false) > 0)
			{
				SwapArrayItems(mapList, j, j+1);
				SwapArrayItems(ownerList, j, j+1);
			}
		}
	}
}