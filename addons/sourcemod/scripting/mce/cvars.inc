/*
* ============================================================================
*
*  MapChooser Extended
*
*  File:          mce/cvars.inc
*  Description:   Config creation and cvar control
* 
* ============================================================================
*/

/* Valve ConVars */
ConVar g_Cvar_Winlimit;
ConVar g_Cvar_Maxrounds;
ConVar g_Cvar_Fraglimit;
ConVar g_Cvar_TimeLimit;
ConVar g_Cvar_Bonusroundtime;
ConVar g_Cvar_MatchClinch;
ConVar g_Cvar_VoteNextLevel;
ConVar g_Cvar_GameType;
ConVar g_Cvar_GameMode;

/* Plugin ConVars */
ConVar g_Cvar_StartTime;
ConVar g_Cvar_StartRounds;
ConVar g_Cvar_StartFrags;
ConVar g_Cvar_ExtendTimeStep;
ConVar g_Cvar_ExtendRoundStep;
ConVar g_Cvar_ExtendFragStep;
ConVar g_Cvar_ExcludeMaps;
ConVar g_Cvar_ExcludeMapsTime;
ConVar g_Cvar_IncludeMaps;
ConVar g_Cvar_IncludeMapsReserved;
ConVar g_Cvar_NoVoteMode;
ConVar g_Cvar_Extends;
ConVar g_Cvar_DontChange;
ConVar g_Cvar_EndOfMapVote;
ConVar g_Cvar_EndOfMapInfo;
ConVar g_cvHUDChannel;
ConVar g_Cvar_VoteDuration;
ConVar g_Cvar_RandomStartTime;
ConVar g_Cvar_CountBots;
ConVar g_Cvar_ForceLowerCase;

/* Mapchooser Extended Plugin ConVars */

ConVar g_Cvar_RunOff;
ConVar g_Cvar_RunOffPercent;
ConVar g_Cvar_BlockSlots;
ConVar g_Cvar_MaxRunOffs;
ConVar g_Cvar_StartTimePercent;
ConVar g_Cvar_EnableStartTimePercent;
ConVar g_Cvar_WarningTime;
ConVar g_Cvar_RunOffWarningTime;
ConVar g_Cvar_MenuStyle;
ConVar g_Cvar_TimerLocation;
ConVar g_Cvar_ExtendPosition;
ConVar g_Cvar_MarkCustomMaps;
ConVar g_Cvar_RandomizeNominations;
ConVar g_Cvar_HideTimer;
ConVar g_Cvar_NoVoteOption;
ConVar g_Cvar_ShufflePerClient;
ConVar g_Cvar_NoRestrictionTimeframeEnable;
ConVar g_Cvar_NoRestrictionTimeframeMinTime;
ConVar g_Cvar_NoRestrictionTimeframeMaxTime;
ConVar g_Cvar_TimerUnlockNoms;
ConVar g_Cvar_LockNominationsAtWarning;
ConVar g_Cvar_ShowNominator;
ConVar g_Cvar_SharedCDMode;
ConVar g_Cvar_CooldownMode;
ConVar g_Cvar_DefaultTier;
ConVar g_Cvar_ShowMapTierInChat;

/* Cache convars values */
bool g_bEndOfMapVote;
bool g_bEndOfMapInfo;
bool g_bNoVoteMode;
bool g_bDontChange;
bool g_bHideTimer;
bool g_bNoVoteOption;
bool g_bShufflePerClient;
bool g_bNoRestrictionTimeframeEnable;
bool g_bLockNominationsAtWarning;
bool g_bShowNominator;
bool g_bCountBots;
bool g_bRunOff;
bool g_bBlockSlots;
bool g_bEnableStartPercent;
bool g_bRandomizeNominations;
bool g_bExtendPositionTop;
bool g_bForceLowerCase;
bool g_bShowMapTierInChat;

