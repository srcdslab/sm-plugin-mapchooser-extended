# MapChooser Extended

Advanced Automated Map Voting with Extensions

# Configuration
## mapchooser_extended.cfg
```
"mapchooser_extended"
{
    "_groups"
    {
        "1" // Numbers start from 1 to infinity, but make sure ordering is correct
        {
            "_name" "Final Fantasy" // Group name (optional)
            "_max" "1" // Maximum 1 consecutive map from this group
            "_cooldown" "10" // Shared cooldown for all maps in this group (cvar: mce_sharedcd_mode)
            "_cooldown_time" "60" // Same as `"CooldownTime"` but this is for groups. Supported formats: "30m", "2h", "1h30m", "2h15m30s", "60" (defaults to minutes)
            "ze_ffvii_mako_reactor_v2_2" {}
            "ze_ffvii_mako_reactor_v3_1" {}
            "ze_ffvii_mako_reactor_v5_3" {}
            "ze_ffvii_mako_reactor_v6_b08" {}
        }
        "2" // Wanderers
        {
            "_name" "Wanderers marathon"
            "_max" "2"
            "_cooldown" "44"
            "_cooldown_time" "12h"
            "ze_ffxiv_wanderers_palace_css" {}
            "ze_ffxiv_wanderers_palace_v4_5s" {}
            "ze_ffxiv_wanderers_palace_v5_2f" {}
            "ze_ffxiv_wanderers_palace_v6css" {}
        }
    }
    "_tiers"
    {
        "1"
        {
            "_name" "Easy"
        }
        "2"
        {
            "_name" "Medium"
        }
        "3"
        {
            "_name" "Hard"
        }
    }
    "example_map"
    {
        "Tier"          "3" // Tier index from the "_tiers" section above
        "MinTime"       "1800" // Minimum server time to allow nomination (example: map can be nominated after 18:00 server time)
        "MaxTime"       "2300" // Maximum server time to allow nomination (example: map cannot be nominated after 23:00 server time)
        "MinPlayers"    "25" // Minimum players required to allow nomination (example: map can be nominated with 25+ players)
        "MaxPlayers"    "50" // Maximum players allowed to keep nomination available (example: map cannot be nominated with 50+ players)
        "CooldownTime"  "24h" // Map cooldown by time (example: after this map is played, wait 24h before it can be nominated again)
        "Cooldown"      "20" // Map cooldown by map count (example: after this map is played, play 20 maps before it can be nominated again)
        "VIP"           "1" // Map can only be nominated by VIPs
        "Admin"         "1" // Map can only be nominated by Admins
        "Leader"        "1" // Map can only be nominated by Leaders
        "Extends"       "3" // Number of extends available
        "ExtendTime"    "15" // Duration of an extend in minutes
        "ExtendRound"   "3" // Number of rounds to extend
        "ExtendFrag"    "100" // Number of frags to extend
        "TimeLimit"     "20" // Time in minutes to enforce map time (mp_timelimit)
    }
}
```

