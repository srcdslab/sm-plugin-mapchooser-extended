/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/menus.inc
*  Description:   Base menu functions for the plugin
*
* ============================================================================
*/

public int Handler_NominateListMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			delete menu;
		}

		case MenuAction_Select:
		{
			char sMap[PLATFORM_MAX_PATH], sParam[PLATFORM_MAX_PATH];
			menu.GetItem(param2, sParam, sizeof(sParam));

			// Nominations are locked, so we block the unnomination
			if (!g_bNEAllowed)
				return 0;

			if (GetNominationByOwner(param1, sMap) && strcmp(sMap, sParam, false) == 0)
			{
				char sName[MAX_NAME_LENGTH];
				GetClientName(param1, sName, sizeof(sName));

				RemoveNominationByOwner(param1);

				char mapnameInChat[64];
				strcopy(mapnameInChat, sizeof(mapnameInChat), sMap);
				int mapTier;
				if (MCE_CanShowMapTierInChat() && (mapTier = GetMapTier(sMap)) && mapTier > 0)
				{
					char tierName[32];
					if (GetTierNameFromInt(mapTier, tierName, sizeof(tierName)))
						Format(mapnameInChat, sizeof(mapnameInChat), "%s %t", sMap, "MapTierDisplay", tierName);
				}

				CPrintToChatAll("{green}[NE]{default} %t", "Map Unnominated", sName, mapnameInChat);
				LogAction(param1, -1, "\"%L\" has removed his nomination \"%s\".", param1, sMap);
			}
		}
	}

	return 0;
}

