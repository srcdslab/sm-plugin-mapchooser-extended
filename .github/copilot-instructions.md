# MapChooser Extended - Copilot Development Guide

## Project Overview
MapChooser Extended is a comprehensive SourceMod plugin suite that provides advanced automated map voting functionality for Source engine game servers. The project consists of four interconnected plugins that work together to manage map selection, nominations, and voting processes.

### Core Components
- **mapchooser_extended.sp** - Main voting system with advanced configuration options
- **nominations_extended.sp** - Player map nomination system with restrictions and permissions
- **rockthevote_extended.sp** - Rock the Vote (RTV) functionality for democratic map changes
- **mapchooser_extended_sounds.sp** - Audio feedback system for voting events

## Architecture and Code Organization

### Directory Structure
```
addons/sourcemod/
├── scripting/
│   ├── mapchooser_extended.sp          # Main plugin
│   ├── nominations_extended.sp         # Nominations system
│   ├── rockthevote_extended.sp        # RTV system
│   ├── mapchooser_extended_sounds.sp  # Sound effects
│   ├── include/
│   │   ├── mapchooser_extended.inc     # API definitions
│   │   └── nominations_extended.inc    # Nominations API
│   ├── mce/                           # MapChooser Extended modules
│   │   ├── globals_variables.inc      # Global variables and enums
│   │   ├── cvars.inc                 # ConVar definitions
│   │   ├── commands.inc              # Console commands
│   │   ├── events.inc                # Game event handlers
│   │   ├── functions.inc             # Public functions
│   │   ├── internal_functions.inc    # Internal helper functions
│   │   ├── menus.inc                 # Vote menu system
│   │   ├── natives.inc               # Native function implementations
│   │   └── forwards.inc              # Forward declarations
│   └── ne/                           # Nominations Extended modules
│       ├── bans.inc                  # Nomination ban system
│       ├── commands.inc              # Nomination commands
│       ├── cookies.inc               # Client preferences
│       ├── cvars.inc                 # ConVar definitions
│       ├── forwards.inc              # Forward declarations
│       ├── functions.inc             # Core nomination functions
│       ├── menus.inc                 # Nomination menus
│       └── natives.inc               # Native implementations
├── translations/                      # Multi-language support
│   ├── mapchooser_extended.phrases.txt
│   └── [language folders]            # (chi, es, fr, ru)
├── configs/
│   ├── mapchooser_extended.cfg        # Main configuration
│   └── mapchooser_extended/           # Map-specific configs
└── ../../../sound/mapchooser/         # Audio files for vote events
    ├── hl1/                          # Half-Life 1 style sounds
    ├── tf2/                          # Team Fortress 2 sounds
    └── tf2_merasmus/                 # TF2 Halloween event sounds
```

### Modular Design Philosophy
The codebase uses a highly modular approach where each `.inc` file contains related functionality:
- **Separation of concerns**: Each module handles specific aspects (CVars, commands, menus, etc.)
- **Shared state**: Global variables are centralized in `globals_variables.inc`
- **API exposure**: Public functions are exposed through include files for other plugins

## Technical Environment

### Language and Platform
- **Language**: SourcePawn (SourceMod scripting language)
- **Platform**: SourceMod 1.12+ (minimum requirement)
- **Compiler**: SourcePawn Compiler (spcomp) via SourceKnight build system
- **Target**: Source Engine game servers (CS:GO, CSS, TF2, etc.)

### Build System
The project uses **SourceKnight** for dependency management and compilation:
- **Configuration**: `sourceknight.yaml` defines dependencies and build targets
- **Dependencies**: Automatically fetches required SourceMod plugins and includes
- **Build Command**: `sourceknight build` (handled by CI/CD)
- **Output**: Compiled `.smx` files in standard SourceMod plugin directory

### Key Dependencies
- **SourceMod**: Core platform (1.12+)
- **MultiColors**: Enhanced chat color support
- **AFKManager**: AFK player detection integration
- **SourceComms**: Player communication management
- **PlayerManager**: Player status and permission management
- **ZLeader**: Leadership/VIP system integration
- **DynamicChannels**: Audio channel management
- **UtilsHelper**: Utility functions library

### Optional Dependencies Pattern
The codebase uses a robust optional dependency system:
```sourcepawn
#undef REQUIRE_PLUGIN
#tryinclude <optional_plugin>
#define REQUIRE_PLUGIN
```
This allows plugins to work without optional dependencies while providing enhanced functionality when available.

## Coding Standards and Best Practices

### SourcePawn Standards
```sourcepawn
#pragma semicolon 1        // Required - enforce semicolons
#pragma newdecls required  // Required - enforce new declaration syntax
```

