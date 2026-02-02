/**
 * UniversalSlashingSimulator - Version Resolver Implementation
 *
 * Based on version-scanner-example.cpp patterns and comprehensive CL research.
 * Complete CL-to-version mapping for Fortnite 1.2 through 19.40.
 */

#include "VersionResolver.h"
#include "../Memory/Memory.h"
#include "../Memory/PatternScanner.h"
#include "../Logging/Log.h"
#include "../../Engine/CoreTypes/FString.h"
#include <cstring>

namespace USS
{
    // Complete CL to version mapping table
    const FVersionResolver::FCLMapping FVersionResolver::s_CLMappings[] = {
        // ========================================================================
        // Chapter 1 - Season 1 (UE 4.16)
        // ========================================================================
        { 3541083,  3681159,  4, 16, 1.20 },   // 1.2
        { 3681159,  3700114,  4, 16, 1.50 },   // 1.5
        { 3700114,  3709086,  4, 16, 1.72 },   // 1.7.2
        { 3709086,  3724489,  4, 16, 1.80 },   // 1.8
        { 3724489,  3757339,  4, 16, 1.82 },   // 1.8.2
        { 3757339,  3775276,  4, 16, 1.90 },   // 1.9
        { 3775276,  3790078,  4, 16, 1.91 },   // 1.9.1

        // ========================================================================
        // Chapter 1 - Season 2 (UE 4.19)
        // ========================================================================
        { 3790078,  3807424,  4, 19, 1.10 },   // 1.10
        { 3807424,  3821117,  4, 19, 1.11 },   // 1.11
        { 3821117,  3841827,  4, 19, 2.00 },   // 2.0
        { 3841827,  3847564,  4, 19, 2.10 },   // 2.1
        { 3847564,  3858292,  4, 19, 2.20 },   // 2.2
        { 3858292,  3870737,  4, 19, 2.30 },   // 2.3
        { 3870737,  3889387,  4, 19, 2.40 },   // 2.4
        { 3889387,  3901517,  4, 19, 2.41 },   // 2.4.1
        { 3901517,  3913157,  4, 19, 2.42 },   // 2.4.2
        { 3913157,  3922182,  4, 19, 2.50 },   // 2.5

        // ========================================================================
        // Chapter 1 - Season 3 (UE 4.20)
        // ========================================================================
        { 3922182,  3935073,  4, 20, 3.00 },   // 3.0
        { 3935073,  3942182,  4, 20, 3.10 },   // 3.1
        { 3942182,  3948073,  4, 20, 3.20 },   // 3.2
        { 3948073,  3968866,  4, 20, 3.30 },   // 3.3
        { 3968866,  3989614,  4, 20, 3.40 },   // 3.4
        { 3989614,  4008490,  4, 20, 3.50 },   // 3.5
        { 4008490,  4019403,  4, 20, 3.51 },   // 3.5.1
        { 4019403,  4039451,  4, 20, 3.52 },   // 3.5.2
        { 4039451,  4072250,  4, 20, 3.60 },   // 3.6

        // ========================================================================
        // Chapter 1 - Season 4 (UE 4.20)
        // ========================================================================
        { 4072250,  4117433,  4, 20, 4.00 },   // 4.0
        { 4117433,  4127312,  4, 20, 4.10 },   // 4.1
        { 4127312,  4166199,  4, 20, 4.20 },   // 4.2
        { 4166199,  4205896,  4, 20, 4.30 },   // 4.3
        { 4205896,  4240749,  4, 20, 4.40 },   // 4.4
        { 4240749,  4276938,  4, 20, 4.50 },   // 4.5

        // ========================================================================
        // Chapter 1 - Season 5 (UE 4.21)
        // ========================================================================
        { 4276938,  4336496,  4, 21, 5.00 },   // 5.0
        { 4336496,  4352937,  4, 21, 5.01 },   // 5.0.1
        { 4352937,  4378021,  4, 21, 5.10 },   // 5.1
        { 4378021,  4395664,  4, 21, 5.20 },   // 5.2
        { 4395664,  4417689,  4, 21, 5.21 },   // 5.2.1
        { 4417689,  4442095,  4, 21, 5.30 },   // 5.3
        { 4442095,  4461277,  4, 21, 5.40 },   // 5.4
        { 4461277,  4476098,  4, 21, 5.41 },   // 5.4.1

        // ========================================================================
        // Chapter 1 - Season 6 (UE 4.21)
        // ========================================================================
        { 4476098,  4526925,  4, 21, 6.00 },   // 6.0
        { 4526925,  4541578,  4, 21, 6.01 },   // 6.0.1
        { 4541578,  4573279,  4, 21, 6.10 },   // 6.1
        { 4573279,  4612618,  4, 21, 6.20 },   // 6.2
        { 4612618,  4629139,  4, 21, 6.21 },   // 6.2.1
        { 4629139,  4667333,  4, 21, 6.30 },   // 6.3
        { 4667333,  4683176,  4, 21, 6.31 },   // 6.3.1

        // ========================================================================
        // Chapter 1 - Season 7 (UE 4.22)
        // ========================================================================
        { 4683176,  4741202,  4, 22, 7.00 },   // 7.0
        { 4741202,  4775217,  4, 22, 7.10 },   // 7.1
        { 4775217,  4801627,  4, 22, 7.20 },   // 7.2
        { 4801627,  4834550,  4, 22, 7.30 },   // 7.3
        { 4834550,  4869070,  4, 22, 7.40 },   // 7.4

        // ========================================================================
        // Chapter 1 - Season 8 (UE 4.22)
        // NOTE: 8.30 introduces new FastArraySerializer
        // ========================================================================
        { 4869070,  4900175,  4, 22, 8.00 },   // 8.0
        { 4900175,  4937820,  4, 22, 8.10 },   // 8.1
        { 4937820,  4975227,  4, 22, 8.20 },   // 8.2
        { 4975227,  5027463,  4, 22, 8.30 },   // 8.3 (NEW FAS!)
        { 5027463,  5046157,  4, 22, 8.40 },   // 8.4
        { 5046157,  5076327,  4, 22, 8.50 },   // 8.5
        { 5076327,  5110300,  4, 22, 8.51 },   // 8.5.1

        // ========================================================================
        // Chapter 1 - Season 9 (UE 4.23 - FNamePool introduced)
        // ========================================================================
        { 5110300,  5176700,  4, 23, 9.00 },   // 9.0
        { 5176700,  5216303,  4, 23, 9.10 },   // 9.1
        { 5216303,  5268528,  4, 23, 9.20 },   // 9.2
        { 5268528,  5332994,  4, 23, 9.30 },   // 9.3
        { 5332994,  5372160,  4, 23, 9.40 },   // 9.4
        { 5372160,  5423082,  4, 23, 9.41 },   // 9.4.1

        // ========================================================================
        // Chapter 1 - Season 10 (X) (UE 4.23)
        // ========================================================================
        { 5423082,  5492370,  4, 23, 10.00 },  // 10.0
        { 5492370,  5545976,  4, 23, 10.10 },  // 10.1
        { 5545976,  5633945,  4, 23, 10.20 },  // 10.2
        { 5633945,  5704620,  4, 23, 10.30 },  // 10.3
        { 5704620,  5826396,  4, 23, 10.31 },  // 10.3.1
        { 5826396,  5878874,  4, 23, 10.40 },  // 10.4

        // ========================================================================
        // Chapter 2 - Season 1 (UE 4.24)
        // ========================================================================
        { 5878874,  6058028,  4, 24, 11.00 },  // 11.0
        { 6058028,  6113816,  4, 24, 11.01 },  // 11.0.1
        { 6113816,  6195466,  4, 24, 11.10 },  // 11.1
        { 6195466,  6316943,  4, 24, 11.20 },  // 11.2
        { 6316943,  6394056,  4, 24, 11.21 },  // 11.2.1
        { 6394056,  6522018,  4, 24, 11.30 },  // 11.3
        { 6522018,  6639283,  4, 24, 11.31 },  // 11.3.1
        { 6639283,  6755567,  4, 24, 11.40 },  // 11.4
        { 6755567,  6870595,  4, 24, 11.50 },  // 11.5

        // ========================================================================
        // Chapter 2 - Season 2 (UE 4.24)
        // ========================================================================
        { 6870595,  7037963,  4, 24, 12.00 },  // 12.0
        { 7037963,  7095426,  4, 24, 12.10 },  // 12.1
        { 7095426,  7190182,  4, 24, 12.20 },  // 12.2
        { 7190182,  7251970,  4, 24, 12.21 },  // 12.2.1
        { 7251970,  7351410,  4, 24, 12.30 },  // 12.3
        { 7351410,  7421103,  4, 24, 12.40 },  // 12.4
        { 7421103,  7499902,  4, 24, 12.41 },  // 12.4.1
        { 7499902,  7609292,  4, 24, 12.50 },  // 12.5
        { 7609292,  7704104,  4, 24, 12.60 },  // 12.6
        { 7704104,  7834553,  4, 24, 12.61 },  // 12.6.1

        // ========================================================================
        // Chapter 2 - Season 3 (UE 4.24)
        // ========================================================================
        { 7834553,  8008725,  4, 24, 13.00 },  // 13.0
        { 8008725,  8090709,  4, 24, 13.20 },  // 13.2
        { 8090709,  8154316,  4, 24, 13.30 },  // 13.3
        { 8154316,  8297117,  4, 24, 13.40 },  // 13.4

        // ========================================================================
        // Chapter 2 - Season 4 (UE 4.24)
        // ========================================================================
        { 8297117,  8490514,  4, 24, 14.00 },  // 14.0
        { 8490514,  8606188,  4, 24, 14.10 },  // 14.1
        { 8606188,  8723043,  4, 24, 14.20 },  // 14.2
        { 8723043,  8775446,  4, 24, 14.30 },  // 14.3
        { 8775446,  8870917,  4, 24, 14.40 },  // 14.4
        { 8870917,  9034168,  4, 24, 14.50 },  // 14.5
        { 9034168,  9141206,  4, 24, 14.60 },  // 14.6

        // ========================================================================
        // Chapter 2 - Season 5 (UE 4.25 - FField introduced)
        // ========================================================================
        { 9141206,  9449003,  4, 25, 15.00 },  // 15.0
        { 9449003,  9562734,  4, 25, 15.10 },  // 15.1
        { 9562734,  9685607,  4, 25, 15.20 },  // 15.2
        { 9685607,  9822221,  4, 25, 15.21 },  // 15.2.1
        { 9822221,  9926083,  4, 25, 15.30 },  // 15.3
        { 9926083, 10033985,  4, 25, 15.40 },  // 15.4
        { 10033985, 10127509, 4, 25, 15.50 },  // 15.5

        // ========================================================================
        // Chapter 2 - Season 6 (UE 4.26)
        // ========================================================================
        { 10127509, 10466661, 4, 26, 16.00 },  // 16.0
        { 10466661, 10639200, 4, 26, 16.10 },  // 16.1
        { 10639200, 10800459, 4, 26, 16.20 },  // 16.2
        { 10800459, 10951243, 4, 26, 16.30 },  // 16.3
        { 10951243, 11100825, 4, 26, 16.40 },  // 16.4
        { 11100825, 11203632, 4, 26, 16.50 },  // 16.5

        // ========================================================================
        // Chapter 2 - Season 7 (UE 4.26)
        // ========================================================================
        { 11203632, 11556442, 4, 26, 17.00 },  // 17.0
        { 11556442, 11724923, 4, 26, 17.10 },  // 17.1
        { 11724923, 11883027, 4, 26, 17.20 },  // 17.2
        { 11883027, 12058785, 4, 26, 17.21 },  // 17.2.1
        { 12058785, 12186007, 4, 26, 17.30 },  // 17.3
        { 12186007, 12343911, 4, 26, 17.40 },  // 17.4
        { 12343911, 12493209, 4, 26, 17.50 },  // 17.5

        // ========================================================================
        // Chapter 2 - Season 8 (UE 4.26)
        // ========================================================================
        { 12493209, 12905909, 4, 26, 18.00 },  // 18.0
        { 12905909, 13039508, 4, 26, 18.10 },  // 18.1
        { 13039508, 13206842, 4, 26, 18.20 },  // 18.2
        { 13206842, 13383027, 4, 26, 18.21 },  // 18.2.1
        { 13383027, 13498980, 4, 26, 18.30 },  // 18.3
        { 13498980, 13692932, 4, 26, 18.40 },  // 18.4

        // ========================================================================
        // Chapter 3 - Season 1 (UE 4.27)
        // ========================================================================
        { 13692932, 14211857, 5, 0, 19.00 },  // 19.0
        { 14211857, 14422223, 5, 0, 19.01 },  // 19.0.1
        { 14422223, 14550713, 5, 0, 19.10 },  // 19.1
        { 14550713, 14786821, 5, 0, 19.20 },  // 19.2
        { 14786821, 14899505, 5, 0, 19.30 },  // 19.3
        { 14899505, 19215531, 5, 0, 19.40 },  // 19.4
    };