Action AttemptNominate(int client, const char[] filter = "")
{
	if (!client)
	{
		ReplyToCommand(client, "[SM] Cannot use this command from server console.");
		return Plugin_Handled;
	}

	if (!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations is currently locked.");
		return Plugin_Handled;
	}

	if (IsClientNomBanned(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Cannot nominate - Nombanned");
		return Plugin_Handled;
	}

	Menu menu = BuildMapMenu(filter[0] ? filter : "", filter[0] ? -1 : client);

	menu.SetTitle("%T", "NE Nominate Title", client);

	menu.Display(client, MENU_TIME_FOREVER);
	return Plugin_Handled;
}

Action AttemptAdminNominate(int client, const char[] filter = "")
{
	if (!client)
		return Plugin_Handled;

	if (!g_bNEAllowed)
	{
		CReplyToCommand(client, "{green}[NE]{default} Map nominations is currently locked.");
		return Plugin_Handled;
	}

	if (IsClientNomBanned(client))
	{
		CPrintToChat(client, "{green}[NE]{default} %t", "Cannot nominate - Nombanned");
		return Plugin_Handled;
	}

	Menu menu = BuildAdminMapMenu(filter[0] ? filter : "");

	menu.SetTitle("%T", "NE Nominate Title", client);
	menu.Display(client, MENU_TIME_FOREVER);
	return Plugin_Handled;
}

void AttemptAdminRemoveMap(int client, const char[] filter = "")
{
	if (!client)
		return;

	Menu AdminRemoveMapMenu = new Menu(Handler_AdminRemoveMapMenu, MENU_ACTIONS_DEFAULT|MenuAction_DisplayItem);

	if (!PopulateNominateListMenu(AdminRemoveMapMenu, client, filter))
	{
		delete AdminRemoveMapMenu;
		CReplyToCommand(client, "{green}[NE]{default} No maps have been nominated.");
		return;
	}

	AdminRemoveMapMenu.SetTitle("Remove nomination", client);
	AdminRemoveMapMenu.Display(client, MENU_TIME_FOREVER);

}

bool PopulateNominateListMenu(Menu menu, int client, const char[] filter = "")
{
	int arraySize = ByteCountToCells(PLATFORM_MAX_PATH);
	ArrayList MapList = new ArrayList(arraySize);
	ArrayList OwnerList = new ArrayList();

	GetNominatedMapList(MapList, OwnerList);
	if (!MapList.Length)
	{
		delete MapList;
		delete OwnerList;
		return false;
	}

	static char map[PLATFORM_MAX_PATH];
	static char display[PLATFORM_MAX_PATH];
	static char description[PLATFORM_MAX_PATH];
	for (int i = 0; i < MapList.Length; i++)
	{
		MapList.GetString(i, map, sizeof(map));

		if (!filter[0] || StrContains(map, filter, false) != -1)
		{
			MCE_GetMapDescription(map, description, sizeof(description));
			MCE_GetMapDisplayName(map, display, sizeof(display));

			if (description[0] != '\0')
				Format(display, sizeof(display), "%s | %s", display, description);

			// Add restriction badges if any
			char badgeBuffer[100];
			if (FormatMapRestrictionBadgesDetailed(map, client, badgeBuffer, sizeof(badgeBuffer), true))
			{
				Format(display, sizeof(display), "%s %s", display, badgeBuffer);
			}

			if (client > 0 && !g_bHideMapsTiers[client])
			{
				char tierName[32];
				if (GetMapTierName(map, tierName, sizeof(tierName)))
					Format(display, sizeof(display), "%s - [%s]", display, tierName);
			}

			int owner = OwnerList.Get(i);

			char sBuffer[64];
			if (owner == client)
				Format(sBuffer, sizeof(sBuffer), "%T", "Unnominate", client);
			else
				Format(sBuffer, sizeof(sBuffer), "%N", owner);

			if (!owner)
				Format(display, sizeof(display), "%s (%T)", display, "Nominated by Admin", client);
			else
				Format(display, sizeof(display), "%s (%s)", display, sBuffer);

			menu.AddItem(map, display);
		}
	}

	delete MapList;
	delete OwnerList;
	return true;
}

// Rows added by BuildMapMenu() on top of the maps
stock bool IsSettingsMenuItem(const char[] info)
{
	return strcmp(info, "hide_tiers") == 0 || strcmp(info, "show_unavailable") == 0 || strcmp(info, "show_nominated") == 0;
}

Menu BuildMapMenu(const char[] filter, int client = -1)
{
	// Always create a new menu - no more global menu reuse
	Menu menu = new Menu(Handler_MapSelectMenu, MENU_ACTIONS_DEFAULT|MenuAction_DrawItem|MenuAction_DisplayItem);

	static char map[PLATFORM_MAX_PATH];
	bool bValid = client > 0 && client <= MaxClients && IsClientInGame(client);
	bool bCached = bValid && AreClientCookiesCached(client);

	// We use \n to create a space between the nomination and the rest of the menu

	if (bCached && !filter[0])
	{
		char buffer[128];

		Format(buffer, sizeof(buffer), "%T: %T", "ShowMapsTiers", client, !g_bHideMapsTiers[client] ? "NE Yes" : "NE No", client);
		menu.AddItem("hide_tiers", buffer);

		Format(buffer, sizeof(buffer), "%T: %T\n ", "Nominations Show Unavailable", client, g_bShowUnavailableMaps[client] ? "NE Yes" : "NE No", client);
		menu.AddItem("show_unavailable", buffer);
	}

	if (bValid)
	{
		char sNominated[PLATFORM_MAX_PATH];
		if (GetNominationByOwner(client, sNominated))
		{
			Format(sNominated, sizeof(sNominated), "%s (%T)\n ", sNominated, "Unnominate", client);
			menu.AddItem("show_nominated", sNominated);
		}
	}

	// When restrictions are not active nothing is unavailable, so every map stays visible.
	bool bHideUnavailable = bCached && !g_bShowUnavailableMaps[client] && AreRestrictionsActive();

	// The pool is sorted and its display names are resolved once, not on every menu build
	UpdateSortedMapList();

	char display[PLATFORM_MAX_PATH];
	for (int i = 0; i < g_SortedMapList.Length; i++)
	{
		g_SortedMapList.GetString(i, map, sizeof(map));

		// If client has cookies cached and doesnt want to show unavailable maps,
		// hide the ones that are currently not nominable.
		if (bHideUnavailable &&
			!(GetMapCooldown(map) == 0 &&
			GetMapCooldownTime(map) < GetTime() &&
			GetMapTimeRestriction(map) == 0 &&
			GetMapPlayerRestriction(map) == 0 &&
			GetMapGroupRestriction(map, client) < 0))
		{
			continue;
		}

		g_SortedMapDisplay.GetString(i, display, sizeof(display));

		if (filter[0] && StrContains(map, filter, false) == -1 && StrContains(display, filter, false) == -1)
			continue;

		menu.AddItem(map, display);
	}

	menu.ExitButton = true;
	return menu;
}

Menu BuildAdminMapMenu(const char[] filter)
{
	Menu menu = new Menu(Handler_AdminMapSelectMenu, MENU_ACTIONS_DEFAULT|MenuAction_DrawItem|MenuAction_DisplayItem);
	ArrayList tempMapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));

	StringMap displayCache = new StringMap();

	static char map[PLATFORM_MAX_PATH];

	// Add the maps from the admin map list
	for (int i = 0; i < g_AdminMapList.Length; i++)
	{
		char display[PLATFORM_MAX_PATH];
		char description[PLATFORM_MAX_PATH];

		g_AdminMapList.GetString(i, map, sizeof(map));

		MCE_GetMapDisplayName(map, display, sizeof(display));
		if (MCE_GetMapDescription(map, description, sizeof(description)) && description[0])
			FormatEx(display, sizeof(display), "%s | %s", display[0] ? display : map, description);

		if (filter[0] && StrContains(map, filter, false) == -1 && StrContains(display, filter, false) == -1)
			continue;

		if (display[0])
			displayCache.SetString(map, display);

		tempMapList.PushString(map);
	}

	if (filter[0])
	{
		// Search normal maps aswell if filter is specified
		for (int i = 0; i < g_MapList.Length; i++)
		{
			char display[PLATFORM_MAX_PATH];
			char description[PLATFORM_MAX_PATH];

			g_MapList.GetString(i, map, sizeof(map));

			MCE_GetMapDisplayName(map, display, sizeof(display));
			if (MCE_GetMapDescription(map, description, sizeof(description)) && description[0])
				FormatEx(display, sizeof(display), "%s | %s", display[0] ? display : map, description);

			if (StrContains(map, filter, false) == -1 && StrContains(display, filter, false) == -1)
				continue;

			if (display[0])
				displayCache.SetString(map, display);

			tempMapList.PushString(map);
		}
	}

	// Sort the maps in alphabetical order
	tempMapList.SortCustom(AlphabeticSortCallback);

	// Add the maps to the menu
	for (int i = 0; i < tempMapList.Length; i++)
	{
		tempMapList.GetString(i, map, sizeof(map));
		char display[PLATFORM_MAX_PATH];
		if (!displayCache.GetString(map, display, sizeof(display)))
			strcopy(display, sizeof(display), map);

		menu.AddItem(map, display);
	}

	delete tempMapList;
	delete displayCache;
	return menu;
}