int g_iStartRounds;
int g_iStartFrags;
int g_iHUDChannel;
int g_iExtendTimeStep;
int g_iExtendRoundStep;
int g_iExtendFragStep;
int g_iExcludeMaps;
int g_iIncludeMaps;
int g_iIncludeMapsReserved;
int g_iMaxExtends;
int g_iVoteDuration;
int g_iPercentRunOff;
int g_iMaxRunOffs;
int g_iRunOffWarningTime;
int g_iStartTimePercent;
int g_iWarningTime;
int g_iStartTime;
int g_iMenuStyle;
int g_iTimerLocation;
int g_iMarkCustomMaps;
int g_iTimeFrameMin;
int g_iTimeFrameMax;
int g_iSharedCDMode;
int g_iDefaultTier;

float g_fRandomStartTime;
float g_fTimerUnlockNoms;

stock void CvarsEngineInit(EngineVersion version)
{
	switch (version)
	{
		case Engine_TF2:
		{
			g_Cvar_VoteNextLevel = FindConVar("sv_vote_issue_nextlevel_allowed");
			g_Cvar_Bonusroundtime = FindConVar("mp_bonusroundtime");
		}

		case Engine_CSGO:
		{
			g_Cvar_VoteNextLevel = FindConVar("mp_endmatch_votenextmap");
			g_Cvar_GameType = FindConVar("game_type");
			g_Cvar_GameMode = FindConVar("game_mode");
			g_Cvar_Bonusroundtime = FindConVar("mp_round_restart_delay");
		}

		case Engine_DODS:
		{
			g_Cvar_Bonusroundtime = FindConVar("dod_bonusroundtime");
		}

		case Engine_CSS:
		{
			g_Cvar_Bonusroundtime = FindConVar("mp_round_restart_delay");
		}

		default:
		{
			g_Cvar_Bonusroundtime = FindConVar("mp_bonusroundtime");
		}
	}
}