    const size_t FVersionResolver::s_CLMappingCount = sizeof(s_CLMappings) / sizeof(s_CLMappings[0]);

    FVersionResolver::FVersionResolver()
        : m_bDetected(false)
    {
    }

    FVersionResolver& FVersionResolver::Get()
    {
        static FVersionResolver Instance;
        return Instance;
    }

    EResult FVersionResolver::DetectVersion()
    {
        if (m_bDetected)
            return EResult::AlreadyInitialized;

        USS_LOG("Starting version detection...");

        if (TryDetectFromVersionInfo())
        {
            USS_LOG("Version detected from embedded version info");
        }
        else if (TryDetectFromCL())
        {
            USS_LOG("Version detected from CL mapping");
        }
        else if (TryDetectFromPatterns())
        {
            USS_LOG("Version detected from memory patterns");
        }
        else
        {
            USS_ERROR("Failed to detect version");

            // Show error message and exit for unsupported version
            MessageBoxA(
                nullptr,
                "UniversalSlashingSimulator could not detect the Fortnite version.\n\n"
                "This build supports Fortnite versions 1.2 through 19.40.\n\n"
                "Please ensure you are running a supported version.",
                "Unsupported Version",
                MB_OK | MB_ICONERROR
            );

            return EResult::InvalidVersion;
        }

        DetermineGeneration();
        ComputeFeatureFlags();

        if (!SupportsVersion(m_VersionInfo))
        {
            USS_ERROR("Detected version is not supported: FN %.2f", m_VersionInfo.FortniteVersion);

            char ErrorMsg[512];
            snprintf(ErrorMsg, sizeof(ErrorMsg),
                "UniversalSlashingSimulator detected Fortnite %.2f (CL %u)\n\n"
                "This version is not supported.\n\n"
                "Supported versions: 1.2 - 19.40",
                m_VersionInfo.FortniteVersion,
                m_VersionInfo.FortniteCL);

            MessageBoxA(nullptr, ErrorMsg, "Unsupported Version", MB_OK | MB_ICONERROR);
            return EResult::InvalidVersion;
        }

        USS_LOG("Detected version: Engine %s, Fortnite %.2f (CL %u)",
            m_VersionInfo.GetEngineVersionString().c_str(),
            m_VersionInfo.FortniteVersion,
            m_VersionInfo.FortniteCL);

        USS_LOG("Generation: %s", m_VersionInfo.GetGenerationName());
        USS_LOG("Features: FNamePool=%d, FField=%d, ChunkedObjects=%d, NewFAS=%d, TObjectPtr=%d",
            m_VersionInfo.bUseFNamePool,
            m_VersionInfo.bUseFField,
            m_VersionInfo.bUseChunkedObjects,
            m_VersionInfo.bUseNewFastArraySerializer,
            m_VersionInfo.bUseTObjectPtr);

        m_bDetected = true;
        return EResult::Success;
    }

