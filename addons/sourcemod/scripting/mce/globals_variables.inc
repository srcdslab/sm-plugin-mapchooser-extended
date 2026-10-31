/*
 * ============================================================================
 *
 *  MapChooser Extended
 *
 *  File:          mce/globals_variables.inc
 *  Description:   Storage of non specific global variables
 * 
 * ============================================================================
 */

enum RoundCounting
{
	RoundCounting_Standard = 0,
	RoundCounting_MvM,
	RoundCounting_ArmsRace,
}

// CSGO requires two cvars to get the game type
enum
{
	GameType_Classic	= 0,
	GameType_GunGame	= 1,
	GameType_Training	= 2,
	GameType_Custom		= 3,
}

enum
{
	GunGameMode_ArmsRace	= 0,
	GunGameMode_Demolition	= 1,
	GunGameMode_DeathMatch	= 2,
}

Handle g_VoteTimer = null;
Handle g_RetryTimer = null;
Handle g_WarningTimer = null;

/* Data Handles */
ArrayList g_MapList = null;
ArrayList g_NominateList = null;
ArrayList g_NominateOwners = null;
StringMap g_OldMapList;
StringMap g_TimeMapList;
ArrayList g_NextMapList = null;
ArrayList g_WarningMapList = null;	// Map list of the pending warning/retry countdown, owned until the vote starts
Menu g_VoteMenu = null;
KeyValues g_Config;

int g_Extends;
int g_TotalRounds;
bool g_HasVoteStarted;
bool g_WaitingForVote;
bool g_MapVoteCompleted;
bool g_PendingRevoteActive;
bool g_ChangeMapAtRoundEnd;
bool g_ChangeMapInProgress;
bool g_HasIntermissionStarted = false;
int g_WarningTimePassed = 0;	// Seconds elapsed in the current warning or retry countdown
int g_mapFileSerial = -1;

int g_NominateCount = 0;
int g_NominateReservedCount = 0;
MapChange g_ChangeTime;
bool g_VoteForced;	// The running vote was forced by sm_mapvote, its runoff is too

/* Mapchooser Extended Data Handles */
ArrayList g_OfficialList = null;

/* Mapchooser Extended Globals */
int g_RunoffCount = 0;
int g_mapOfficialFileSerial = -1;
char g_sGameModName[64];
bool g_WarningInProgress = false;
bool g_SaveCDOnMapEnd = true;
bool g_ZLeader = false;
bool g_DynamicChannels = false;

char g_sNominations[MAXPLAYERS + 1][PLATFORM_MAX_PATH];

RoundCounting g_RoundCounting = RoundCounting_Standard;

/* Upper bound of how many team there could be */
#define MAXTEAMS 10
int g_winCount[MAXTEAMS];

int g_ObjectiveEnt = -1;

enum TimerLocation
{
	TimerLocation_Hint = 0,
	TimerLocation_Center = 1,
	TimerLocation_Chat = 2,
}

enum WarningType
{
	WarningType_Vote,
	WarningType_Revote,
}

enum CooldownMode
{
	CooldownMode_Both = 0,        // Use Cooldown and CooldownTime (default)
	CooldownMode_CounterOnly = 1, // Use only Cooldown (based on map count)
	CooldownMode_TimeOnly = 2,    // Use only CooldownTime (based on time)
	CooldownMode_Priority = 3,    // Use CooldownTime first, then Cooldown
}

/* Cooldown Mode Control */
CooldownMode g_CooldownMode = CooldownMode_Both;

#define VOTE_EXTEND "##extend##"
#define VOTE_DONTCHANGE "##dontchange##"

/* Mapchooser Extended Defines */
#define LINE_ONE "##lineone##"
#define LINE_TWO "##linetwo##"
#define LINE_SPACER "##linespacer##"
#define FAILURE_TIMER_LENGTH 5

char g_sExcludeMapsTime[32];

/* Map Tiers Global Variables */
ArrayList g_MapTiers;