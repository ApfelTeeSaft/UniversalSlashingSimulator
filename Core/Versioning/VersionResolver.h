/**
 * UniversalSlashingSimulator - Version Resolver
 *
 * Detects and resolves the current engine and Fortnite version.
 * Uses pattern scanning and memory analysis to determine version.
 * All version-specific behavior branching is based on this resolver.
 *
 * Version detection strategy:
 * 1. Try to read engine version from embedded version info
 * 2. Fall back to CL-based detection via pattern scanning
 * 3. Map CL to known Fortnite/Engine versions
 * 4. Compute feature flags based on version
 */

#pragma once

#include "../Common.h"
#include "VersionInfo.h"

namespace USS
{
    USS_INTERFACE IVersionResolver
    {
    public:
        virtual ~IVersionResolver() = default;

        virtual EResult DetectVersion() = 0;

        virtual const FVersionInfo& GetVersionInfo() const = 0;

        virtual bool SupportsVersion(const FVersionInfo& Info) const = 0;

        virtual bool IsVersionDetected() const = 0;
    };

    class FVersionResolver : public IVersionResolver
    {
    public:
        FVersionResolver();
        ~FVersionResolver() override = default;

        USS_NON_COPYABLE(FVersionResolver)
        USS_NON_MOVABLE(FVersionResolver)

        EResult DetectVersion() override;
        const FVersionInfo& GetVersionInfo() const override;
        bool SupportsVersion(const FVersionInfo& Info) const override;
        bool IsVersionDetected() const override;

        static FVersionResolver& Get();

    private:
        bool TryDetectFromVersionInfo();
        bool TryDetectFromCL();
        bool TryDetectFromPatterns();

        bool MapCLToVersion(uint32 CL);
        void ComputeFeatureFlags();
        void DetermineGeneration();

        FVersionInfo m_VersionInfo;
        bool m_bDetected;

        struct FCLMapping
        {
            uint32 CLMin;
            uint32 CLMax;
            uint32 EngineMajor;
            uint32 EngineMinor;
            double FortniteVersion;
        };

        static const FCLMapping s_CLMappings[];
        static const size_t s_CLMappingCount;
    };

    inline FVersionResolver& GetVersionResolver()
    {
        return FVersionResolver::Get();
    }

}