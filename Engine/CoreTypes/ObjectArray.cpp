/**
 * UniversalSlashingSimulator - Object Array Implementation
 */

#include "ObjectArray.h"
#include "../../Core/Memory/Memory.h"
#include "../../Core/Logging/Log.h"
#include "../../Core/Versioning/VersionResolver.h"

namespace USS
{
    //=========================================================================
    // FFixedObjectArray Implementation (UE4.11-4.20)
    //=========================================================================

    FFixedObjectArray::FFixedObjectArray()
        : m_BaseAddress(0)
        , m_ObjectsPtr(0)
        , m_NumElements(0)
        , m_MaxElements(0)
        , m_ItemSize(0x18)  // Default: Object(8) + Flags(4) + ClusterIndex(4) + SerialNumber(4)
        , m_bInitialized(false)
    {
    }

    EResult FFixedObjectArray::Initialize(uintptr Address)
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        if (Address == 0)
            return EResult::Failed;

        m_BaseAddress = Address;

        // Read fixed array layout
        // Offsets are approximate and may need adjustment per-version
        // struct TUObjectArray {
        //     FUObjectItem* Objects;   // 0x00
        //     int32 MaxElements;       // 0x08
        //     int32 NumElements;       // 0x0C
        // }

        // The FUObjectArray contains TUObjectArray at offset 0x10
        uintptr TUObjectArrayAddr = m_BaseAddress + 0x10;

