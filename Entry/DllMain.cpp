/**
 * UniversalSlashingSimulator - DLL Entry Point
 *
 * Main entry point for the DLL. Initializes all core systems
 *
 * Injection flow:
 * 1. DLL_PROCESS_ATTACH
 * 2. Create initialization thread
 * 3. Initialize logging
 * 4. Parse command-line arguments
 * 5. Initialize engine core
 * 6. Initialize STW systems (GameMode, Missions, Inventory, Building)
 *
 * Command-Line Arguments (USS-specific):
 * -USS_Mission=<MissionBlueprint>    Mission blueprint to load
 * -USS_Map=<MapName>                 Map to load
 * -USS_Difficulty=<1-140>            Difficulty level
 * -USS_MaxPlayers=<1-4>              Maximum players
 * -USS_Zone=<ZoneName>               Zone name
 * -USS_NoMissions                    Disable missions
 * -USS_NoInventory                   Disable inventory
 * -USS_NoBuilding                    Disable building
 * -USS_Debug                         Enable debug mode
 */

#include "../Core/Common.h"
#include "../Core/Logging/Log.h"
#include "../Engine/EngineCore.h"
#include "../STW/GameMode/STWGameMode.h"
#include "../STW/Missions/MissionManager.h"
#include "../STW/Inventory/InventoryManager.h"
#include "../STW/Building/BuildingManager.h"
#include <string>
#include <sstream>

namespace USS
{
    // Forward declarations
    DWORD WINAPI InitializationThread(LPVOID lpParam);
    void ParseCommandLineArguments(FSTWGameConfig& Config);
    bool GetCommandLineArg(const char* ArgName, std::string& OutValue);
    bool HasCommandLineArg(const char* ArgName);
    int GetCommandLineArgInt(const char* ArgName, int DefaultValue);

    // Global module handle
    static HMODULE g_hModule = nullptr;

    // Initialization state
    static bool g_bInitialized = false;
    static bool g_bDebugMode = false;

    /**
     * Parse command-line arguments and populate game config
     */
    void ParseCommandLineArguments(FSTWGameConfig& Config)
    {
        // Get command line
        const char* CmdLine = GetCommandLineA();
        if (!CmdLine)
            return;

        USS_LOG("Parsing command line: %s", CmdLine);

        // Mission blueprint
        std::string MissionArg;
        if (GetCommandLineArg("-USS_Mission", MissionArg))
        {
            Config.MissionBlueprint = MissionArg;
            USS_LOG("  Mission: %s", MissionArg.c_str());
        }

        // Map name
        std::string MapArg;
        if (GetCommandLineArg("-USS_Map", MapArg))
        {
            Config.MapName = MapArg;
            USS_LOG("  Map: %s", MapArg.c_str());
        }

        // Zone name
        std::string ZoneArg;
        if (GetCommandLineArg("-USS_Zone", ZoneArg))
        {
            Config.ZoneName = ZoneArg;
            USS_LOG("  Zone: %s", ZoneArg.c_str());
        }

        // Difficulty level
        int Difficulty = GetCommandLineArgInt("-USS_Difficulty", Config.DifficultyLevel);
        if (Difficulty >= 1 && Difficulty <= 140)
        {
            Config.DifficultyLevel = Difficulty;
            Config.DefaultDifficulty = Difficulty;
            USS_LOG("  Difficulty: %d", Difficulty);
        }

        // Max players
        int MaxPlayers = GetCommandLineArgInt("-USS_MaxPlayers", Config.MaxPlayers);
        if (MaxPlayers >= 1 && MaxPlayers <= 4)
        {
            Config.MaxPlayers = MaxPlayers;
            USS_LOG("  MaxPlayers: %d", MaxPlayers);
        }

        // Feature flags
        if (HasCommandLineArg("-USS_NoMissions"))
        {
            Config.bEnableMissions = false;
            USS_LOG("  Missions: DISABLED");
        }

        if (HasCommandLineArg("-USS_NoInventory"))
        {
            Config.bEnableInventory = false;
            USS_LOG("  Inventory: DISABLED");
        }

        if (HasCommandLineArg("-USS_NoBuilding"))
        {
            Config.bEnableBuilding = false;
            USS_LOG("  Building: DISABLED");
        }

        if (HasCommandLineArg("-USS_Debug"))
        {
            g_bDebugMode = true;
            USS_LOG("  Debug Mode: ENABLED");
        }
    }

    /**
     * Get a command-line argument value
     * Format: -ArgName=Value or -ArgName="Value With Spaces"
     */
    bool GetCommandLineArg(const char* ArgName, std::string& OutValue)
    {
        const char* CmdLine = GetCommandLineA();
        if (!CmdLine || !ArgName)
            return false;

        // Find argument
        const char* ArgPos = strstr(CmdLine, ArgName);
        if (!ArgPos)
            return false;

        // Skip to value (after '=')
        const char* ValueStart = ArgPos + strlen(ArgName);
        if (*ValueStart != '=')
            return false;

        ValueStart++;  // Skip '='

        // Handle quoted values
        if (*ValueStart == '"')
        {
            ValueStart++;
            const char* ValueEnd = strchr(ValueStart, '"');
            if (ValueEnd)
            {
                OutValue = std::string(ValueStart, ValueEnd - ValueStart);
                return true;
            }
        }
        else
        {
            // Unquoted - read until space or end
            const char* ValueEnd = ValueStart;
            while (*ValueEnd && *ValueEnd != ' ' && *ValueEnd != '\t')
                ValueEnd++;

            OutValue = std::string(ValueStart, ValueEnd - ValueStart);
            return true;
        }

        return false;
    }

