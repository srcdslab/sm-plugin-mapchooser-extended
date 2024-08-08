/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/bans.inc
*  Description:   Handles NE bans system (credits to MCU Team)
* 
* ============================================================================
*/

#define NOMBAN_NOTBANNED -1
#define NOMBAN_PERMANENT 0

Handle g_hNomBanStatus = INVALID_HANDLE; // Format("length:timeIssued")
Handle g_hAdminNomBan = INVALID_HANDLE; // Admin who gave the nomban

int g_iNomBanLength[MAXPLAYERS+1];
int g_iNomBanStart[MAXPLAYERS+1];

char g_sNomBanAdmin[MAXPLAYERS+1][PLATFORM_MAX_PATH];

stock void InitNomBanCookies()
{
	g_hNomBanStatus = RegClientCookie("NE_nomban_status", "Client's nomban info (Ban lenght : Date Issued)", CookieAccess_Protected);
	g_hAdminNomBan = RegClientCookie("NE_nomban_admin", "Admin who nombanned", CookieAccess_Protected);
}

stock void ReadClientNombanCookies(int client)
{
	char buffer[128];
	GetClientCookie(client, g_hNomBanStatus, buffer, sizeof(buffer));
	if (strcmp(buffer, "") == 0)
	{
		g_iNomBanStart[client] = NOMBAN_NOTBANNED;
		g_iNomBanLength[client] = NOMBAN_NOTBANNED;
		return;
	}

	char sBuffer[2][64];
	ExplodeString(buffer, ":", sBuffer, sizeof(sBuffer), sizeof(sBuffer[]), true);
	g_iNomBanLength[client] = StringToInt(sBuffer[0]);
	g_iNomBanStart[client] = StringToInt(sBuffer[1]);

	if (IsClientNomBanned(client))
		RemoveNominationByOwner(client);
}

public void PrepareNomStatusMenu(int client, int target)
{
	Menu menu = CreateMenu(NomStatusMenu_Handler);
	menu.SetTitle("%T", "NomStatus Menu Title", client, target);

	char buffer[PLATFORM_MAX_PATH];
	if (!IsClientNomBanned(target))
	{
		Format(buffer, sizeof(buffer), "%T", "Menu - Not Nombanned", client);
		menu.AddItem("b", buffer, ITEMDRAW_DISABLED);
		menu.Display(client, MENU_TIME_FOREVER);
		return;
	}

	if (g_iNomBanLength[target] == NOMBAN_PERMANENT)
	{
		Format(buffer, sizeof(buffer), "%T", "Menu - Nomban Duration", client, "Permanent");
		menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);
	}
	else
	{
		int seconds = g_iNomBanLength[target];
		int minutes = seconds / 60;
		int hours = minutes / 60;
		int days = hours / 24;

		char time[64];

		if (days > 0)
			Format(time, sizeof(time), "%d days, %d hours, %d minutes", days, hours%24, minutes%60);
		else if (hours > 0)
			Format(time, sizeof(time), "%d hours %d minutes", hours, minutes%60);
		else if (minutes > 0)
			Format(time, sizeof(time), "%d minutes", minutes);
		else
			Format(time, sizeof(time), "%d seconds", seconds);

		Format(buffer, sizeof(buffer), "%t", "Menu - Nomban Duration", time);
		menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);

		menu.AddItem(" ", " ", ITEMDRAW_SPACER);

		int end = g_iNomBanStart[target] + g_iNomBanLength[target];
		FormatTime(time, sizeof(time), "%c", end);
		Format(buffer, sizeof(buffer), "%T", "Menu - Nomban end", client, time);
		menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);

		int timeleftS = end - GetTime();
		int timeleftM = timeleftS / 60;
		int timeleftH = timeleftM / 60;
		int timeleftD = timeleftH / 24;

		if (timeleftD > 0)
			Format(time, sizeof(time), "%d days, %d hours, %d minutes", timeleftD, timeleftH%24, timeleftM%60);
		else if (timeleftH > 0)
			Format(time, sizeof(time), "%d hours, %d minutes, %d seconds", timeleftH, timeleftM%60, timeleftS%60);
		else if (timeleftM > 0)
			Format(time, sizeof(time), "%d minutes, %d seconds", timeleftM, timeleftS%60);
		else
			Format(time, sizeof(time), "%d seconds", timeleftS);

		Format(buffer, sizeof(buffer), "%T", "Menu - Nomban timeleft", client, time);
		menu.AddItem("timeleft", buffer, ITEMDRAW_DISABLED);

		menu.AddItem(" ", " ", ITEMDRAW_SPACER);
	}

	Format(buffer, sizeof(buffer), "%T", "Menu - Nomban Admin", client, g_sNomBanAdmin[client]);
	menu.AddItem(buffer, buffer, ITEMDRAW_DISABLED);

	menu.Display(client, MENU_TIME_FOREVER);
}

public int NomStatusMenu_Handler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_End:
			delete menu;
	}
	return 0;
}

