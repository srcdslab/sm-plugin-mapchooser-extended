/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/internal_functions.inc
*  Description:   All internal functions mostly used by mce_functions + natives
* 
* ============================================================================
*/

NominateResult InternalNominateMap(char[] map, bool force, int owner)
{
	if (!IsMapValidEx(map))
	{
		return Nominate_InvalidMap;
	}

	/* Map already in the vote */
	if (FindStringInArray(g_NominateList, map) != -1)
	{
		return Nominate_AlreadyInVote;
	}

	int index;

	/* Look to replace an existing nomination by this client - Nominations made with owner = 0 aren't replaced */
	if (owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);
		Forward_OnNominationRemoved(oldmap, owner);

		strcopy(g_sNominations[owner], sizeof(g_sNominations[]), map);
		SetArrayString(g_NominateList, index, map);
		AlphabeticSortMapLists(g_NominateList, g_NominateOwners);
		return Nominate_Replaced;
	}

	/* Too many nominated maps. */
	if (g_NominateCount >= GetVoteSize(0) && !force)
	{
		return Nominate_VoteFull;
	}

	PushArrayString(g_NominateList, map);
	PushArrayCell(g_NominateOwners, owner);
	if (owner == 0 && g_NominateReservedCount < GetVoteSize(1))
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
		ClearStoredNomination(owner_);

		if (owner_ == 0)
			g_NominateReservedCount--;
		else
			g_NominateCount--;
	}

	strcopy(g_sNominations[owner], sizeof(g_sNominations[]), map);
	AlphabeticSortMapLists(g_NominateList, g_NominateOwners);

	return Nominate_Added;
}

void InternalGetNominationByOwner(int owner, char[] buffer)
{
	int index;
	if (owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		GetArrayString(g_NominateList, index, buffer, PLATFORM_MAX_PATH);
	}
}

bool InternalRemoveNominationByMap(char[] map)
{
	for (int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, i, oldmap, PLATFORM_MAX_PATH);

		if (strcmp(map, oldmap, false) == 0)
		{
			Forward_OnNominationRemoved(oldmap, GetArrayCell(g_NominateOwners, i));

			int owner = GetArrayCell(g_NominateOwners, i);
			if (owner)
				g_NominateCount--;
			else
				g_NominateReservedCount--;

			RemoveFromArray(g_NominateList, i);
			RemoveFromArray(g_NominateOwners, i);
			ClearStoredNomination(owner);
			return true;
		}
	}

	return false;
}

bool InternalRemoveNominationByOwner(int owner)
{
	int index;

	if (owner && ((index = FindValueInArray(g_NominateOwners, owner)) != -1))
	{
		char oldmap[PLATFORM_MAX_PATH];
		GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);

		Forward_OnNominationRemoved(oldmap, owner);

		RemoveFromArray(g_NominateList, index);
		RemoveFromArray(g_NominateOwners, index);
		ClearStoredNomination(owner);
		g_NominateCount--;

		return true;
	}

	return false;
}

bool InternalIsMapOfficial(const char[] mapname)
{
	int officialMapIndex = FindStringInArray(g_OfficialList, mapname);
	return (officialMapIndex > -1);
}

stock int InternalGetMapCooldown(const char[] map)
{
	int Cooldown = g_iExcludeMaps;
	if (!g_Config)
		return Cooldown;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return Cooldown;

	return g_Config.GetNum("Cooldown", Cooldown);
}

public int InternalGetMapMaxExtends(const char[] map)
{
	int extends = g_iMaxExtends;
	if (!g_Config)
		return extends;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return extends;

	return g_Config.GetNum("Extends", extends);
}

public int InternalGetMapExtendTime(const char[] map)
{
	int time = g_iExtendTimeStep;
	if (!g_Config)
		return time;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return time;

	return g_Config.GetNum("ExtendTime", time);
}

public int InternalGetMapExtendRound(const char[] map)
{
	int round = g_iExtendRoundStep;
	if (!g_Config)
		return round;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return round;

	return g_Config.GetNum("ExtendRound", round);
}