    /**
     * Check if a command-line flag is present
     */
    bool HasCommandLineArg(const char* ArgName)
    {
        const char* CmdLine = GetCommandLineA();
        if (!CmdLine || !ArgName)
            return false;

        return strstr(CmdLine, ArgName) != nullptr;
    }

    /**
     * Get an integer command-line argument
     */
    int GetCommandLineArgInt(const char* ArgName, int DefaultValue)
    {
        std::string Value;
        if (!GetCommandLineArg(ArgName, Value))
            return DefaultValue;

        try
        {
            return std::stoi(Value);
        }
        catch (...)
        {
            return DefaultValue;
        }
    }

    DWORD WINAPI InitializationThread(LPVOID lpParam)
    {
        EResult Result = Log::Initialize(true, "USS_Log.txt");
        if (Result != EResult::Success)
        {
            MessageBoxA(nullptr, "Failed to initialize logging", "USS Error", MB_ICONERROR);
            return 1;
        }

        USS_LOG("========================================");
        USS_LOG("  UniversalSlashingSimulator v0.1.0");
        USS_LOG("  STW Gameserver Framework");
        USS_LOG("========================================");
        USS_LOG("");

        // Wait for game to be ready
        // TODO: find trigger in log, perhaps we need a way to trick the game into going to the STW lobby in headless?
        USS_LOG("Waiting for game initialization...");
        Sleep(5000);  // Placeholder

        USS_LOG("Initializing engine core...");
        Result = GetEngineCore().Initialize();

        if (Result != EResult::Success)
        {
            USS_FATAL("Engine core initialization failed: %s", ResultToString(Result));
            MessageBoxA(nullptr, "Failed to initialize engine core", "USS Error", MB_ICONERROR);
            return 1;
        }

        // Log version info
        const auto& Version = GetEngineCore().GetVersionInfo();
        USS_LOG("");
        USS_LOG("=== Version Information ===");
        USS_LOG("Engine:   UE %s", Version.GetEngineVersionString().c_str());
        USS_LOG("Fortnite: %.2f", Version.FortniteVersion);
        USS_LOG("CL:       %u", Version.FortniteCL);
        USS_LOG("Generation: %s", Version.GetGenerationName());
        USS_LOG("");
        USS_LOG("=== Feature Flags ===");
        USS_LOG("FNamePool:        %s", Version.bUseFNamePool ? "Yes" : "No");
        USS_LOG("FField:           %s", Version.bUseFField ? "Yes" : "No");
        USS_LOG("ChunkedObjects:   %s", Version.bUseChunkedObjects ? "Yes" : "No");
        USS_LOG("NewFastArray:     %s", Version.bUseNewFastArraySerializer ? "Yes" : "No");
        USS_LOG("TObjectPtr:       %s", Version.bUseTObjectPtr ? "Yes" : "No");
        USS_LOG("");

        // Initialize STW systems
        USS_LOG("Initializing STW systems...");

        // GameMode configuration with defaults
        FSTWGameConfig GameConfig;
        GameConfig.bEnableMissions = true;
        GameConfig.bEnableInventory = true;
        GameConfig.bEnableBuilding = true;
        GameConfig.MaxPlayers = 4;
        GameConfig.DefaultDifficulty = 1;
        GameConfig.DifficultyLevel = 1;
        GameConfig.MissionType = EMissionType::FarmsteadDefense;
        GameConfig.ZoneName = "Zone_Onboarding_FarmsteadFort";
        GameConfig.MissionBlueprint = "Mission_FarmsteadFort_C";

        // Parse command-line arguments to override defaults
        USS_LOG("");
        USS_LOG("=== Command Line Configuration ===");
        ParseCommandLineArguments(GameConfig);
        USS_LOG("");

        // Initialize GameMode (this initializes all subsystems)
        Result = GetSTWGameMode().Initialize(GameConfig);
        if (Result != EResult::Success)
        {
            USS_ERROR("STW GameMode initialization failed: %s", ResultToString(Result));
            // Continue anyway - partial functionality may still work
        }
        else
        {
            USS_LOG("STW systems initialized successfully");
        }

        USS_LOG("");
        USS_LOG("UniversalSlashingSimulator initialized successfully");
        USS_LOG("========================================");

        g_bInitialized = true;
        return 0;
    }

    void Shutdown()
    {
        if (!g_bInitialized)
            return;

        USS_LOG("Shutting down UniversalSlashingSimulator...");

        GetSTWGameMode().Shutdown();

        GetEngineCore().Shutdown();

        Log::Shutdown();

        g_bInitialized = false;
    }

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        USS::g_hModule = hModule;

        CreateThread(nullptr, 0, USS::InitializationThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        USS::Shutdown();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
