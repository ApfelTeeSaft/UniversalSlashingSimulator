/**
 * UniversalSlashingSimulator - Object Array Abstraction
 *
 * Provides a version-agnostic interface for accessing the global
 * object array (GObjects). Implementations differ between:
 * - UE4.11-4.20: Fixed direct array (FUObjectItem*)
 * - UE4.21+: Chunked indirect array (FUObjectItem**)
 */

#pragma once

#include "../../Core/Common.h"

namespace USS
{
    // FUObjectItem flags
    enum class EInternalObjectFlags : int32
    {
        None              = 0,
        Native            = 1 << 25,
        Async             = 1 << 26,
        AsyncLoading      = 1 << 27,
        Unreachable       = 1 << 28,
        PendingKill       = 1 << 29,
        RootSet           = 1 << 30,
        NoStrongReference = 1 << 31,
    };

    // Object item wrapper (version-agnostic)
    struct FObjectItem
    {
        void* Object;           // UObject*
        int32 Flags;
        int32 ClusterIndex;
        int32 SerialNumber;

        bool IsUnreachable() const
        {
            return (Flags & static_cast<int32>(EInternalObjectFlags::Unreachable)) != 0;
        }

        bool IsPendingKill() const
        {
            return (Flags & static_cast<int32>(EInternalObjectFlags::PendingKill)) != 0;
        }

        bool IsRootSet() const
        {
            return (Flags & static_cast<int32>(EInternalObjectFlags::RootSet)) != 0;
        }
    };

    // Object array interface
    USS_INTERFACE IObjectArray
    {
    public:
        virtual ~IObjectArray() = default;

        // Get number of objects
        virtual int32 Num() const = 0;

        // Get object by index (returns UObject*)
        virtual void* GetByIndex(int32 Index) const = 0;

        // Get object item by index
        virtual bool GetItemByIndex(int32 Index, FObjectItem& OutItem) const = 0;

        // Check if index is valid
        virtual bool IsValidIndex(int32 Index) const = 0;

        // Initialize from memory address
        virtual EResult Initialize(uintptr Address) = 0;

        // Check if initialized
        virtual bool IsInitialized() const = 0;
    };

    // Fixed object array implementation (UE4.11-4.20)
    class FFixedObjectArray : public IObjectArray
    {
    public:
        FFixedObjectArray();
        ~FFixedObjectArray() override = default;

        int32 Num() const override;
        void* GetByIndex(int32 Index) const override;
        bool GetItemByIndex(int32 Index, FObjectItem& OutItem) const override;
        bool IsValidIndex(int32 Index) const override;
        EResult Initialize(uintptr Address) override;
        bool IsInitialized() const override;

    private:
        // Internal layout for fixed array
        // struct FUObjectArray {
        //     int32 ObjFirstGCIndex;
        //     int32 ObjLastNonGCIndex;
        //     int32 MaxObjectsNotConsideredByGC;
        //     bool  OpenForDisregardForGC;
        //     FUObjectItem* Objects;  // Direct pointer
        //     int32 MaxElements;
        //     int32 NumElements;
        // }

        uintptr m_BaseAddress;
        uintptr m_ObjectsPtr;
        int32 m_NumElements;
        int32 m_MaxElements;
        size_t m_ItemSize;
        bool m_bInitialized;
    };

    // Chunked object array implementation (UE4.21+)
    class FChunkedObjectArray : public IObjectArray
    {
    public:
        FChunkedObjectArray();
        ~FChunkedObjectArray() override = default;

        int32 Num() const override;
        void* GetByIndex(int32 Index) const override;
        bool GetItemByIndex(int32 Index, FObjectItem& OutItem) const override;
        bool IsValidIndex(int32 Index) const override;
        EResult Initialize(uintptr Address) override;
        bool IsInitialized() const override;

    private:
        // Internal layout for chunked array
        // struct FChunkedFixedUObjectArray {
        //     FUObjectItem** Objects;  // Pointer to chunk pointers
        //     FUObjectItem* PreAllocatedObjects;
        //     int32 MaxElements;
        //     int32 NumElements;
        //     int32 MaxChunks;
        //     int32 NumChunks;
        // }

        static constexpr int32 ElementsPerChunk = 64 * 1024;  // 65536

        uintptr m_BaseAddress;
        uintptr m_ChunksPtr;
        int32 m_NumElements;
        int32 m_MaxElements;
        int32 m_NumChunks;
        size_t m_ItemSize;
        bool m_bInitialized;
    };

    std::unique_ptr<IObjectArray> CreateObjectArray();

}