public int Handler_MapSelectMenu(Menu menu, MenuAction action, int param1, int param2)
{
	int iCurrentTime = GetTime();
	bool restrictionsActive = AreRestrictionsActive();

	switch (action)
	{
		case MenuAction_End:
		{
			delete menu;
		}

		case MenuAction_Select:
		{
			static char map[PLATFORM_MAX_PATH];
			char name[MAX_NAME_LENGTH];
			menu.GetItem(param2, map, sizeof(map));

			if (strcmp(map, "show_unavailable", false) == 0)
			{
				g_bShowUnavailableMaps[param1] = !g_bShowUnavailableMaps[param1];
				AttemptNominate(param1, "");
				return 0;
			}

			if (strcmp(map, "hide_tiers", false) == 0)
			{
				g_bHideMapsTiers[param1] = !g_bHideMapsTiers[param1];
				AttemptNominate(param1, "");
				return 0;
			}

			if (!g_bNEAllowed)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "Nominations Locked");
				return 0;
			}

			if (g_Player_NominationDelay[param1] > iCurrentTime)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "Wait to Nominate", g_Player_NominationDelay[param1] - iCurrentTime);
				menu.DisplayAt(param1, GetMenuSelectionPosition(), MENU_TIME_FOREVER);
				return 0;
			}

			GetClientName(param1, name, MAX_NAME_LENGTH);

			if (strcmp(map, "show_nominated", false) == 0)
			{
				char sNominated[PLATFORM_MAX_PATH];
				if (GetNominationByOwner(param1, sNominated))
				{
					char sNominatedDisplay[PLATFORM_MAX_PATH];
					strcopy(sNominatedDisplay, sizeof(sNominatedDisplay), sNominated);
					char tierName[32];
					if (MCE_CanShowMapTierInChat() && GetMapTierName(sNominated, tierName, sizeof(tierName)))
						Format(sNominatedDisplay, sizeof(sNominatedDisplay), "%s %t", sNominated, "MapTierDisplay", tierName);

					RemoveNominationByOwner(param1);
					CPrintToChatAll("{green}[NE]{default} %t", "Map Unnominated", name, sNominatedDisplay);
					LogAction(param1, -1, "\"%L\" has removed his nomination \"%s\".", param1, sNominated);
					Forward_OnMapNominationRemove(param1, sNominated);
				}

				// The nomination may already be gone (vote started, removed by an admin)
				return 0;
			}

			if (IsMapRestricted(param1, map))
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "Selected Map Nomination Restricted");
				return 0;
			}

			NominateResult result = NominateMap(map, false, param1);

			/* Don't need to check for InvalidMap because the menu did that already */
			if (result == Nominate_AlreadyInVote)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "NE Map Already Nominated");
				return 0;
			}
			else if (result == Nominate_VoteFull)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "NE Max Nominations");
				return 0;
			}

			/* Map was nominated! - Disable the menu item and update the trie */
			g_mapTrie.SetValue(map, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);
			g_Player_NominationDelay[param1] = iCurrentTime + g_Cvar_NominateDelay.IntValue;

			char mapnameInChat[64];
			strcopy(mapnameInChat, sizeof(mapnameInChat), map);
			int mapTier;
			if (MCE_CanShowMapTierInChat() && (mapTier = GetMapTier(map)) && mapTier > 0)
			{
				char tierName[32];
				if (GetTierNameFromInt(mapTier, tierName, sizeof(tierName)))
					Format(mapnameInChat, sizeof(mapnameInChat), "%s %t", map, "MapTierDisplay", tierName);
			}

			if (result == Nominate_Added)
			{
				CPrintToChatAll("{green}[NE]{default} %t", "NE Map Nominated", name, mapnameInChat);
				LogMessage("%L has nominated %s", param1, map);
				Forward_OnPublicMapInsert(param1, map, IsMapVIPRestricted(map), IsMapLeaderRestricted(map), IsMapAdminRestricted(map));
			}
			else if (result == Nominate_Replaced)
			{
				CPrintToChatAll("{green}[NE]{default} %t", "NE Map Nomination Changed", name, mapnameInChat);
				LogMessage("%L has changed their nomination to %s", param1, map);
				Forward_OnPublicMapReplaced(param1, map, IsMapVIPRestricted(map), IsMapLeaderRestricted(map), IsMapAdminRestricted(map));
			}
		}

		case MenuAction_DrawItem:
		{
			static char map[PLATFORM_MAX_PATH];
			menu.GetItem(param2, map, sizeof(map));

			if (IsSettingsMenuItem(map))
				return ITEMDRAW_DEFAULT;

			int status;
			if (g_mapTrie.GetValue(map, status))
			{
				if ((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
				{
					if ((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
					{
						return ITEMDRAW_DISABLED;
					}

					if (restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
					{
						return ITEMDRAW_DISABLED;
					}

					if ((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
					{
						return ITEMDRAW_DISABLED;
					}
				}
			}

			if (IsMapRestricted(param1, map))
			{
				return ITEMDRAW_DISABLED;
			}

			return ITEMDRAW_DEFAULT;
		}

		case MenuAction_DisplayItem:
		{
			static char map[PLATFORM_MAX_PATH];
			menu.GetItem(param2, map, sizeof(map));

			// Keep the text of the settings rows, they are not maps
			if (IsSettingsMenuItem(map))
				return 0;

			int mark = g_Cvar_MarkCustomMaps.IntValue;
			bool official;

			static char buffer[100];
			static char display[150];
			static char displayName[PLATFORM_MAX_PATH];
			static char description[PLATFORM_MAX_PATH];

			MCE_GetMapDescription(map, description, sizeof(description));
			MCE_GetMapDisplayName(map, displayName, sizeof(displayName));

			if (description[0] != '\0')
				Format(displayName, sizeof(displayName), "%s | %s", displayName, description);

			if (mark)
				official = IsMapOfficial(map);

			if (mark && !official)
			{
				switch (mark)
				{
					case 1:
					{
						Format(buffer, sizeof(buffer), "%T", "Custom Marked", param1, displayName);
					}

					case 2:
					{
						Format(buffer, sizeof(buffer), "%T", "Custom", param1, displayName);
					}
				}
			}
			else
				strcopy(buffer, sizeof(buffer), displayName);

			bool displayMapTier = false;
			if (!g_bHideMapsTiers[param1])
			{
				char tierName[32];
				if (GetMapTierName(map, tierName, sizeof(tierName)))
				{
					Format(buffer, sizeof(buffer), "%s - [%s]", buffer, tierName);
					displayMapTier = true;
				}
			}

			int status;
			if (g_mapTrie.GetValue(map, status))
			{
				if ((status & MAPSTATUS_DISABLED) == MAPSTATUS_DISABLED)
				{
					if ((status & MAPSTATUS_EXCLUDE_CURRENT) == MAPSTATUS_EXCLUDE_CURRENT)
					{
						Format(display, sizeof(display), "%s (%T)", buffer, "NE Current Map", param1);
						return RedrawMenuItem(display);
					}

					if (restrictionsActive && (status & MAPSTATUS_EXCLUDE_PREVIOUS) == MAPSTATUS_EXCLUDE_PREVIOUS)
					{
						int Cooldown = GetMapCooldown(map);
						Format(display, sizeof(display), "%s (%T %d)", buffer, "NE Recently Played", param1, Cooldown);
						return RedrawMenuItem(display);
					}

					if ((status & MAPSTATUS_EXCLUDE_NOMINATED) == MAPSTATUS_EXCLUDE_NOMINATED)
					{
						Format(display, sizeof(display), "%s (%T)", buffer, "NE Nominated", param1);
						return RedrawMenuItem(display);
					}
				}
			}

			int Cooldown = GetMapCooldownTime(map);
			if (restrictionsActive && Cooldown > iCurrentTime)
			{
				int Seconds = Cooldown - iCurrentTime;
				char time[16];
				CustomFormatTime(Seconds, time, sizeof(time));
				Format(display, sizeof(display), "%s (%T %s)", buffer, "NE Recently Played", param1, time);
				return RedrawMenuItem(display);
			}

			// Format restriction badges + Check if we have any badges to display
			bool hasBadges;
			char badgeBuffer[100];
			if (FormatMapRestrictionBadgesDetailed(map, param1, badgeBuffer, sizeof(badgeBuffer), false))
			{
				Format(buffer, sizeof(buffer), "%s %s", buffer, badgeBuffer);
				hasBadges = true;
			}

			// If we have badges or custom mark, return the formatted buffer
			if (hasBadges || (mark && !official))
				return RedrawMenuItem(buffer);

			if (displayMapTier)
				return RedrawMenuItem(buffer);
				
			return 0;
		}
	}

	return 0;
}

public int Handler_AdminMapSelectMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			delete menu;
		}

		case MenuAction_Select:
		{
			static char map[PLATFORM_MAX_PATH];
			menu.GetItem(param2, map, sizeof(map));

			if (!CheckCommandAccess(param1, "sm_nominate_ignore", ADMFLAG_CHEATS, true))
			{
				if (IsMapRestricted(param1, map))
				{
					CPrintToChat(param1, "{green}[NE]{default} %t", "Selected Map Nomination Restricted");
					return 0;
				}
			}

			NominateResult result = NominateMap(map, true, 0);

			if (result == Nominate_InvalidMap)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "Map was not found", map);
				return 0;
			}

			if (result > Nominate_Replaced)
			{
				/* We assume already in vote is the casue because the maplist does a Map Validity check and we forced, so it can't be full */
				CPrintToChat(param1, "{green}[NE]{default} %t", "NE Map Already In Vote", map);
				return 0;
			}

			/* Map was nominated! - Disable the menu item and update the trie */
			g_mapTrie.SetValue(map, MAPSTATUS_DISABLED|MAPSTATUS_EXCLUDE_NOMINATED);

			CPrintToChat(param1, "{green}[NE]{default} %t", "NE Map Inserted", map);
			CPrintToChatAll("{green}[NE]{default} %t", "Client inserted Map", param1, map);

			LogAction(param1, -1, "[NE] \"%L\" has inserted map \"%s\".", param1, map);
			Forward_OnAdminMapInsert(param1, map);
		}

		case MenuAction_DrawItem:
		{
			if (!CheckCommandAccess(param1, "sm_nominate_ignore", ADMFLAG_CHEATS, true))
			{
				return Handler_MapSelectMenu(menu, action, param1, param2);
			}

			return ITEMDRAW_DEFAULT;
		}

		case MenuAction_DisplayItem:
		{
			return Handler_MapSelectMenu(menu, action, param1, param2);
		}
	}

	return 0;
}

