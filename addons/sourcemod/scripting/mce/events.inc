/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/events.inc
*  Description:   Event hooking and forwarding
* 
* ============================================================================
*/

stock void EventsInit(EngineVersion version)
{
	if(g_Cvar_Winlimit != INVALID_HANDLE || g_Cvar_Maxrounds != INVALID_HANDLE)
	{
		switch(version)
		{
			case Engine_TF2:
			{
				HookEvent("teamplay_win_panel", Event_TeamPlayWinPanel);
				HookEvent("teamplay_restart_round", Event_TFRestartRound);
				HookEvent("arena_win_panel", Event_TeamPlayWinPanel);
				HookEvent("pve_win_panel", Event_MvMWinPanel);
			}

			case Engine_NuclearDawn:
			{
				HookEvent("round_win", Event_RoundEnd);
			}

			case Engine_CSGO:
			{
				HookEvent("round_end", Event_RoundEnd);
				HookEvent("cs_win_panel_match", Event_WinPanel);
				HookEvent("cs_intermission", Event_Intermission);
				HookEvent("announce_phase_end", Event_PhaseEnd);
				g_Cvar_MatchClinch = FindConVar("mp_match_can_clinch");
			}

			case Engine_DODS:
			{
				HookEvent("dod_round_win", Event_RoundEnd);
			}

			default:
			{
				HookEvent("round_end", Event_RoundEnd);
				HookEvent("cs_win_panel_match", Event_WinPanel);
			}
		}
	}

	if(g_Cvar_Fraglimit != INVALID_HANDLE)
		HookEvent("player_death", Event_PlayerDeath);
}

public void Event_TFRestartRound(Handle event, const char[] name, bool dontBroadcast)
{
	/* Game got restarted - reset our round count tracking */
	g_TotalRounds = 0;
}

public void Event_TeamPlayWinPanel(Handle event, const char[] name, bool dontBroadcast)
{
	if(g_ChangeMapAtRoundEnd)
	{
		g_ChangeMapAtRoundEnd = false;
		CreateTimer(2.0, Timer_ChangeMap, INVALID_HANDLE, TIMER_FLAG_NO_MAPCHANGE);
		g_ChangeMapInProgress = true;
	}

	int bluescore = GetEventInt(event, "blue_score");
	int redscore = GetEventInt(event, "red_score");

	if(GetEventInt(event, "round_complete") == 1 || strcmp(name, "arena_win_panel") == 0)
	{
		g_TotalRounds++;

		if(!GetArraySize(g_MapList) || g_HasVoteStarted || g_MapVoteCompleted || !g_bEndOfMapVote)
			return;

		CheckMaxRounds(g_TotalRounds);

		switch(GetEventInt(event, "winning_team"))
		{
			case 3:
			{
				CheckWinLimit(bluescore);
			}
			case 2:
			{
				CheckWinLimit(redscore);
			}
			//We need to do nothing on winning_team == 0 this indicates stalemate.
			default:
			{
				return;
			}
		}
	}
}

public void Event_MvMWinPanel(Handle event, const char[] name, bool dontBroadcast)
{
	if(GetEventInt(event, "winning_team") == 2)
	{
		int objectiveEnt = EntRefToEntIndex(g_ObjectiveEnt);
		if(objectiveEnt != INVALID_ENT_REFERENCE)
		{
			g_TotalRounds = GetEntProp(g_ObjectiveEnt, Prop_Send, "m_nMannVsMachineWaveCount");
			CheckMaxRounds(g_TotalRounds);
		}
	}
}

public void Event_Intermission(Handle event, const char[] name, bool dontBroadcast)
{
	g_HasIntermissionStarted = true;
}

public void Event_PhaseEnd(Handle event, const char[] name, bool dontBroadcast)
{
	/* announce_phase_end fires for both half time and the end of the map, but intermission fires first for end of the map. */
	if(g_HasIntermissionStarted)
		return;

	/* No intermission yet, so this must be half time. Swap the score counters. */
	int t_score = g_winCount[2];
	g_winCount[2] =  g_winCount[3];
	g_winCount[3] = t_score;
}

