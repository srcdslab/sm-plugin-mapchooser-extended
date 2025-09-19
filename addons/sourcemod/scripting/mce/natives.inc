/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/natives.inc
*  Description:   Natives handlers for the MCE API
* 
* ============================================================================
*/

stock void API_NativesInit()
{
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
	CreateNative("GetNominationByOwner", Native_GetNominationByOwner);
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
	CreateNative("GetMapMaxExtends", Native_GetMapMaxExtends);
	CreateNative("GetMapExtendTime", Native_GetMapExtendTime);
	CreateNative("GetMapExtendRounds", Native_GetMapExtendRound);
	CreateNative("GetMapExtendFrags", Native_GetMapExtendFrag);
	CreateNative("AreRestrictionsActive", Native_AreRestrictionsActive);
	CreateNative("SimulateMapEnd", Native_SimulateMapEnd);
	CreateNative("MCE_GetMapDescription", Native_GetMapDescription);
	CreateNative("MCE_GetMapDisplayName", Native_GetMapDisplayName);
}

/* Add natives to allow nominate and initiate vote to be call */

/* native  bool NominateMap(const char[] map, bool force, &NominateError:error); */
public int Native_NominateMap(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return view_as<int>(InternalNominateMap(map, GetNativeCell(2), GetNativeCell(3)));
}

/* native  bool GetNominationByOwner(owner, const char[] buffer); */
public int Native_GetNominationByOwner(Handle plugin, int numParams)
{
	int client = GetNativeCell(1);
	char map[PLATFORM_MAX_PATH];
	InternalGetNominationByOwner(client, map);

	if (!map[0])
		return 0;

	SetNativeString(2, map, PLATFORM_MAX_PATH);
	return 1;
}

/* native  bool RemoveNominationByMap(const char[] map); */
public int Native_RemoveNominationByMap(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return view_as<int>(InternalRemoveNominationByMap(map));
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

	LogAction(-1, -1, "[MCE] Starting map vote countdown because players RTV.");

	SetupWarningTimer(WarningType_Vote, when, inputarray);
	//InitiateVote(when, inputarray);
	return 0;
}

public int Native_CanVoteStart(Handle plugin, int numParams)
{
	return view_as<int>(CanVoteStart());
}

public int Native_CheckVoteDone(Handle plugin, int numParams)
{
	return view_as<int>(g_MapVoteCompleted);
}

public int Native_EndOfMapVoteEnabled(Handle plugin, int numParams)
{
	return view_as<int>(g_bEndOfMapVote);
}

public int Native_GetExcludeMapList(Handle plugin, int numParams)
{
	Handle array = view_as<Handle>(GetNativeCell(1));
	if (array == INVALID_HANDLE)
		return 0;

	static char map[PLATFORM_MAX_PATH];
	StringMapSnapshot OldMapListSnapshot = g_OldMapList.Snapshot();
	for (int i = 0; i < OldMapListSnapshot.Length; i++)
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

	if (maparray == INVALID_HANDLE)
		return 0;

	static char map[PLATFORM_MAX_PATH];

	for (int i = 0; i < GetArraySize(g_NominateList); i++)
	{
		GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);
		PushArrayString(maparray, map);

		// If the optional parameter for an owner list was passed, then we need to fill that out as well
		if (ownerarray != INVALID_HANDLE)
		{
			int index = GetArrayCell(g_NominateOwners, i);
			PushArrayCell(ownerarray, index);
		}
	}
	return 0;
}

public int Native_IsMapOfficial(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return view_as<int>(InternalIsMapOfficial(map));
}

public int Native_IsWarningTimer(Handle plugin, int numParams)
{
	return view_as<int>(g_WarningInProgress);
}

public int Native_CanNominate(Handle plugin, int numParams)
{
	if (g_HasVoteStarted)
	{
		return view_as<int>(CanNominate_No_VoteInProgress);
	}

	if (g_MapVoteCompleted)
	{
		return view_as<int>(CanNominate_No_VoteComplete);
	}

	if (g_NominateCount >= GetVoteSize())
	{
		return view_as<int>(CanNominate_No_VoteFull);
	}

	return view_as<int>(CanNominate_Yes);
}