    const FVersionInfo& FVersionResolver::GetVersionInfo() const
    {
        return m_VersionInfo;
    }

    bool FVersionResolver::SupportsVersion(const FVersionInfo& Info) const
    {
        if (Info.EngineVersionMajor == 4)
        {
            return Info.EngineVersionMinor >= 16 && Info.EngineVersionMinor <= 27;
        }
        else if (Info.EngineVersionMajor == 5)
        {
            return Info.EngineVersionMinor <= 2;
        }
        return false;
    }

    bool FVersionResolver::IsVersionDetected() const
    {
        return m_bDetected;
    }

    bool FVersionResolver::TryDetectFromVersionInfo()
    {
        // Try to find version string in memory
        // Pattern: "++Fortnite+Release-XX.XX" or similar

        const char* VersionPattern = "2B 2B 46 6F 72 74 6E 69 74 65 2B 52 65 6C 65 61 73 65"; // "++Fortnite+Release"
        auto Result = Memory::FindPatternIDA(VersionPattern);

        if (!Result)
        {
            USS_LOG("Version string pattern not found");
            return false;
        }

        // Parse version from string after pattern
        // Format: "++Fortnite+Release-XX.XX-CL-XXXXXXX"
        char VersionStr[64] = {};
        for (int i = 0; i < 63; ++i)
        {
            char c = 0;
            Memory::Read<char>(Result.Address + i, c);
            if (c == 0 || c == '-' && i > 30)
                break;
            VersionStr[i] = c;
        }

        USS_LOG("Found version string: %s", VersionStr);

        const char* CLStart = strstr(VersionStr, "CL-");
        if (CLStart)
        {
            uint32 CL = 0;
            if (sscanf(CLStart, "CL-%u", &CL) == 1 && CL > 0)
            {
                return MapCLToVersion(CL);
            }
        }

        return false;
    }

