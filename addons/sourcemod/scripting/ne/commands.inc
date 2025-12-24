/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/commands.inc
*  Description:   Console command creation and hooking
* 
* ============================================================================
*/

stock void CommandsInit()
{
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
}

public Action Command_Addmap(int client, int args)
{
	if (args == 0)
	{
		AttemptAdminNominate(client);
		return Plugin_Handled;
	}

	if (args != 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_addmap <mapname>");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int status;
	if (!GetTrieValue(g_mapTrie, mapname, status))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map Not In Pool", mapname);
		AttemptAdminNominate(client, mapname);
		return Plugin_Handled;
	}

	if (!CheckCommandAccess(client, "sm_nominate_ignore", ADMFLAG_CHEATS, true))
	{
		bool restrictionsActive = AreRestrictionsActive();

		if (GetTrieValue(g_mapTrie, mapname, status))
		{
			if ((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
			{
				if ((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
					CPrintToChat(client, "{green}[NE]{default} %t", "Can't Nominate Current Map");

				if (restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
				{
					int Cooldown = GetMapCooldown(mapname);
					CPrintToChat(client, "{green}[NE]{default} %t (%d)", "Map in Exclude List", Cooldown);
				}

				if ((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
					CPrintToChat(client, "{green}[NE]{default} %t", "Map Already Nominated");

				return Plugin_Handled;
			}
		}

		int iCurrentTime = GetTime();
		int Cooldown = GetMapCooldownTime(mapname);
		if (restrictionsActive && Cooldown > iCurrentTime)
		{
			int Seconds = Cooldown - iCurrentTime;
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Cooldown Time Error", Seconds / 3600, (Seconds % 3600) / 60);

			return Plugin_Handled;
		}

		int TimeRestriction = GetMapTimeRestriction(mapname);
		if (restrictionsActive && TimeRestriction)
		{
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Time Error", TimeRestriction / 60, TimeRestriction % 60);

			return Plugin_Handled;
		}

		int PlayerRestriction = GetMapPlayerRestriction(mapname);
		if (restrictionsActive && PlayerRestriction)
		{
			if (PlayerRestriction < 0)
				CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MinPlayers Error", PlayerRestriction * -1);
			else
				CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MaxPlayers Error", PlayerRestriction);

			return Plugin_Handled;
		}

		int GroupRestriction = GetMapGroupRestriction(mapname);
		if (restrictionsActive && GroupRestriction >= 0)
		{
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Group Error", GroupRestriction);
			return Plugin_Handled;
		}
	}

	NominateResult result = NominateMap(mapname, true, 0);

	if (result > Nominate_InvalidMap)
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map was not found", mapname);
		return Plugin_Handled;
	}

	if (result > Nominate_Replaced)
	{
		/* We assume already in vote is the casue because the maplist does a Map Validity check and we forced, so it can't be full */
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map Already In Vote", mapname);

		return Plugin_Handled;
	}

	/* Map was nominated! - Disable the menu item and update the trie */
	SetTrieValue(g_mapTrie, mapname, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);

	CReplyToCommand(client, "{green}[NE]{default} %t", "Map Inserted", mapname);
	CPrintToChatAll("{green}[NE]{default} %t", "Client inserted Map", client, mapname);

	LogAction(client, -1, "\"%L\" has inserted map \"%s\".", client, mapname);
	Forward_OnAdminMapInsert(client, mapname);
	return Plugin_Handled;
}

public Action Command_Removemap(int client, int args)
{
	if (args == 0 && client > 0)
	{
		AttemptAdminRemoveMap(client);
		return Plugin_Handled;
	}

	if (args != 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_removemap <mapname>");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int status;
	if (!GetTrieValue(g_mapTrie, mapname, status))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map was not found", mapname);
		AttemptAdminRemoveMap(client, mapname);
		return Plugin_Handled;
	}

	if (!RemoveNominationByMap(mapname))
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Map Not In Pool", mapname);

		return Plugin_Handled;
	}

	CReplyToCommand(client, "{green}[NE]{default} %t", "Map Removed", mapname);
	CPrintToChatAll("{green}[NE]{default} %t", "Client removed Map", client, mapname);

	LogAction(client, -1, "\"%L\" has removed map \"%s\" from nominations.", client, mapname);
	Forward_OnMapNominationRemove(client, mapname);
	return Plugin_Handled;
}

public Action Command_AddExclude(int client, int args)
{
	if (args < 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_exclude <mapname> [cooldown] [mode]");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int cooldown = 0;
	int mode = 0;
	if (args >= 2)
	{
		static char buffer[8];
		GetCmdArg(2, buffer, sizeof(buffer));
		cooldown = StringToInt(buffer);
	}
	if (args >= 3)
	{
		static char buffer[8];
		GetCmdArg(3, buffer, sizeof(buffer));
		mode = StringToInt(buffer);
	}

	int status;
	if (!GetTrieValue(g_mapTrie, mapname, status))
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
	if (args < 1)
	{
		CReplyToCommand(client, "{green}[NE]{default} Usage: {lightgreen}sm_nominate_exclude_time <mapname> [cooldown] [mode]");
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));

	int cooldown = 0;
	int mode = 0;
	if (args >= 2)
	{
		static char buffer[16];
		GetCmdArg(2, buffer, sizeof(buffer));
		cooldown = TimeStrToSeconds(buffer);
	}
	if (args >= 3)
	{
		static char buffer[8];
		GetCmdArg(3, buffer, sizeof(buffer));
		mode = StringToInt(buffer);
	}

	int status;
	if (!GetTrieValue(g_mapTrie, mapname, status))
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
	if (GetCmdArgs() != 2)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Nomban usage");
		return Plugin_Handled;
	}
	
	char target_argument[64];
	GetCmdArg(1, target_argument, sizeof(target_argument));
	
	int target = -1;
	if ((target = FindTarget(client, target_argument, true, false)) == -1)
	{
		return Plugin_Handled;
	}

	if (IsClientNomBanned(target))
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
	if (GetCmdArgs() < 1)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "UnNomban usage");
		return Plugin_Handled;
	}
	
	char target_argument[64];
	GetCmdArg(1, target_argument, sizeof(target_argument));
	
	int target = -1;
	if ((target = FindTarget(client, target_argument, true, false)) == -1)
		return Plugin_Handled;

	if (!IsClientNomBanned(target))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Already not nombanned", target);
		return Plugin_Handled;
	}
	
	UnNomBanClient(target, client);
		
	return Plugin_Handled;
}

public Action Command_NombanList(int client, int args)
{
	if (client == 0)
	{
		PrintToServer("--------------------");
		PrintToServer("Nomban List");
		PrintToServer("--------------------");

		for (int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientNomBanned(i))
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

public Action Command_DisableNE(int client, int args)
{
	if (!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Nominations already disabled");
		return Plugin_Handled;
	}

	if (IsNextMapAlreadySet())
	{
		char nextMap[PLATFORM_MAX_PATH];
		GetNextMap(nextMap, sizeof(nextMap));
		CReplyToCommand(client, "{green}[NE]{default} %t", "Nextmap", nextMap);
		return Plugin_Handled;
	}

	g_bNEAllowed = false;
	CPrintToChatAll("{green}[NE]{default} %t", "Nominations Locked");
	return Plugin_Handled;
}

public Action Command_EnableNE(int client, int args)
{
	if (g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Nominations already enabled");
		return Plugin_Handled;
	}

	if (IsNextMapAlreadySet())
	{
		char nextMap[PLATFORM_MAX_PATH];
		GetNextMap(nextMap, sizeof(nextMap));
		CReplyToCommand(client, "{green}[NE]{default} %t", "Nextmap", nextMap);
		return Plugin_Handled;
	}

	g_bNEAllowed = true;
	g_NominationDelay = 0;
	CPrintToChatAll("{green}[NE]{default} %t", "Nominations Unlocked");
	return Plugin_Handled;
}

public Action Command_Say(int client, int args)
{
	if (!client)
		return Plugin_Continue;

	static char text[192];
	if (!GetCmdArgString(text, sizeof(text)))
		return Plugin_Continue;

	int startidx = 0;
	if (text[strlen(text)-1] == '"')
	{
		text[strlen(text)-1] = '\0';
		startidx = 1;
	}

	ReplySource old = SetCmdReplySource(SM_REPLY_TO_CHAT);

	if (strcmp(text[startidx], "nominate", false) == 0)
	{
		if (IsNominateAllowed(client))
		{
			int iCurrentTime = GetTime();
			if (g_NominationDelay > iCurrentTime)
				CReplyToCommand(client, "{green}[NE]{default} %t", "Nominations unlock countdown", g_NominationDelay - iCurrentTime);
			if (!g_bNEAllowed)
			{
				CReplyToCommand(client, "{green}[NE]{default} %t", "Nominations Locked");
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
	if (!client || !IsNominateAllowed(client))
		return Plugin_Handled;

	int iCurrentTime = GetTime();
	if (g_NominationDelay > iCurrentTime)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Nominations unlock countdown", g_NominationDelay - iCurrentTime);
		return Plugin_Handled;
	}

	if (!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} %t", "Nominations Locked");
		return Plugin_Handled;
	}
	
	if (args == 0)
	{
		AttemptNominate(client);
		return Plugin_Handled;
	}

	if (g_Player_NominationDelay[client] > iCurrentTime)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Wait to Nominate", g_Player_NominationDelay[client] - iCurrentTime);
		return Plugin_Handled;
	}

	static char mapname[PLATFORM_MAX_PATH];
	GetCmdArg(1, mapname, sizeof(mapname));


	int status;
	if (!GetTrieValue(g_mapTrie, mapname, status))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Not In Pool", mapname);
		AttemptNominate(client, mapname);
		return Plugin_Handled;
	}

	bool restrictionsActive = AreRestrictionsActive();

	if ((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
	{
		if ((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
			CPrintToChat(client, "{green}[NE]{default} %t", "Can't Nominate Current Map");

		if (restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
		{
			int Cooldown = GetMapCooldown(mapname);
			CPrintToChat(client, "{green}[NE]{default} %t (%d)", "Map in Exclude List", Cooldown);
		}

		if ((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Already Nominated");

		return Plugin_Handled;
	}

	int Cooldown = GetMapCooldownTime(mapname);
	if (restrictionsActive && Cooldown > iCurrentTime)
	{
		int Seconds = Cooldown - iCurrentTime;
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Cooldown Time Error", Seconds / 3600, (Seconds % 3600) / 60);

		return Plugin_Handled;
	}

	bool adminRestriction = IsClientMapAdminRestricted(mapname, client);
	if (restrictionsActive && adminRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Admin Error");

		return Plugin_Handled;
	}

	bool VIPRestriction = IsClientMapVIPRestricted(mapname, client);
	if (restrictionsActive && VIPRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate VIP Error");

		return Plugin_Handled;
	}

	#if defined _zleader_included
	bool LeaderRestriction = IsClientMapLeaderRestricted(mapname, client);
	if (restrictionsActive && LeaderRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Leader Error");

		return Plugin_Handled;
	}
	#endif

	int TimeRestriction = GetMapTimeRestriction(mapname);
	if (restrictionsActive && TimeRestriction)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Time Error", TimeRestriction / 60, TimeRestriction % 60);

		return Plugin_Handled;
	}

	int PlayerRestriction = GetMapPlayerRestriction(mapname);
	if (restrictionsActive && PlayerRestriction)
	{
		if (PlayerRestriction < 0)
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MinPlayers Error", PlayerRestriction * -1);
		else
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate MaxPlayers Error", PlayerRestriction);

		return Plugin_Handled;
	}

	int GroupRestriction = GetMapGroupRestriction(mapname, client);
	if (restrictionsActive && GroupRestriction >= 0)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Map Nominate Group Error", GroupRestriction);
		return Plugin_Handled;
	}

	NominateResult result = NominateMap(mapname, false, client);
	
	char mapnameInChat[64];
	strcopy(mapnameInChat, sizeof(mapnameInChat), mapname);
	int mapTier;
	if (MCE_CanShowMapTierInChat() && (mapTier = GetMapTier(mapname)) && mapTier > 0)
	{
		char tierName[32];
		if (GetTierNameFromInt(mapTier, tierName, sizeof(tierName)))
			Format(mapnameInChat, sizeof(mapnameInChat), "%s %t", mapname, "MapTierDisplay", tierName);
	}
	
	if (result > Nominate_Replaced)
	{
		if (result == Nominate_AlreadyInVote)
			CPrintToChat(client, "{green}[NE]{default} %t", "Map Already In Vote", mapnameInChat);
		else if (result == Nominate_VoteFull)
			CPrintToChat(client, "{green}[NE]{default} %t", "Max Nominations");

		return Plugin_Handled;
	}

	/* Map was nominated! - Disable the menu item and update the trie */
	SetTrieValue(g_mapTrie, mapname, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);
	g_Player_NominationDelay[client] = iCurrentTime + GetConVarInt(g_Cvar_NominateDelay);

	static char name[MAX_NAME_LENGTH];
	GetClientName(client, name, sizeof(name));

	if (result == Nominate_Added)
	{
		CPrintToChatAll("{green}[NE]{default} %t", "Map Nominated", name, mapnameInChat);
		LogMessage("%L has nominated %s", client, mapname);
		Forward_OnPublicMapInsert(client, mapname, IsMapVIPRestricted(mapname), IsMapLeaderRestricted(mapname), IsMapAdminRestricted(mapname));
	}
	else if (result == Nominate_Replaced)
	{
		CPrintToChatAll("{green}[NE]{default} %t", "Map Nomination Changed", name, mapnameInChat);
		LogMessage("%L has changed their nomination to %s", client, mapname);
		Forward_OnPublicMapReplaced(client, mapname, IsMapVIPRestricted(mapname), IsMapLeaderRestricted(mapname), IsMapAdminRestricted(mapname));
	}

	return Plugin_Continue;
}

public Action Command_UnNominate(int client, int args)
{
	if (!client)
		return Plugin_Handled;

	char map[PLATFORM_MAX_PATH];
	if (!g_bNEAllowed || !GetNominationByOwner(client, map) || !RemoveNominationByOwner(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Unable To Unnominate");
		return Plugin_Handled;
	}
	
	char mapnameInChat[64];
	strcopy(mapnameInChat, sizeof(mapnameInChat), map);
	int mapTier;
	if (MCE_CanShowMapTierInChat() && (mapTier = GetMapTier(map)) && mapTier > 0)
	{
		char tierName[32];
		if (GetTierNameFromInt(mapTier, tierName, sizeof(tierName)))
			Format(mapnameInChat, sizeof(mapnameInChat), "%s %t", map, "MapTierDisplay", tierName);
	}
	
	CPrintToChatAll("{green}[NE]{default} %t", "Map Unnominated", client, mapnameInChat);
	LogAction(client, -1, "\"%L\" has removed his nomination \"%s\".", client, map);

	return Plugin_Handled;
}

public Action Command_NomStatus(int client, int args)
{
	if (client == 0)
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Can only use command in game");
		return Plugin_Handled;
	}

	int target = client;

	if (args > 0)
	{
		char targ[64];
		GetCmdArg(1, targ, sizeof(targ));
		target = FindTarget(client, targ, true, false);
	}

	if (!AreClientCookiesCached(target))
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
		for (int i = 0; i < GetArraySize(MapList); i++)
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

	if (!PopulateNominateListMenu(NominateListMenu, client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "No Maps Nominated");
		return Plugin_Handled;
	}

	SetMenuTitle(NominateListMenu, "Nominated Maps", client);
	DisplayMenu(NominateListMenu, client, MENU_TIME_FOREVER);

	return Plugin_Handled;
}