public int Native_ExcludeMap(Handle plugin, int numParams)
{
	if (!InternalAreRestrictionsActive())
		return 0;

	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown;
	int Mode = GetNativeCell(3);

	if (Mode == 0)
	{
		// In TimeOnly mode, don't exclude via counter
		if (g_CooldownMode == CooldownMode_TimeOnly)
			return 0;
			
		Cooldown = InternalGetMapCooldown(map);
	}
	else if (Mode == 1)
	{
		Cooldown = GetNativeCell(2);
	}
	else if (Mode == 2)
	{
		g_OldMapList.GetValue(map, Cooldown);
		int NewCooldown = GetNativeCell(2);
		if (NewCooldown > Cooldown)
			Cooldown = NewCooldown;
	}

	g_OldMapList.SetValue(map, Cooldown, true);
	InternalStoreMapCooldowns();

	return 1;
}

public int Native_ExcludeMapTime(Handle plugin, int numParams)
{
	if (!InternalAreRestrictionsActive())
		return 0;

	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown;
	int Mode = GetNativeCell(3);
	int iCurrentTime = GetTime();

	if (Mode == 0)
	{
		// In CounterOnly mode, don't exclude via time
		if (g_CooldownMode == CooldownMode_CounterOnly)
			return 0;
			
		Cooldown = InternalGetMapCooldownTime(map);
	}
	else if (Mode == 1)
	{
		Cooldown = GetNativeCell(2);
	}
	else if (Mode == 2)
	{
		g_TimeMapList.GetValue(map, Cooldown);
		int NewCooldown = iCurrentTime + GetNativeCell(2);
		if (NewCooldown > Cooldown)
			Cooldown = GetNativeCell(2);
	}

	Cooldown += iCurrentTime;
	g_TimeMapList.SetValue(map, Cooldown, true);
	InternalStoreMapCooldowns();

	return 1;
}

public int Native_GetMapCooldown(Handle plugin, int numParams)
{
	if (!InternalAreRestrictionsActive())
		return 0;

	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int Cooldown = 0;
	g_OldMapList.GetValue(map, Cooldown);

	return Cooldown;
}

public int Native_GetMapCooldownTime(Handle plugin, int numParams)
{
	if (!InternalAreRestrictionsActive())
		return 0;

	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

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

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMinTime(map);
}

public int Native_GetMapMaxTime(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMaxTime(map);
}

public int Native_GetMapMinPlayers(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMinPlayers(map);
}

public int Native_GetMapMaxPlayers(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMaxPlayers(map);
}

public int Native_GetMapTimeRestriction(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapTimeRestriction(map);
}

public int Native_GetMapPlayerRestriction(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapPlayerRestriction(map);
}

public int Native_GetMapGroups(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);
	int size = GetNativeCell(3);

	if (len <= 0)
		return -999;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int[] groups = new int[size];
	int found = InternalGetMapGroups(map, groups, size);
	if (found >= 0)
		SetNativeArray(2, groups, size);
	return found;
}