    bool FVersionResolver::TryDetectFromCL()
    {
        // Try to find CL using TimmiesAwesomeOffsetFinder
        // This uses pattern scanning to find the GetEngineVersion function

        // Pattern for GetEngineVersion (varies by version)
        // @timmie
        uintptr_t CLAddr = PatternScanner::Get()->FindGetEngineVersion();

        if (CLAddr == 0)
        {
            USS_LOG("CL detection via pattern failed");
            return false;
        }

        static FString(*GetEngineVersion)() = decltype(GetEngineVersion)(CLAddr);

        FString EngineVersion = GetEngineVersion();
        std::string EngineVer = EngineVersion.ToString();

        size_t DashPos = EngineVer.find("-");
        size_t PlusPos = EngineVer.find("+++");

        if (DashPos == std::string::npos || PlusPos == std::string::npos || PlusPos <= DashPos)
        {
            USS_LOG("Failed to parse CL from EngineVersion: %s", EngineVer.c_str());
            return false;
        }

        std::string CLStr = EngineVer.substr(DashPos + 1, PlusPos - (DashPos + 1));
        uint32 CL = static_cast<uint32>(std::strtoul(CLStr.c_str(), nullptr, 10));

        USS_LOG("Found CL from version function: %u", CL);
        return MapCLToVersion(CL);
    }

