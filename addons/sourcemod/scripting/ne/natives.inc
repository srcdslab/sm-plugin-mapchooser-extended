/*
* ============================================================================
*
*  Nominations Extended
*
*  File:          ne/natives.inc
*  Description:   Natives handlers for the NE API
* 
* ============================================================================
*/

stock void API_NativesInit()
{
	CreateNative("GetNominationPool", Native_GetNominationPool);
	CreateNative("PushMapIntoNominationPool", Native_PushMapIntoNominationPool);
	CreateNative("PushMapsIntoNominationPool", Native_PushMapsIntoNominationPool);
	CreateNative("RemoveMapFromNominationPool", Native_RemoveMapFromNominationPool);
	CreateNative("RemoveMapsFromNominationPool", Native_RemoveMapsFromNominationPool);
	CreateNative("ToggleNominations", Native_ToggleNominations);
}

public int Native_GetNominationPool(Handle plugin, int numArgs)
{
	SetNativeCellRef(1, g_MapList);

	return 0;
}

public int Native_PushMapIntoNominationPool(Handle plugin, int numArgs)
{
	char map[PLATFORM_MAX_PATH];

	GetNativeString(1, map, PLATFORM_MAX_PATH);

	g_MapList.ShiftUp(0);
	g_MapList.SetString(0, map);

	UpdateMapTrie();

	return 0;
}

public int Native_PushMapsIntoNominationPool(Handle plugin, int numArgs)
{
	ArrayList maps = GetNativeCell(1);

	for (int i = 0; i < maps.Length; i++)
	{
		char map[PLATFORM_MAX_PATH];
		maps.GetString(i, map, PLATFORM_MAX_PATH);

		if (g_MapList.FindString(map) == -1)
		{
			g_MapList.ShiftUp(0);
			g_MapList.SetString(0, map);
		}
	}

	delete maps;

	UpdateMapTrie();

	return 0;
}

public int Native_RemoveMapFromNominationPool(Handle plugin, int numArgs)
{
	char map[PLATFORM_MAX_PATH];

	GetNativeString(1, map, PLATFORM_MAX_PATH);

	int idx;

	if ((idx = g_MapList.FindString(map)) != -1)
		g_MapList.Erase(idx);

	UpdateMapTrie();

	return 0;
}

public int Native_RemoveMapsFromNominationPool(Handle plugin, int numArgs)
{
	ArrayList maps = GetNativeCell(1);

	for (int i = 0; i < maps.Length; i++)
	{
		char map[PLATFORM_MAX_PATH];
		maps.GetString(i, map, PLATFORM_MAX_PATH);

		int idx = -1;

		if ((idx = g_MapList.FindString(map)) != -1)
			g_MapList.Erase(idx);
	}

	delete maps;

	UpdateMapTrie();

	return 0;
}

public int Native_ToggleNominations(Handle plugin, int numArgs)
{
	bool toggle = GetNativeCell(1);

	if (toggle)
		g_bNEAllowed = false;
	else
		g_bNEAllowed = true;

	return 1;
}
