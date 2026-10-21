/**
 * vim: set ts=4 :
 * =============================================================================
 * MapChooser Extended Sounds
 * Sound support for Mapchooser Extended
 * Inspired by QuakeSounds 2.7
 *
 * MapChooser Extended Sounds (C)2011-2014 Powerlord (Ross Bemrose)
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

#pragma newdecls required
#pragma semicolon 1

#include <sourcemod>
#include <mapchooser>
#include <mapchooser_extended>
#include <sdktools>

#define VERSION "1.10.8"

#define CONFIG_DIRECTORY "configs/mapchooser_extended/sounds"
#define DEFAULT_SOUND_SET "tf2"
#define SET_NAME_MAX_LENGTH 64

// 0-60, even though we don't ever call 0
// Counter-intuitive note: This array has 61 elements, not 60
#define COUNTER_MAX_SIZE 60
// The number of digits in the previous number
#define COUNTER_MAX_SIZE_DIGITS 2

#define NUM_TYPES 5

// CVar Handles
ConVar g_Cvar_EnableSounds = null;
ConVar g_Cvar_EnableCounterSounds = null;
ConVar g_Cvar_SoundSet = null;
ConVar g_Cvar_DownloadAllSounds = null;

// Data Handles
ArrayList g_TypeNames = null; // Maps SoundEvent enumeration values to KeyValue section names
ArrayList g_SetNames = null;
StringMap g_SoundFiles = null;
StringMap g_CurrentSoundSet = null; // Lazy "pointer" to the current sound set.  Updated on cvar change or map change.

//Global variables
bool g_DownloadAllSounds;
bool g_bLate = false;

enum SoundEvent
{
	SoundEvent_Counter = 0,
	SoundEvent_VoteStart = 1,
	SoundEvent_VoteEnd = 2,
	SoundEvent_VoteWarning = 3,
	SoundEvent_RunoffWarning = 4,
}

enum SoundType
{
	SoundType_None,
	SoundType_Sound,
	SoundType_Builtin,
	SoundType_Event
}

enum struct SoundStore
{
	char SoundStore_Value[PLATFORM_MAX_PATH];
	SoundType SoundStore_Type;
}

public Plugin myinfo = 
{
	name = "Mapchooser Extended Sounds",
	author = "Powerlord",
	description = "Sound support for Mapchooser Extended",
	version = VERSION,
	url = "https://forums.alliedmods.net/showthread.php?t=156974"
}

// Map enum values to their named values
// This is used for searching later.
stock void PopulateTypeNamesArray()
{
	if (g_TypeNames == null)
	{
		g_TypeNames = new ArrayList(ByteCountToCells(SET_NAME_MAX_LENGTH), NUM_TYPES);
		g_TypeNames.SetString(view_as<int>(SoundEvent_Counter), "counter");
		g_TypeNames.SetString(view_as<int>(SoundEvent_VoteStart), "vote start");
		g_TypeNames.SetString(view_as<int>(SoundEvent_VoteEnd), "vote end");
		g_TypeNames.SetString(view_as<int>(SoundEvent_VoteWarning), "vote warning");
		g_TypeNames.SetString(view_as<int>(SoundEvent_RunoffWarning), "runoff warning");
	}
}

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	g_bLate = late;
	return APLRes_Success;
}

public void OnPluginStart()
{
	g_Cvar_EnableSounds = CreateConVar("mce_sounds_enablesounds", "1", "Enable this plugin.  Sounds will still be downloaded (if applicable) even if the plugin is disabled this way.", FCVAR_NONE, true, 0.0, true, 1.0);
	g_Cvar_EnableCounterSounds = CreateConVar("mce_sounds_enablewarningcountersounds", "1", "Enable sounds to be played during warning counter.  If this is disabled, map vote warning, start, and stop sounds still play.", FCVAR_NONE, true, 0.0, true, 1.0);
	g_Cvar_SoundSet = CreateConVar("mce_sounds_soundset", DEFAULT_SOUND_SET, "Sound set to use, optimized for TF by default.  Sound sets are defined in addons/sourcemod/configs/mapchooser_extended/sound  Takes effect immediately if sm_mapvote_downloadallsounds is 1, otherwise at map change.", FCVAR_NONE);
	g_Cvar_DownloadAllSounds = CreateConVar("mce_sounds_downloadallsounds", "0", "Force players to download all sound sets, so sets can be dynamically changed during the map. Defaults to off. Takes effect at map change.", FCVAR_NONE, true, 0.0, true, 1.0);
	CreateConVar("mce_sounds_version", VERSION, "Mapchooser Extended Sounds Version", FCVAR_DONTRECORD|FCVAR_SPONLY|FCVAR_REPLICATED);

	AutoExecConfig(true, "mapchooser_extended_sounds");

	RegAdminCmd("mce_sounds_reload", Command_Reload, ADMFLAG_CONVARS, "Reload Mapchooser Sound configuration file.");
	RegAdminCmd("sm_mapvote_reload_sounds", Command_Reload, ADMFLAG_CONVARS, "Deprecated: use mce_sounds_reload");

	RegAdminCmd("mce_sounds_list_soundsets", Command_List_Soundsets, ADMFLAG_CONVARS, "List available Mapchooser Extended sound sets.");
	RegAdminCmd("sm_mapvote_list_soundsets", Command_List_Soundsets, ADMFLAG_CONVARS, "Deprecated: use mce_sounds_list_soundsets");

	PopulateTypeNamesArray();
	// LoadSounds needs to be  executed even if the plugin is "disabled" via the sm_mapvote_enablesounds cvar.

	g_SetNames = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
	g_SoundFiles = new StringMap();
	LoadSounds();
	g_Cvar_SoundSet.AddChangeHook(SoundSetChanged);

	if (g_bLate)
		OnConfigsExecuted();
}

public void OnConfigsExecuted()
{
	g_DownloadAllSounds = g_Cvar_DownloadAllSounds.BoolValue;

	SetSoundSetFromCVar();
	
	if (g_DownloadAllSounds)
	{
		BuildDownloadsTableAll();
	}
	else
	{
		BuildDownloadsTable(g_CurrentSoundSet);
	}
}

stock void SetSoundSetFromCVar()
{
	char soundSet[SET_NAME_MAX_LENGTH];
	
	// Store which sound set is in use
	g_Cvar_SoundSet.GetString(soundSet, sizeof(soundSet));
	
	// Unknown sound set from config file, reset to default
	if (g_SetNames.FindString(soundSet) == -1 && !StrEqual(soundSet, DEFAULT_SOUND_SET, true))
	{
		g_Cvar_SoundSet.RestoreDefault();
		g_Cvar_SoundSet.GetString(soundSet, sizeof(soundSet));
	}
	
	SetCurrentSoundSet(soundSet);
}

public void SoundSetChanged(ConVar cvar, char[] oldValue, char[] newValue)
{
	if (g_SetNames.FindString(newValue) == -1)
	{
		LogError("New sound set not found: %s", newValue);
		cvar.SetString(oldValue);
	}
	else if (g_DownloadAllSounds)
	{
		SetCurrentSoundSet(newValue);
	}
}

public void OnMapVoteStarted()
{
	PlaySound(SoundEvent_VoteStart);
}

public void OnMapVoteEnd(const char[] map)
{
	PlaySound(SoundEvent_VoteEnd);
}

public void OnMapVoteWarningStart()
{
	PlaySound(SoundEvent_VoteWarning);
}

public void OnMapVoteRunnoffWarningStart()
{
	PlaySound(SoundEvent_RunoffWarning);
}

public void OnMapVoteWarningTick(int time)
{
	if (g_Cvar_EnableSounds.BoolValue && g_Cvar_EnableCounterSounds.BoolValue) {
		char currentType[SET_NAME_MAX_LENGTH];
		StringMap counterTrie;
		
		if (g_CurrentSoundSet != null)
		{
			if (g_TypeNames.GetString(view_as<int>(SoundEvent_Counter), currentType, sizeof(currentType)) > 0 && g_CurrentSoundSet.GetValue(currentType, counterTrie))
			{
				char key[5];
				IntToString(time, key, sizeof(key));

				SoundStore soundData;
				if (!counterTrie.GetArray(key, soundData, sizeof(soundData)))
				{
					return;
				}

				if (soundData.SoundStore_Type == SoundType_Event)
				{
					Event broadcastEvent = CreateEvent("teamplay_broadcast_audio");
					if (broadcastEvent == null)
					{
						#if defined DEBUG
						LogError("Could not create teamplay_broadcast_event. This may be because there are no players connected.");
						#endif
						return;
					}
					broadcastEvent.SetInt("team", -1);
					broadcastEvent.SetString("sound", soundData.SoundStore_Value);
					broadcastEvent.Fire();
				}
				else
				{
					EmitSoundToAll(soundData.SoundStore_Value);
				}
			}
		}
	}
}

public Action Command_Reload(int client, int args)
{
	LoadSounds();
	SetSoundSetFromCVar();
	ReplyToCommand(client, "[MCES] Reloaded sound configuration.");
	return Plugin_Handled;
}

public Action Command_List_Soundsets(int client, int args)
{
	int setCount = g_SetNames.Length;
	ReplyToCommand(client, "[SM] The following %d sound sets are installed:", setCount);
	for (int i = 0; i < setCount; i++)
	{
		char setName[SET_NAME_MAX_LENGTH];
		g_SetNames.GetString(i, setName, sizeof(setName));
		ReplyToCommand(client, "[SM] %s", setName);
	}
	return Plugin_Handled;
}

stock void PlaySound(SoundEvent event)
{
	if (g_Cvar_EnableSounds.BoolValue)
	{
		if (g_CurrentSoundSet != null)
		{
			char currentType[SET_NAME_MAX_LENGTH];
			
			if (g_TypeNames.GetString(view_as<int>(event), currentType, sizeof(currentType)) > 0)
			{
				SoundStore soundData;
				if (!g_CurrentSoundSet.GetArray(currentType, soundData, sizeof(soundData)))
					return;

				if (soundData.SoundStore_Type == SoundType_Event)
				{
					Event broadcastEvent = CreateEvent("teamplay_broadcast_audio");
					if (broadcastEvent == null)
					{
						#if defined DEBUG
						LogError("Could not create teamplay_broadcast_event. This may be because there are no players connected.");
						#endif
						return;
					}
					broadcastEvent.SetInt("team", -1);
					broadcastEvent.SetString("sound", soundData.SoundStore_Value);
					broadcastEvent.Fire();
				}
				else
				{
					EmitSoundToAll(soundData.SoundStore_Value);
				}
			}
		}
	}

}

stock void SetCurrentSoundSet(char[] soundSet)
{
	// Save a reference to the Trie for the current sound set, for use in the forwards below.
	// Also do error checking to make sure the set exists.
	if (!g_SoundFiles.GetValue(soundSet, g_CurrentSoundSet))
	{
		SetFailState("Could not load sound set");
	}
}

// Load the list of sounds sounds from the configuration file
// This should be done on plugin load.
// This looks really complicated, but it really isn't.
stock void LoadSounds()
{
	CloseSoundArrayHandles();
	
	char directoryPath[PLATFORM_MAX_PATH];
	char modName[SET_NAME_MAX_LENGTH];
	
	GetGameFolderName(modName, sizeof(modName));
	
	BuildPath(Path_SM, directoryPath, sizeof(directoryPath), CONFIG_DIRECTORY);

	DirectoryListing directory = OpenDirectory(directoryPath);
	if (directory != null)
	{
		char dirEntry[PLATFORM_MAX_PATH];
		while (directory.GetNext(dirEntry, sizeof(dirEntry)))
		{
			KeyValues soundsKV = new KeyValues("MapchooserSoundsList");
			char filePath[PLATFORM_MAX_PATH];
			
			Format(filePath, sizeof(filePath), "%s/%s", directoryPath, dirEntry);
			
			if (!DirExists(filePath))
			{
				soundsKV.ImportFromFile(filePath);
				
				if (soundsKV.GotoFirstSubKey())
				{
					// Iterate through the sets
					do
					{
						StringMap setTrie = new StringMap();
						char currentSet[SET_NAME_MAX_LENGTH];
						bool builtinSet = false;
						
						soundsKV.GetSectionName(currentSet, sizeof(currentSet));
						
						if (g_SetNames.FindString(currentSet) == -1)
						{
							// Add to the list of sound sets
							g_SetNames.PushString(currentSet);
						}
						else
						{
							SetFailState("Duplicate sound set: %s", currentSet);
						}
						
						if (StrEqual(currentSet, modName, false))
						{
							builtinSet = true;
						}
						
						if (soundsKV.GotoFirstSubKey()) {
							// Iterate through each sound in the set
							do
							{
								char currentType[SET_NAME_MAX_LENGTH];
								soundsKV.GetSectionName(currentType, sizeof(currentType));
								// Type to enum mapping
								SoundEvent typeKey = view_as<SoundEvent>(g_TypeNames.FindString(currentType));
								
								switch(typeKey)
								{
									case SoundEvent_Counter:
									{
										// Counter is special, as it has multiple values
										StringMap counterTrie = new StringMap();
										
										if (soundsKV.GotoFirstSubKey())
										{
											do
											{
												// Get the current key
												char time[COUNTER_MAX_SIZE_DIGITS + 1];
												
												soundsKV.GetSectionName(time, sizeof(time));
												
												SoundStore soundData;
												
												// new key = StringToInt(time);
												
												soundData.SoundStore_Type =  RetrieveSound(soundsKV, builtinSet, soundData.SoundStore_Value, PLATFORM_MAX_PATH);
												if (soundData.SoundStore_Type == SoundType_None)
												{
													continue;
												}
												
												// This seems wrong, but this is documented on the forums here: https://forums.alliedmods.net/showthread.php?t=151942
												counterTrie.SetArray(time, soundData, sizeof(soundData));
												
												//SetArrayString(counterArray, key, soundFile);
											} while (soundsKV.GotoNextKey());
											soundsKV.GoBack();
										}
										
										setTrie.SetValue(currentType, view_as<int>(counterTrie));
									}
									
									// Set the sounds directly for other types
									default:
									{
										SoundStore soundData;
										
										soundData.SoundStore_Type = RetrieveSound(soundsKV, builtinSet, soundData.SoundStore_Value, PLATFORM_MAX_PATH);
										
										if (soundData.SoundStore_Type == SoundType_None)
										{
											continue;
										}

										setTrie.SetArray(currentType, soundData, sizeof(soundData));
									}
								}
							} while (soundsKV.GotoNextKey());
							soundsKV.GoBack();
						}
						g_SoundFiles.SetValue(currentSet, setTrie);
					} while (soundsKV.GotoNextKey());
				}
			}
			delete soundsKV;
		}
		delete directory;
	}
	
	if (g_SetNames.Length == 0)
	{
		SetFailState("Could not locate any sound sets.");
	}
}

// Internal LoadSounds function to get sound and type 
stock SoundType RetrieveSound(KeyValues soundsKV, bool isBuiltin, char[] soundFile, int soundFileSize)
{
	if (isBuiltin)
	{
		// event is considered before builtin, as it has related game data and should always be used in preference to builtin
		soundsKV.GetString("event", soundFile, soundFileSize);
		
		if (!StrEqual(soundFile, ""))
		{
			return SoundType_Event;
		}
		
		soundsKV.GetString("builtin", soundFile, soundFileSize);
		if (!StrEqual(soundFile, ""))
		{
			return SoundType_Builtin;
		}
	}
	
	soundsKV.GetString("sound", soundFile, soundFileSize);

	if (!StrEqual(soundFile, ""))
	{
		return SoundType_Sound;
	}
	
	// Whoops, didn't find this sound
	return SoundType_None;
}

// Preload all sounds in a set
stock void BuildDownloadsTable(StringMap currentSoundSet)
{
	if (currentSoundSet != null)
	{
		for (int i = 0; i < g_TypeNames.Length; i++)
		{
			char currentType[SET_NAME_MAX_LENGTH];
			g_TypeNames.GetString(i, currentType, sizeof(currentType));

			SoundEvent typeKey = view_as<SoundEvent>(i);

			switch(typeKey)
			{
				case SoundEvent_Counter:
				{
					StringMap counterTrie;
					if (currentSoundSet.GetValue(currentType, counterTrie))
					{
						// Skip value 0
						for (int j = 1; j <= COUNTER_MAX_SIZE; ++j)
						{
							char key[5];
							IntToString(j, key, sizeof(key));
							
							SoundStore soundData;
							counterTrie.GetArray(key, soundData, sizeof(soundData));
							if (soundData.SoundStore_Type != SoundType_Event)
							{
								CacheSound(soundData);
							}
						}
					}
				}
				
				default:
				{
					SoundStore soundData;
					currentSoundSet.GetArray(currentType, soundData, sizeof(soundData));
					
					if (soundData.SoundStore_Type != SoundType_Event)
					{
						CacheSound(soundData);
					}
				}
			}
		}
	}
}

// Load each set and build its download table
stock void BuildDownloadsTableAll()
{
	for (int i = 0; i < g_SetNames.Length; i++)
	{
		char currentSet[SET_NAME_MAX_LENGTH];
		StringMap currentSoundSet;
		g_SetNames.GetString(i, currentSet, sizeof(currentSet));
		
		if (g_SoundFiles.GetValue(currentSet, currentSoundSet))
		{
			BuildDownloadsTable(currentSoundSet);
		}
	}
}

// Found myself repeating this code, so I pulled it into a separate function
stock void CacheSound(SoundStore soundData)
{
	if (soundData.SoundStore_Type == SoundType_Builtin)
	{
		PrecacheSound(soundData.SoundStore_Value);
	}
	else if (soundData.SoundStore_Type == SoundType_Sound)
	{
		if (PrecacheSound(soundData.SoundStore_Value))
		{
			char downloadLocation[PLATFORM_MAX_PATH];
			Format(downloadLocation, sizeof(downloadLocation), "sound/%s", soundData.SoundStore_Value);
			AddFileToDownloadsTable(downloadLocation);
		} else {
			LogMessage("Failed to load sound: %s", soundData.SoundStore_Value);
		}
	}
}

// Close all the handles that are children and grandchildren of the g_SoundFiles trie.
stock void CloseSoundArrayHandles()
{
	// Close all open handles in the sound set
	for (int i = 0; i < g_SetNames.Length; i++)
	{
		char currentSet[SET_NAME_MAX_LENGTH];
		StringMap trieHandle;
		Handle arrayHandle;
		
		g_SetNames.GetString(i, currentSet, sizeof(currentSet));
		if (!g_SoundFiles.GetValue(currentSet, trieHandle) || trieHandle == null)
			continue;

		// "counter" is an adt_trie, close that too
		if (trieHandle.GetValue("counter", arrayHandle) && arrayHandle != null)
			delete arrayHandle;

		delete trieHandle;
	}
	g_SoundFiles.Clear();
	g_SetNames.Clear();
	
	g_CurrentSoundSet = null;
}