### Naming Conventions
- **Global variables**: Prefix with `g_` (e.g., `g_MapList`, `g_VoteTimer`)
- **Functions**: PascalCase (e.g., `CreateMapVote`, `CheckMapRestrictions`)
- **Local variables**: camelCase (e.g., `mapName`, `clientId`)
- **Constants**: UPPER_CASE (e.g., `MAX_MAP_LENGTH`, `VOTE_DURATION`)
- **Enums**: PascalCase with descriptive names (e.g., `RoundCounting`, `GameType_Classic`)

### Memory Management
```sourcepawn
// Use delete for cleanup - never check for null first
delete someHandle;  // Automatically sets to null

// Use StringMap/ArrayList instead of arrays where appropriate
StringMap mapData = new StringMap();
ArrayList playerList = new ArrayList();

// Avoid .Clear() on containers - use delete and recreate
delete g_MapList;
g_MapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
```

### Database Operations
```sourcepawn
// ALL SQL queries MUST be asynchronous
Database.Query(SQLCallback, query, data);

// Use transactions for multiple related queries
Transaction txn = new Transaction();
txn.AddQuery(query1);
txn.AddQuery(query2);
db.Execute(txn, SQLTxnSuccess, SQLTxnFailure);

// Always escape user input
char escaped[256];
db.Escape(userInput, escaped, sizeof(escaped));
```

### Error Handling and Validation
```sourcepawn
// Check API call results
if (!IsValidClient(client)) {
    return;
}

// Handle invalid handles properly
if (timer != null) {
    KillTimer(timer);
    timer = null;
}

// Use translations for all user-facing messages
PrintToChat(client, "%t", "Translation_Key", arg1, arg2);
```

## Configuration System

### Map Configuration Structure
Maps are configured in `addons/sourcemod/configs/mapchooser_extended.cfg` using KeyValues format:
```
"mapchooser_extended"
{
    "_groups"           // Map groups with shared restrictions
    {
        "1"
        {
            "_name"           "Group Name"
            "_max"            "1"          // Max consecutive maps from group
            "_cooldown"       "10"         // Group cooldown (maps)
            "_cooldown_time"  "60"         // Group cooldown (time)
            "map1" {}
            "map2" {}
        }
    }
    
    "individual_map"
    {
        "MinTime"       "1800"    // Min server time (HHMM)
        "MaxTime"       "2300"    // Max server time (HHMM)
        "MinPlayers"    "25"      // Minimum players required
        "MaxPlayers"    "50"      // Maximum players allowed
        "CooldownTime"  "24h"     // Time-based cooldown
        "Cooldown"      "20"      // Map-based cooldown
        "VIP"           "1"       // VIP-only nomination
        "Admin"         "1"       // Admin-only nomination
        "Leader"        "1"       // Leader-only nomination
        "Extends"       "3"       // Number of extends allowed
        "ExtendTime"    "15"      // Minutes per extend
        "ExtendRound"   "3"       // Rounds per extend
        "ExtendFrag"    "100"     // Frags per extend
        "TimeLimit"     "20"      // Enforce mp_timelimit
    }
}
```

## Development Workflows

### Setting Up Development Environment
1. **Clone repository**: Standard git clone
2. **Install SourceKnight**: Required for building and dependency management
3. **Run build**: `sourceknight build` to download dependencies and compile
4. **Testing**: Deploy to development SourceMod server for testing

### Building and Testing
```bash
# Build all plugins (via CI or locally with SourceKnight)
sourceknight build

# Output locations:
# - Compiled plugins: .sourceknight/package/common/addons/sourcemod/plugins/
# - Include files: .sourceknight/package/common/addons/sourcemod/scripting/include/
# - Configs: .sourceknight/package/common/addons/sourcemod/configs/
# - Translations: .sourceknight/package/common/addons/sourcemod/translations/
```

### CI/CD Pipeline
The project uses GitHub Actions (`.github/workflows/ci.yml`) for:
- **Automated building** on push/PR
- **Dependency resolution** via SourceKnight
- **Package creation** with all necessary files
- **Release management** for tagged versions

## Key Files and Their Purposes

### Core Plugin Files
- **mapchooser_extended.sp**: Main voting logic, vote menus, map selection algorithms
- **nominations_extended.sp**: Player nomination system, restriction checking, permission handling
- **rockthevote_extended.sp**: RTV vote tracking, threshold management, vote triggering
- **mapchooser_extended_sounds.sp**: Audio feedback system for voting events

### Sound System
The sound system (`mapchooser_extended_sounds.sp`) provides audio feedback for voting events:
- **Game-specific sounds**: Different sound sets for HL1, TF2, and TF2 Halloween events
- **Event-based audio**: Sounds for vote start, vote end, map change, etc.
- **Client preferences**: Players can disable sounds via client preferences
- **Auto-download**: Sounds are automatically downloaded to clients when needed