public int InternalGetMapExtendFrag(const char[] map)
{
	int frag = g_iExtendFragStep;
	if (!g_Config)
		return frag;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return frag;

	return g_Config.GetNum("ExtendFrag", frag);
}

public int InternalGetMapTimeLimit(const char[] map)
{
	int time = g_Cvar_TimeLimit.IntValue;
	if (!g_Config)
		return time;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return time;

	return g_Config.GetNum("TimeLimit", time);
}

stock int InternalGetMapCooldownTime(const char[] map)
{
	int Cooldown = 0;
	if (!g_Config)
		return Cooldown;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return Cooldown;

	char time[16];
	g_Config.GetString("CooldownTime", time, sizeof(time), "");
	if (time[0])
		Cooldown = TimeStrToSeconds(time);
	else
		Cooldown = TimeStrToSeconds(g_sExcludeMapsTime);

	return Cooldown;
}

stock int InternalGetMapMinTime(const char[] map)
{
	int MinTime = 0;
	if (!g_Config)
		return MinTime;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return MinTime;
	
	MinTime = g_Config.GetNum("MinTime", MinTime);
	return MinTime;
}

stock int InternalGetMapMaxTime(const char[] map)
{
	int MaxTime = 0;
	if (!g_Config)
		return MaxTime;
	
	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return MaxTime;
	
	MaxTime = g_Config.GetNum("MaxTime", MaxTime);
	return MaxTime;
}

stock int InternalGetMapMinPlayers(const char[] map)
{
	int MinPlayers = 0;
	if (!g_Config)
		return MinPlayers;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return MinPlayers;

	MinPlayers = g_Config.GetNum("MinPlayers", MinPlayers);
	return MinPlayers;
}

stock int InternalGetMapMaxPlayers(const char[] map)
{
	int MaxPlayers = 0;
	if (!g_Config)
		return MaxPlayers;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return MaxPlayers;

	MaxPlayers = g_Config.GetNum("MaxPlayers", MaxPlayers);
	return MaxPlayers;
}

stock int InternalGetMapGroupID(const char[] map)
{
	int group = 0;
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		if (!g_Config.GotoFirstSubKey(false))
		{
			g_Config.Rewind();
			return group;
		}

		do
		{
			char groupstr[8];
			g_Config.GetSectionName(groupstr, sizeof(groupstr));
			int group_ = StringToInt(groupstr);
			if (g_Config.JumpToKey(map, false))
			{
				group = group_;
				g_Config.Rewind();
				return group;
			}
		} while(g_Config.GotoNextKey());

		g_Config.Rewind();
	}

	return group;
}

stock int InternalGetMapGroups(const char[] map, int[] groups, int size)
{
	int found = 0;
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		if (!g_Config.GotoFirstSubKey(false))
		{
			g_Config.Rewind();
			return -999;
		}

		do
		{
			char groupstr[8];
			g_Config.GetSectionName(groupstr, sizeof(groupstr));
			int group = StringToInt(groupstr);
			if (g_Config.JumpToKey(map, false))
			{
				groups[found++] = group;
				if (found >= size)
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
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		if (g_Config.JumpToKey(groupstr, false))
		{
			int max = g_Config.GetNum("_max", -1);
			g_Config.Rewind();
			return max;
		}

		g_Config.Rewind();
	}

	return -1;
}

stock int InternalGetGroupCooldown(int group)
{
	char groupstr[8];
	IntToString(group, groupstr, sizeof(groupstr));
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		if (g_Config.JumpToKey(groupstr, false))
		{
			int cd = g_Config.GetNum("_cooldown", -1);
			g_Config.Rewind();
			return cd;
		}

		g_Config.Rewind();
	}

	return -1;
}

stock int InternalGetGroupCooldownTime(int group)
{
	char groupstr[8];
	IntToString(group, groupstr, sizeof(groupstr));
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		if (g_Config.JumpToKey(groupstr, false))
		{
			char time[16];
			g_Config.GetString("_cooldown_time", time, sizeof(time), "");
			if (time[0])
			{
				g_Config.Rewind();
				return TimeStrToSeconds(time);
			}
			g_Config.Rewind();
		}
	}
	return 0;
}

