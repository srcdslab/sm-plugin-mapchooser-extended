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
 * @param force			Vote forced by sm_mapvote, ignores mce_endvote and a completed vote.
 */
void InitiateVote(MapChange when, ArrayList inputlist=null, bool force=false)
{
	g_WaitingForVote = true;
	g_WarningInProgress = false;
	int MenuRandomShuffleStart = 0;
	int MenuRandomShuffleStop = 0;

	// Check if a vote is in progress first
	if (IsVoteInProgress())
	{
		// Can't start a vote, try again in 5 seconds.
		CPrintToChatAll("{green}[MCE]{default} %t", "Cannot Start Vote", FAILURE_TIMER_LENGTH);

		g_WarningTimePassed = 0;

		DataPack data;
		g_RetryTimer = CreateDataTimer(1.0, Timer_StartMapVote, data, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);

		/* Mapchooser Extended */
		data.WriteCell(FAILURE_TIMER_LENGTH);

		if (g_bRunOff && g_RunoffCount > 0)
			data.WriteString("Revote Warning");
		else
			data.WriteString("Vote Warning");
		/* End Mapchooser Extended */

		data.WriteCell(view_as<int>(when));
		data.WriteCell(view_as<int>(force));
		data.WriteCell(view_as<int>(inputlist));
		data.Reset();
		return;
	}

	/* If the main map vote has completed (and chosen result) and its currently changing (not a delayed change) we block further attempts */
	if (g_MapVoteCompleted && g_ChangeMapInProgress)
	{
		g_WaitingForVote = false;
		return;
	}

	CheckMapRestrictions(true, true);
	CreateNextVote();

	g_ChangeTime = when;

	g_WaitingForVote = false;

	g_HasVoteStarted = true;

	Handle menuStyle = GetMenuStyleHandle(view_as<MenuStyle>(g_iMenuStyle));
	if (menuStyle != null)
	{
		g_VoteMenu = CreateMenuEx(menuStyle, Handler_MapVoteMenu, MenuAction_End | MenuAction_Display | MenuAction_DisplayItem | MenuAction_VoteCancel);
	}
	else
	{
		// You chose... poorly
		g_VoteMenu = new Menu(Handler_MapVoteMenu, MenuAction_End | MenuAction_Display | MenuAction_DisplayItem | MenuAction_VoteCancel);
	}

	// Block Vote Slots
	if (g_bBlockSlots)
	{
		Handle radioStyle = GetMenuStyleHandle(MenuStyle_Radio);

		if (g_VoteMenu.Style == radioStyle)
		{
			g_BlockedSlots = true;
			g_VoteMenu.AddItem(LINE_ONE, "Choose something...", ITEMDRAW_DISABLED);
			g_VoteMenu.AddItem(LINE_TWO, "...will ya?", ITEMDRAW_DISABLED);
			MenuRandomShuffleStart += 2;

			if (!g_bNoVoteOption) {
				g_VoteMenu.AddItem(LINE_SPACER, "", ITEMDRAW_SPACER);
				MenuRandomShuffleStart++;
			}
		}
		else
			g_BlockedSlots = false;
	}

	if (g_bNoVoteOption)
		g_VoteMenu.OptionFlags = MENUFLAG_BUTTON_NOVOTE;

	g_VoteMenu.SetTitle("Vote Nextmap");
	g_VoteMenu.VoteResultCallback = Handler_MapVoteFinished;

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

	/* No input given - Use our internal nominations and maplist */
	if (inputlist == null)
	{
		g_PendingRevoteActive = false;
		int allMapsSize = (PLATFORM_MAX_PATH * 3);
		char[] allMapsBuffer = new char[allMapsSize];

		ArrayList randomizeList = null;
		if (g_bRandomizeNominations)
			randomizeList = g_NominateList.Clone();

		int nominateCount = g_NominateList.Length;

		int voteSize = GetVoteSize(2);

		/* Smaller of the two - It should be impossible for nominations to exceed the size though (cvar changed mid-map?) */
		int nominationsToAdd = nominateCount >= voteSize ? voteSize : nominateCount;

		if (g_bExtendPositionTop) {
			AddExtendToMenu(g_VoteMenu, when);
			MenuRandomShuffleStart++;
		}

		for (int i = 0; i < nominationsToAdd; i++)
		{
			g_NominateList.GetString(i, map, PLATFORM_MAX_PATH);

			if (randomizeList == null)
			{
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}

			RemoveStringFromArray(g_NextMapList, map);

			/* Notify Nominations that this map is now free */
			Forward_OnNominationRemoved(map, g_NominateOwners.Get(i));
		}

		/* Clear out the rest of the nominations array */
		for (int i = nominationsToAdd; i < nominateCount; i++)
		{
			g_NominateList.GetString(i, map, PLATFORM_MAX_PATH);
			/* These maps shouldn't be excluded from the vote as they weren't really nominated at all */

			/* Notify Nominations that this map is now free */
			Forward_OnNominationRemoved(map, g_NominateOwners.Get(i));
		}

		/* There should currently be 'nominationsToAdd' unique maps in the vote */

		int i = nominationsToAdd;
		int count = 0;
		int availableMaps = g_NextMapList.Length;

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
			g_NextMapList.GetString(count, map, PLATFORM_MAX_PATH);
			count++;

			if (randomizeList == null)
			{
				/* Insert the map and increment our count */
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}
			else
				randomizeList.PushString(map);
			i++;

			//Run out of maps, this will have to do.
			if (count >= availableMaps)
				break;
		}

		if (randomizeList != null)
		{
			// Fisher-Yates Shuffle
			for (int j = randomizeList.Length - 1; j >= 1; j--)
			{
				int k = GetRandomInt(0, j);
				randomizeList.SwapAt(j, k);
			}

			for (int j = 0; j < randomizeList.Length; j++)
			{
				randomizeList.GetString(j, map, PLATFORM_MAX_PATH);
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}

			delete randomizeList;
		}

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
		int size = inputlist.Length;

		int allMapsSize = (PLATFORM_MAX_PATH * 3);
		char[] allMapsBuffer = new char[allMapsSize];

		for (int i = 0; i < size; i++)
		{
			inputlist.GetString(i, map, PLATFORM_MAX_PATH);

			if (IsMapValidEx(map))
			{
				FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, map);
				AddMapItem(map);
			}
			// New in Mapchooser Extended
			else if (strcmp(map, VOTE_DONTCHANGE) == 0)
			{
				g_VoteMenu.AddItem(VOTE_DONTCHANGE, "Don't Change");
			}
			else if (strcmp(map, VOTE_EXTEND) == 0)
			{
				if (InternalGetMapMaxExtends(map) - g_Extends > 0)
					FormatEx(allMapsBuffer, allMapsSize, "%s\n- %s", allMapsBuffer, "Extend");

				g_VoteMenu.AddItem(VOTE_EXTEND, "Extend Map");
			}
		}
		delete inputlist;

		if (allMapsBuffer[0])
			LogAction(-1, -1, "[MCE] Maps List: %s", allMapsBuffer);
	}

	int voteDuration = g_iVoteDuration;

	//SetMenuExitButton(g_VoteMenu, false);

	if (GetVoteSize(2) <= GetMaxPageItems(g_VoteMenu.Style))
	{
		//This is necessary to get items 9 and 0 as usable voting items
		g_VoteMenu.Pagination = MENU_NO_PAGINATION;
	}

	if (g_bShufflePerClient)
		MenuShufflePerClient(g_VoteMenu, MenuRandomShuffleStart, g_VoteMenu.ItemCount - MenuRandomShuffleStop);

	g_VoteMenu.DisplayVoteToAll(voteDuration);

	/* Call OnMapVoteStarted() Forward */
	Call_StartForward(g_MapVoteStartForward); // Deprecated
	Call_Finish();

	Call_StartForward(g_MapVoteStartedForward);
	Call_Finish();

	CPrintToChatAll("{green}[MCE]{default} %t", "Nextmap Voting Started");
}

