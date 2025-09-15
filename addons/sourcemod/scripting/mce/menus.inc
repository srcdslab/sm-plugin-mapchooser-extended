/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/menus.inc
*  Description:   Base menu functions for the plugin
* 
* ============================================================================
*/

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
	if (IsVoteInProgress())
	{
		// Can't start a vote, try again in 5 seconds.
		//g_RetryTimer = CreateTimer(5.0, Timer_StartMapVote, _, TIMER_FLAG_NO_MAPCHANGE);

		CPrintToChatAll("{green}[MCE]{default} %t", "Cannot Start Vote", FAILURE_TIMER_LENGTH);
		Handle data;
		g_RetryTimer = CreateDataTimer(1.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);

		/* Mapchooser Extended */
		WritePackCell(data, FAILURE_TIMER_LENGTH);

		if (g_bRunOff && g_RunoffCount > 0)
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
	if (g_MapVoteCompleted && g_ChangeMapInProgress)
		return;

	CheckMapRestrictions(true, true);
	CreateNextVote();

	g_ChangeTime = when;

	g_WaitingForVote = false;

	g_HasVoteStarted = true;

	Handle menuStyle = GetMenuStyleHandle(view_as<MenuStyle>(g_iMenuStyle));
	if (menuStyle != INVALID_HANDLE)
	{
		g_VoteMenu = CreateMenuEx(menuStyle, Handler_MapVoteMenu, MenuAction_End | MenuAction_Display | MenuAction_DisplayItem | MenuAction_VoteCancel);
	}
	else
	{
		// You chose... poorly
		g_VoteMenu = CreateMenu(Handler_MapVoteMenu, MenuAction_End | MenuAction_Display | MenuAction_DisplayItem | MenuAction_VoteCancel);
	}

	// Block Vote Slots
	if (g_bBlockSlots)
	{
		Handle radioStyle = GetMenuStyleHandle(MenuStyle_Radio);

		if (GetMenuStyle(g_VoteMenu) == radioStyle)
		{
			g_BlockedSlots = true;
			AddMenuItem(g_VoteMenu, LINE_ONE, "Choose something...", ITEMDRAW_DISABLED);
			AddMenuItem(g_VoteMenu, LINE_TWO, "...will ya?", ITEMDRAW_DISABLED);
			MenuRandomShuffleStart += 2;

			if (!g_bNoVoteOption) {
				AddMenuItem(g_VoteMenu, LINE_SPACER, "", ITEMDRAW_SPACER);
				MenuRandomShuffleStart++;
			}
		}
		else
			g_BlockedSlots = false;
	}

	if (g_bNoVoteOption)
		SetMenuOptionFlags(g_VoteMenu, MENUFLAG_BUTTON_NOVOTE);

	SetMenuTitle(g_VoteMenu, "Vote Nextmap");
	SetVoteResultCallback(g_VoteMenu, Handler_MapVoteFinished);

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

	/* No input given - Use our internal nominations and maplist, unless a pending revote exists */
	if (inputlist == INVALID_HANDLE && !g_PendingRevoteActive)
	{
		int allMapsSize = (PLATFORM_MAX_PATH * 3);
		char[] allMapsBuffer = new char[allMapsSize];

		Handle randomizeList = INVALID_HANDLE;
		if (g_bRandomizeNominations)
			randomizeList = CloneArray(g_NominateList);

		int nominateCount = GetArraySize(g_NominateList);

		int voteSize = GetVoteSize(2);

		/* Smaller of the two - It should be impossible for nominations to exceed the size though (cvar changed mid-map?) */
		int nominationsToAdd = nominateCount >= voteSize ? voteSize : nominateCount;

		if (g_bExtendPositionTop) {
			AddExtendToMenu(g_VoteMenu, when);
			MenuRandomShuffleStart++;
		}

		for (int i = 0; i < nominationsToAdd; i++)
		{
			GetArrayString(g_NominateList, i, map, PLATFORM_MAX_PATH);

			if (randomizeList == INVALID_HANDLE)
			{
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}

			RemoveStringFromArray(g_NextMapList, map);

			/* Notify Nominations that this map is now free */
			Forward_OnNominationRemoved(map, GetArrayCell(g_NominateOwners, i));
		}

		/* Clear out the rest of the nominations array */
		for (int i = nominationsToAdd; i < nominateCount; i++)
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

		if (i < voteSize && availableMaps == 0)
		{
			if (i == 0)
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

			if (randomizeList == INVALID_HANDLE)
			{
				/* Insert the map and increment our count */
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}
			else
				PushArrayString(randomizeList, map);
			i++;

			//Run out of maps, this will have to do.
			if (count >= availableMaps)
				break;
		}

		if (randomizeList != INVALID_HANDLE)
		{
			// Fisher-Yates Shuffle
			for (int j = GetArraySize(randomizeList) - 1; j >= 1; j--)
			{
				int k = GetRandomInt(0, j);
				SwapArrayItems(randomizeList, j, k);
			}

			for (int j = 0; j < GetArraySize(randomizeList); j++)
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

		if (!g_bExtendPositionTop) {
			if (InternalGetMapMaxExtends(map) - g_Extends > 0)
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, "Extend");
			AddExtendToMenu(g_VoteMenu, when);
			MenuRandomShuffleStop++;
		}

		if (allMapsBuffer[0])
			LogAction(-1, -1, "[MCE] Maps List: %s", allMapsBuffer);
	}
	else // We were given a list of maps to start the vote with (or a pending revote exists)
	{
		int size = GetArraySize(inputlist);

		int allMapsSize = (PLATFORM_MAX_PATH * 3);
		char[] allMapsBuffer = new char[allMapsSize];

		for (int i = 0; i < size; i++)
		{
			GetArrayString(inputlist, i, map, PLATFORM_MAX_PATH);

			if (IsMapValidEx(map))
			{
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}
			// New in Mapchooser Extended
			else if (strcmp(map, VOTE_DONTCHANGE) == 0)
			{
				AddMenuItem(g_VoteMenu, VOTE_DONTCHANGE, "Don't Change");
			}
			else if (strcmp(map, VOTE_EXTEND) == 0)
			{
				if (InternalGetMapMaxExtends(map) - g_Extends > 0)
					FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, "Extend");

				AddMenuItem(g_VoteMenu, VOTE_EXTEND, "Extend Map");
			}
		}
		delete inputlist;

		if (allMapsBuffer[0])
			LogAction(-1, -1, "[MCE] Maps List: %s", allMapsBuffer);
	}

	int voteDuration = g_iVoteDuration;

	//SetMenuExitButton(g_VoteMenu, false);

	if (GetVoteSize(2) <= GetMaxPageItems(GetMenuStyle(g_VoteMenu)))
	{
		//This is necessary to get items 9 and 0 as usable voting items
		SetMenuPagination(g_VoteMenu, MENU_NO_PAGINATION);
	}

	if (g_bShufflePerClient)
		MenuShufflePerClient(g_VoteMenu, MenuRandomShuffleStart, GetMenuItemCount(g_VoteMenu) - MenuRandomShuffleStop);

	VoteMenuToAll(g_VoteMenu, voteDuration);

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

	if (strcmp(map, VOTE_EXTEND, false) == 0)
	{
		g_Extends++;

		int time;
		if (GetMapTimeLimit(time))
		{
			if (time > 0)
				ExtendMapTimeLimit(InternalGetMapExtendTime(map)*60);
		}

		if (g_Cvar_Winlimit != INVALID_HANDLE)
		{
			int winlimit = GetConVarInt(g_Cvar_Winlimit);
			if (winlimit)
				SetConVarInt(g_Cvar_Winlimit, winlimit + InternalGetMapExtendRound(map));
		}

		if (g_Cvar_Maxrounds != INVALID_HANDLE)
		{
			int maxrounds = GetConVarInt(g_Cvar_Maxrounds);
			if (maxrounds)
				SetConVarInt(g_Cvar_Maxrounds, maxrounds + InternalGetMapExtendRound(map));
		}

		if (g_Cvar_Fraglimit != INVALID_HANDLE)
		{
			int fraglimit = GetConVarInt(g_Cvar_Fraglimit);
			if (fraglimit)
				SetConVarInt(g_Cvar_Fraglimit, fraglimit + InternalGetMapExtendFrag(map));
		}

		char sCurrentMap[64];
		GetCurrentMap(sCurrentMap, sizeof(sCurrentMap));
		int iExentedLeft = InternalGetMapMaxExtends(sCurrentMap) - g_Extends;

		CPrintToChatAll("{green}[MCE]{default} %t", "Current Map Extended", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);
		CPrintToChatAll("{green}[MCE]{default} %t", "Available Extends", iExentedLeft);

		LogAction(-1, -1, "[MCE] Voting for next map has finished. \nThe current map has been extended. (Received \"%d\"\%% of %d votes) \nAvailable Extends: %d", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes, iExentedLeft);

		if (g_bLockNominationsAtWarning)
		{
			CreateTimer(g_fTimerUnlockNoms, UnlockNominations, _, TIMER_FLAG_NO_MAPCHANGE);
		}

		// We extended, so we'll have to vote again.
		g_RunoffCount = 0;
		g_HasVoteStarted = false;
		SetupTimeleftTimer();

	}
	else if (strcmp(map, VOTE_DONTCHANGE, false) == 0)
	{
		CPrintToChatAll("{green}[MCE]{default} %t", "Current Map Stays", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);
		LogAction(-1, -1, "[MCE] Current map continues! The Vote has spoken! (Received \"%d\"\%% of %d votes)", RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0), num_votes);

		if (g_bLockNominationsAtWarning)
		{
			CreateTimer(g_fTimerUnlockNoms, UnlockNominations, _, TIMER_FLAG_NO_MAPCHANGE);
		}

		g_RunoffCount = 0;
		g_HasVoteStarted = false;
		SetupTimeleftTimer();
	}
	else
	{
		if (g_ChangeTime == MapChange_MapEnd)
		{
			SetNextMap(map);
			Forward_OnSetNextMap(map);
		}
		else if (g_ChangeTime == MapChange_Instant)
		{
			Handle data;
			CreateDataTimer(4.0, Timer_ChangeMap, data);
			WritePackString(data, map);
			g_ChangeMapInProgress = false;
		}
		else // MapChange_RoundEnd
		{
			SetNextMap(map);
			Forward_OnSetNextMap(map);
			g_ChangeMapAtRoundEnd = true;
		}

		g_HasVoteStarted = false;
		g_MapVoteCompleted = true;
		g_PendingRevoteActive = false;

		int percent = RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0);

		// Check who nominated the map
		char sPlayer[32], sBuffer[64];
		for (int i = 1; i <= MaxClients; i++)
		{
			if (strcmp(g_sNominations[i], map, false) != 0)
				continue;
			if (!IsClientInGame(i))
				continue;
			FormatEx(sPlayer, sizeof(sPlayer), "%N", i);
			break;
		}

		if (sPlayer[0] && g_bShowNominator)
			FormatEx(sBuffer, sizeof(sBuffer), "- %t %s", "Nominated by", sPlayer);

		CPrintToChatAll("{green}[MCE]{default} %t %s", "Nextmap Voting Finished", map, percent, num_votes, sBuffer);
		LogAction(-1, -1, "[MCE] Voting for next map has finished. \nNextmap: %s. (Received \"%d\"\%% of %d votes) %s", map, percent, num_votes, sBuffer);

		// Vote has finished, clear nominations
		for (int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientConnected(i) || !IsClientInGame(i))
				continue;

			ClearStoredNomination(i);
		}
	}
}