stock void CvarsInit()
{
	g_Cvar_Winlimit = FindConVar("mp_winlimit");
	g_Cvar_Maxrounds = FindConVar("mp_maxrounds");
	g_Cvar_Fraglimit = FindConVar("mp_fraglimit");
	g_Cvar_TimeLimit = FindConVar("mp_timelimit");

	g_Cvar_ForceLowerCase = CreateConVar("mce_force_lowercase", "1", "Force lowercase map names", _, true, 0.0, true, 1.0);

	g_Cvar_EndOfMapVote = CreateConVar("mce_endvote", "1", "Specifies if MapChooser should run an end of map vote", _, true, 0.0, true, 1.0);
	g_Cvar_EndOfMapInfo = CreateConVar("mce_endmap_info", "1", "Specifies if MapChooser should print a message with nextmap when the map end.", _, true, 0.0, true, 1.0);
	g_cvHUDChannel = CreateConVar("mce_hud_channel", "1", "Channel for the HUD messages", _, true, 0.0, true, 5.0);

	g_Cvar_StartTime = CreateConVar("mce_starttime", "10", "Specifies when to start the vote based on time remaining.", _, true, 1.0);
	g_Cvar_RandomStartTime = CreateConVar("mce_random_starttime", "30.0", "The max interval time to add up to the original interval time for map vote in seconds", _, true, 1.0, true, 180.0);
	g_Cvar_StartRounds = CreateConVar("mce_startround", "2", "Specifies when to start the vote based on rounds remaining. Use 0 on DoD:S, CS:S, and TF2 to start vote during bonus round time", _, true, 0.0);
	g_Cvar_StartFrags = CreateConVar("mce_startfrags", "5", "Specifies when to start the vote base on frags remaining.", _, true, 1.0);
	g_Cvar_ExtendTimeStep = CreateConVar("mce_extend_timestep", "15", "Specifies how much many more minutes each extension makes", _, true, 5.0);
	g_Cvar_ExtendRoundStep = CreateConVar("mce_extend_roundstep", "5", "Specifies how many more rounds each extension makes", _, true, 1.0);
	g_Cvar_ExtendFragStep = CreateConVar("mce_extend_fragstep", "10", "Specifies how many more frags are allowed when map is extended.", _, true, 5.0);
	g_Cvar_ExcludeMaps = CreateConVar("mce_exclude", "5", "Specifies how many past maps to exclude from the vote.", _, true, 0.0);
	g_Cvar_ExcludeMapsTime = CreateConVar("mce_exclude_time", "5h", "Specifies how long in minutes an old map is excluded from the vote.");
	g_Cvar_IncludeMaps = CreateConVar("mce_include", "5", "Specifies how many maps to include in the vote.", _, true, 2.0, true, 7.0);
	g_Cvar_IncludeMapsReserved = CreateConVar("mce_include_reserved", "2", "Specifies how many private/random maps to include in the vote.", _, true, 0.0, true, 5.0);
	g_Cvar_NoVoteMode = CreateConVar("mce_novote", "1", "Specifies whether or not MapChooser should pick a map if no votes are received.", _, true, 0.0, true, 1.0);
	g_Cvar_Extends = CreateConVar("mce_extend", "0", "Number of extensions allowed each map.", _, true, 0.0);
	g_Cvar_DontChange = CreateConVar("mce_dontchange", "1", "Specifies if a 'Don't Change' option should be added to early votes", _, true, 0.0);
	g_Cvar_VoteDuration = CreateConVar("mce_voteduration", "20", "Specifies how long the mapvote should be available for.", _, true, 5.0);
	g_Cvar_CountBots = CreateConVar("mce_count_bots", "1", "Should we count bots as players for Min/MaxPlayers ?", FCVAR_NOTIFY, true, 0.0, true, 1.0);

	// MapChooser Extended cvars
	CreateConVar("mce_version", MCE_VERSION, "MapChooser Extended Version", FCVAR_SPONLY|FCVAR_NOTIFY|FCVAR_DONTRECORD);

	g_Cvar_RunOff = CreateConVar("mce_runoff", "1", "Hold run off votes if winning choice has less than a certain percentage of votes", _, true, 0.0, true, 1.0);
	g_Cvar_RunOffPercent = CreateConVar("mce_runoffpercent", "50", "If winning choice has less than this percent of votes, hold a runoff", _, true, 0.0, true, 100.0);
	g_Cvar_BlockSlots = CreateConVar("mce_blockslots", "0", "Block slots to prevent accidental votes.  Only applies when Voice Command style menus are in use.", _, true, 0.0, true, 1.0);
	//g_Cvar_BlockSlotsCount = CreateConVar("mce_blockslots_count", "2", "Number of slots to block.", _, true, 1.0, true, 3.0);
	g_Cvar_MaxRunOffs = CreateConVar("mce_maxrunoffs", "1", "Number of run off votes allowed each map.", _, true, 0.0);
	g_Cvar_StartTimePercent = CreateConVar("mce_start_percent", "35", "Specifies when to start the vote based on percents.", _, true, 0.0, true, 100.0);
	g_Cvar_EnableStartTimePercent = CreateConVar("mce_start_percent_enable", "0", "Enable or Disable percentage calculations when to start vote.", _, true, 0.0, true, 1.0);
	g_Cvar_WarningTime = CreateConVar("mce_warningtime", "15.0", "Warning time in seconds.", _, true, 0.0, true, 60.0);
	g_Cvar_RunOffWarningTime = CreateConVar("mce_runoffvotewarningtime", "5.0", "Warning time for runoff vote in seconds.", _, true, 0.0, true, 30.0);
	g_Cvar_MenuStyle = CreateConVar("mce_menustyle", "0", "Menu Style.  0 is the game's default, 1 is the older Valve style that requires you to press Escape to see the menu, 2 is the newer 1-9 button Voice Command style, unavailable in some games.", _, true, 0.0, true, 2.0);
	g_Cvar_TimerLocation = CreateConVar("mce_warningtimerlocation", "0", "Location for the warning timer text. 0 is HintBox, 1 is Center text, 2 is Chat.  Defaults to HintBox.", _, true, 0.0, true, 2.0);
	g_Cvar_MarkCustomMaps = CreateConVar("mce_markcustommaps", "1", "Mark custom maps in the vote list. 0 = Disabled, 1 = Mark with *, 2 = Mark with phrase.", _, true, 0.0, true, 2.0);
	g_Cvar_ExtendPosition = CreateConVar("mce_extendposition", "0", "Position of Extend/Don't Change options. 0 = at end, 1 = at start.", _, true, 0.0, true, 1.0);
	g_Cvar_RandomizeNominations = CreateConVar("mce_randomizeorder", "0", "Randomize map order?", _, true, 0.0, true, 1.0);
	g_Cvar_HideTimer = CreateConVar("mce_hidetimer", "0", "Hide the MapChooser Extended warning timer", _, true, 0.0, true, 1.0);
	g_Cvar_NoVoteOption = CreateConVar("mce_addnovote", "1", "Add \"No Vote\" to vote menu?", _, true, 0.0, true, 1.0);
	g_Cvar_ShufflePerClient = CreateConVar("mce_shuffle_per_client", "1", "Random shuffle map vote menu per client?", _, true, 0.0, true, 1.0);
	g_Cvar_NoRestrictionTimeframeEnable = CreateConVar("mce_no_restriction_timeframe_enable", "1", "Enable timeframe where all nomination restrictions and cooldowns are disabled?", _, true, 0.0, true, 1.0);
	g_Cvar_NoRestrictionTimeframeMinTime = CreateConVar("mce_no_restriction_timeframe_mintime", "0100", "Start of the timeframe where all nomination restrictions and cooldowns are disabled (Format: HHMM)", _, true, 0000.0, true, 2359.0);
	g_Cvar_NoRestrictionTimeframeMaxTime = CreateConVar("mce_no_restriction_timeframe_maxtime", "0700", "End of the timeframe where all nomination restrictions and cooldowns are disabled (Format: HHMM)", _, true, 0000.0, true, 2359.0);
	g_Cvar_LockNominationsAtWarning = CreateConVar("mce_locknominationswarning", "1", "Lock nominations when the warning start for vote", _, true, 0.0, true, 1.0);
	g_Cvar_TimerUnlockNoms = CreateConVar("mce_locknominations_timer", "15.0", "Unlock nominations after a vote. Time in seconds.", _, true, 0.0, true, 60.0);
	g_Cvar_ShowNominator = CreateConVar("mce_shownominator", "1", "See who nominated the map which won the vote", _, true, 0.0, true, 1.0);
	g_Cvar_SharedCDMode = CreateConVar("mce_sharedcd_mode", "1", "Group maps with shared cooldowns mode (See ExcludeMap native docs for mode)", _, true, 0.0, true, 2.0);
	g_Cvar_CooldownMode = CreateConVar("mce_cooldown_mode", "0", "Cooldown mode: 0 = Both (Cooldown + CooldownTime), 1 = Counter only (Cooldown), 2 = Time only (CooldownTime), 3 = Priority (CooldownTime first, then Cooldown)", _, true, 0.0, true, 3.0);
	g_Cvar_DefaultTier = CreateConVar("mce_default_tier", "0", "Default tier number to give for maps that don't have the tier param specified [0 = don't show any tier]", _, true, 0.0);
	g_Cvar_ShowMapTierInChat = CreateConVar("mce_showmaptier_in_chat", "1", "See the map tier name in chat", _, true, 0.0, true, 1.0);
	
	AutoExecConfig(true, "mapchooser_extended");

	// Hooks cvars changes 
	g_Cvar_ForceLowerCase.AddChangeHook(OnConVarChanged);
	g_Cvar_EndOfMapVote.AddChangeHook(OnConVarChanged);
	g_Cvar_EndOfMapInfo.AddChangeHook(OnConVarChanged);
	g_cvHUDChannel.AddChangeHook(OnConVarChanged);
	g_Cvar_StartTime.AddChangeHook(OnConVarChanged);
	g_Cvar_RandomStartTime.AddChangeHook(OnConVarChanged);
	g_Cvar_StartRounds.AddChangeHook(OnConVarChanged);
	g_Cvar_StartFrags.AddChangeHook(OnConVarChanged);
	g_Cvar_ExtendTimeStep.AddChangeHook(OnConVarChanged);
	g_Cvar_ExtendRoundStep.AddChangeHook(OnConVarChanged);
	g_Cvar_ExtendFragStep.AddChangeHook(OnConVarChanged);
	g_Cvar_ExcludeMaps.AddChangeHook(OnConVarChanged);
	g_Cvar_ExcludeMapsTime.AddChangeHook(OnConVarChanged);
	g_Cvar_IncludeMaps.AddChangeHook(OnConVarChanged);
	g_Cvar_IncludeMapsReserved.AddChangeHook(OnConVarChanged);
	g_Cvar_NoVoteMode.AddChangeHook(OnConVarChanged);
	g_Cvar_Extends.AddChangeHook(OnConVarChanged);
	g_Cvar_DontChange.AddChangeHook(OnConVarChanged);
	g_Cvar_VoteDuration.AddChangeHook(OnConVarChanged);
	g_Cvar_CountBots.AddChangeHook(OnConVarChanged);
	g_Cvar_RunOff.AddChangeHook(OnConVarChanged);
	g_Cvar_RunOffPercent.AddChangeHook(OnConVarChanged);
	g_Cvar_BlockSlots.AddChangeHook(OnConVarChanged);
	g_Cvar_MaxRunOffs.AddChangeHook(OnConVarChanged);
	g_Cvar_StartTimePercent.AddChangeHook(OnConVarChanged);
	g_Cvar_EnableStartTimePercent.AddChangeHook(OnConVarChanged);
	g_Cvar_WarningTime.AddChangeHook(OnConVarChanged);
	g_Cvar_RunOffWarningTime.AddChangeHook(OnConVarChanged);
	g_Cvar_MenuStyle.AddChangeHook(OnConVarChanged);
	g_Cvar_TimerLocation.AddChangeHook(OnConVarChanged);
	g_Cvar_ExtendPosition.AddChangeHook(OnConVarChanged);
	g_Cvar_MarkCustomMaps.AddChangeHook(OnConVarChanged);
	g_Cvar_RandomizeNominations.AddChangeHook(OnConVarChanged);
	g_Cvar_HideTimer.AddChangeHook(OnConVarChanged);
	g_Cvar_NoVoteOption.AddChangeHook(OnConVarChanged);
	g_Cvar_ShufflePerClient.AddChangeHook(OnConVarChanged);
	g_Cvar_NoRestrictionTimeframeEnable.AddChangeHook(OnConVarChanged);
	g_Cvar_NoRestrictionTimeframeMinTime.AddChangeHook(OnConVarChanged);
	g_Cvar_NoRestrictionTimeframeMaxTime.AddChangeHook(OnConVarChanged);
	g_Cvar_TimerUnlockNoms.AddChangeHook(OnConVarChanged);
	g_Cvar_LockNominationsAtWarning.AddChangeHook(OnConVarChanged);
	g_Cvar_ShowNominator.AddChangeHook(OnConVarChanged);
	g_Cvar_SharedCDMode.AddChangeHook(OnConVarChanged);
	g_Cvar_CooldownMode.AddChangeHook(OnConVarChanged);
	g_Cvar_DefaultTier.AddChangeHook(OnConVarChanged);
	
	// Cache cvars values
	g_bForceLowerCase = GetConVarBool(g_Cvar_ForceLowerCase);
	g_bEndOfMapVote = GetConVarBool(g_Cvar_EndOfMapVote);
	g_bEndOfMapInfo = GetConVarBool(g_Cvar_EndOfMapInfo);
	g_bNoVoteMode = GetConVarBool(g_Cvar_NoVoteMode);
	g_bDontChange = GetConVarBool(g_Cvar_DontChange);
	g_bHideTimer = GetConVarBool(g_Cvar_HideTimer);
	g_bNoVoteOption = GetConVarBool(g_Cvar_NoVoteOption);
	g_bShufflePerClient = GetConVarBool(g_Cvar_ShufflePerClient);
	g_bNoRestrictionTimeframeEnable = GetConVarBool(g_Cvar_NoRestrictionTimeframeEnable);
	g_bLockNominationsAtWarning = GetConVarBool(g_Cvar_LockNominationsAtWarning);
	g_bShowNominator = GetConVarBool(g_Cvar_ShowNominator);
	g_bCountBots = GetConVarBool(g_Cvar_CountBots);
	g_bRunOff = GetConVarBool(g_Cvar_RunOff);
	g_bBlockSlots = GetConVarBool(g_Cvar_BlockSlots);
	g_bEnableStartPercent = GetConVarBool(g_Cvar_EnableStartTimePercent);
	g_bRandomizeNominations = GetConVarBool(g_Cvar_RandomizeNominations);
	g_bExtendPositionTop = GetConVarBool(g_Cvar_ExtendPosition);
	g_bShowMapTierInChat = GetConVarBool(g_Cvar_ShowMapTierInChat);
	
	GetConVarString(g_Cvar_ExcludeMapsTime, g_sExcludeMapsTime, sizeof(g_sExcludeMapsTime));

	g_iStartRounds = GetConVarInt(g_Cvar_StartRounds);
	g_iStartFrags = GetConVarInt(g_Cvar_StartFrags);
	g_iHUDChannel = GetConVarInt(g_cvHUDChannel);
	g_iExtendTimeStep = GetConVarInt(g_Cvar_ExtendTimeStep);
	g_iExtendRoundStep = GetConVarInt(g_Cvar_ExtendRoundStep);
	g_iExtendFragStep = GetConVarInt(g_Cvar_ExtendFragStep);
	g_iExcludeMaps = GetConVarInt(g_Cvar_ExcludeMaps);
	g_iIncludeMaps = GetConVarInt(g_Cvar_IncludeMaps);
	g_iIncludeMapsReserved = GetConVarInt(g_Cvar_IncludeMapsReserved);
	g_iMaxExtends = GetConVarInt(g_Cvar_Extends);
	g_iVoteDuration = GetConVarInt(g_Cvar_VoteDuration);
	g_iPercentRunOff = GetConVarInt(g_Cvar_RunOffPercent);
	g_iMaxRunOffs = GetConVarInt(g_Cvar_MaxRunOffs);
	g_iStartTimePercent = GetConVarInt(g_Cvar_StartTimePercent);
	g_iWarningTime = GetConVarInt(g_Cvar_WarningTime);
	g_iRunOffWarningTime = GetConVarInt(g_Cvar_RunOffWarningTime);
	g_iStartTime = GetConVarInt(g_Cvar_StartTime);
	g_iMenuStyle = GetConVarInt(g_Cvar_MenuStyle);
	g_iTimerLocation = GetConVarInt(g_Cvar_TimerLocation);
	g_iMarkCustomMaps = GetConVarInt(g_Cvar_MarkCustomMaps);
	g_iTimeFrameMin = GetConVarInt(g_Cvar_NoRestrictionTimeframeMinTime);
	g_iTimeFrameMax = GetConVarInt(g_Cvar_NoRestrictionTimeframeMaxTime);
	g_iSharedCDMode = GetConVarInt(g_Cvar_SharedCDMode);
	g_CooldownMode = view_as<CooldownMode>(GetConVarInt(g_Cvar_CooldownMode));
	g_iDefaultTier = GetConVarInt(g_Cvar_DefaultTier);
	
	g_fRandomStartTime = GetConVarFloat(g_Cvar_RandomStartTime);
	g_fTimerUnlockNoms = GetConVarFloat(g_Cvar_TimerUnlockNoms);
}

