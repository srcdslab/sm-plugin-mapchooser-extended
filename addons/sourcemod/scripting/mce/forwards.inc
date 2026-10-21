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

GlobalForward g_NominationsResetForward = null;
GlobalForward g_NominationDisconnect = null;
GlobalForward g_MapVoteStartedForward = null;
GlobalForward g_SetNextMapForward = null;

/* Mapchooser Extended Forwards */
GlobalForward g_MapVoteWarningStartForward = null;
GlobalForward g_MapVoteWarningTickForward = null;
GlobalForward g_MapVoteStartForward = null;
GlobalForward g_MapVoteEndForward = null;
GlobalForward g_MapVoteRunoffStartForward = null;

stock void API_ForwardsInit()
{
	g_NominationsResetForward = new GlobalForward("OnNominationRemoved", ET_Ignore, Param_String, Param_Cell);
	g_NominationDisconnect= new GlobalForward("OnNominationDisconnect", ET_Ignore, Param_String, Param_Cell);
	g_MapVoteStartedForward = new GlobalForward("OnMapVoteStarted", ET_Ignore);
	g_SetNextMapForward = new GlobalForward("OnSetNextMap", ET_Ignore, Param_String);

	//MapChooser Extended Forwards
	g_MapVoteStartForward = new GlobalForward("OnMapVoteStart", ET_Ignore); // Deprecated
	g_MapVoteEndForward = new GlobalForward("OnMapVoteEnd", ET_Ignore, Param_String);
	g_MapVoteWarningStartForward = new GlobalForward("OnMapVoteWarningStart", ET_Ignore);
	g_MapVoteWarningTickForward = new GlobalForward("OnMapVoteWarningTick", ET_Ignore, Param_Cell);
	g_MapVoteRunoffStartForward = new GlobalForward("OnMapVoteRunnoffWarningStart", ET_Ignore);
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