public void Handler_MapVoteFinished(Handle menu,
									int num_votes,
									int num_clients,
									const int[][] client_info,
									int num_items,
									const int[][] item_info)
{
	// Implement revote logic - Only run this block if revotes are enabled and this isn't the last revote
	if (g_bRunOff && num_items > 1 && g_RunoffCount < g_iMaxRunOffs)
	{
		g_RunoffCount++;
		int highest_votes = item_info[0][VOTEINFO_ITEM_VOTES];
		int required_percent = g_iPercentRunOff;
		int required_votes = RoundToCeil(float(num_votes) * float(g_iPercentRunOff) / 100.0);

		if (highest_votes == item_info[1][VOTEINFO_ITEM_VOTES])
		{
			g_HasVoteStarted = false;

			//Revote is needed
			ArrayList mapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH + 1));

			for (int i = 0; i < num_items; i++)
			{
				if (item_info[i][VOTEINFO_ITEM_VOTES] == highest_votes)
				{
					static char map[PLATFORM_MAX_PATH];

					GetMapItem(menu, item_info[i][VOTEINFO_ITEM_INDEX], map, PLATFORM_MAX_PATH);
					PushArrayString(mapList, map);
				}
				else
					break;
			}

			g_PendingRevoteActive = true;
			LogAction(-1, -1, "[MCE] The top maps had the same number of votes. A revote is needed!");
			CPrintToChatAll("{green}[MCE]{default} %t", "Tie Vote", GetArraySize(mapList));

			SetupWarningTimer(WarningType_Revote, view_as<MapChange>(g_ChangeTime), mapList);
			return;
		}
		else if (highest_votes < required_votes)
		{
			g_HasVoteStarted = false;

			//Revote is needed
			ArrayList mapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH + 1));

			static char map1[PLATFORM_MAX_PATH];
			GetMapItem(menu, item_info[0][VOTEINFO_ITEM_INDEX], map1, PLATFORM_MAX_PATH);

			PushArrayString(mapList, map1);

			// We allow more than two maps for a revote if they are tied
			for (int i = 1; i < num_items; i++)
			{
				if (GetArraySize(mapList) < 2 || item_info[i][VOTEINFO_ITEM_VOTES] == item_info[i - 1][VOTEINFO_ITEM_VOTES])
				{
					static char map[PLATFORM_MAX_PATH];
					GetMapItem(menu, item_info[i][VOTEINFO_ITEM_INDEX], map, PLATFORM_MAX_PATH);
					mapList.PushString(map);
				}
				else
					break;
			}

			g_PendingRevoteActive = true;
			LogAction(-1, -1, "[MCE] No map has received more than \"%d\"\%% of the vote.\nA revote is needed!", required_percent);

			CPrintToChatAll("{green}[MCE]{default} %t", "Revote Is Needed", required_percent);
			CPrintToChatAll("{green}[MCE]{default} %t", "Revote Is Needed New Vote");

			SetupWarningTimer(WarningType_Revote, view_as<MapChange>(g_ChangeTime), mapList);
			return;
		}
	}

	// No revote needed, continue as normal.
	Handler_VoteFinishedGeneric(menu, num_votes, num_clients, client_info, num_items, item_info);
}

