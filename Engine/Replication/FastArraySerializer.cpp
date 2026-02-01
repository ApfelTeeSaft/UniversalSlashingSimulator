/**
 * UniversalSlashingSimulator - Fast Array Serializer Implementation
 */

#include "FastArraySerializer.h"
#include "../../Core/Memory/Memory.h"
#include "../../Core/Logging/Log.h"
#include "../../Core/Versioning/VersionResolver.h"

namespace USS
{
    //=========================================================================
    // FLegacyFastArraySerializer Implementation (Pre-8.30)
    //=========================================================================

    FLegacyFastArraySerializer::FLegacyFastArraySerializer()
        : m_FastArrayPtr(nullptr)
        , m_ItemSize(0)
        , m_ItemsOffset(0)
        , m_ItemsDataPtr(0)
        , m_ItemsNum(0)
        , m_ItemsMax(0)
        , m_IDCounterOffset(0x60)  // Default offset
        , m_bInitialized(false)
    {
    }

    EResult FLegacyFastArraySerializer::Initialize(void* FastArrayPtr, size_t ItemSize, int32 ItemsOffset)
    {
        if (!FastArrayPtr)
            return EResult::InvalidParameter;

        m_FastArrayPtr = FastArrayPtr;
        m_ItemSize = ItemSize;
        m_ItemsOffset = ItemsOffset;

        uintptr BaseAddr = reinterpret_cast<uintptr>(FastArrayPtr);

        // Read TArray structure at ItemsOffset
        // TArray layout: Data*, Num, Max
        uintptr TArrayAddr = BaseAddr + ItemsOffset;

        if (!Memory::Read<uintptr>(TArrayAddr + 0x00, m_ItemsDataPtr))
        {
            USS_ERROR("Failed to read Items.Data from FastArraySerializer");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TArrayAddr + 0x08, m_ItemsNum))
        {
            USS_ERROR("Failed to read Items.Num from FastArraySerializer");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TArrayAddr + 0x0C, m_ItemsMax))
        {
            USS_ERROR("Failed to read Items.Max from FastArraySerializer");
            return EResult::Failed;
        }

        USS_LOG("LegacyFastArraySerializer initialized: Num=%d, Max=%d, ItemSize=%zu",
            m_ItemsNum, m_ItemsMax, m_ItemSize);

        m_bInitialized = true;
        return EResult::Success;
    }

    int32 FLegacyFastArraySerializer::Num() const
    {
        if (!m_bInitialized)
            return 0;

        // Re-read in case it changed
        int32 CurrentNum = 0;
        uintptr TArrayAddr = reinterpret_cast<uintptr>(m_FastArrayPtr) + m_ItemsOffset;
        Memory::Read<int32>(TArrayAddr + 0x08, CurrentNum);
        return CurrentNum;
    }

    void* FLegacyFastArraySerializer::GetItem(int32 Index) const
    {
        if (!m_bInitialized || Index < 0 || Index >= Num())
            return nullptr;

        // Re-read data pointer in case array was reallocated
        uintptr DataPtr = 0;
        uintptr TArrayAddr = reinterpret_cast<uintptr>(m_FastArrayPtr) + m_ItemsOffset;
        Memory::Read<uintptr>(TArrayAddr + 0x00, DataPtr);

        if (DataPtr == 0)
            return nullptr;

        return reinterpret_cast<void*>(DataPtr + (Index * m_ItemSize));
    }

    int32 FLegacyFastArraySerializer::GetItemReplicationID(int32 Index) const
    {
        void* Item = GetItem(Index);
        if (!Item)
            return -1;

        int32 ReplicationID = -1;
        Memory::Read<int32>(reinterpret_cast<uintptr>(Item) + ReplicationIDOffset, ReplicationID);
        return ReplicationID;
    }

    int32 FLegacyFastArraySerializer::GetItemReplicationKey(int32 Index) const
    {
        void* Item = GetItem(Index);
        if (!Item)
            return -1;

        int32 ReplicationKey = -1;
        Memory::Read<int32>(reinterpret_cast<uintptr>(Item) + ReplicationKeyOffset, ReplicationKey);
        return ReplicationKey;
    }

    bool FLegacyFastArraySerializer::IsItemDirty(int32 Index) const
    {
        // In legacy format, items are dirty if ReplicationKey differs from cached
        // For now, assume all items need checking
        return true;
    }

    void FLegacyFastArraySerializer::MarkItemDirty(int32 Index)
    {
        void* Item = GetItem(Index);
        if (!Item)
            return;

        // Increment ReplicationKey
        int32 CurrentKey = GetItemReplicationKey(Index);
        Memory::Write<int32>(reinterpret_cast<uintptr>(Item) + ReplicationKeyOffset, CurrentKey + 1);
    }

    void FLegacyFastArraySerializer::MarkAllDirty()
    {
        int32 Count = Num();
        for (int32 i = 0; i < Count; ++i)
        {
            MarkItemDirty(i);
        }
    }

    int32 FLegacyFastArraySerializer::GetArrayReplicationKey() const
    {
        // Legacy format doesn't have a separate array replication key
        // Return max item key as approximation
        int32 MaxKey = 0;
        int32 Count = Num();
        for (int32 i = 0; i < Count; ++i)
        {
            int32 Key = GetItemReplicationKey(i);
            if (Key > MaxKey)
                MaxKey = Key;
        }
        return MaxKey;
    }

    int32 FLegacyFastArraySerializer::GetIDCounter() const
    {
        if (!m_bInitialized)
            return 0;

        int32 IDCounter = 0;
        uintptr BaseAddr = reinterpret_cast<uintptr>(m_FastArrayPtr);
        Memory::Read<int32>(BaseAddr + m_IDCounterOffset, IDCounter);
        return IDCounter;
    }

    void FLegacyFastArraySerializer::IncrementArrayReplicationKey()
    {
        // No-op for legacy format
    }

    void FLegacyFastArraySerializer::RegisterChangeCallback(FFastArrayChangeCallback Callback)
    {
        if (Callback)
        {
            m_Callbacks.push_back(std::move(Callback));
        }
    }

    bool FLegacyFastArraySerializer::IsInitialized() const
    {
        return m_bInitialized;
    }

    //=========================================================================
    // FNewFastArraySerializer Implementation (Post-8.30)
    //=========================================================================

    FNewFastArraySerializer::FNewFastArraySerializer()
        : m_FastArrayPtr(nullptr)
        , m_ItemSize(0)
        , m_ItemsOffset(0)
        , m_ItemsDataPtr(0)
        , m_ItemsNum(0)
        , m_ItemsMax(0)
        , m_ArrayReplicationKeyOffset(0x68)  // Default for post-8.30
        , m_IDCounterOffset(0x6C)
        , m_DeltaFlagsOffset(0x70)
        , m_CachedArrayReplicationKey(0)
        , m_bInitialized(false)
    {
    }

    EResult FNewFastArraySerializer::Initialize(void* FastArrayPtr, size_t ItemSize, int32 ItemsOffset)
    {
        if (!FastArrayPtr)
            return EResult::InvalidParameter;

        m_FastArrayPtr = FastArrayPtr;
        m_ItemSize = ItemSize;
        m_ItemsOffset = ItemsOffset;

        uintptr BaseAddr = reinterpret_cast<uintptr>(FastArrayPtr);

        // Read TArray structure
        uintptr TArrayAddr = BaseAddr + ItemsOffset;

        if (!Memory::Read<uintptr>(TArrayAddr + 0x00, m_ItemsDataPtr))
        {
            USS_ERROR("Failed to read Items.Data from FastArraySerializer");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TArrayAddr + 0x08, m_ItemsNum))
        {
            USS_ERROR("Failed to read Items.Num from FastArraySerializer");
            return EResult::Failed;
        }

        if (!Memory::Read<int32>(TArrayAddr + 0x0C, m_ItemsMax))
        {
            USS_ERROR("Failed to read Items.Max from FastArraySerializer");
            return EResult::Failed;
        }

        // Read ArrayReplicationKey (new in post-8.30)
        Memory::Read<int32>(BaseAddr + m_ArrayReplicationKeyOffset, m_CachedArrayReplicationKey);

        USS_LOG("NewFastArraySerializer initialized: Num=%d, Max=%d, ArrayKey=%d",
            m_ItemsNum, m_ItemsMax, m_CachedArrayReplicationKey);

        m_bInitialized = true;
        return EResult::Success;
    }

    int32 FNewFastArraySerializer::Num() const
    {
        if (!m_bInitialized)
            return 0;

        int32 CurrentNum = 0;
        uintptr TArrayAddr = reinterpret_cast<uintptr>(m_FastArrayPtr) + m_ItemsOffset;
        Memory::Read<int32>(TArrayAddr + 0x08, CurrentNum);
        return CurrentNum;
    }

    void* FNewFastArraySerializer::GetItem(int32 Index) const
    {
        if (!m_bInitialized || Index < 0 || Index >= Num())
            return nullptr;

        uintptr DataPtr = 0;
        uintptr TArrayAddr = reinterpret_cast<uintptr>(m_FastArrayPtr) + m_ItemsOffset;
        Memory::Read<uintptr>(TArrayAddr + 0x00, DataPtr);

        if (DataPtr == 0)
            return nullptr;

        return reinterpret_cast<void*>(DataPtr + (Index * m_ItemSize));
    }

    int32 FNewFastArraySerializer::GetItemReplicationID(int32 Index) const
    {
        void* Item = GetItem(Index);
        if (!Item)
            return -1;

        int32 ReplicationID = -1;
        Memory::Read<int32>(reinterpret_cast<uintptr>(Item) + ReplicationIDOffset, ReplicationID);
        return ReplicationID;
    }

    int32 FNewFastArraySerializer::GetItemReplicationKey(int32 Index) const
    {
        void* Item = GetItem(Index);
        if (!Item)
            return -1;

        int32 ReplicationKey = -1;
        Memory::Read<int32>(reinterpret_cast<uintptr>(Item) + ReplicationKeyOffset, ReplicationKey);
        return ReplicationKey;
    }

    bool FNewFastArraySerializer::IsItemDirty(int32 Index) const
    {
        void* Item = GetItem(Index);
        if (!Item)
            return false;

        // Compare item's MostRecentArrayReplicationKey with current ArrayReplicationKey
        int32 ItemArrayKey = 0;
        Memory::Read<int32>(reinterpret_cast<uintptr>(Item) + MostRecentArrayReplicationKeyOffset, ItemArrayKey);

        return ItemArrayKey != GetArrayReplicationKey();
    }

    void FNewFastArraySerializer::MarkItemDirty(int32 Index)
    {
        void* Item = GetItem(Index);
        if (!Item)
            return;

        uintptr ItemAddr = reinterpret_cast<uintptr>(Item);

        // Increment ReplicationKey
        int32 CurrentKey = GetItemReplicationKey(Index);
        Memory::Write<int32>(ItemAddr + ReplicationKeyOffset, CurrentKey + 1);

        // Update MostRecentArrayReplicationKey to current
        int32 ArrayKey = GetArrayReplicationKey();
        Memory::Write<int32>(ItemAddr + MostRecentArrayReplicationKeyOffset, ArrayKey);

        // Notify callbacks
        FFastArrayChange Change;
        Change.Type = FFastArrayChange::EChangeType::Modified;
        Change.Index = Index;
        Change.ReplicationID = GetItemReplicationID(Index);
        NotifyChange(Change);
    }

    void FNewFastArraySerializer::MarkAllDirty()
    {
        IncrementArrayReplicationKey();

        int32 Count = Num();
        for (int32 i = 0; i < Count; ++i)
        {
            MarkItemDirty(i);
        }
    }

    int32 FNewFastArraySerializer::GetArrayReplicationKey() const
    {
        if (!m_bInitialized)
            return 0;

        int32 ArrayKey = 0;
        uintptr BaseAddr = reinterpret_cast<uintptr>(m_FastArrayPtr);
        Memory::Read<int32>(BaseAddr + m_ArrayReplicationKeyOffset, ArrayKey);
        return ArrayKey;
    }

    int32 FNewFastArraySerializer::GetIDCounter() const
    {
        if (!m_bInitialized)
            return 0;

        int32 IDCounter = 0;
        uintptr BaseAddr = reinterpret_cast<uintptr>(m_FastArrayPtr);
        Memory::Read<int32>(BaseAddr + m_IDCounterOffset, IDCounter);
        return IDCounter;
    }

    void FNewFastArraySerializer::IncrementArrayReplicationKey()
    {
        if (!m_bInitialized)
            return;

        int32 CurrentKey = GetArrayReplicationKey();
        uintptr BaseAddr = reinterpret_cast<uintptr>(m_FastArrayPtr);
        Memory::Write<int32>(BaseAddr + m_ArrayReplicationKeyOffset, CurrentKey + 1);
    }

    void FNewFastArraySerializer::RegisterChangeCallback(FFastArrayChangeCallback Callback)
    {
        if (Callback)
        {
            m_Callbacks.push_back(std::move(Callback));
        }
    }

    bool FNewFastArraySerializer::IsInitialized() const
    {
        return m_bInitialized;
    }

    void FNewFastArraySerializer::NotifyChange(const FFastArrayChange& Change)
    {
        for (const auto& Callback : m_Callbacks)
        {
            if (Callback)
            {
                Callback(Change);
            }
        }
    }

    //=========================================================================
    // Factory Function
    //=========================================================================

    std::unique_ptr<IFastArraySerializer> CreateFastArraySerializer()
    {
        const auto& Version = GetVersionResolver().GetVersionInfo();

        if (Version.bUseNewFastArraySerializer)
        {
            USS_LOG("Creating NewFastArraySerializer (post-8.30 format)");
            return std::make_unique<FNewFastArraySerializer>();
        }
        else
        {
            USS_LOG("Creating LegacyFastArraySerializer (pre-8.30 format)");
            return std::make_unique<FLegacyFastArraySerializer>();
        }
    }

    //=========================================================================
    // FFastArrayChangeDetector Implementation
    //=========================================================================

    FFastArrayChangeDetector::FFastArrayChangeDetector()
        : m_Serializer(nullptr)
        , m_LastArrayReplicationKey(0)
        , m_LastItemCount(0)
    {
    }

    EResult FFastArrayChangeDetector::Initialize(IFastArraySerializer* Serializer)
    {
        if (!Serializer || !Serializer->IsInitialized())
            return EResult::InvalidParameter;

        m_Serializer = Serializer;
        Reset();
        return EResult::Success;
    }

    int32 FFastArrayChangeDetector::DetectChanges(std::vector<FFastArrayChange>& OutChanges)
    {
        OutChanges.clear();

        if (!m_Serializer)
            return 0;

        int32 CurrentCount = m_Serializer->Num();
        int32 CurrentArrayKey = m_Serializer->GetArrayReplicationKey();

        // Check for array-level reset
        if (CurrentArrayKey != m_LastArrayReplicationKey && m_Serializer->IsNewFormat())
        {
            // Array key changed - could be mass update
            // Check each item for changes
        }

        // Detect removals (items that were present but aren't now)
        for (size_t i = 0; i < m_LastItemIDs.size(); ++i)
        {
            int32 OldID = m_LastItemIDs[i];
            bool bFound = false;

            for (int32 j = 0; j < CurrentCount; ++j)
            {
                if (m_Serializer->GetItemReplicationID(j) == OldID)
                {
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                FFastArrayChange Change;
                Change.Type = FFastArrayChange::EChangeType::Removed;
                Change.Index = static_cast<int32>(i);
                Change.ReplicationID = OldID;
                OutChanges.push_back(Change);
            }
        }

        // Detect additions and modifications
        for (int32 i = 0; i < CurrentCount; ++i)
        {
            int32 CurrentID = m_Serializer->GetItemReplicationID(i);
            int32 CurrentKey = m_Serializer->GetItemReplicationKey(i);

            // Check if this item existed before
            bool bIsNew = true;
            bool bIsModified = false;

            for (size_t j = 0; j < m_LastItemIDs.size(); ++j)
            {
                if (m_LastItemIDs[j] == CurrentID)
                {
                    bIsNew = false;

                    // Check if key changed (item was modified)
                    if (j < m_LastItemKeys.size() && m_LastItemKeys[j] != CurrentKey)
                    {
                        bIsModified = true;
                    }
                    break;
                }
            }

            if (bIsNew)
            {
                FFastArrayChange Change;
                Change.Type = FFastArrayChange::EChangeType::Added;
                Change.Index = i;
                Change.ReplicationID = CurrentID;
                OutChanges.push_back(Change);
            }
            else if (bIsModified)
            {
                FFastArrayChange Change;
                Change.Type = FFastArrayChange::EChangeType::Modified;
                Change.Index = i;
                Change.ReplicationID = CurrentID;
                OutChanges.push_back(Change);
            }
        }

        // Update cached state
        m_LastArrayReplicationKey = CurrentArrayKey;
        m_LastItemCount = CurrentCount;

        m_LastItemIDs.clear();
        m_LastItemKeys.clear();
        m_LastItemIDs.reserve(CurrentCount);
        m_LastItemKeys.reserve(CurrentCount);

        for (int32 i = 0; i < CurrentCount; ++i)
        {
            m_LastItemIDs.push_back(m_Serializer->GetItemReplicationID(i));
            m_LastItemKeys.push_back(m_Serializer->GetItemReplicationKey(i));
        }

        return static_cast<int32>(OutChanges.size());
    }

    void FFastArrayChangeDetector::Reset()
    {
        if (!m_Serializer)
            return;

        m_LastArrayReplicationKey = m_Serializer->GetArrayReplicationKey();
        m_LastItemCount = m_Serializer->Num();

        m_LastItemIDs.clear();
        m_LastItemKeys.clear();
        m_LastItemIDs.reserve(m_LastItemCount);
        m_LastItemKeys.reserve(m_LastItemCount);

        for (int32 i = 0; i < m_LastItemCount; ++i)
        {
            m_LastItemIDs.push_back(m_Serializer->GetItemReplicationID(i));
            m_LastItemKeys.push_back(m_Serializer->GetItemReplicationKey(i));
        }
    }

}