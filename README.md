# Mapchooser Extended

Advanced Automated Map Voting with Extensions

# Configuration
## mapchooser_extended.cfg
```
"mapchooser_extended"
{
    "_groups"
    {
        "1" // Numbers start from 1 to infinite but make sure its ordered correctly
        {
            "_max" "1" // Maximum 1 consecutive map from this group
            "ze_ffvii_mako_reactor_v2_2" {}
            "ze_ffvii_mako_reactor_v3_1" {}
            "ze_ffvii_mako_reactor_v5_3" {}
            "ze_ffvii_mako_reactor_v6_b08" {}
        }
        "2" // wanderers
        {
            "_max" "2" // Maximum 2 consecutive maps from this group
            "ze_ffxiv_wanderers_palace_css" {}
            "ze_ffxiv_wanderers_palace_v4_5s" {}
            "ze_ffxiv_wanderers_palace_v5_2f" {}
            "ze_ffxiv_wanderers_palace_v6css" {}
        }
    }
    "example_map"
    {
        "MinTime"       "1800" // Min server time for make map available to nominate (Example: map will be able to nominate after 18:00 by server time)
        "MaxTime"       "2300" // Max server time for make map unavailable to nominate (Example: map will not be able to nominate after 23:00 by server time)
        "MinPlayers"    "25" // How many players min required for make map available to nominate (Example: if 25+ players on server the map will be able to nominate)
        "MaxPlayers"    "50" // How many players max required for make map unavailable to nominate (Example: if 50+ players on server the map will not be able to nominate)
        "CooldownTime"  "24h" // Map CooldownTime (Example: after this map played players should wait another 24 hours to nominate this map again)        
        "Cooldown"      "20" // Map cooldown (Example: after this map played players should play another 20 maps to nominate this map again)        
        "VIP"           "1" // Map can only be nominated by VIPs
    }
}
```

# Cvars and Commands
## MapChooser Extended:
### Cvars
- mce_version - MapChooser Extended Version.
- mce_endvote - Specifies if MapChooser should run an end of map vote.
- mce_starttime - Specifies when to start the vote based on time remaining.
- mce_startround - Specifies when to start the vote based on rounds remaining. Use 0 on DoD:S, CS:S, and TF2 to start vote during bonus round time.
- mce_startfrags - Specifies when to start the vote base on frags remaining.
- mce_extend_timestep - Specifies how much many more minutes each extension makes.
- mce_extend_roundstep - Specifies how many more rounds each extension makes.
- mce_extend_fragstep - Specifies how many more frags are allowed when map is extended.
- mce_exclude - Specifies how many past maps to exclude from the vote.
- mce_exclude_time - Specifies how long in minutes an old map is excluded from the vote.
- mce_include - Specifies how many maps to include in the vote.
- mce_include_reserved - Specifies how many private/random maps to include in the vote.
- mce_novote - Specifies whether or not MapChooser should pick a map if no votes are received.
- mce_extend - Number of extensions allowed each map.
- mce_dontchange - Specifies if a 'Don't Change' option should be added to early votes.
- mce_voteduration - Specifies how long the mapvote should be available for.
- mce_runoff - Hold run off votes if winning choice has less than a certain percentage of votes.
- mce_runoffpercent - If winning choice has less than this percent of votes, hold a runoff.
- mce_blockslots - Block slots to prevent accidental votes. Only applies when Voice Command style menus are in use.
- mce_blockslots_count - Number of slots to block.
- mce_maxrunoffs - Number of run off votes allowed each map.
- mce_start_percent - Specifies when to start the vote based on percents.
- mce_start_percent_enable - Enable or Disable percentage calculations when to start vote.
- mce_warningtime - Warning time in seconds.
- mce_runoffvotewarningtime - Warning time for runoff vote in seconds.
- mce_warningtimerlocation - Location for the warning timer text. 0 is HintBox, 1 is Center text, 2 is Chat. Defaults to HintBox.
- mce_markcustommaps - Mark custom maps in the vote list. 0 = Disabled, 1 = Mark with *, 2 = Mark with phrase.
- mce_extendposition - Position of Extend/Don't Change options. 0 = at end, 1 = at start.
- mce_randomizeorder - Randomize map order?
- mce_hidetimer - Hide the MapChooser Extended warning timer.
- mce_addnovote - Add "No Vote" to vote menu?
- mce_shuffle_per_client - Random shuffle map vote menu per client?
- mce_no_restriction_timeframe_enable - Enable timeframe where all nomination restrictions and cooldowns are disabled?
- mce_no_restriction_timeframe_mintime - Start of the timeframe where all nomination restrictions and cooldowns are disabled (Format: HHMM).
- mce_no_restriction_timeframe_maxtime - End of the timeframe where all nomination restrictions and cooldowns are disabled (Format: HHMM).
### Commands
#### Public
- sm_extends - Shows how many extends are left on the current map.
- sm_extendsleft - Shows how many extends are left on the current map.
#### Admin
- mce_reload_maplist - Reload the Official Maplist file.
- sm_mapvote - Forces MapChooser to attempt to run a map vote now.
- sm_setnextmap - Sets the nextmap
## Nominations Extended:
### Cvars
- ne_version - Nominations Extended Version
- sm_nominate_excludeold - Specifies if the current map should be excluded from the Nominations list.
- sm_nominate_excludecurrent - Specifies if the MapChooser excluded maps should also be excluded from Nominations.
- sm_nominate_initialdelay - Time in seconds before first Nomination can be made.
- sm_nominate_delay - Delay between nominations.
- sm_nominate_vip_timeframe - Specifies if the should be a timeframe where only VIPs can nominate maps
- sm_nominate_vip_timeframe_mintime - Start of the timeframe where only VIPs can nominate maps (Format: HHMM)
- sm_nominate_vip_timeframe_maxtime - End of the timeframe where only VIPs can nominate maps (Format: HHMM)
### Commands
#### Public
- sm_nominate - Nominate a map
- sm_nomlist - List of nominated maps
#### Admin
- sm_nominate_addmap - Forces a map to be on the next mapvote.
- sm_nominate_removemap - Removes a map from Nominations.
- sm_nominate_exclude - Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.
- sm_nominate_exclude_time - Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.
## Rockthevote Extended:
### Cvars
- sm_rtv_steam_needed - Percentage of players needed to rockthevote (Def 60%).
- sm_rtv_nosteam_needed - Percentage of No-Steam players added to rockthevote calculation (Def 45%).
- sm_rtv_minplayers - Number of players required before RTV will be enabled.
- sm_rtv_initialdelay - Time (in seconds) before first RTV can be held.
- sm_rtv_interval - Time (in seconds) after a failed RTV before another can be held.
- sm_rtv_changetime - When to change the map after a succesful RTV: 0 - Instant, 1 - RoundEnd, 2 - MapEnd.
- sm_rtv_postvoteaction - What to do with RTV's after a mapvote has completed. 0 - Allow, success = instant change, 1 - Deny.
- sm_rtv_autodisable - Automatically disable RTV when map time is over.
- sm_rtv_afk_time - AFK Time in seconds after which a player is not counted in the rtv ratio.
### Commands
#### Public
- sm_rtv - Vote to change the map
#### Admin
- sm_forcertv - Force an RTV vote.
- sm_disablertv - Disable the RTV command.
- sm_enablertv - Enable the RTV command.
- sm_debugrtv - Check the current RTV calculation.

## Credits
- Powerlord
- Zuko
- Alliedmodders LLC
- Botox
- zaCade
- neon
