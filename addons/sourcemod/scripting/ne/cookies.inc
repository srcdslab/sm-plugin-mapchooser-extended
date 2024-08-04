/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/cookies.inc
*  Description:   Handles NE clients preferences
* 
* ============================================================================
*/

Handle g_hShowUnavailableMaps = INVALID_HANDLE;
bool g_bShowUnavailableMaps[MAXPLAYERS + 1] = { false, ... };

stock void CookiesInit()
{
	SetCookieMenuItem(NE_CookieHandler, 0, "Nominations Extended Settings");
	g_hShowUnavailableMaps = RegClientCookie("NE_hide_unavailable", "Hide unavailable maps from the nominations list", CookieAccess_Protected);

	InitNomBanCookies();

	if (g_bLate)
	{
		for(int i = 1; i <= MaxClients; i++)
		{
			if (!IsClientInGame(i))
				continue;
			if (IsFakeClient(i))
				continue;
			if (!AreClientCookiesCached(i))
				continue;

			OnClientCookiesCached(i);
		}
	}
}

public void OnClientCookiesCached(int client)
{
	ReadClientCookies(client);
}

public void ReadClientCookies(int client) {
	char buffer[128];
	GetClientCookie(client, g_hShowUnavailableMaps, buffer, sizeof(buffer));
	// If no cookies was found (null), set the value to false as initial value
	// Client can choose to enable the settings from the settings or nominate menu
	// Otherwise, apply the value from the cookie
	g_bShowUnavailableMaps[client] = (buffer[0] != '\0') ? view_as<bool>(StringToInt(buffer)) : false;

	ReadClientNombanCookies(client);
}

public void SetClientCookies(int client)
{
	if (!AreClientCookiesCached(client) || IsFakeClient(client))
		return;

	char sValue[8];
	Format(sValue, sizeof(sValue), "%i", g_bShowUnavailableMaps[client]);
	SetClientCookie(client, g_hShowUnavailableMaps, sValue);
	SaveClientNombanStatus(client);
}

public void NE_CookieHandler(int client, CookieMenuAction action, any info, char[] buffer, int maxlen)
{
	switch (action)
	{
		case CookieMenuAction_SelectOption:
		{
			NE_ClientSettings_Menu(client);
		}
	}
}

public void NE_ClientSettings_Menu(int client)
{
	Menu menu = new Menu(NE_ClientSettingsHandler, MENU_ACTIONS_ALL);
	menu.SetTitle("%T %T", "Nominate Title", client, "Client Settings", client);

	bool bCached = AreClientCookiesCached(client);
	char buffer[128];
	if (bCached)
		Format(buffer, sizeof(buffer), "%T: %T", "Nominations Show Unavailable", client, g_bShowUnavailableMaps[client] ? "Yes" : "No", client);
	else
		Format(buffer, sizeof(buffer), "%T", "Can not Load Cookies", client);
	
	menu.AddItem("ShowMap", buffer, bCached ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(client, MENU_TIME_FOREVER);
}

public int NE_ClientSettingsHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_Select:
		{
			char info[64];
			menu.GetItem(param2, info, sizeof(info));
			if (strcmp(info, "ShowMap", false) == 0)
				g_bShowUnavailableMaps[param1] = !g_bShowUnavailableMaps[param1];

			NE_ClientSettings_Menu(param1);
		}
		case MenuAction_Cancel:
		{
			ShowCookieMenu(param1);
		}
		case MenuAction_End:
		{
			delete menu;
		}
	}
	return 0;
}