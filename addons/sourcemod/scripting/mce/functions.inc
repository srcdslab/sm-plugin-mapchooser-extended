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
	g_MapList = new ArrayList(arraySize);
	g_NominateList = new ArrayList(arraySize);
	g_NominateOwners = new ArrayList(1);
	g_OldMapList = new StringMap();
	g_TimeMapList = new StringMap();
	g_NextMapList = new ArrayList(arraySize);
	g_OfficialList = new ArrayList(arraySize);

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
	else if (strcmp(folder, "csgo") == 0 && g_Cvar_GameType.IntValue == GameType_GunGame &&
		g_Cvar_GameMode.IntValue == GunGameMode_ArmsRace)
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

stock void InitializeMapTiers()
{
	if (!g_Config)
		return;

	g_Config.Rewind();

	if (!g_Config.JumpToKey("_tiers"))
	{
		delete g_MapTiers;
		return;
	}

	if (!g_Config.GotoFirstSubKey())
	{
		delete g_MapTiers;
		return;
	}

	int lastNum = 0;

	delete g_MapTiers;
	g_MapTiers = new ArrayList(ByteCountToCells(64));

	do
	{
		// we need an ordered list of tier numbers
		char tierNumStr[3];
		g_Config.GetSectionName(tierNumStr, sizeof(tierNumStr));

		int tierNum = StringToInt(tierNumStr);
		if (!tierNum || (lastNum && tierNum == lastNum))
		{
			LogError("[MCE] Map Tiers Error: Found an unordered list in \"_tiers\" section");
			delete g_MapTiers;
			return;
		}

		lastNum = tierNum;

		char name[32];
		g_Config.GetString("_name", name, sizeof(name));
		g_MapTiers.PushString(name);
	} while (g_Config.GotoNextKey());
}

stock void InitializeGroupSettings()
{
	if (!g_Config)
		return;

	char sCurrentMap[PLATFORM_MAX_PATH];
	GetCurrentMap(sCurrentMap, sizeof(sCurrentMap));

	int groups[32];
	int iTotalGrpFound = InternalGetMapGroups(sCurrentMap, groups, sizeof(groups));
	bool changed = false;

	for (int i = 0; i < iTotalGrpFound; ++i)
	{
		int groupID = groups[i];
		int iGrpCooldown = InternalGetGroupCooldown(groupID);
		int iGrpCooldownTime = InternalGetGroupCooldownTime(groupID);

		if (iGrpCooldown <= 0 && iGrpCooldownTime <= 0)
			continue;

		LogMessage("Map %s is in group %d with a shared cooldown of %d maps / %d seconds", sCurrentMap, groupID, iGrpCooldown, iGrpCooldownTime);
		if (InternalSetGroupCooldownToMaps(groupID, iGrpCooldown, iGrpCooldownTime))
			changed = true;
	}

	if (changed)
		InternalStoreMapCooldowns();
}

stock void InitializeMapVoteSettings()
{
	if (ReadMapList(g_MapList, g_mapFileSerial, "mapchooser", MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_MAPSFOLDER) != null)
	{
		if (g_mapFileSerial == -1)
			LogError("Unable to create a valid map list.");
	}

	// Disable the next level vote in TF2 and CS:GO
	// In TF2, this has two effects: 1. Stop the next level vote (which overlaps rtv functionality).
	// 2. Stop the built-in end level vote.  This is the only thing that happens in CS:GO
	if (g_Cvar_VoteNextLevel != null)
		g_Cvar_VoteNextLevel.SetBool(false);

	SetupTimeleftTimer();

	g_MapVoteCompleted = false;

	g_TotalRounds = 0;
	g_Extends = 0;

	ClearNominationsLists();

	for (int i = 0; i < MAXTEAMS; i++)
		g_winCount[i] = 0;

	/* Check if mapchooser will attempt to start mapvote during bonus round time */
	if ((g_Cvar_Bonusroundtime != null) && !g_iStartRounds)
	{
		if (!g_iStartTime && g_Cvar_Bonusroundtime.FloatValue <= view_as<float>(g_iVoteDuration))
			LogError("Warning - Bonus Round Time shorter than Vote Time. Votes during bonus round may not have time to complete");
	}
}

stock void ConfigureBonusRoundTime()
{
	// Change the mp_bonusroundtime max so that we have time to display the vote
	// If you display a vote during bonus time good defaults are 17 vote duration and 19 mp_bonustime
	if (g_Cvar_Bonusroundtime != null)
		g_Cvar_Bonusroundtime.SetBounds(ConVarBound_Upper, true, 30.0);
}

