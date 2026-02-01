/**
 * UniversalSlashingSimulator - Fast Array Serializer Abstraction
 *
 * Provides version-agnostic handling of FFastArraySerializer, which changed
 * significantly at Fortnite 8.30 (around UE4.22).
 *
 * FFastArraySerializer is used for efficient replication of dynamic arrays
 * like inventory items, building pieces, and other game state.
 *
 * Pre-8.30 Layout:
 *   - Simple array with per-element dirty tracking
 *   - ReplicationID, ReplicationKey per item
 *
 * Post-8.30 Layout:
 *   - Restructured with ArrayReplicationKey
 *   - Different delta serialization format
 *   - IDCounter for unique element identification
 */

#pragma once

#include "../../Core/Common.h"
#include <vector>
#include <functional>

namespace USS
{
    /**
     * Fast array item base - common to all versions
     */
    struct FFastArrayItem
    {
        int32 ReplicationID;        // Unique identifier for this item
        int32 ReplicationKey;       // Change counter

        // Cached state
        bool bIsDirty;
        bool bIsNew;
        bool bIsRemoved;

        FFastArrayItem()
            : ReplicationID(-1)
            , ReplicationKey(-1)
            , bIsDirty(false)
            , bIsNew(false)
            , bIsRemoved(false)
        {}
    };

    /**
     * Fast array change event
     */
    struct FFastArrayChange
    {
        enum class EChangeType
        {
            None,
            Added,
            Modified,
            Removed,
            Reset,
        };

        EChangeType Type;
        int32 Index;
        int32 ReplicationID;

        FFastArrayChange()
            : Type(EChangeType::None)
            , Index(-1)
            , ReplicationID(-1)
        {}
    };

    /**
     * Callback for array changes
     */
    using FFastArrayChangeCallback = std::function<void(const FFastArrayChange&)>;

    /**
     * Fast array serializer interface
     *
     * This handles the version differences in how Fortnite
     * serializes and replicates dynamic arrays.
     */
    USS_INTERFACE IFastArraySerializer
    {
    public:
        virtual ~IFastArraySerializer() = default;

        /**
         * Initialize from a native FFastArraySerializer*
         * @param FastArrayPtr - Pointer to native struct
         * @param ItemSize - Size of each array element
         * @param ItemsOffset - Offset to Items array within struct
         */
        virtual EResult Initialize(void* FastArrayPtr, size_t ItemSize, int32 ItemsOffset) = 0;

        /**
         * Get number of items in array
         */
        virtual int32 Num() const = 0;

        /**
         * Get item by index
         * @param Index - Array index
         * @return Pointer to item data, or nullptr if invalid
         */
        virtual void* GetItem(int32 Index) const = 0;

        /**
         * Get item's replication ID
         */
        virtual int32 GetItemReplicationID(int32 Index) const = 0;

        /**
         * Get item's replication key
         */
        virtual int32 GetItemReplicationKey(int32 Index) const = 0;

        /**
         * Check if item is dirty (needs replication)
         */
        virtual bool IsItemDirty(int32 Index) const = 0;

        /**
         * Mark item as dirty
         */
        virtual void MarkItemDirty(int32 Index) = 0;

        /**
         * Mark all items as dirty
         */
        virtual void MarkAllDirty() = 0;

        /**
         * Get array replication key (post-8.30)
         */
        virtual int32 GetArrayReplicationKey() const = 0;

        /**
         * Get ID counter (post-8.30)
         */
        virtual int32 GetIDCounter() const = 0;

        /**
         * Increment array replication key
         */
        virtual void IncrementArrayReplicationKey() = 0;

        /**
         * Register callback for array changes
         */
        virtual void RegisterChangeCallback(FFastArrayChangeCallback Callback) = 0;

        /**
         * Check if using new serializer format (post-8.30)
         */
        virtual bool IsNewFormat() const = 0;

        /**
         * Check if initialized
         */
        virtual bool IsInitialized() const = 0;
    };

    /**
     * Pre-8.30 Fast Array Serializer
     *
     * Layout:
     * struct FFastArraySerializer {
     *     TArray<ItemType> Items;           // 0x00 - Standard TArray
     *     TMap<int32, int32> ItemMap;       // 0x10 - ReplicationID -> Index
     *     int32 IDCounter;                  // 0x60 - Next ID to assign
     * };
     *
     * Each item contains:
     *     int32 ReplicationID;
     *     int32 ReplicationKey;
     *     int32 MostRecentArrayReplicationKey;
     */
    class FLegacyFastArraySerializer : public IFastArraySerializer
    {
    public:
        FLegacyFastArraySerializer();
        ~FLegacyFastArraySerializer() override = default;