public void Handler_VoteFinishedGeneric(Menu menu,
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

		if (g_Cvar_Winlimit != null)
		{
			int winlimit = g_Cvar_Winlimit.IntValue;
			if (winlimit)
				g_Cvar_Winlimit.SetInt(winlimit + InternalGetMapExtendRound(map));
		}

		if (g_Cvar_Maxrounds != null)
		{
			int maxrounds = g_Cvar_Maxrounds.IntValue;
			if (maxrounds)
				g_Cvar_Maxrounds.SetInt(maxrounds + InternalGetMapExtendRound(map));
		}

		if (g_Cvar_Fraglimit != null)
		{
			int fraglimit = g_Cvar_Fraglimit.IntValue;
			if (fraglimit)
				g_Cvar_Fraglimit.SetInt(fraglimit + InternalGetMapExtendFrag(map));
		}

		char sCurrentMap[64];
		GetCurrentMap(sCurrentMap, sizeof(sCurrentMap));
		int iExentedLeft = InternalGetMapMaxExtends(sCurrentMap) - g_Extends;
		int percent = RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0);

		CPrintToChatAll("{green}[MCE]{default} %t", "Current Map Extended", percent, num_votes);
		CPrintToChatAll("{green}[MCE]{default} %t", "Available Extends", iExentedLeft);

		LogAction(-1, -1, "[MCE] Voting for next map has finished. \nThe current map has been extended. (Received \"%d\"\%% of %d votes) \nAvailable Extends: %d", percent, num_votes, iExentedLeft);

		if (g_bLockNominationsAtWarning)
		{
			CreateTimer(g_fTimerUnlockNoms, UnlockNominations, _, TIMER_FLAG_NO_MAPCHANGE);
		}

		// We extended, so we'll have to vote again.
		g_RunoffCount = 0;
		g_HasVoteStarted = false;
		g_PendingRevoteActive = false;
		SetupTimeleftTimer();
		ClearNominationsLists();

	}
	else if (strcmp(map, VOTE_DONTCHANGE, false) == 0)
	{
		int percent = RoundToFloor(float(item_info[0][VOTEINFO_ITEM_VOTES])/float(num_votes)*100.0);

		CPrintToChatAll("{green}[MCE]{default} %t", "Current Map Stays", percent, num_votes);
		LogAction(-1, -1, "[MCE] Current map continues! The Vote has spoken! (Received \"%d\"\%% of %d votes)", percent, num_votes);

		if (g_bLockNominationsAtWarning)
		{
			CreateTimer(g_fTimerUnlockNoms, UnlockNominations, _, TIMER_FLAG_NO_MAPCHANGE);
		}

		g_RunoffCount = 0;
		g_HasVoteStarted = false;
		g_PendingRevoteActive = false;
		SetupTimeleftTimer();
		ClearNominationsLists();
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
			DataPack data;
			CreateDataTimer(4.0, Timer_ChangeMap, data);
			data.WriteString(map);
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

		int owner = -1;
		bool foundNomination = false;

		if (g_bShowNominator)
		{
			static char nominatedMap[PLATFORM_MAX_PATH];
			int nominateCount = g_NominateList.Length;
			for (int i = 0; i < nominateCount; i++)
			{
				g_NominateList.GetString(i, nominatedMap, sizeof(nominatedMap));
				if (strcmp(nominatedMap, map, false) == 0)
				{
					owner = g_NominateOwners.Get(i);
					foundNomination = true;
					break;
				}
			}
		}

		// Cache Nominator Name
		char ownerName[MAX_NAME_LENGTH];
		bool isOwnerConnected = (g_bShowNominator && foundNomination && owner > 0 && IsClientInGame(owner));
		if (isOwnerConnected)
			GetClientName(owner, ownerName, sizeof(ownerName));

		// Cache Map Display Name with Tier (Static)
		char mapInChat[128];
		int mapTier = InternalGetMapTier(map);
		if (InternalShowMapTierInChat() && mapTier > 0)
		{
			char tierName[32];
			InternalGetTierNameFromInt(mapTier, tierName, sizeof(tierName));
			FormatEx(mapInChat, sizeof(mapInChat), "%s (%s)", map, tierName);
		}
		else
		{
			strcopy(mapInChat, sizeof(mapInChat), map);
		}

		// Per-client chat loop
		for (int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientInGame(i) || (IsFakeClient(i) && !IsClientSourceTV(i)))
				continue;

			CPrintToChat(i, "{green}[MCE]{default} %t", "Nextmap Voting Finished", mapInChat, percent, num_votes);

			if (g_bShowNominator && foundNomination && owner > 0)
			{
				if (isOwnerConnected)
					CPrintToChat(i, "{green}[MCE]{default} %t %s", "Nominated by", ownerName);
				else
					CPrintToChat(i, "{green}[MCE]{default} %t %t", "Nominated by", "Disconnected Player");
			}

			ClearStoredNomination(i);
		}

		// Build Log String (Server Language)
		char sPlayerLog[128];
		if (g_bShowNominator && foundNomination && owner > 0)
		{
			SetGlobalTransTarget(LANG_SERVER);
			if (isOwnerConnected)
				FormatEx(sPlayerLog, sizeof(sPlayerLog), " \n%t %s", "Nominated by", ownerName);
			else
				FormatEx(sPlayerLog, sizeof(sPlayerLog), " \n%t %t", "Nominated by", "Disconnected Player");
		}

		LogAction(-1, -1, "[MCE] Voting for next map has finished. \nNextmap: %s. (Received \"%d\"%% of %d votes)%s", 
			map, percent, num_votes, sPlayerLog);

		ClearNominationsLists();
	}
}

