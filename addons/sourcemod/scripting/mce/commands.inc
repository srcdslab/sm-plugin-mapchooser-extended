/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/commands.inc
*  Description:   Console command creation and hooking
* 
* ============================================================================
*/

stock void CommandsInit()
{
	RegAdminCmd("sm_mapvote", Command_Mapvote, ADMFLAG_CHANGEMAP, "sm_mapvote - Forces MapChooser to attempt to run a map vote now.");
	RegAdminCmd("sm_setnextmap", Command_SetNextmap, ADMFLAG_CHANGEMAP, "sm_setnextmap <map>");

	// Mapchooser Extended Commands
	RegAdminCmd("mce_reload_maplist", Command_ReloadMaps, ADMFLAG_CHANGEMAP, "mce_reload_maplist - Reload the Official Maplist file.");

	RegConsoleCmd("sm_extends", Command_ExtendsLeft, "sm_extends - Shows how many extends are left on the current map.");
	RegConsoleCmd("sm_extendsleft", Command_ExtendsLeft, "sm_extendsleft - Shows how many extends are left on the current map.");

	RegConsoleCmd("sm_showmapcfg", Command_ShowConfig, "Shows all config information about the current map.");
	RegConsoleCmd("sm_showmapconfig", Command_ShowConfig, "Shows all config information about the current map.");

	RegConsoleCmd("sm_mcversion", Command_Version, "Mapchooser version");
	RegConsoleCmd("sm_mceversion", Command_Version, "Mapchooser version");
}

public Action Command_Version(int client, int args)
{
	CPrintToChat(client, "{green}[MCE]{default} Version %s", MCE_VERSION);
	return Plugin_Handled;
}

public Action Command_SetNextmap(int client, int args)
{
	if (args < 1)
	{
		CReplyToCommand(client, "{green}[MCE]{default} Usage: {lightgreen}sm_setnextmap <map>");
		return Plugin_Handled;
	}

	static char map[PLATFORM_MAX_PATH];
	GetCmdArg(1, map, PLATFORM_MAX_PATH);

	if (!IsMapValid(map))
	{
		CReplyToCommand(client, "{green}[MCE]{default} %t", "Map was not found", map);
		return Plugin_Handled;
	}

	CShowActivity(client, "%t", "Changed Next Map", map);
	LogAction(client, -1, "[MCE] \"%L\" changed nextmap to \"%s\"", client, map);

	SetNextMap(map);
	Forward_OnSetNextMap(map);
	g_MapVoteCompleted = true;

	return Plugin_Handled;
}

public Action Command_ReloadMaps(int client, int args)
{
	InitializeOfficialMapList();
	return Plugin_Handled;
}

public Action Command_ShowConfig(int client, int args)
{
	char map[PLATFORM_MAX_PATH];

	if (args == 0)
		GetCurrentMap(map, sizeof(map));
	else
	{
		GetCmdArg(1, map, sizeof(map));
		if (FindStringInArray(g_MapList, map) == -1)
		{
			CReplyToCommand(client, "{green}[MCE]{default} %t", "Map was not found", map);
			return Plugin_Handled;
		}
	}

	int extends = InternalGetMapMaxExtends(map);
	int extendtime = InternalGetMapExtendTime(map);
	int extendround = InternalGetMapExtendRound(map);
	int extendfrag = InternalGetMapExtendFrag(map);
	int cooldown = InternalGetMapCooldown(map);
	int minplayer = InternalGetMapMinPlayers(map);
	int maxplayer = InternalGetMapMaxPlayers(map);
	int mintime = InternalGetMapMinTime(map);
	int maxtime = InternalGetMapMaxTime(map);
	bool adminonly = InternalGetMapAdminRestriction(map);
	bool viponly = InternalGetMapVIPRestriction(map);
	bool leaderonly = InternalGetMapLeaderRestriction(map);
	// char desc[MAX_DESCRIPTION_LENGTH];
	// bool descr = InternalGetMapDescription(map, desc, sizeof(desc));

	if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
	{
		CPrintToChat(client, "{green}[MCE]{default} %t", "See console for output");
	}
	
	PrintToConsole(client, "-----------------------------------------");
	PrintToConsole(client, "Showing config info for: %s", map);
	PrintToConsole(client, "-----------------------------------------");
	PrintToConsole(client, "%-15s %5d", "Extends: ", extends);
	PrintToConsole(client, "%-15s %5d", "Extend Time: ", extendtime);
	PrintToConsole(client, "%-15s %5d", "Extend Round: ", extendround);
	PrintToConsole(client, "%-15s %5d", "Extend Frag: ", extendfrag);
	PrintToConsole(client, "%-15s %5d", "Cooldown: ", cooldown);
	PrintToConsole(client, "%-15s %5d", "Min Players: ", minplayer);
	PrintToConsole(client, "%-15s %5d", "Max Players: ", maxplayer);
	PrintToConsole(client, "%-15s %5d", "MinTime: ", mintime);
	PrintToConsole(client, "%-15s %5d", "MaxTime: ", maxtime);
	PrintToConsole(client, "%-15s %5b", "Admin Only: ", adminonly);
	PrintToConsole(client, "%-15s %5b", "VIP Only: ", viponly);
	PrintToConsole(client, "%-15s %5b", "Leader Only: ", leaderonly);
	// PrintToConsole(client, "%-15s %5s %s", "Description: ", descr?"Yes:":"No", desc);
	PrintToConsole(client, "-----------------------------------------");
	InternalShowMapGroups(client, map);
	return Plugin_Handled;
}

public Action Command_ExtendsLeft(int client, int args)
{
	char map[PLATFORM_MAX_PATH];
	GetCurrentMap(map, sizeof(map));

	int max = InternalGetMapMaxExtends(map);
	int left = max - g_Extends;

	CReplyToCommand(client, "{green}[MCE]{default} %t", "Available Extends", left);
	return Plugin_Handled;
}

public Action Command_Mapvote(int client, int args)
{
	CShowActivity2(client, "{green}[MCE]{olive} ", "{default}%t", "Initiated Vote Map");
	LogAction(client, -1, "[MCE] \"%L\" Initiated a map vote.", client);

	SetupWarningTimer(WarningType_Vote, MapChange_MapEnd, INVALID_HANDLE, true);

	//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);

	return Plugin_Handled;
}