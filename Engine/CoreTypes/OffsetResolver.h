/**
 * UniversalSlashingSimulator - Offset Resolver
 *
 * Provides stubbed offset resolution for engine and game structures.
 * All offsets are stubbed and designed to be replaced by an external
 * offset finder library in the future. @timmie
 *
 * IMPORTANT: No hardcoded offsets should exist outside this module.
 * All offset access must go through this resolver.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Core/Versioning/VersionInfo.h"

namespace USS
{
    // Offset categories
    enum class EOffsetCategory : uint8
    {
        UObject,
        UField,         // Pre-4.25 field base
        UStruct,
        UClass,
        UFunction,
        UProperty,      // Pre-4.25
        FField,         // 4.25+
        FProperty,      // 4.25+
        FFieldClass,    // 4.25+ field class info
        Actor,          // Actor base
        Controller,     // Player controller
        Pawn,           // Player pawn
        Inventory,      // Inventory system
        Building,       // Building system
        Mission,        // Mission system
    };

    // Complete offset table for all required offsets
    struct FOffsetTable
    {
        // UObject offsets
        struct
        {
            int32 Vtable;          // 0x00
            int32 ObjectFlags;     // 0x08
            int32 InternalIndex;   // 0x0C
            int32 Class;           // 0x10
            int32 Name;            // 0x18
            int32 Outer;           // 0x20
        } UObject;

        // UField offsets (pre-4.25)
        struct
        {
            int32 Next;            // UField::Next
        } UField;

        // UStruct offsets
        struct
        {
            int32 SuperStruct;     // UStruct::SuperStruct
            int32 Children;        // UStruct::Children (UField*)
            int32 ChildProperties; // UStruct::ChildProperties (FField*, 4.25+)
            int32 PropertiesSize;  // UStruct::PropertiesSize
            int32 MinAlignment;    // UStruct::MinAlignment
        } UStruct;

        // UClass offsets
        struct
        {
            int32 ClassDefaultObject;
            int32 ClassConstructor;
        } UClass;

        // UFunction offsets
        struct
        {
            int32 FunctionFlags;
            int32 NumParms;
            int32 ParmsSize;
            int32 ReturnValueOffset;
            int32 Func;            // Native function pointer
        } UFunction;

        // UProperty offsets (pre-4.25)
        struct
        {
            int32 ElementSize;
            int32 Offset;
            int32 PropertyFlags;
            int32 Next;
        } UProperty;

        // FField offsets (4.25+)
        struct
        {
            int32 ClassPrivate;
            int32 Owner;
            int32 Next;
            int32 NamePrivate;
            int32 FlagsPrivate;
        } FField;

        // FProperty offsets (4.25+)
        struct
        {
            int32 ElementSize;
            int32 Offset;
            int32 PropertyFlags;
        } FProperty;

        // Controller offsets (version-specific)
        struct
        {
            int32 BuildPreviewMarker;      // 0x1788 in baseline
            int32 CurrentBuildableClass;   // 0x1940 in baseline
            int32 PreviousBuildableClass;  // 0x1948 in baseline
            int32 EditBuildingActor;       // 0x1A48 in baseline
            int32 QuickBars;               // 0x1A88 in baseline
            int32 BuildPreviewMarkerMID;   // 0x1928 in baseline
            int32 CheatManager;
            int32 PlayerState;
        } Controller;

        // Pawn offsets
        struct
        {
            int32 Controller;
            int32 PlayerState;
            int32 CustomizationLoadout;
            int32 CharacterParts;
        } Pawn;

        // Inventory offsets
        struct
        {
            int32 WorldInventory;
            int32 ItemInstances;
            int32 ReplicatedEntries;
        } Inventory;

        // Building offsets
        struct
        {
            int32 BuildingActor;
            int32 BuildingMaterial;
            int32 BuildingEditMode;
        } Building;

        // Global function addresses (stubbed)
        struct
        {
            uintptr StaticConstructObject_Internal;
            uintptr StaticLoadObject;
            uintptr SpawnActor;
            uintptr ProcessEvent;
            uintptr GObjects;
            uintptr GNames;
            uintptr GWorld;
        } Functions;

        // Initialize with default values
        FOffsetTable()
        {
            memset(this, 0, sizeof(*this));

            // Set known baseline offsets (UE 4.16)
            // These are placeholders - actual offsets come from resolver

            // UObject (stable across versions)
            UObject.Vtable = 0x00;
            UObject.ObjectFlags = 0x08;
            UObject.InternalIndex = 0x0C;
            UObject.Class = 0x10;
            UObject.Name = 0x18;
            UObject.Outer = 0x20;

            // UField
            UField.Next = 0x28;

            // UStruct (varies significantly)
            UStruct.SuperStruct = 0x30;
            UStruct.Children = 0x38;
            UStruct.ChildProperties = 0x00;  // 0 = not present (pre-4.25)
            UStruct.PropertiesSize = 0x40;
            UStruct.MinAlignment = 0x44;
        }
    };

    // Offset resolver interface
    USS_INTERFACE IOffsetResolver
    {
    public:
        virtual ~IOffsetResolver() = default;

        // Resolve offsets for the current version
        virtual EResult ResolveOffsets(const FVersionInfo& Version) = 0;

        // Get resolved offset table
        virtual const FOffsetTable& GetOffsets() const = 0;

        // Check if offsets are resolved
        virtual bool IsResolved() const = 0;

        // Get specific offset by category and name
        virtual int32 GetOffset(EOffsetCategory Category, const char* Name) const = 0;

        // Get specific offset by category string and name (convenience)
        virtual int32 GetOffset(const char* CategoryName, const char* Name) const = 0;

        // Get function address
        virtual uintptr GetFunctionAddress(const char* Name) const = 0;
    };

    // Stub offset resolver (returns placeholder values)
    class FStubOffsetResolver : public IOffsetResolver
    {
    public:
        FStubOffsetResolver();
        ~FStubOffsetResolver() override = default;

        EResult ResolveOffsets(const FVersionInfo& Version) override;
        const FOffsetTable& GetOffsets() const override;
        bool IsResolved() const override;
        int32 GetOffset(EOffsetCategory Category, const char* Name) const override;
        int32 GetOffset(const char* CategoryName, const char* Name) const override;
        uintptr GetFunctionAddress(const char* Name) const override;

        // Get singleton
        static FStubOffsetResolver& Get();

    private:
        void ApplyVersionSpecificOffsets(const FVersionInfo& Version);

        FOffsetTable m_Offsets;
        bool m_bResolved;
    };

    // Convenience function
    inline IOffsetResolver& GetOffsetResolver()
    {
        return FStubOffsetResolver::Get();
    }

}