public void OnConVarChanged(ConVar convar, char[] oldValue, char[] newValue)
{
	if (convar == g_Cvar_ForceLowerCase)
		g_bForceLowerCase = GetConVarBool(g_Cvar_ForceLowerCase);
	else if (convar == g_Cvar_EndOfMapVote)
		g_bEndOfMapVote = GetConVarBool(g_Cvar_EndOfMapVote);
	else if (convar == g_Cvar_EndOfMapInfo)
		g_bEndOfMapInfo = GetConVarBool(g_Cvar_EndOfMapInfo);
	else if (convar == g_cvHUDChannel)
		g_iHUDChannel = GetConVarInt(g_cvHUDChannel);
	else if (convar == g_Cvar_StartTime)
		g_iStartTime = GetConVarInt(g_Cvar_StartTime);
	else if (convar == g_Cvar_RandomStartTime)
		g_fRandomStartTime = GetConVarFloat(g_Cvar_RandomStartTime);
	else if (convar == g_Cvar_StartRounds)
		g_iStartRounds = GetConVarInt(g_Cvar_StartRounds);
	else if (convar == g_Cvar_StartFrags)
		g_iStartFrags = GetConVarInt(g_Cvar_StartFrags);
	else if (convar == g_Cvar_ExtendTimeStep)
		g_iExtendTimeStep = GetConVarInt(g_Cvar_ExtendTimeStep);
	else if (convar == g_Cvar_ExtendRoundStep)
		g_iExtendRoundStep = GetConVarInt(g_Cvar_ExtendRoundStep);
	else if (convar == g_Cvar_ExtendFragStep)
		g_iExtendFragStep = GetConVarInt(g_Cvar_ExtendFragStep);
	else if (convar == g_Cvar_ExcludeMaps)
		g_iExcludeMaps = GetConVarInt(g_Cvar_ExcludeMaps);
	else if (convar == g_Cvar_ExcludeMapsTime)
		GetConVarString(g_Cvar_ExcludeMapsTime, g_sExcludeMapsTime, sizeof(g_sExcludeMapsTime));
	else if (convar == g_Cvar_IncludeMaps)
		g_iIncludeMaps = GetConVarInt(g_Cvar_IncludeMaps);
	else if (convar == g_Cvar_IncludeMapsReserved)
		g_iIncludeMapsReserved = GetConVarInt(g_Cvar_IncludeMapsReserved);
	else if (convar == g_Cvar_NoVoteMode)
		g_bNoVoteMode = GetConVarBool(g_Cvar_NoVoteMode);
	else if (convar == g_Cvar_Extends)
		g_iMaxExtends = GetConVarInt(g_Cvar_Extends);
	else if (convar == g_Cvar_DontChange)
		g_bDontChange = GetConVarBool(g_Cvar_DontChange);
	else if (convar == g_Cvar_VoteDuration)
		g_iVoteDuration = GetConVarInt(g_Cvar_VoteDuration);
	else if (convar == g_Cvar_CountBots)
		g_bCountBots = GetConVarBool(g_Cvar_CountBots);
	else if (convar == g_Cvar_RunOff)
		g_bRunOff = GetConVarBool(g_Cvar_RunOff);
	else if (convar == g_Cvar_RunOffPercent)
		g_iPercentRunOff = GetConVarInt(g_Cvar_RunOffPercent);
	else if (convar == g_Cvar_BlockSlots)
		g_bBlockSlots = GetConVarBool(g_Cvar_BlockSlots);
	else if (convar == g_Cvar_MaxRunOffs)
		g_iMaxRunOffs = GetConVarInt(g_Cvar_MaxRunOffs);
	else if (convar == g_Cvar_StartTimePercent)
		g_iStartTimePercent = GetConVarInt(g_Cvar_StartTimePercent);
	else if (convar == g_Cvar_EnableStartTimePercent)
		g_bEnableStartPercent = GetConVarBool(g_Cvar_EnableStartTimePercent);
	else if (convar == g_Cvar_WarningTime)
		g_iWarningTime = GetConVarInt(g_Cvar_WarningTime);
	else if (convar == g_Cvar_RunOffWarningTime)
		g_iRunOffWarningTime = GetConVarInt(g_Cvar_RunOffWarningTime);
	else if (convar == g_Cvar_MenuStyle)
		g_iMenuStyle = GetConVarInt(g_Cvar_MenuStyle);
	else if (convar == g_Cvar_TimerLocation)
		g_iTimerLocation = GetConVarInt(g_Cvar_TimerLocation);
	else if (convar == g_Cvar_ExtendPosition)
		g_bExtendPositionTop = GetConVarBool(g_Cvar_ExtendPosition);
	else if (convar == g_Cvar_MarkCustomMaps)
		g_iMarkCustomMaps = GetConVarInt(g_Cvar_MarkCustomMaps);
	else if (convar == g_Cvar_RandomizeNominations)
		g_bRandomizeNominations = GetConVarBool(g_Cvar_RandomizeNominations);
	else if (convar == g_Cvar_HideTimer)
		g_bHideTimer = GetConVarBool(g_Cvar_HideTimer);
	else if (convar == g_Cvar_NoVoteOption)
		g_bNoVoteOption = GetConVarBool(g_Cvar_NoVoteOption);
	else if (convar == g_Cvar_ShufflePerClient)
		g_bShufflePerClient = GetConVarBool(g_Cvar_ShufflePerClient);
	else if (convar == g_Cvar_NoRestrictionTimeframeEnable)
		g_bNoRestrictionTimeframeEnable = GetConVarBool(g_Cvar_NoRestrictionTimeframeEnable);
	else if (convar == g_Cvar_NoRestrictionTimeframeMinTime)
		g_iTimeFrameMin = GetConVarInt(g_Cvar_NoRestrictionTimeframeMinTime);
	else if (convar == g_Cvar_NoRestrictionTimeframeMaxTime)
		g_iTimeFrameMax = GetConVarInt(g_Cvar_NoRestrictionTimeframeMaxTime);
	else if (convar == g_Cvar_TimerUnlockNoms)
		g_fTimerUnlockNoms = GetConVarFloat(g_Cvar_TimerUnlockNoms);
	else if (convar == g_Cvar_LockNominationsAtWarning)
		g_bLockNominationsAtWarning = GetConVarBool(g_Cvar_LockNominationsAtWarning);
	else if (convar == g_Cvar_ShowNominator)
		g_bShowNominator = GetConVarBool(g_Cvar_ShowNominator);
	else if (convar == g_Cvar_SharedCDMode)
		g_iSharedCDMode = GetConVarInt(g_Cvar_SharedCDMode);
	else if (convar == g_Cvar_CooldownMode)
		g_CooldownMode = view_as<CooldownMode>(GetConVarInt(g_Cvar_CooldownMode));
	else if (convar == g_Cvar_DefaultTier)
		g_iDefaultTier = GetConVarInt(g_Cvar_DefaultTier);
	else if (convar == g_Cvar_ShowMapTierInChat)
		g_bShowMapTierInChat = GetConVarBool(g_Cvar_ShowMapTierInChat);
}
