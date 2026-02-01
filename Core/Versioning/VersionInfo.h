/**
 * UniversalSlashingSimulator - Version Information
 *
 * Contains version structures and feature flags used throughout
 * the project. The FVersionInfo struct represents detected
 * engine and Fortnite version information.
 */

#pragma once

#include "../Common.h"

namespace USS
{
    // Engine version ranges for feature detection
    enum class EEngineGeneration : uint8
    {
        Unknown = 0,
        UE4_16_19,      // 4.16 - 4.19 (GNames, UProperty, fixed objects)
        UE4_20_22,      // 4.20 - 4.22 (chunked objects)
        UE4_23_24,      // 4.23 - 4.24 (FNamePool)
        UE4_25,         // 4.25 (FField/FProperty)
        UE4_26_27,      // 4.26 - 4.27 (TObjectPtr prep)
        UE5_0,          // 5.0+ (TObjectPtr)
        UE5_1_Plus      // 5.1+ (further changes)
    };

    // Version information structure
    struct FVersionInfo
    {
        // Engine version
        uint32 EngineVersionMajor;      // 4 or 5
        uint32 EngineVersionMinor;      // 16, 19, 23, 25, 27, etc.
        uint32 EngineVersionPatch;      // Patch number

        // Fortnite version
        double FortniteVersion;          // e.g., 1.8, 8.30, 16.00
        uint32 FortniteSeasonMajor;      // Season number
        uint32 FortniteSeasonMinor;      // Minor patch
        uint32 FortniteCL;               // Changelist number

        // Computed generation
        EEngineGeneration Generation;

        // Feature flags (computed from version)
        bool bUseFNamePool;             // >= 4.23
        bool bUseFField;                // >= 4.25
        bool bUseChunkedObjects;        // >= 4.21
        bool bUseNewFastArraySerializer;// FN >= 8.30
        bool bUseTObjectPtr;            // >= 5.0
        bool bSupportsSTW;              // Has STW support

        // Initialize with defaults
        FVersionInfo()
            : EngineVersionMajor(0)
            , EngineVersionMinor(0)
            , EngineVersionPatch(0)
            , FortniteVersion(0.0)
            , FortniteSeasonMajor(0)
            , FortniteSeasonMinor(0)
            , FortniteCL(0)
            , Generation(EEngineGeneration::Unknown)
            , bUseFNamePool(false)
            , bUseFField(false)
            , bUseChunkedObjects(false)
            , bUseNewFastArraySerializer(false)
            , bUseTObjectPtr(false)
            , bSupportsSTW(true)
        {}

        // Check if version is valid
        bool IsValid() const
        {
            return EngineVersionMajor > 0 && Generation != EEngineGeneration::Unknown;
        }

        // Get engine version as string (e.g., "4.23.1")
        std::string GetEngineVersionString() const
        {
            char Buffer[32];
            snprintf(Buffer, sizeof(Buffer), "%u.%u.%u",
                EngineVersionMajor, EngineVersionMinor, EngineVersionPatch);
            return Buffer;
        }

        // Get Fortnite version as string (e.g., "8.30")
        std::string GetFortniteVersionString() const
        {
            char Buffer[32];
            snprintf(Buffer, sizeof(Buffer), "%.2f", FortniteVersion);
            return Buffer;
        }

        // Get generation name
        const char* GetGenerationName() const
        {
            switch (Generation)
            {
            case EEngineGeneration::UE4_16_19:   return "UE4.16-4.19";
            case EEngineGeneration::UE4_20_22:   return "UE4.20-4.22";
            case EEngineGeneration::UE4_23_24:   return "UE4.23-4.24";
            case EEngineGeneration::UE4_25:      return "UE4.25";
            case EEngineGeneration::UE4_26_27:   return "UE4.26-4.27";
            case EEngineGeneration::UE5_0:       return "UE5.0";
            case EEngineGeneration::UE5_1_Plus:  return "UE5.1+";
            default:                             return "Unknown";
            }
        }
    };

}