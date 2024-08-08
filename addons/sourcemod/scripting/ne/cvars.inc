/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/cvars.inc
*  Description:   Config creation and cvar control
* 
* ============================================================================
*/

// MCE Convars 
ConVar g_Cvar_MarkCustomMaps;

// Nominations Extended Convars
ConVar g_Cvar_ExcludeOld;
ConVar g_Cvar_ExcludeCurrent;
ConVar g_Cvar_InitialDelay;
ConVar g_Cvar_NominateDelay;
ConVar g_Cvar_MaxBanTime;

// VIP Nomination Convars
ConVar g_Cvar_VIPTimeframe;
ConVar g_Cvar_VIPTimeframeMinTime;
ConVar g_Cvar_VIPTimeframeMaxTime;

stock void CvarsInit()
{
	g_Cvar_ExcludeOld = CreateConVar("sm_nominate_excludeold", "1", "Specifies if the current map should be excluded from the Nominations list", 0, true, 0.00, true, 1.0);
	g_Cvar_ExcludeCurrent = CreateConVar("sm_nominate_excludecurrent", "1", "Specifies if the MapChooser excluded maps should also be excluded from Nominations", 0, true, 0.00, true, 1.0);
	g_Cvar_InitialDelay = CreateConVar("sm_nominate_initialdelay", "60.0", "Time in seconds before first Nomination can be made", 0, true, 0.00);
	g_Cvar_NominateDelay = CreateConVar("sm_nominate_delay", "3.0", "Delay between nominations", 0, true, 0.00, true, 60.00);

	g_Cvar_VIPTimeframe = CreateConVar("sm_nominate_vip_timeframe", "1", "Specifies if the should be a timeframe where only VIPs can nominate maps", 0, true, 0.00, true, 1.0);
	g_Cvar_VIPTimeframeMinTime = CreateConVar("sm_nominate_vip_timeframe_mintime", "1800", "Start of the timeframe where only VIPs can nominate maps (Format: HHMM)", 0, true, 0000.00, true, 2359.0);
	g_Cvar_VIPTimeframeMaxTime = CreateConVar("sm_nominate_vip_timeframe_maxtime", "2200", "End of the timeframe where only VIPs can nominate maps (Format: HHMM)", 0, true, 0000.00, true, 2359.0);

	g_Cvar_MaxBanTime = CreateConVar("sm_nominate_max_ban_time", "10080", "Maximum time a client can be nombanned in minutes (for non rcon+)", 0, true);

	// Nominations Extended cvars
	CreateConVar("ne_version", NE_VERSION, "Nominations Extended Version", FCVAR_SPONLY|FCVAR_NOTIFY|FCVAR_DONTRECORD);

	AutoExecConfig(true, "nominations_extended");
}

stock void FindMCECvars()
{
	// This is an MCE cvar... this plugin requires MCE to be loaded.  Granted, this plugin SHOULD have an MCE dependency.
	g_Cvar_MarkCustomMaps = FindConVar("mce_markcustommaps");
}