    bool FVersionResolver::TryDetectFromPatterns()
    {
        // Try to detect version from characteristic patterns

        // @timmie

        // Check for FField (UE 4.25+)
        uintptr_t FFieldAddr = TAOF::FindPattern(TAOF::Patterns::ProcessEvent_423Plus);

        // Check for FNamePool (UE 4.23+)
        uintptr_t FNamePoolAddr = TAOF::FindPattern(TAOF::Patterns::GNames_FNamePool);

        // Check for older GNames (Pre-4.23)
        uintptr_t OldGNamesAddr = TAOF::FindPattern(TAOF::Patterns::GNames_Pre423);

        if (FFieldAddr && FNamePoolAddr)
        {
            // UE 4.25+
            m_VersionInfo.EngineVersionMajor = 4;
            m_VersionInfo.EngineVersionMinor = 25;
            m_VersionInfo.FortniteVersion = 15.00;
            m_VersionInfo.FortniteCL = 9500000;
            USS_LOG("Detected UE 4.25+ from FField pattern");
            return true;
        }
        else if (FNamePoolAddr)
        {
            // UE 4.23-4.24
            m_VersionInfo.EngineVersionMajor = 4;
            m_VersionInfo.EngineVersionMinor = 23;
            m_VersionInfo.FortniteVersion = 9.00;
            m_VersionInfo.FortniteCL = 5200000;
            USS_LOG("Detected UE 4.23+ from FNamePool pattern");
            return true;
        }
        else if (OldGNamesAddr)
        {
            // Pre-4.23 - check for chunked objects (4.21+)
            uintptr_t ChunkedAddr = TAOF::FindPattern(TAOF::Patterns::GObjects_421Plus);
            if (ChunkedAddr)
            {
                // UE 4.21-4.22
                m_VersionInfo.EngineVersionMajor = 4;
                m_VersionInfo.EngineVersionMinor = 21;
                m_VersionInfo.FortniteVersion = 5.00;
                m_VersionInfo.FortniteCL = 4300000;
                USS_LOG("Detected UE 4.21+ from chunked GObjects pattern");
            }
            else
            {
                // UE 4.16-4.20
                m_VersionInfo.EngineVersionMajor = 4;
                m_VersionInfo.EngineVersionMinor = 16;
                m_VersionInfo.FortniteVersion = 1.80;
                m_VersionInfo.FortniteCL = 3724489;
                USS_LOG("Detected UE 4.16-4.20 from old GNames pattern");
            }
            return true;
        }

        USS_WARN("Pattern-based version detection failed, using fallback...");

        // Default to baseline version for development/testing
        m_VersionInfo.EngineVersionMajor = 4;
        m_VersionInfo.EngineVersionMinor = 16;
        m_VersionInfo.FortniteVersion = 1.80;
        m_VersionInfo.FortniteCL = 3724489;

        return true;
    }

