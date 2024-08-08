/**
 * vim: set ts=4 :
 * =============================================================================
 * Nominations Extended
 * Allows players to nominate maps for Mapchooser
 *
 * Nominations Extended (C)2012-2013 Powerlord (Ross Bemrose)
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */


#include <sourcemod>
#include <mapchooser>
#include <mapchooser_extended>
#include <multicolors>
#include <clientprefs>

#pragma semicolon 1
#pragma newdecls required

#undef REQUIRE_PLUGIN
#tryinclude <zleader>
#define REQUIRE_PLUGIN

#define NE_VERSION "1.12.2"

#include "ne/cvars.inc"
#include "ne/forwards.inc"
#include "ne/functions.inc"
#include "ne/cookies.inc"
#include "ne/bans.inc"
#include "ne/commands.inc"
#include "ne/natives.inc"
#include "ne/menus.inc"

public Plugin myinfo =
{
	name = "Map Nominations Extended",
	author = "SRCDSLab Team, tilgep & koen (Based on Powerlord, AlliedModders LLC, MCU)",
	description = "Provides Map Nominations",
	version = NE_VERSION,
	url = "https://forums.alliedmods.net/showthread.php?t=156974"
};

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("clientprefs.phrases");
	LoadTranslations("nominations.phrases");
	LoadTranslations("basetriggers.phrases"); // for Next Map phrase
	LoadTranslations("mapchooser_extended.phrases");

	InitializeMapLists();
	CvarsInit();
	CommandsInit();
	CookiesInit();

	// Timer Checker
	CreateTimer(60.0, Timer_NomBansChecker, _, TIMER_REPEAT);
}

public APLRes AskPluginLoad2(Handle hThis, bool bLate, char[] err, int iErrLen)
{
	g_bLate = bLate;
	RegPluginLibrary("nominations");

	// Initializes all main natives and forwards.
	API_NativesInit();
	API_ForwardsInit();
	return APLRes_Success;
}

public void OnAllPluginsLoaded()
{
	FindMCECvars();
}

public void OnMapEnd()
{
	MapEndCleanUp();
}

public void OnClientDisconnect(int client)
{
	SetClientCookies(client);
}

public void OnConfigsExecuted()
{
	VerifyMapLists();
	g_bNEAllowed = false;
	InitTimerDelayNominate();
	UpdateMapTrie();
	UpdateMapMenus();
}