public void Event_WeaponRank(Handle event, const char[] name, bool dontBroadcast)
{
	int rank = GetEventInt(event, "weaponrank");
	if(rank > g_TotalRounds)
	{
		g_TotalRounds = rank;
		CheckMaxRounds(g_TotalRounds);
	}
}

public void Event_WinPanel(Handle event, const char[] name, bool dontBroadcast)
{
	if (!g_bEndOfMapInfo)
		return;

	char nextMap[64];
	if(!GetNextMap(nextMap, sizeof(nextMap)))
		return;

	bool bDynamicAvailable = false;
	int iChannel = 1;
	int iHUDChannel = -1;

	iChannel = g_iHUDChannel;
	if (iChannel < 0 || iChannel > 6)
		iChannel = 1;

	bDynamicAvailable = g_DynamicChannels && CanTestFeatures() && GetFeatureStatus(FeatureType_Native, "GetDynamicChannel") == FeatureStatus_Available;

#if defined _DynamicChannels_included_
	if (bDynamicAvailable)
		iHUDChannel = GetDynamicChannel(iChannel);
#endif

	Handle g_hHud = CreateHudSynchronizer();
	SetHudTextParams(-1.0, 0.89, 7.0, 255, 71, 1, 1, 1, 4.0, 0.6, 0.6);

	if (!bDynamicAvailable && g_hHud != INVALID_HANDLE || bDynamicAvailable)
	{
		for(int i = 1; i <= MaxClients; i++)
		{
			if(!IsClientInGame(i) || IsFakeClient(i))
				continue;

			if (bDynamicAvailable)
				ShowHudText(i, iHUDChannel, "Next Map %s", nextMap);
			else
			{
				ClearSyncHud(i, g_hHud);
				ShowSyncHudText(i, g_hHud, "Next Map: %s", nextMap);
			}
		}
	}

	CPrintToChatAll("%t", "Nextmap", nextMap);
	CPrintToChatAll("%t", "Nextmap", nextMap);
	CPrintToChatAll("%t", "Nextmap", nextMap);	

	delete g_hHud;
}

/* You ask, why don't you just use team_score event? And I answer... Because CSS doesn't. */
public void Event_RoundEnd(Handle event, const char[] name, bool dontBroadcast)
{
	if(g_RoundCounting == RoundCounting_ArmsRace)
		return;

	if(g_ChangeMapAtRoundEnd)
	{
		g_ChangeMapAtRoundEnd = false;
		CreateTimer(2.0, Timer_ChangeMap, INVALID_HANDLE, TIMER_FLAG_NO_MAPCHANGE);
		g_ChangeMapInProgress = true;
	}

	int winner;
	if(strcmp(name, "round_win") == 0 || strcmp(name, "dod_round_win") == 0)
		winner = GetEventInt(event, "team"); // Nuclear Dawn & DoD:S
	else
		winner = GetEventInt(event, "winner");

	if(winner == 0 || winner == 1 || !g_bEndOfMapVote)
		return;

	if(winner >= MAXTEAMS)
		SetFailState("Mod exceed maximum team count - Please file a bug report.");

	g_TotalRounds++;

	g_winCount[winner]++;

	if(!GetArraySize(g_MapList) || g_HasVoteStarted || g_MapVoteCompleted)
	{
		return;
	}

	CheckWinLimit(g_winCount[winner]);
	CheckMaxRounds(g_TotalRounds);
}

public void Event_PlayerDeath(Handle event, const char[] name, bool dontBroadcast)
{
	if(!GetArraySize(g_MapList) || g_Cvar_Fraglimit == INVALID_HANDLE || g_HasVoteStarted)
		return;

	if(!GetConVarInt(g_Cvar_Fraglimit) || !g_bEndOfMapVote)
		return;

	if(g_MapVoteCompleted)
		return;

	int fragger = GetClientOfUserId(GetEventInt(event, "attacker"));

	if(!fragger)
		return;

	if(GetClientFrags(fragger) >= (GetConVarInt(g_Cvar_Fraglimit) - g_iStartFrags))
	{
		if(!g_WarningInProgress || g_WarningTimer == INVALID_HANDLE)
		{
			SetupWarningTimer(WarningType_Vote, MapChange_MapEnd);
			//InitiateVote(MapChange_MapEnd, INVALID_HANDLE);
		}
	}
}