public void Handler_MapVoteFinished(Menu menu,
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
					mapList.PushString(map);
				}
				else
					break;
			}

			g_PendingRevoteActive = true;
			LogAction(-1, -1, "[MCE] The top maps had the same number of votes. A revote is needed!");
			CPrintToChatAll("{green}[MCE]{default} %t", "Tie Vote", mapList.Length);

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

			mapList.PushString(map1);

			// We allow more than two maps for a revote if they are tied
			for (int i = 1; i < num_items; i++)
			{
				if (mapList.Length < 2 || item_info[i][VOTEINFO_ITEM_VOTES] == item_info[i - 1][VOTEINFO_ITEM_VOTES])
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

public int Handler_MapVoteMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			g_VoteMenu = null;
			delete menu;
		}

		case MenuAction_Display:
		{
			static char buffer[255];
			Format(buffer, sizeof(buffer), "%T", "Vote Nextmap", param1);
			Panel panel = view_as<Panel>(param2);
			panel.SetTitle(buffer);
			//DrawPanelText(panel, "Warning: The Position of the Maps are different for each Player.");
		}

		case MenuAction_DisplayItem:
		{
			char map[PLATFORM_MAX_PATH];
			char buffer[255];

			menu.GetItem(param2, map, PLATFORM_MAX_PATH, _, _, _, param1);

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
				char formattedDisplay[PLATFORM_MAX_PATH];
				InternalFormatMapDisplay(map, formattedDisplay, sizeof(formattedDisplay), param1, false);

				// Handle custom map markings
				if (g_iMarkCustomMaps == 1 && !InternalIsMapOfficial(map))
				{
					Format(buffer, sizeof(buffer), "%T", "Custom Marked", param1, formattedDisplay);
				}
				else if (g_iMarkCustomMaps == 2 && !InternalIsMapOfficial(map))
				{
					Format(buffer, sizeof(buffer), "%T", "Custom", param1, formattedDisplay);
				}
				else
				{
					// Use the formatted display as-is
					strcopy(buffer, sizeof(buffer), formattedDisplay);
				}
			}

			char mapDisplay[32]
			if (InternalGetMapTierName(map, mapDisplay, sizeof(mapDisplay)))
				Format(buffer, sizeof(buffer), "%s [%s]", buffer, mapDisplay);

			if (buffer[0] != '\0')
			{
				return RedrawMenuItem(buffer);
			}
			// End Mapchooser Extended
		}

		case MenuAction_VoteCancel:
		{
			if (param1 == VoteCancel_NoVotes)
			{
				CPrintToChatAll("{green}[MCE]{default} %t", "MCE No Votes Cast");
			}

			// If we receive 0 votes, pick at random.
			if (param1 == VoteCancel_NoVotes && g_bNoVoteMode)
			{
				int count = menu.ItemCount;

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
					menu.GetItem(item, map, PLATFORM_MAX_PATH, _, _, _, param1);
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