public int Native_GetMapGroupRestriction(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return -999;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	int groups[32];
	int groupsfound = InternalGetMapGroups(map, groups, sizeof(groups));

	for (int group = 0; group < groupsfound; group ++)
	{
		int groupcur = 0;
		int groupmax = InternalGetGroupMax(groups[group]);

		if (groupmax >= 0)
		{
			static char map_[PLATFORM_MAX_PATH];
			int groups_[32];
			for (int i = 0; i < GetArraySize(g_NominateList); i++)
			{
				GetArrayString(g_NominateList, i, map_, PLATFORM_MAX_PATH);
				int tmp = InternalGetMapGroups(map_, groups_, sizeof(groups_));
				if (FindIntInArray(groups_, tmp, groups[group]) != -1)
					groupcur++;
			}

			if (groupcur >= groupmax)
			{
				// Check if client has nominated a map in the same group and can change their nomination
				bool okay = false;
				if (client >= 1 && client <= MaxClients)
				{
					int index = FindValueInArray(g_NominateOwners, client);
					if (index != -1)
					{
						static char oldmap[PLATFORM_MAX_PATH];
						GetArrayString(g_NominateList, index, oldmap, PLATFORM_MAX_PATH);
						static int oldgroups[32];
						int tmp = InternalGetMapGroups(oldmap, oldgroups, sizeof(oldgroups));
						if (FindIntInArray(groups_, tmp, groups[group]) != -1)
							okay = true;
					}
				}

				if (!okay)
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

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	// Check if client should bypass admin restrictions
	if (client >= 1 && client <= MaxClients)
	{
		// Client has bypass flag, dont return admin restrictions
		if (CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC))
			return 0;

		// Client has ban flag, dont return admin restrictions
		if (CheckCommandAccess(client, "sm_nominate_ban", ADMFLAG_BAN))
			return 0;
	}

	return InternalGetMapAdminRestriction(map);
}

public int Native_IsMapAdminRestricted(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapAdminRestriction(map);
}

public int Native_IsClientMapVIPRestricted(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	// Check if client should bypass vip restrictions
	if (client >= 1 && client <= MaxClients)
	{
		// Client has bypass flag, dont return vip restrictions
		if (CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_GENERIC))
			return 0;

		// Client has vip flag, dont return vip restrictions
		if (CheckCommandAccess(client, "sm_nominate_vip", ADMFLAG_CUSTOM1))
			return 0;
	}

	return InternalGetMapVIPRestriction(map);
}

public int Native_IsMapVIPRestricted(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapVIPRestriction(map);
}

public int Native_IsClientMapLeaderRestricted(Handle plugin, int numParams)
{
	int client = GetNativeCell(2);
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

#if defined _zleader_included
	bool bLeaderNativeAvailable = g_ZLeader && CanTestFeatures() && GetFeatureStatus(FeatureType_Native, "ZL_IsPossibleLeader") == FeatureStatus_Available;
	// Check if client should bypass leader restrictions
	if (bLeaderNativeAvailable)
	{
		if (client >= 1 && client <= MaxClients)
		{
			// Client has leader.ini access, dont return leader restrictions
			if (ZL_IsPossibleLeader(client))
				return 0;
		}
	}
#endif

	return InternalGetMapLeaderRestriction(map);
}

public int Native_IsMapLeaderRestricted(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapLeaderRestriction(map);
}

public int Native_GetExtendsLeft(Handle plugin, int numParams)
{
	char map[PLATFORM_MAX_PATH];
	GetCurrentMap(map, sizeof(map));
	return InternalGetMapMaxExtends(map) - g_Extends;
}

public int Native_GetMapMaxExtends(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapMaxExtends(map);
}

public int Native_GetMapExtendTime(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapExtendTime(map);
}

public int Native_GetMapExtendRound(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapExtendRound(map);
}

public int Native_GetMapExtendFrag(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);

	if (len <= 0)
		return 0;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	return InternalGetMapExtendFrag(map);
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

public int Native_GetMapDescription(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);
	if (len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	len = GetNativeCell(3);
	char[] buffer = new char[len];
	bool result = InternalGetMapDescription(map, buffer, len);

	SetNativeString(2, buffer, len);
	return result;
}

public int Native_GetMapDisplayName(Handle plugin, int numParams)
{
	int len;
	GetNativeStringLength(1, len);
	if (len <= 0)
		return false;

	char[] map = new char[len+1];
	GetNativeString(1, map, len+1);

	len = GetNativeCell(3);
	char[] buffer = new char[len];
	bool result = InternalGetMapDisplayName(map, buffer, len);

	// If the map display name is not found, use the original map name
	if (!result)
		strcopy(buffer, len, map);

	SetNativeString(2, buffer, len);
	return result;
}