stock void InternalSetGroupCooldownToMaps(int group, int cooldown)
{
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		char groupstr[8];
		IntToString(group, groupstr, sizeof(groupstr));
		if (g_Config.JumpToKey(groupstr, false))
		{
			if (!g_Config.GotoFirstSubKey(false))
			{
				g_Config.Rewind();
				return;
			}

			do
			{
				char map[PLATFORM_MAX_PATH];
				g_Config.GetSectionName(map, sizeof(map));
				if (strncmp(map, "_", 1) == 0) // Skip: All groups settings, always starting with "_"
					continue;

				// Use time-based cooldown instead of counter-based
				int cooldownTime = InternalGetGroupCooldownTime(group);
				if (cooldownTime > 0)
				{
					ExcludeMapTime(map, cooldownTime, g_iSharedCDMode);
				}
				else
				{
					ExcludeMap(map, cooldown, g_iSharedCDMode);
				}
			}
			while (g_Config.GotoNextKey());

			g_Config.Rewind();
		}
	}
}

stock void InternalGetGroupName(int group)
{
	char groupstr[8];
	IntToString(group, groupstr, sizeof(groupstr));
	if (g_Config && g_Config.JumpToKey("_groups"))
	{
		if (g_Config.JumpToKey(groupstr, false))
		{
			char sGrpName[PLATFORM_MAX_PATH];
			g_Config.GetString("_name", sGrpName, sizeof(sGrpName), "This group has no name configured.");
			g_Config.Rewind();
			return;
		}

		g_Config.Rewind();
	}

	return;
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
	int NumPlayers = GetClientCountEx(g_bCountBots);
	int MinPlayers = InternalGetMapMinPlayers(map);
	int MaxPlayers = InternalGetMapMaxPlayers(map);

	if (MinPlayers && NumPlayers < MinPlayers)
		return NumPlayers - MinPlayers;

	if (MaxPlayers && NumPlayers > MaxPlayers)
		return NumPlayers - MaxPlayers;

	return 0;
}

stock bool InternalAreRestrictionsActive()
{
	if (!g_bNoRestrictionTimeframeEnable)
		return true;

	char sTime[8];
	FormatTime(sTime, sizeof(sTime), "%H%M");

	int CurTime = StringToInt(sTime);
	int MinTime = g_iTimeFrameMin;
	int MaxTime = g_iTimeFrameMax;

	//Wrap around.
	CurTime = (CurTime <= MinTime) ? CurTime + 2400 : CurTime;
	MaxTime = (MaxTime <= MinTime) ? MaxTime + 2400 : MaxTime;

	if ((MinTime <= CurTime <= MaxTime))
	{
		return false;
	}

	return true;
}

stock bool InternalGetMapAdminRestriction(const char[] map)
{
	if (!g_Config)
		return false;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return false;

	int iAdmin = 0;
	iAdmin = g_Config.GetNum("Admin", iAdmin);
	return view_as<bool>(iAdmin);
}

stock bool InternalGetMapVIPRestriction(const char[] map)
{
	if (!g_Config)
		return false;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return false;

	int VIP = 0;
	VIP = g_Config.GetNum("VIP", VIP);
	return view_as<bool>(VIP);
}

stock bool InternalGetMapLeaderRestriction(const char[] map)
{
	if (!g_Config)
		return false;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return false;

	int Leader = 0;
	Leader = g_Config.GetNum("Leader", Leader);
	return view_as<bool>(Leader);
}

stock bool InternalGetMapDescription(const char[] map, char[] buffer, int maxlen)
{
	if (!g_Config)
		return false;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return false;

	g_Config.GetString("Description", buffer, maxlen);
	if (!buffer[0])
		return false;

	return true;
}

stock bool InternalGetMapDisplayName(const char[] map, char[] buffer, int maxlen)
{
	if (!g_Config)
		return false;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return false;

	g_Config.GetString("DisplayName", buffer, maxlen);
	if (!buffer[0])
		return false;

	return true;
}