public int Handler_AdminRemoveMapMenu(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_End:
		{
			delete menu;
		}

		case MenuAction_Select:
		{
			static char map[PLATFORM_MAX_PATH];
			menu.GetItem(param2, map, sizeof(map));

			if (!RemoveNominationByMap(map))
			{
				CReplyToCommand(param1, "{green}[NE]{default} %t", "NE Map Not In Pool", map);
				return 0;
			}

			CReplyToCommand(param1, "{green}[NE]{default} %t", "Map Removed", map);
			CPrintToChatAll("{green}[NE]{default} %t", "Client removed Map", param1, map);

			LogAction(param1, -1, "\"%L\" has removed map \"%s\" from nominations.", param1, map);
			Forward_OnMapNominationRemove(param1, map);
		}
	}

	return 0;
}

bool FormatMapRestrictionBadgesDetailed(const char[] map, int client, char[] buffer, int maxlength, bool nominatedlist = false)
{
	bool restrictionsActive = AreRestrictionsActive();

	// If map is recently played, don't show badges (it's not nominable anyway)
	int cooldownTime = GetMapCooldownTime(map);
	if (restrictionsActive && cooldownTime > GetTime())
		return false;

	bool hasBadges = false;
	char badges[200];

	if (restrictionsActive)
	{
		bool adminRestriction = IsClientMapAdminRestricted(map);
		if (adminRestriction)
		{
			Format(badges, sizeof(badges), "%s%T|", badges, "Admin Restriction", client);
			hasBadges = true;
		}

		bool VIPRestriction = IsClientMapVIPRestricted(map);
		if (VIPRestriction)
		{
			Format(badges, sizeof(badges), "%s%T|", badges, "VIP Restriction", client);
			hasBadges = true;
		}

		#if defined _zleader_included
		bool LeaderRestriction = IsClientMapLeaderRestricted(map);
		if (LeaderRestriction)
		{
			Format(badges, sizeof(badges), "%s%T|", badges, "Leader Restriction", client);
			hasBadges = true;
		}
		#endif

		if (!nominatedlist)
		{
			// Time restriction with value
			int timeRestriction = GetMapTimeRestriction(map);
			if (timeRestriction > 0)
			{
				Format(badges, sizeof(badges), "%s%T|", badges, "Map Time Restriction", client, "+", timeRestriction / 60, timeRestriction % 60);
				hasBadges = true;
			}

			// Player restriction with value
			int playerRestriction = GetMapPlayerRestriction(map);
			if (playerRestriction != 0)
			{
				if (playerRestriction < 0)
						Format(badges, sizeof(badges), "%s%T|", badges, "Map Player Restriction", client, "+", playerRestriction * -1);
					else
						Format(badges, sizeof(badges), "%s%T|", badges, "Map Player Restriction", client, "-", playerRestriction);
				hasBadges = true;
			}

			// Group restriction with value
			int groupRestriction = GetMapGroupRestriction(map, client);
			if (groupRestriction >= 0)
			{
				Format(badges, sizeof(badges), "%s%T|", badges, "Map Group Restriction", client, groupRestriction);
				hasBadges = true;
			}
		}
	}

	if (hasBadges)
	{
		// Remove trailing |
		int len = strlen(badges);
		if (len > 0 && badges[len-1] == '|')
		{
			badges[len-1] = '\0';
		}

		// Format with brackets
		Format(buffer, maxlength, "[%s]", badges);
		return true;
	}

	return false;
}