        if (!Memory::Read<uintptr>(TUObjectArrayAddr + 0x00, m_ObjectsPtr))
        {
            USS_ERROR("Failed to read Objects pointer from FUObjectArray");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TUObjectArrayAddr + 0x08, m_MaxElements))
        {
            USS_ERROR("Failed to read MaxElements from FUObjectArray");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TUObjectArrayAddr + 0x0C, m_NumElements))
        {
            USS_ERROR("Failed to read NumElements from FUObjectArray");
            return EResult::Failed;
        }

        USS_LOG("FFixedObjectArray initialized: NumElements=%d, MaxElements=%d",
            m_NumElements, m_MaxElements);

        m_bInitialized = true;
        return EResult::Success;
    }

    int32 FFixedObjectArray::Num() const
    {
        return m_NumElements;
    }

    void* FFixedObjectArray::GetByIndex(int32 Index) const
    {
        if (!IsValidIndex(Index))
            return nullptr;

        uintptr ItemAddr = m_ObjectsPtr + (Index * m_ItemSize);

        void* Object = nullptr;
        if (!Memory::Read<void*>(ItemAddr, Object))
            return nullptr;

        return Object;
    }

    bool FFixedObjectArray::GetItemByIndex(int32 Index, FObjectItem& OutItem) const
    {
        if (!IsValidIndex(Index))
            return false;

        uintptr ItemAddr = m_ObjectsPtr + (Index * m_ItemSize);

        if (!Memory::Read<void*>(ItemAddr + 0x00, OutItem.Object))
            return false;
        if (!Memory::Read<int32>(ItemAddr + 0x08, OutItem.Flags))
            return false;
        if (!Memory::Read<int32>(ItemAddr + 0x0C, OutItem.ClusterIndex))
            return false;
        if (!Memory::Read<int32>(ItemAddr + 0x10, OutItem.SerialNumber))
            return false;

        return true;
    }

    bool FFixedObjectArray::IsValidIndex(int32 Index) const
    {
        return m_bInitialized && Index >= 0 && Index < m_NumElements;
    }

    bool FFixedObjectArray::IsInitialized() const
    {
        return m_bInitialized;
    }

    //=========================================================================
    // FChunkedObjectArray Implementation (UE4.21+)
    //=========================================================================

    FChunkedObjectArray::FChunkedObjectArray()
        : m_BaseAddress(0)
        , m_ChunksPtr(0)
        , m_NumElements(0)
        , m_MaxElements(0)
        , m_NumChunks(0)
        , m_ItemSize(0x18)
        , m_bInitialized(false)
    {
    }

    EResult FChunkedObjectArray::Initialize(uintptr Address)
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        if (Address == 0)
            return EResult::Failed;

        m_BaseAddress = Address;

        // Read chunked array layout
        // struct FChunkedFixedUObjectArray {
        //     FUObjectItem** Objects;       // 0x00 - Chunks array
        //     FUObjectItem* PreAllocated;   // 0x08
        //     int32 MaxElements;            // 0x10
        //     int32 NumElements;            // 0x14
        //     int32 MaxChunks;              // 0x18
        //     int32 NumChunks;              // 0x1C
        // }

        uintptr TUObjectArrayAddr = m_BaseAddress + 0x10;

        if (!Memory::Read<uintptr>(TUObjectArrayAddr + 0x00, m_ChunksPtr))
        {
            USS_ERROR("Failed to read Chunks pointer from FChunkedObjectArray");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TUObjectArrayAddr + 0x10, m_MaxElements))
        {
            USS_ERROR("Failed to read MaxElements from FChunkedObjectArray");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TUObjectArrayAddr + 0x14, m_NumElements))
        {
            USS_ERROR("Failed to read NumElements from FChunkedObjectArray");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TUObjectArrayAddr + 0x1C, m_NumChunks))
        {
            USS_ERROR("Failed to read NumChunks from FChunkedObjectArray");
            return EResult::Failed;
        }

        USS_LOG("FChunkedObjectArray initialized: NumElements=%d, NumChunks=%d",
            m_NumElements, m_NumChunks);

        m_bInitialized = true;
        return EResult::Success;
    }

    int32 FChunkedObjectArray::Num() const
    {
        return m_NumElements;
    }

    void* FChunkedObjectArray::GetByIndex(int32 Index) const
    {
        if (!IsValidIndex(Index))
            return nullptr;

        int32 ChunkIndex = Index / ElementsPerChunk;
        int32 WithinChunkIndex = Index % ElementsPerChunk;

        // Get chunk pointer
        uintptr ChunkPtrAddr = m_ChunksPtr + (ChunkIndex * sizeof(uintptr));
        uintptr ChunkPtr = 0;

        if (!Memory::Read<uintptr>(ChunkPtrAddr, ChunkPtr))
            return nullptr;

        if (ChunkPtr == 0)
            return nullptr;

        // Get object from chunk
        uintptr ItemAddr = ChunkPtr + (WithinChunkIndex * m_ItemSize);

        void* Object = nullptr;
        if (!Memory::Read<void*>(ItemAddr, Object))
            return nullptr;

        return Object;
    }

    bool FChunkedObjectArray::GetItemByIndex(int32 Index, FObjectItem& OutItem) const
    {
        if (!IsValidIndex(Index))
            return false;

        int32 ChunkIndex = Index / ElementsPerChunk;
        int32 WithinChunkIndex = Index % ElementsPerChunk;

        // Get chunk pointer
        uintptr ChunkPtrAddr = m_ChunksPtr + (ChunkIndex * sizeof(uintptr));
        uintptr ChunkPtr = 0;

        if (!Memory::Read<uintptr>(ChunkPtrAddr, ChunkPtr))
            return false;

        if (ChunkPtr == 0)
            return false;

        // Get item from chunk
        uintptr ItemAddr = ChunkPtr + (WithinChunkIndex * m_ItemSize);

        if (!Memory::Read<void*>(ItemAddr + 0x00, OutItem.Object))
            return false;
        if (!Memory::Read<int32>(ItemAddr + 0x08, OutItem.Flags))
            return false;
        if (!Memory::Read<int32>(ItemAddr + 0x0C, OutItem.ClusterIndex))
            return false;
        if (!Memory::Read<int32>(ItemAddr + 0x10, OutItem.SerialNumber))
            return false;

        return true;
    }

    bool FChunkedObjectArray::IsValidIndex(int32 Index) const
    {
        return m_bInitialized && Index >= 0 && Index < m_NumElements;
    }

    bool FChunkedObjectArray::IsInitialized() const
    {
        return m_bInitialized;
    }

    //=========================================================================
    // Factory Function
    //=========================================================================

    std::unique_ptr<IObjectArray> CreateObjectArray()
    {
        const auto& Version = GetVersionResolver().GetVersionInfo();

        if (Version.bUseChunkedObjects)
        {
            USS_LOG("Creating FChunkedObjectArray for UE %s",
                Version.GetEngineVersionString().c_str());
            return std::make_unique<FChunkedObjectArray>();
        }
        else
        {
            USS_LOG("Creating FFixedObjectArray for UE %s",
                Version.GetEngineVersionString().c_str());
            return std::make_unique<FFixedObjectArray>();
        }
    }

}