# Cvars and Commands
## MapChooser Extended:
### Cvars
- mce_version (default: plugin version) - MapChooser Extended Version.
- mce_force_lowercase (default: 1) - Force lowercase map names.
- mce_endvote (default: 1) - Specifies if MapChooser should run an end of map vote.
- mce_endmap_info (default: 1) - Print a nextmap message at map end.
- mce_hud_channel (default: 1) - Channel for HUD messages.
- mce_starttime (default: 10) - Specifies when to start the vote based on time remaining.
- mce_random_starttime (default: 30.0) - Max random delay added to vote start timer (seconds).
- mce_startround (default: 2) - Specifies when to start the vote based on rounds remaining.
- mce_startfrags (default: 5) - Specifies when to start the vote based on frags remaining.
- mce_extend_timestep (default: 15) - Extra minutes added per extension.
- mce_extend_roundstep (default: 5) - Extra rounds added per extension.
- mce_extend_fragstep (default: 10) - Extra frags added per extension.
- mce_exclude (default: 5) - Number of past maps to exclude from votes.
- mce_exclude_time (default: 5h) - Duration map is excluded from votes.
- mce_include (default: 5) - Number of maps included in the vote.
- mce_include_reserved (default: 2) - Number of private/random maps included in vote.
- mce_novote (default: 1) - Pick a map if no votes are received.
- mce_extend (default: 0) - Number of extensions allowed each map.
- mce_dontchange (default: 1) - Add a "Don't Change" option to early votes.
- mce_voteduration (default: 20) - Vote duration in seconds.
- mce_count_bots (default: 1) - Count bots for MinPlayers/MaxPlayers checks.
- mce_runoff (default: 1) - Hold runoff vote if winner is below threshold.
- mce_runoffpercent (default: 50) - Minimum winning percent before runoff.
- mce_blockslots (default: 0) - Block slots to prevent accidental votes.
- mce_maxrunoffs (default: 1) - Number of runoff votes allowed each map.
- mce_start_percent (default: 35) - Vote start threshold based on percent.
- mce_start_percent_enable (default: 0) - Enable percentage-based vote start.
- mce_warningtime (default: 15.0) - Warning time in seconds.
- mce_runoffvotewarningtime (default: 5.0) - Runoff warning time in seconds.
- mce_menustyle (default: 0) - Vote menu style.
- mce_warningtimerlocation (default: 0) - Warning timer location (0 HintBox, 1 Center, 2 Chat).
- mce_markcustommaps (default: 1) - Mark custom maps in vote list.
- mce_extendposition (default: 0) - Position of Extend/Don't Change options.
- mce_randomizeorder (default: 0) - Randomize vote map order.
- mce_hidetimer (default: 0) - Hide warning timer.
- mce_addnovote (default: 1) - Add "No Vote" option to vote menu.
- mce_shuffle_per_client (default: 1) - Shuffle vote menu per client.
- mce_no_restriction_timeframe_enable (default: 1) - Disable nomination restrictions during timeframe.
- mce_no_restriction_timeframe_mintime (default: 0100) - Start of unrestricted timeframe (HHMM).
- mce_no_restriction_timeframe_maxtime (default: 0700) - End of unrestricted timeframe (HHMM).
- mce_locknominationswarning (default: 1) - Lock nominations when vote warning starts.
- mce_locknominations_timer (default: 15.0) - Unlock nominations delay after vote (seconds).
- mce_shownominator (default: 1) - Show who nominated winning map.
- mce_sharedcd_mode (default: 1) - Shared cooldown mode for grouped maps.
- mce_cooldown_mode (default: 0) - Cooldown evaluation mode.
- mce_showmaptier_in_chat (default: 1) - Show map tier name in chat.
### Commands
#### Public
- sm_extends, sm_extendsleft - Shows how many extends are left on the current map.
- sm_showmapcfg, sm_showmapconfig - Shows all config information about the map.
- sm_mcversion, sm_mceversion - Print current MapChooser version.
#### Admin
- mce_reload_maplist - Reload the official map list file.
- sm_mapvote - Forces MapChooser to attempt to run a map vote now.
- sm_setnextmap - Set the next map.
## Nominations Extended:
### Cvars
- ne_version (default: plugin version) - Nominations Extended Version.
- sm_nominate_excludeold (default: 1) - Exclude current map from nominations.
- sm_nominate_excludecurrent (default: 1) - Exclude maps currently excluded by MapChooser.
- sm_nominate_initialdelay (default: 60.0) - Delay before first nomination is allowed (seconds).
- sm_nominate_delay (default: 3.0) - Delay between nominations (seconds).
- sm_nominate_vip_timeframe (default: 1) - Enable VIP-only nomination timeframe.
- sm_nominate_vip_timeframe_mintime (default: 1800) - VIP timeframe start (HHMM).
- sm_nominate_vip_timeframe_maxtime (default: 2200) - VIP timeframe end (HHMM).
- sm_nominate_max_ban_time (default: 10080) - Maximum nomban duration (minutes, non-rcon+).
### Commands
#### Public
- sm_nom, sm_nominate - Nominate a map.
- sm_noms, sm_nomlist - List nominated maps.
- sm_unnominate, sm_unnom - Remove your nomination.
- sm_nomstatus - Show a player's current nomination-ban status.
#### Admin
- sm_nominate_force_lock - Force lock nominations.
- sm_nominate_force_unlock - Force unlock nominations.
- sm_nominate_addmap - Forces a map to be on the next mapvote.
- sm_nominate_removemap - Removes a map from Nominations.
- sm_nominate_exclude - Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.
- sm_nominate_exclude_time - Forces a map to be inserted into the recently played maps. Effectively blocking the map from being nominated.
- sm_nomban - Ban a client from nominating.
- sm_nombanlist - View a list of nomination-banned clients.
- sm_unnomban, sm_nomunban, sm_unomban - Unban a client from nominating.
## Rock The Vote Extended:
### Cvars
- sm_rtv_needed (default: 0.65) - Percentage of Steam players added to RTV calculation.
- sm_rtv_needed_nosteam (default: 0.45) - Percentage of No-Steam players added to RTV calculation.
- sm_rtv_minplayers (default: 0) - Number of players required before RTV is enabled.
- sm_rtv_initialdelay (default: 30.0) - Time in seconds before first RTV can be held.
- sm_rtv_interval (default: 240.0) - Time in seconds after a failed RTV before another can be held.
- sm_rtv_changetime (default: 0) - Map change timing after successful RTV (0 Instant, 1 RoundEnd, 2 MapEnd).
- sm_rtv_postvoteaction (default: 0) - RTV behavior after a map vote has completed.
- sm_rtv_autodisable (default: 0) - Automatically disable RTV when map time is over.
- sm_rtv_afk_time (default: 180) - AFK time in seconds after which a player is not counted in RTV ratio.
### Commands
#### Public
- sm_rtv - Vote to change the map.
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
- maxime1907
- .Rushaway
- tilgep
- notkoen
- JMorell
- tokKurumi
- lameskydiver
- TR1D
- Vauff
- Snowy