stock void InternalGetMapStatusBadges(const char[] map, char[] buffer, int maxlen, int client)
{
	char badges[128];

	InternalGetMapStatusBadge_Leader(map, badges, sizeof(badges), client);
	InternalGetMapStatusBadge_Admin(map, badges, sizeof(badges), client);
	InternalGetMapStatusBadge_VIP(map, badges, sizeof(badges), client);

	// Remove trailing |
	int len = strlen(badges);
	if (len > 0 && badges[len-1] == '|')
	{
		badges[len-1] = '\0';
	}

	Format(buffer, maxlen, "%s", badges);
}

stock void InternalGetMapStatusBadge_Leader(const char[] map, char[] buffer, int maxlen, int client)
{
	if (InternalGetMapLeaderRestriction(map))
	{
		Format(buffer, maxlen, "%s%T|", buffer, "Leader Restriction", client);
	}
}

stock void InternalGetMapStatusBadge_Admin(const char[] map, char[] buffer, int maxlen, int client)
{
	if (InternalGetMapAdminRestriction(map))
	{
		Format(buffer, maxlen, "%s%T|", buffer, "Admin Restriction", client);
	}
}

stock void InternalGetMapStatusBadge_VIP(const char[] map, char[] buffer, int maxlen, int client)
{
	if (InternalGetMapVIPRestriction(map))
	{
		Format(buffer, maxlen, "%s%T|", buffer, "VIP Restriction", client);
	}
}

stock void InternalFormatMapDisplay(const char[] map, char[] buffer, int maxlen, int client, bool includeBadges = true)
{
	char displayName[PLATFORM_MAX_PATH];
	char cleanName[PLATFORM_MAX_PATH];
	char badges[128];
	char description[128];

	// Get display name or use original map name
	if (InternalGetMapDisplayName(map, displayName, sizeof(displayName)))
	{
		strcopy(cleanName, sizeof(cleanName), displayName);
	}
	else
	{
		strcopy(cleanName, sizeof(cleanName), map);
	}

	// Get status badges
	if (includeBadges)
	{
		InternalGetMapStatusBadges(map, badges, sizeof(badges), client);
	}

	// Get description if available
	bool hasDescription = InternalGetMapDescription(map, description, sizeof(description));

	if (hasDescription && badges[0] != '\0')
	{
		Format(buffer, maxlen, "%s [%s | %s]", cleanName, badges, description);
	}
	else if (badges[0] != '\0')
	{
		Format(buffer, maxlen, "%s [%s]", cleanName, badges);
	}
	else if (hasDescription)
	{
		Format(buffer, maxlen, "%s [%s]", cleanName, description);
	}
	else
	{
		strcopy(buffer, maxlen, cleanName);
	}
}