stock void ClearNominationsLists()
{
	// Wipe out our nominations list
	g_NominateCount = 0;
	g_NominateReservedCount = 0;
	g_NominateOwners.Clear();
	g_NominateList.Clear();
}

stock void CleanOnMapEnd()
{
	g_HasVoteStarted = false;
	g_WaitingForVote = false;
	g_PendingRevoteActive = false;
	g_ChangeMapAtRoundEnd = false;
	g_ChangeMapInProgress = false;
	g_HasIntermissionStarted = false;

	g_VoteTimer = null;
	g_RetryTimer = null;
	g_WarningTimer = null;
	g_RunoffCount = 0;

	static char map[PLATFORM_MAX_PATH];
	int iCurrentTime = GetTime();
	int Cooldown;

	if (g_SaveCDOnMapEnd)
	{
		GetCurrentMap(map, PLATFORM_MAX_PATH);
		if (g_bForceLowerCase)
			StringToLowerCase(map);

		// Apply cooldown mode based on cvar
		switch (g_CooldownMode)
		{
			case CooldownMode_Both:
			{
				// Default behavior: save both cooldown types
				Cooldown = InternalGetMapCooldown(map);
				g_OldMapList.SetValue(map, Cooldown, true);

				Cooldown = iCurrentTime + InternalGetMapCooldownTime(map) - RoundToFloor(GetGameTime());
				g_TimeMapList.SetValue(map, Cooldown, true);
			}
			case CooldownMode_CounterOnly:
			{
				// Save only counter-based cooldown
				Cooldown = InternalGetMapCooldown(map);
				g_OldMapList.SetValue(map, Cooldown, true);
			}
			case CooldownMode_TimeOnly:
			{
				// Save only time-based cooldown
				Cooldown = iCurrentTime + InternalGetMapCooldownTime(map) - RoundToFloor(GetGameTime());
				g_TimeMapList.SetValue(map, Cooldown, true);
			}
			case CooldownMode_Priority:
			{
				// Priority to time-based cooldown, fallback to counter
				int timeCooldown = InternalGetMapCooldownTime(map);
				if (timeCooldown > 0)
				{
					Cooldown = iCurrentTime + timeCooldown - RoundToFloor(GetGameTime());
					g_TimeMapList.SetValue(map, Cooldown, true);
				}
				else
				{
					Cooldown = InternalGetMapCooldown(map);
					g_OldMapList.SetValue(map, Cooldown, true);
				}
			}
		}
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
	delete TimeMapListSnapshot;
}

stock void NominationsOnClientDisconnect(client)
{
	int index = g_NominateOwners.FindValue(client);

	if (index == -1)
		return;

	char oldmap[PLATFORM_MAX_PATH];
	g_NominateList.GetString(index, oldmap, PLATFORM_MAX_PATH);
	Forward_OnNominationRemoved(oldmap, g_NominateOwners.Get(index));
	Forward_OnNominationDisconnect(oldmap, g_NominateOwners.Get(index));

	g_NominateOwners.Erase(index);
	g_NominateList.Erase(index);
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
			if (g_WarningTimer == null)
			{
				if (g_VoteTimer != null)
				{
					delete g_VoteTimer;
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
	g_VoteTimer = null;

	if (!g_WarningInProgress || g_WarningTimer == null)
		SetupWarningTimer(WarningType_Vote);
	return Plugin_Continue;
}

public Action Timer_StartMapVote(Handle timer, DataPack data)
{
	static int timePassed;

	// This is still necessary because InitiateVote still calls this directly via the retry timer
	if (!g_MapList.Length || !g_bEndOfMapVote || g_MapVoteCompleted || g_HasVoteStarted)
	{
		g_WarningTimer = null;
		return Plugin_Stop;
	}

	data.Reset();
	int warningMaxTime = data.ReadCell();
	int warningTimeRemaining = warningMaxTime - timePassed;

	char warningPhrase[32];
	data.ReadString(warningPhrase, sizeof(warningPhrase));

	g_bRevoteWarning = warningTimeRemaining == 0 && strcmp(warningPhrase, "Revote Warning") == 0;

	if (warningTimeRemaining <= 0)
	{
		if (g_bRevoteWarning)
			LogAction(-1, -1, "[MCE] Revote for next map has started.");
		else
			LogAction(-1, -1, "[MCE] Voting for next map has started.");

		g_bRevoteWarning = false;
	}

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
			g_RetryTimer = null;
		}
		else
			g_WarningTimer = null;

		timePassed = 0;
		MapChange mapChange = view_as<MapChange>(data.ReadCell());
		ArrayList hndl = view_as<ArrayList>(data.ReadCell());

		InitiateVote(mapChange, hndl);

		return Plugin_Stop;
	}

	return Plugin_Continue;
}

public void CheckWinLimit(int winner_score)
{
	if (g_Cvar_Winlimit != null)
	{
		int winlimit = g_Cvar_Winlimit.IntValue;
		if (winlimit)
		{
			if (winner_score >= (winlimit - g_iStartRounds))
			{
				if (!g_WarningInProgress || g_WarningTimer == null)
				{
					SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
					//InitiateVote(MapChange_MapEnd, null);
				}
			}
		}
	}

	if (g_Cvar_MatchClinch != null && g_Cvar_Maxrounds != null)
	{
		bool clinch = g_Cvar_MatchClinch.BoolValue;

		if (clinch)
		{
			int maxrounds = g_Cvar_Maxrounds.IntValue;
			int winlimit = RoundFloat(maxrounds / 2.0);

			if (winner_score == winlimit - 1)
			{
				if (!g_WarningInProgress || g_WarningTimer == null)
				{
					SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
					//InitiateVote(MapChange_MapEnd, null);
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
	else if (g_Cvar_Maxrounds != null)
		maxrounds = g_Cvar_Maxrounds.IntValue;
	else
		return;

	if (maxrounds)
	{
		if (roundcount >= (maxrounds - g_iStartRounds))
		{
			if (!g_WarningInProgress || g_WarningTimer == null)
			{
				SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
				//InitiateVote(MapChange_MapEnd, null);
			}
		}
	}
}

public Action Timer_ChangeMap(Handle hTimer, DataPack dp)
{
	g_ChangeMapInProgress = false;

	char map[PLATFORM_MAX_PATH];

	if (dp == null)
	{
		if (!GetNextMap(map, PLATFORM_MAX_PATH))
		{
			//No passed map and no set nextmap. fail!
			return Plugin_Stop;
		}
	}
	else
	{
		dp.Reset();
		dp.ReadString(map, PLATFORM_MAX_PATH);
	}

	ForceChangeLevel(map, "Map Vote");

	return Plugin_Stop;
}

bool RemoveStringFromArray(ArrayList array, char[] str)
{
	int index = array.FindString(str);
	if (index != -1)
	{
		array.Erase(index);
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
	g_NextMapList.Clear();

	static char map[PLATFORM_MAX_PATH];
	ArrayList tempMaps = g_MapList.Clone();

	GetCurrentMap(map, PLATFORM_MAX_PATH);
	if (g_bForceLowerCase)
		StringToLowerCase(map);

	RemoveStringFromArray(tempMaps, map);

	if (ShouldApplyCounterCooldown() && tempMaps.Length > g_iExcludeMaps && InternalAreRestrictionsActive())
	{
		StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
		for (int i = 0; i < OldMapListSnapshot.Length; i++)
		{
			OldMapListSnapshot.GetKey(i, map, sizeof(map));
			RemoveStringFromArray(tempMaps, map);
		}
		delete OldMapListSnapshot;
	}

	if (ShouldApplyTimeCooldown() && InternalAreRestrictionsActive())
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
	int limit = (voteSize < tempMaps.Length ? voteSize : tempMaps.Length);

	// group -> number of maps nominated from group
	StringMap groupmap = new StringMap();
	char groupstr[8];

	// populate groupmap with maps from nomination list
	static char map_[PLATFORM_MAX_PATH];
	int groups_[32];
	for (int i = 0; i < g_NominateList.Length; i++)
	{
		g_NominateList.GetString(i, map_, PLATFORM_MAX_PATH);
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
			b = GetRandomInt(0, tempMaps.Length - 1);
			tempMaps.GetString(b, map, PLATFORM_MAX_PATH);

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
		g_NextMapList.PushString(map);
		tempMaps.Erase(b);
	}

	delete groupmap;
	delete tempMaps;
}

bool CanVoteStart()
{
	if (g_WaitingForVote || g_HasVoteStarted || g_PendingRevoteActive)
		return false;

	return true;
}

// Functions new to Mapchooser Extended
stock void SetupWarningTimer(WarningType type, MapChange when=MapChange_MapEnd, ArrayList mapList=null, bool force=false)
{
	if (!g_MapList.Length || g_ChangeMapInProgress || g_HasVoteStarted || (!force && ((when == MapChange_MapEnd && !g_bEndOfMapVote) || g_MapVoteCompleted)))
		return;

	bool interrupted = false;
	if (g_WarningInProgress && g_WarningTimer != null)
	{
		interrupted = true;
		delete g_WarningTimer;
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
		}

		case WarningType_Revote:
		{
			cvarTime = g_iRunOffWarningTime;
			strcopy(translationKey, sizeof(translationKey), "Revote Warning");
		}
	}

	if (!interrupted)
	{
		Call_StartForward(type == WarningType_Vote ? g_MapVoteWarningStartForward : g_MapVoteRunoffStartForward);
		Call_Finish();
	}

	DataPack data;
	g_WarningTimer = CreateDataTimer(1.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);
	data.WriteCell(view_as<int>(cvarTime));
	data.WriteString(translationKey);
	data.WriteCell(view_as<int>(when));
	data.WriteCell(view_as<int>(mapList));
	data.Reset();
}

stock void InitializeOfficialMapList()
{
	// If this fails, we want it to have an empty adt_array
	if (ReadMapList(g_OfficialList,
		g_mapOfficialFileSerial,
		"official",
		MAPLIST_FLAG_CLEARARRAY|MAPLIST_FLAG_NO_DEFAULT)
		!= null)
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
	if (g_Cvar_TimeLimit != null)
	{
		char sCurrentMap[PLATFORM_MAX_PATH];
		GetCurrentMap(sCurrentMap, sizeof(sCurrentMap));
		int iMapTimeLimit = InternalGetMapTimeLimit(sCurrentMap);
		g_Cvar_TimeLimit.SetInt(iMapTimeLimit);
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
	g_VoteMenu.AddItem(map, map);
}

stock void GetMapItem(Menu menu, int position, char[] map, int mapLen)
{
	menu.GetItem(position, map, mapLen, _, _, _, -1);
}

stock void AddExtendToMenu(Menu menu, MapChange when)
{
	/* Do we add any special items? */
	// Moved for Mapchooser Extended

	char map[PLATFORM_MAX_PATH];
	GetCurrentMap(map, sizeof(map));

	if ((when == MapChange_Instant || when == MapChange_RoundEnd) && g_bDontChange)
	{
		menu.AddItem(VOTE_DONTCHANGE, "Don't Change");
	}
	else if (view_as<bool>(g_iMaxExtends) && g_Extends < InternalGetMapMaxExtends(map))
	{
		menu.AddItem(VOTE_EXTEND, "Extend Map");
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
	for (int i = 0; i < g_NominateList.Length; i++)
	{
		int client = g_NominateOwners.Get(i);
		if (!client)
			continue;

		if (CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC, true))
			continue;

		bool remove;
		g_NominateList.GetString(i, map, PLATFORM_MAX_PATH);

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
			Forward_OnNominationRemoved(map, g_NominateOwners.Get(i));

			g_NominateList.Erase(i);
			g_NominateOwners.Erase(i);
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

stock ClearStoredNomination(int client)
{
	g_sNominations[client][0] = '\0';
}

stock void StringToLowerCase(char[] input)
{
	int i = 0;
	int x;
	while ((x = input[i]) != '\0')
	{
		if ('A' <= x <= 'Z')
			input[i] += ('a' - 'A');
		i++;
	}
}

stock void AlphabeticSortMapLists(ArrayList mapList, ArrayList ownerList)
{
	int size = mapList.Length;

	if (size <= 1)
		return;

	for (int i = 0; i < size - 1; i++)
	{
		for (int j = 0; j < size - i - 1; j++)
		{
			char map1[PLATFORM_MAX_PATH], map2[PLATFORM_MAX_PATH];
			mapList.GetString(j, map1, sizeof(map1));
			mapList.GetString(j+1, map2, sizeof(map2));

			if (strcmp(map1, map2, false) > 0)
			{
				mapList.SwapAt(j, j+1);
				ownerList.SwapAt(j, j+1);
			}
		}
	}
}

// Helper functions for cooldown modes
stock bool ShouldApplyCounterCooldown()
{
	return (g_CooldownMode == CooldownMode_Both || g_CooldownMode == CooldownMode_CounterOnly || g_CooldownMode == CooldownMode_Priority);
}

stock bool ShouldApplyTimeCooldown()
{
	return (g_CooldownMode == CooldownMode_Both || g_CooldownMode == CooldownMode_TimeOnly || g_CooldownMode == CooldownMode_Priority);
}
