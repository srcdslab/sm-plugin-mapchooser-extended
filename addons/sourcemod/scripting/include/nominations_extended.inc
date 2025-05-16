#if defined _nominations_extended_included_
  #endinput
#endif
#define _nominations_extended_included_

#define NE_V_MAJOR "1"
#define NE_V_MINOR "12"
#define NE_V_PATCH "11"

#define NE_VERSION NE_V_MAJOR..."."...NE_V_MINOR..."."...NE_V_PATCH

/**
 * Get the current nomination pool
 * 
 * @param pool   	ArrayList to store the current pool return
 * @noreturn
 */
native int GetNominationPool(ArrayList &pool);

/**
 * Insert map from the current nomination pool
 * 
 * @param map   	Map of maps to insert in the pool
 * @noreturn
 */
native int PushMapIntoNominationPool(char[] map);

/**
 * Insert maps into the current nomination pool
 * 
 * @param maps   	ArrayList to insert
 * @noreturn
 */
native int PushMapsIntoNominationPool(ArrayList maps);

/**
 * Remove map from the current nomination pool
 * 
 * @param map   	Map name to remove
 * @noreturn
 */
native int RemoveMapFromNominationPool(char[] map);

/**
 * Remove maps from the current nomination pool
 * 
 * @param maps   	Numbers of maps to remove
 * @noreturn
 */
native int RemoveMapsFromNominationPool(ArrayList maps);

/**
 *	Toggle the lock/unlock nomination mechanism.
 *
 * @param lock			Lock with true and unlock with false.
 * @noreturn
*/
native void ToggleNominations(bool lock);

/**
 * Called when a map get nominated
 * 
 * @param client    	Client UserId
 * @param mapname     	Name of the nap nominated
 * @param IsVIP     	Is VIP Nomination
 * @param IsLeader     	Is Leader Nomination
 * @param IsAdmin     	Is Admin Nomination
 * @noreturn
 */
forward void NE_OnPublicMapInsert(int client, char[] mapname, bool IsVIP, bool IsLeader, bool IsAdmin);

/**
 * Called when a map get replaced by the owner
 * 
 * @param client    	Client UserId
 * @param mapname     	Name of the nap nominated
 * @param IsVIP     	Is VIP Nomination
 * @param IsLeader     	Is Leader Nomination
 * @param IsAdmin     	Is Admin Nomination
 * @noreturn
 */
forward void NE_OnPublicMapReplaced(int client, char[] mapname, bool IsVIP, bool IsLeader, bool IsAdmin);

/**
 * Called when a map get inserted by an admin
 * 
 * @param client    	Admin UserId
 * @param mapname     	Name of the nap inserted
 * @noreturn
 */
forward void NE_OnAdminMapInsert(int client, char[] mapname);

/**
 * Called when a map get removed
 * 
 * @param client    	Client UserId
 * @param mapname     	Name of the map inserted
 * @noreturn
 */
forward void NE_OnMapNominationRemove(int client, char[] mapname);

public SharedPlugin __pl_nominations_extended =
{
	name = "nominations",
	file = "nominations_extended.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_nominations_extended_SetNTVOptional()
{
	MarkNativeAsOptional("GetNominationPool");
	MarkNativeAsOptional("PushMapIntoNominationPool");
	MarkNativeAsOptional("PushMapsIntoNominationPool");
	MarkNativeAsOptional("RemoveMapFromNominationPool");
	MarkNativeAsOptional("RemoveMapsFromNominationPool");
	MarkNativeAsOptional("ToggleNominations");
}
#endif