stock void InternalRestoreMapCooldowns()
{
	char sCooldownFile[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, sCooldownFile, sizeof(sCooldownFile), "configs/mapchooser_extended/cooldowns.cfg");

	if (!FileExists(sCooldownFile))
	{
		LogMessage("Could not find cooldown file: \"%s\"", sCooldownFile);
		return;
	}

	KeyValues Cooldowns = new KeyValues("mapchooser_extended");

	if (!Cooldowns.ImportFromFile(sCooldownFile))
	{
		LogMessage("Unable to load cooldown file: \"%s\"", sCooldownFile);
		delete Cooldowns;
		return;
	}

	if (!Cooldowns.GotoFirstSubKey(true))
	{
		LogMessage("Unable to goto first sub key: \"%s\"", sCooldownFile);
		delete Cooldowns;
		return;
	}

	int Cooldown;
	char map[PLATFORM_MAX_PATH];

	do
	{
		if (!Cooldowns.GetSectionName(map, sizeof(map)))
		{
			LogMessage("Unable to get section name: \"%s\"", sCooldownFile);
			delete Cooldowns;
			return;
		}

		if (g_bForceLowerCase)
			StringToLowerCase(map);

		if ((Cooldown = Cooldowns.GetNum("Cooldown", -1)) > 0)
		{
			LogMessage("Restored cooldown: %s -> %d", map, Cooldown);
			g_OldMapList.SetValue(map, Cooldown, true);
		}

		if ((Cooldown = Cooldowns.GetNum("CooldownTime", -1)) > 0)
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

	if (!FileExists(sCooldownFile))
	{
		LogMessage("Could not find cooldown file: \"%s\"", sCooldownFile);
		return;
	}

	KeyValues Cooldowns = new KeyValues("mapchooser_extended");

	int Cooldown;
	char map[PLATFORM_MAX_PATH];

	StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
	for (int i = 0; i < OldMapListSnapshot.Length; i++)
	{
		OldMapListSnapshot.GetKey(i, map, sizeof(map));
		if (g_bForceLowerCase)
			StringToLowerCase(map);

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
	for (int i = 0; i < TimeMapListSnapshot.Length; i++)
	{
		TimeMapListSnapshot.GetKey(i, map, sizeof(map));
		if (g_bForceLowerCase)
			StringToLowerCase(map);

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

	if (!Cooldowns.ExportToFile(sCooldownFile))
	{
		LogMessage("Unable to export cooldown file: \"%s\"", sCooldownFile);
		delete Cooldowns;
		return;
	}

	delete Cooldowns;
}

stock void InternalShowMapGroups(int client, const char[] map)
{
	if (!g_Config) 
	{
		PrintToConsole(client, "Config is not loaded, no group information.");
		return;
	}
	g_Config.Rewind();
	PrintToConsole(client, "-----------------------------------------");
	PrintToConsole(client, "Group information");
	PrintToConsole(client, "-----------------------------------------");
	if (!g_Config.JumpToKey("_groups"))
	{
		PrintToConsole(client, "No groups in the config.");
		return;
	}

	if (!g_Config.GotoFirstSubKey())
	{
		PrintToConsole(client, "No groups in the config.");
		return;
	}

	char groupName[PLATFORM_MAX_PATH];
	int count = 0;
	
	do
	{
		if (!g_Config.JumpToKey(map))
			continue;
		g_Config.GoBack();
		
		count++;
		g_Config.GetString("_name", groupName, sizeof(groupName), "This group has no name configured.");
		PrintToConsole(client, "[%03d] Group Name: \"%s\"", count, groupName);

		int max = g_Config.GetNum("_max", -1);
		int cd = g_Config.GetNum("_cooldown", -1);
		char time[16];
		g_Config.GetString("_cooldown_time", time, sizeof(time), "");

		if (max > 0)
		{
			PrintToConsole(client, "[%03d] Group Max: %d", count, max);
		}
		if (cd > 0)
		{
			PrintToConsole(client, "[%03d] Group Cooldown: %d", count, cd);
		}
		if (time[0])
		{
			PrintToConsole(client, "[%03d] Group Time Cooldown: %s", count, time);
		}
		PrintToConsole(client, "-----------------------------------");
	}
	while(g_Config.GotoNextKey());

	PrintToConsole(client, "%s is in %d groups", map, count);
	PrintToConsole(client, "-----------------------------------------");
}

stock bool IsMapValidEx(const char[] map)
{
	char displayName[PLATFORM_MAX_PATH];
	FindMapResult result = FindMap(map, displayName, sizeof(displayName));
	return (result == FindMap_Found);
}

/* Tiers Internal functions */
stock int InternalGetMapTier(const char[] map)
{
	if (!g_MapTiers)
		return 0;

	int tier = g_iDefaultTier;
	if (!g_Config)
		return tier;

	g_Config.Rewind();
	if (!g_Config.JumpToKey(map))
		return tier;

	tier = g_Config.GetNum("Tier", tier);
	return tier;
}

stock bool InternalGetMapTierName(const char[] map, char[] tierName, int maxlen)
{		
	int tier = InternalGetMapTier(map);
	return InternalGetTierNameFromInt(tier, tierName, maxlen);
}

stock bool InternalGetTierNameFromInt(int tier, char[] tierName, int maxlen)
{
	if (!g_MapTiers)
		return false;

	int arrayLength = GetArraySize(g_MapTiers);
	if (!arrayLength || tier <= 0 || tier > arrayLength)
		return false;

	GetArrayString(g_MapTiers, tier - 1, tierName, maxlen);
	return true;
}

stock int InternalGetTiersCount()
{
	if (!g_MapTiers)
		return 0;

	return GetArraySize(g_MapTiers);
}

stock bool InternalShowMapTierInChat()
{
	return g_bShowMapTierInChat;
}