public void PrepareNombanListMenu(int client)
{
	Menu menu = CreateMenu(Nombanlist_Handler);

	char info[64], display[64];
	int total = 0;

	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientNomBanned(i))
			continue;

		int uid = GetClientUserId(i);

		Format(info, sizeof(info), "%d", uid);
		Format(display, sizeof(display), "[#%d] %N", uid, i);
		menu.AddItem(info, display);

		total++;
	}

	if (total == 0)
	{
		Format(display, sizeof(display), "%T", "No clients nombanned", client);
		menu.AddItem("nothing", display, ITEMDRAW_DISABLED);
	}
	
	menu.Display(client, MENU_TIME_FOREVER);
}

public int Nombanlist_Handler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			char buffer[64];
			GetMenuItem(menu, param2, buffer, sizeof(buffer));
			
			int uid = StringToInt(buffer);
			int client = GetClientOfUserId(uid);

			if (client == 0)
			{
				CPrintToChat(param1, "{green}[NE]{default} %t", "NombanList Client Not valid");
				PrepareNombanListMenu(param1);
			}
			else
			{
				PrepareNomStatusMenu(param1, client);
			}
		}
		case MenuAction_End:
			delete menu;
	}

	return 0;
}

public void NomBanClient(int target, int duration, int admin)
{
	char buffer[64];

	if (admin == 0)
		Format(g_sNomBanAdmin[target], sizeof(g_sNomBanAdmin[]), "[Console]");
	else
	{
		GetClientAuthId(admin, AuthId_Steam2, buffer, sizeof(buffer), false);
		Format(g_sNomBanAdmin[target], sizeof(g_sNomBanAdmin[]), "%N (%s)", admin, buffer);
	}

	// Format = length:timeIssued
	int issued = GetTime();

	g_iNomBanLength[target] = duration;
	g_iNomBanStart[target] = issued;

	// Store to cookies
	SaveClientNombanStatus(target);
	
	if (RemoveNominationByOwner(target))
		CPrintToChat(target, "{green}[NE]{default} %t", "Nomination removed on nomban");

	if (duration == NOMBAN_PERMANENT)
	{
		CShowActivity2(admin, "{green}[NE] {olive}","{default}%t", "Nombanned permanent", target);
		LogAction(admin, target, "%L nombanned %L permanently.", admin, target);
		return;
	}

	CShowActivity2(admin, "{green}[NE] {olive}","{default}%t", "Nombanned", target, duration/60);
	LogAction(admin, target, "%L nombanned %L for %d minutes.", admin, target, duration/60);
}

public void UnNomBanClient(int client, int admin)
{
	g_iNomBanLength[client] = NOMBAN_NOTBANNED;
	g_iNomBanStart[client] = NOMBAN_NOTBANNED;
	g_sNomBanAdmin[client] = "";
	
	SaveClientNombanStatus(client);

	CShowActivity2(admin, "{green}[NE] {olive}", "%t", "UnNombanned", client);
	LogAction(admin, client, "%L unnombanned %L", admin, client);

	Menu menu = CreateMenu(NomStatusMenu_Handler);
	menu.SetTitle("%T", "NomStatus Menu Title", client, client);
	menu.AddItem("s", " ", ITEMDRAW_SPACER);

	char sTitle[64];
	Format(sTitle, sizeof(sTitle), "%T", "Menu Got UnNombanned", client);
	menu.AddItem("b", sTitle, ITEMDRAW_DISABLED);

	menu.Display(client, 15);
}

stock void SaveClientNombanStatus(int client)
{
	if (g_iNomBanLength[client] != NOMBAN_NOTBANNED)
	{
		char buffer[128];
		Format(buffer, sizeof(buffer), "%d:%d", g_iNomBanLength[client], g_iNomBanStart[client]);
		SetClientCookie(client, g_hNomBanStatus, buffer);
	}
	else
	{
		SetClientCookie(client, g_hNomBanStatus, "");
	}

	SetClientCookie(client, g_hAdminNomBan, g_sNomBanAdmin[client]);
}

stock bool IsClientNomBanned(int client)
{
	if (!IsClientInGame(client) || !AreClientCookiesCached(client) || g_iNomBanLength[client] == NOMBAN_NOTBANNED)
		return false;

	if (g_iNomBanLength[client] == NOMBAN_PERMANENT || GetTime() < g_iNomBanStart[client] + g_iNomBanLength[client])
		return true;

	return false;
}

Action Timer_NomBansChecker(Handle timer)
{
	for (int i=1; i<MaxClients+1; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
			continue;
		if (g_iNomBanLength[i] == NOMBAN_PERMANENT)
			continue;
		if (g_iNomBanLength[i] == NOMBAN_NOTBANNED)
			continue;

		// Check the time of the nomban and compare it to the current time to see if it has expired
		if (g_iNomBanStart[i] + g_iNomBanLength[i] < GetTime())
			UnNomBanClient(i, 0);
	}

	return Plugin_Continue;
}