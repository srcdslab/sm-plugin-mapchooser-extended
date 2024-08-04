/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/forwards.inc
*  Description:   Forwards handlers for the MCE API
* 
* ============================================================================
*/

Handle g_NominationsResetForward = INVALID_HANDLE;
Handle g_NominationDisconnect = INVALID_HANDLE;
Handle g_MapVoteStartedForward = INVALID_HANDLE;
Handle g_SetNextMapForward = INVALID_HANDLE;

/* Mapchooser Extended Forwards */
Handle g_MapVoteWarningStartForward = INVALID_HANDLE;
Handle g_MapVoteWarningTickForward = INVALID_HANDLE;
Handle g_MapVoteStartForward = INVALID_HANDLE;
Handle g_MapVoteEndForward = INVALID_HANDLE;
Handle g_MapVoteRunoffStartForward = INVALID_HANDLE;

stock void API_ForwardsInit()
{
	g_NominationsResetForward = CreateGlobalForward("OnNominationRemoved", ET_Ignore, Param_String, Param_Cell);
	g_NominationDisconnect= CreateGlobalForward("OnNominationDisconnect", ET_Ignore, Param_String, Param_Cell);
	g_MapVoteStartedForward = CreateGlobalForward("OnMapVoteStarted", ET_Ignore);
	g_SetNextMapForward = CreateGlobalForward("OnSetNextMap", ET_Ignore, Param_String);

	//MapChooser Extended Forwards
	g_MapVoteStartForward = CreateGlobalForward("OnMapVoteStart", ET_Ignore); // Deprecated
	g_MapVoteEndForward = CreateGlobalForward("OnMapVoteEnd", ET_Ignore, Param_String);
	g_MapVoteWarningStartForward = CreateGlobalForward("OnMapVoteWarningStart", ET_Ignore);
	g_MapVoteWarningTickForward = CreateGlobalForward("OnMapVoteWarningTick", ET_Ignore, Param_Cell);
	g_MapVoteRunoffStartForward = CreateGlobalForward("OnMapVoteRunnoffWarningStart", ET_Ignore);
}

stock void Forward_OnNominationRemoved(char[] oldmap, int owner)
{
	Call_StartForward(g_NominationsResetForward);
	Call_PushString(oldmap);
	Call_PushCell(owner);
	Call_Finish();
}

stock void Forward_OnNominationDisconnect(char[] oldmap, int owner)
{
	Call_StartForward(g_NominationDisconnect);
	Call_PushString(oldmap);
	Call_PushCell(owner);
	Call_Finish();
}

stock void Forward_OnSetNextMap(char[] map)
{
	Call_StartForward(g_SetNextMapForward);
	Call_PushString(map);
	Call_Finish();
}