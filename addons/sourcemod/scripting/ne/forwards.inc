/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/forwards.inc
*  Description:   Forwards handlers for the NE API
* 
* ============================================================================
*/

Handle g_hOnPublicMapInsert = INVALID_HANDLE;
Handle g_hOnPublicMapReplaced = INVALID_HANDLE;
Handle g_hOnAdminMapInsert = INVALID_HANDLE;
Handle g_hOnMapNominationRemove = INVALID_HANDLE;

stock void API_ForwardsInit()
{
	g_hOnPublicMapInsert = CreateGlobalForward("NE_OnPublicMapInsert", ET_Ignore, Param_Cell, Param_String, Param_Cell, Param_Cell, Param_Cell);
	g_hOnPublicMapReplaced = CreateGlobalForward("NE_OnPublicMapReplaced", ET_Ignore, Param_Cell, Param_String, Param_Cell, Param_Cell, Param_Cell);
	g_hOnAdminMapInsert = CreateGlobalForward("NE_OnAdminMapInsert", ET_Ignore, Param_Cell, Param_String);
	g_hOnMapNominationRemove = CreateGlobalForward("NE_OnMapNominationRemove", ET_Ignore, Param_Cell, Param_String);
}

stock void Forward_OnPublicMapInsert(int client, char[] mapname, bool IsVIP, bool IsLeader, bool IsAdmin)
{
	Call_StartForward(g_hOnPublicMapInsert);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_PushCell(IsVIP);
	Call_PushCell(IsLeader);
	Call_PushCell(IsAdmin);
	Call_Finish();
}

stock void Forward_OnPublicMapReplaced(int client, char[] mapname, bool IsVIP, bool IsLeader, bool IsAdmin)
{
	Call_StartForward(g_hOnPublicMapReplaced);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_PushCell(IsVIP);
	Call_PushCell(IsLeader);
	Call_PushCell(IsAdmin);
	Call_Finish();
}

stock void Forward_OnAdminMapInsert(int client, char[] mapname)
{
	Call_StartForward(g_hOnAdminMapInsert);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_Finish();
}

stock void Forward_OnMapNominationRemove(int client, char[] mapname)
{
	Call_StartForward(g_hOnMapNominationRemove);
	Call_PushCell(client);
	Call_PushString(mapname);
	Call_Finish();
}