    bool FVersionResolver::MapCLToVersion(uint32 CL)
    {
        for (size_t i = 0; i < s_CLMappingCount; ++i)
        {
            const auto& Mapping = s_CLMappings[i];

            if (CL >= Mapping.CLMin && CL < Mapping.CLMax)
            {
                m_VersionInfo.FortniteCL = CL;
                m_VersionInfo.EngineVersionMajor = Mapping.EngineMajor;
                m_VersionInfo.EngineVersionMinor = Mapping.EngineMinor;
                m_VersionInfo.FortniteVersion = Mapping.FortniteVersion;
                m_VersionInfo.FortniteSeasonMajor = static_cast<uint32>(Mapping.FortniteVersion);
                m_VersionInfo.FortniteSeasonMinor = static_cast<uint32>((Mapping.FortniteVersion - m_VersionInfo.FortniteSeasonMajor) * 100);

                USS_LOG("Mapped CL %u to Fortnite %.2f (UE %u.%u)",
                    CL, Mapping.FortniteVersion, Mapping.EngineMajor, Mapping.EngineMinor);
                return true;
            }
        }

        USS_WARN("Unknown CL: %u - not in mapping table", CL);
        return false;
    }

    void FVersionResolver::DetermineGeneration()
    {
        uint32 Major = m_VersionInfo.EngineVersionMajor;
        uint32 Minor = m_VersionInfo.EngineVersionMinor;

        if (Major == 4)
        {
            if (Minor <= 19)
                m_VersionInfo.Generation = EEngineGeneration::UE4_16_19;
            else if (Minor <= 22)
                m_VersionInfo.Generation = EEngineGeneration::UE4_20_22;
            else if (Minor <= 24)
                m_VersionInfo.Generation = EEngineGeneration::UE4_23_24;
            else if (Minor == 25)
                m_VersionInfo.Generation = EEngineGeneration::UE4_25;
            else
                m_VersionInfo.Generation = EEngineGeneration::UE4_26_27;
        }
        else if (Major == 5)
        {
            if (Minor == 0)
                m_VersionInfo.Generation = EEngineGeneration::UE5_0;
            else
                m_VersionInfo.Generation = EEngineGeneration::UE5_1_Plus;
        }
        else
        {
            m_VersionInfo.Generation = EEngineGeneration::Unknown;
        }
    }

    void FVersionResolver::ComputeFeatureFlags()
    {
        uint32 Major = m_VersionInfo.EngineVersionMajor;
        uint32 Minor = m_VersionInfo.EngineVersionMinor;
        double FN = m_VersionInfo.FortniteVersion;

        // FNamePool introduced in UE 4.23 (FN 9.0)
        m_VersionInfo.bUseFNamePool = (Major == 4 && Minor >= 23) || Major >= 5;

        // FField introduced in UE 4.25 (FN 15.0)
        m_VersionInfo.bUseFField = (Major == 4 && Minor >= 25) || Major >= 5;

        // Chunked UObject array in UE 4.21+ (FN 5.0+)
        m_VersionInfo.bUseChunkedObjects = (Major == 4 && Minor >= 21) || Major >= 5;

        // New FastArraySerializer in FN 8.30+
        m_VersionInfo.bUseNewFastArraySerializer = FN >= 8.30;

        // TObjectPtr in UE 5.0+
        m_VersionInfo.bUseTObjectPtr = Major >= 5;

        // STW support (check version range)
        m_VersionInfo.bSupportsSTW = FN <= 20.00;
    }

}