### Version Management
The project uses a centralized version system:
- **MCE_VERSION**: Defined in include files and used across all plugins
- **Plugin info**: Consistent author and description information
- **Semantic versioning**: Follows MAJOR.MINOR.PATCH format
- **Git tags**: Version releases are tagged in the repository

### Module Files (mce/)
- **globals_variables.inc**: Shared state, enums, global handles
- **cvars.inc**: ConVar definitions and change handlers
- **commands.inc**: Console command implementations
- **events.inc**: Game event handlers (round start/end, player events)
- **functions.inc**: Core public functions, vote management
- **menus.inc**: Vote menu creation and handling
- **natives.inc**: API functions for other plugins

### API Headers (include/)
- **mapchooser_extended.inc**: Public API definitions, forwards, natives
- **nominations_extended.inc**: Nomination system API, callback definitions

## Common Development Tasks

### Adding New ConVars
1. Define in appropriate `cvars.inc` file
2. Add to `CreateConVars()` function
3. Implement change handler if needed
4. Update configuration documentation

### Adding New Commands
1. Add to appropriate `commands.inc` file
2. Register in `OnPluginStart()`
3. Implement permission checks
4. Add translation strings if needed

### Modifying Vote Logic
1. Locate relevant functions in `functions.inc` or `menus.inc`
2. Consider impact on other plugins (check forwards)
3. Test with various map configurations
4. Ensure translation compatibility

### Adding Map Restrictions
1. Modify map configuration parsing in `functions.inc`
2. Add restriction checking in nomination validation
3. Update vote eligibility checking
4. Document new configuration options

## Performance Considerations

### Optimization Guidelines
- **Minimize timer usage**: Cache expensive operations
- **Optimize frequent functions**: Pay attention to complexity in vote checking and map validation
- **Database efficiency**: Use async queries, batch operations where possible
- **Memory management**: Properly clean up handles and avoid leaks
- **String operations**: Minimize string manipulation in frequently called functions

### Critical Performance Areas
- **Map eligibility checking**: Called frequently during nominations
- **Vote menu generation**: Avoid O(n) operations where possible
- **Player restriction validation**: Cache permission states
- **Database queries**: Use prepared statements and async operations

## Troubleshooting Common Issues

### Build Issues
- **Missing dependencies**: Check `sourceknight.yaml` and ensure all repositories are accessible
- **Compilation errors**: Verify SourceMod version compatibility and include paths
- **Version conflicts**: Ensure dependency versions match requirements

### Runtime Issues
- **Database connectivity**: Check async query error handling
- **Permission errors**: Verify client validation and permission checking
- **Memory leaks**: Check handle cleanup and StringMap/ArrayList usage
- **Vote timing**: Verify timer management and cleanup

### Integration Issues
- **Plugin conflicts**: Check native function availability and forwards
- **Translation missing**: Ensure all phrases are defined in translation files
- **Configuration errors**: Validate KeyValues format and map name consistency

## Testing and Validation

### Test Environment Setup
1. **Development server**: Set up test SourceMod installation
2. **Plugin loading**: Test individual and combined plugin loading
3. **Map rotation**: Test with representative map list
4. **Player simulation**: Use bots or multiple clients for voting tests

### Critical Test Cases
- **Basic voting**: End-of-map votes with various player counts
- **Nominations**: Player nomination with restrictions and permissions
- **RTV functionality**: Vote thresholds and timing
- **Configuration changes**: Live config reloading and validation
- **Database operations**: Connection handling and query reliability
- **Multi-language**: Translation loading and display

### Debugging Tools
- **SourceMod logging**: Use `LogMessage()` for debugging
- **Client console**: Check for client-side errors
- **Server console**: Monitor plugin loading and errors
- **Database logs**: Check SQL query execution and errors

## Notes for AI Assistants

### Code Analysis Tips
- **Follow includes**: Understand the modular structure by following `#include` statements
- **Check forwards**: Look for forward declarations to understand plugin interactions
- **Validate handles**: Always check handle validity before use
- **Memory patterns**: Look for proper cleanup patterns with `delete`

### Modification Guidelines
- **Minimal changes**: Prefer small, targeted modifications
- **Test compatibility**: Consider impact on dependent plugins
- **Maintain patterns**: Follow existing code patterns and structures
- **Update documentation**: Keep README and configuration examples current

### Common Gotchas
- **Handle lifecycle**: SourceMod handles require explicit cleanup
- **Async operations**: Database queries and forwards can have timing dependencies
- **Client validation**: Always validate client indices and connection state
- **Translation timing**: Ensure translation files are loaded before use