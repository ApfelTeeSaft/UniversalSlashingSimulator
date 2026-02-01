/**
 * UniversalSlashingSimulator - Inventory Manager
 *
 * Manages player inventory, quickbars, and item operations.
 * Handles STW-specific inventory logic like durability, crafting, etc.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include "InventoryTypes.h"
#include <unordered_map>
#include <functional>
#include <memory>

namespace USS
{
    // Inventory event callback
    using FInventoryEventCallback = std::function<void(const FInventoryChangeEvent&)>;

    class FInventoryManager
    {
    public:
        FInventoryManager();
        ~FInventoryManager();

        USS_NON_COPYABLE(FInventoryManager)
        USS_NON_MOVABLE(FInventoryManager)

        // Lifecycle
        EResult Initialize(void* PlayerController);
        void Shutdown();
        void Update();

        // Item queries
        const FInventoryItem* GetItem(const std::string& ItemId) const;
        const FInventoryItem* GetItemBySlot(int32 SlotIndex) const;
        std::vector<const FInventoryItem*> GetItemsByCategory(EItemCategory Category) const;
        int32 GetItemCount(const std::string& TemplateId) const;
        bool HasItem(const std::string& TemplateId, int32 MinCount = 1) const;

        // Item operations
        EResult AddItem(const FInventoryItem& Item);
        EResult RemoveItem(const std::string& ItemId, int32 Count = 1);
        EResult MoveItem(const std::string& ItemId, int32 NewSlot);
        EResult StackItems(const std::string& SourceId, const std::string& TargetId);

        // Resource shortcuts
        int32 GetWoodCount() const;
        int32 GetStoneCount() const;
        int32 GetMetalCount() const;
        EResult ConsumeResources(EResourceType Type, int32 Amount);

        // Quickbar
        const FQuickbar* GetQuickbar(int32 Index) const;
        int32 GetCurrentQuickbarSlot(int32 QuickbarIndex) const;
        EResult SetQuickbarSlot(int32 QuickbarIndex, int32 SlotIndex, const std::string& ItemId);
        EResult ClearQuickbarSlot(int32 QuickbarIndex, int32 SlotIndex);
        EResult SelectQuickbarSlot(int32 QuickbarIndex, int32 SlotIndex);

        // Equipment
        const FInventoryItem* GetEquippedWeapon() const;
        const FInventoryItem* GetEquippedPickaxe() const;
        EResult EquipItem(const std::string& ItemId);
        EResult UnequipItem(const std::string& ItemId);
        EResult SwapWeaponSlots(int32 SlotA, int32 SlotB);

        // Durability (STW-specific)
        float GetItemDurability(const std::string& ItemId) const;
        EResult UseItemDurability(const std::string& ItemId, float Amount);
        EResult RepairItem(const std::string& ItemId);
        bool IsItemBroken(const std::string& ItemId) const;

        // Crafting (STW-specific)
        bool CanCraftItem(const std::string& SchematicId) const;
        EResult CraftItem(const std::string& SchematicId, int32 Count = 1);
        std::vector<FCraftingRecipe> GetAvailableRecipes() const;

        // Ammo
        int32 GetAmmoCount(const std::string& AmmoType) const;
        EResult ConsumeAmmo(const std::string& AmmoType, int32 Amount);
        EResult ReloadWeapon(const std::string& WeaponId);

        // Inventory limits
        int32 GetMaxInventorySlots() const { return m_MaxSlots; }
        int32 GetUsedSlots() const { return static_cast<int32>(m_Items.size()); }
        int32 GetFreeSlots() const { return m_MaxSlots - GetUsedSlots(); }
        bool HasFreeSlot() const { return GetFreeSlots() > 0; }

        // Events
        void RegisterEventCallback(FInventoryEventCallback Callback);
        void OnProcessEvent(void* Object, void* Function, void* Params);

        // Sync with engine
        void SyncFromEngine();
        void SyncToEngine();

    private:
        void NotifyChange(const FInventoryChangeEvent& Event);
        int32 FindFreeSlot() const;
        std::string GenerateItemId() const;
        const FInventoryItem* FindResourceItem(EResourceType Type) const;

        // Items by ID
        std::unordered_map<std::string, FInventoryItem> m_Items;

        // Slot mapping
        std::unordered_map<int32, std::string> m_SlotToItem;

        // Quickbars (0 = primary/weapons, 1 = secondary/build)
        FQuickbar m_Quickbars[2];

        // Currently equipped
        std::string m_EquippedWeaponId;
        std::string m_EquippedPickaxeId;

        // Limits
        int32 m_MaxSlots;
        int32 m_MaxStackSize;

        // Engine references
        UObjectWrapper m_PlayerController;
        UObjectWrapper m_InventoryComponent;  // UFortInventory*
        UObjectWrapper m_QuickbarComponent;   // UFortQuickBars*

        // Callbacks
        std::vector<FInventoryEventCallback> m_EventCallbacks;

        // ID counter for generated IDs
        mutable uint32 m_ItemIdCounter;
    };

    // Global inventory access (for current local player)
    FInventoryManager& GetLocalInventoryManager();

}