public int Handler_MapVoteMenu(Handle menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			g_VoteMenu = INVALID_HANDLE;
			delete menu;
		}

		case MenuAction_Display:
		{
			static char buffer[255];
			Format(buffer, sizeof(buffer), "%T", "Vote Nextmap", param1);
			Handle panel = view_as<Handle>(param2);
			SetPanelTitle(panel, buffer);
			//DrawPanelText(panel, "Warning: The Position of the Maps are different for each Player.");
		}

		case MenuAction_DisplayItem:
		{
			char map[PLATFORM_MAX_PATH];
			char buffer[255];

			GetMenuItem(menu, param2, map, PLATFORM_MAX_PATH, _, _, _, param1);

			if (strcmp(map, VOTE_EXTEND, false) == 0)
			{
				Format(buffer, sizeof(buffer), "%T", "Extend Map", param1);
			}
			else if (strcmp(map, VOTE_DONTCHANGE, false) == 0)
			{
				Format(buffer, sizeof(buffer), "%T", "Dont Change", param1);
			}
			// Mapchooser Extended
			else if (strcmp(map, LINE_ONE, false) == 0)
			{
				Format(buffer, sizeof(buffer),"%T", "Line One", param1);
			}
			else if (strcmp(map, LINE_TWO, false) == 0)
			{
				Format(buffer, sizeof(buffer),"%T", "Line Two", param1);
			}
			// Note that the first part is to discard the spacer line
			else if (strcmp(map, LINE_SPACER, false) != 0)
			{
				if (g_iMarkCustomMaps == 1 && !InternalIsMapOfficial(map))
				{
					Format(buffer, sizeof(buffer), "%T", "Custom Marked", param1, map);
				}
				else if (g_iMarkCustomMaps == 2 && !InternalIsMapOfficial(map))
				{
					Format(buffer, sizeof(buffer), "%T", "Custom", param1, map);
				}
				else if (InternalGetMapVIPRestriction(map))
				{
					Format(buffer, sizeof(buffer), "%s (%T)", map, "VIP Nomination", param1);
				}
				else if (InternalGetMapLeaderRestriction(map))
				{
					Format(buffer, sizeof(buffer), "%s (%T)", map, "Leader Nomination", param1);
				}
			}

			if (buffer[0] != '\0')
			{
				return RedrawMenuItem(buffer);
			}
			// End Mapchooser Extended
		}

		case MenuAction_VoteCancel:
		{
			// If we receive 0 votes, pick at random.
			if (param1 == VoteCancel_NoVotes && g_bNoVoteMode)
			{
				int count = GetMenuItemCount(menu);

				int item;
				static char map[PLATFORM_MAX_PATH];

				do
				{
					int startInt = 0;
					if (g_BlockedSlots)
					{
						if (g_AddNoVote)
						{
							startInt = 2;
						}
						else
						{
							startInt = 3;
						}
					}
					item = GetRandomInt(startInt, count - 1);
					GetMenuItem(menu, item, map, PLATFORM_MAX_PATH, _, _, _, param1);
				}
				while(strcmp(map, VOTE_EXTEND, false) == 0);

				SetNextMap(map);
				Forward_OnSetNextMap(map);
				LogAction(-1, -1, "[MCE] No votes has been receive. Pickup a random map. Nextmap is : %s", map);
				g_MapVoteCompleted = true;
			}
			else
				LogAction(-1, -1, "[MCE] No votes has been receive.");

			g_HasVoteStarted = false;
		}
	}

	return 0;
}
