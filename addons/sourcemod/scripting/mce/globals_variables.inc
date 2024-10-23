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

Handle g_VoteTimer = INVALID_HANDLE;
Handle g_RetryTimer = INVALID_HANDLE;
Handle g_WarningTimer = INVALID_HANDLE;

/* Data Handles */
Handle g_MapList = INVALID_HANDLE;
Handle g_NominateList = INVALID_HANDLE;
Handle g_NominateOwners = INVALID_HANDLE;
StringMap g_OldMapList;
StringMap g_TimeMapList;
Handle g_NextMapList = INVALID_HANDLE;
Handle g_VoteMenu = INVALID_HANDLE;
KeyValues g_Config;

int g_Extends;
int g_TotalRounds;
bool g_HasVoteStarted;
bool g_WaitingForVote;
bool g_MapVoteCompleted;
bool g_ChangeMapAtRoundEnd;
bool g_ChangeMapInProgress;
bool g_HasIntermissionStarted = false;
int g_mapFileSerial = -1;

int g_NominateCount = 0;
int g_NominateReservedCount = 0;
MapChange g_ChangeTime;

/* Mapchooser Extended Data Handles */
Handle g_OfficialList = INVALID_HANDLE;

/* Mapchooser Extended Globals */
int g_RunoffCount = 0;
int g_mapOfficialFileSerial = -1;
char g_sGameModName[64];
bool g_WarningInProgress = false;
bool g_AddNoVote = false;
bool g_SaveCDOnMapEnd = true;
bool g_ZLeader = false;
bool g_DynamicChannels = false;

char g_sNominatorBuffer[1024];

RoundCounting g_RoundCounting = RoundCounting_Standard;

/* Upper bound of how many team there could be */
#define MAXTEAMS 10
int g_winCount[MAXTEAMS];

bool g_BlockedSlots = false;
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

#define VOTE_EXTEND "##extend##"
#define VOTE_DONTCHANGE "##dontchange##"

/* Mapchooser Extended Defines */
#define LINE_ONE "##lineone##"
#define LINE_TWO "##linetwo##"
#define LINE_SPACER "##linespacer##"
#define FAILURE_TIMER_LENGTH 5

char g_sExcludeMapsTime[32];