        EResult Initialize(void* FastArrayPtr, size_t ItemSize, int32 ItemsOffset) override;
        int32 Num() const override;
        void* GetItem(int32 Index) const override;
        int32 GetItemReplicationID(int32 Index) const override;
        int32 GetItemReplicationKey(int32 Index) const override;
        bool IsItemDirty(int32 Index) const override;
        void MarkItemDirty(int32 Index) override;
        void MarkAllDirty() override;
        int32 GetArrayReplicationKey() const override;
        int32 GetIDCounter() const override;
        void IncrementArrayReplicationKey() override;
        void RegisterChangeCallback(FFastArrayChangeCallback Callback) override;
        bool IsNewFormat() const override { return false; }
        bool IsInitialized() const override;

    private:
        void* m_FastArrayPtr;
        size_t m_ItemSize;
        int32 m_ItemsOffset;

        // Cached TArray info
        uintptr m_ItemsDataPtr;
        int32 m_ItemsNum;
        int32 m_ItemsMax;

        // Internal offsets within item
        static constexpr int32 ReplicationIDOffset = 0;
        static constexpr int32 ReplicationKeyOffset = 4;

        // Offset to IDCounter within FFastArraySerializer
        int32 m_IDCounterOffset;

        std::vector<FFastArrayChangeCallback> m_Callbacks;
        bool m_bInitialized;
    };

    /**
     * Post-8.30 Fast Array Serializer
     *
     * Layout (restructured):
     * struct FFastArraySerializer {
     *     TArray<ItemType> Items;                // 0x00
     *     FFastArraySerializerGuidReferences GuidReferencesMap;  // 0x10
     *     int32 ArrayReplicationKey;             // 0x68 (new!)
     *     int32 IDCounter;                       // 0x6C
     *     EFastArraySerializerDeltaFlags DeltaFlags;  // 0x70
     * };
     *
     * Each item now contains:
     *     FFastArraySerializerItem base:
     *         int32 ReplicationID;
     *         int32 ReplicationKey;
     *         int32 MostRecentArrayReplicationKey;
     */
    class FNewFastArraySerializer : public IFastArraySerializer
    {
    public:
        FNewFastArraySerializer();
        ~FNewFastArraySerializer() override = default;

        EResult Initialize(void* FastArrayPtr, size_t ItemSize, int32 ItemsOffset) override;
        int32 Num() const override;
        void* GetItem(int32 Index) const override;
        int32 GetItemReplicationID(int32 Index) const override;
        int32 GetItemReplicationKey(int32 Index) const override;
        bool IsItemDirty(int32 Index) const override;
        void MarkItemDirty(int32 Index) override;
        void MarkAllDirty() override;
        int32 GetArrayReplicationKey() const override;
        int32 GetIDCounter() const override;
        void IncrementArrayReplicationKey() override;
        void RegisterChangeCallback(FFastArrayChangeCallback Callback) override;
        bool IsNewFormat() const override { return true; }
        bool IsInitialized() const override;

    private:
        void NotifyChange(const FFastArrayChange& Change);

        void* m_FastArrayPtr;
        size_t m_ItemSize;
        int32 m_ItemsOffset;

        // Cached TArray info
        uintptr m_ItemsDataPtr;
        int32 m_ItemsNum;
        int32 m_ItemsMax;

        // Internal offsets within item
        static constexpr int32 ReplicationIDOffset = 0;
        static constexpr int32 ReplicationKeyOffset = 4;
        static constexpr int32 MostRecentArrayReplicationKeyOffset = 8;

        // Offsets within FFastArraySerializer (post-8.30)
        int32 m_ArrayReplicationKeyOffset;
        int32 m_IDCounterOffset;
        int32 m_DeltaFlagsOffset;

        int32 m_CachedArrayReplicationKey;

        std::vector<FFastArrayChangeCallback> m_Callbacks;
        bool m_bInitialized;
    };

    /**
     * Factory function to create appropriate serializer based on version
     */
    std::unique_ptr<IFastArraySerializer> CreateFastArraySerializer();

    /**
     * Helper to detect fast array changes between ticks
     */
    class FFastArrayChangeDetector
    {
    public:
        FFastArrayChangeDetector();

        /**
         * Initialize with a fast array serializer
         */
        EResult Initialize(IFastArraySerializer* Serializer);

        /**
         * Detect changes since last check
         * @param OutChanges - Output list of changes
         * @return Number of changes detected
         */
        int32 DetectChanges(std::vector<FFastArrayChange>& OutChanges);

        /**
         * Reset tracking state
         */
        void Reset();

    private:
        IFastArraySerializer* m_Serializer;
        int32 m_LastArrayReplicationKey;
        std::vector<int32> m_LastItemKeys;  // ReplicationKey per item
        std::vector<int32> m_LastItemIDs;   // ReplicationID per item
        int32 m_LastItemCount;
    };

}