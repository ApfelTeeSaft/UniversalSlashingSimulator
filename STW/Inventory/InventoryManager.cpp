/**
 * UniversalSlashingSimulator - Inventory Manager Implementation
 */

#include "InventoryManager.h"
#include "../../Core/Logging/Log.h"
#include "../../Engine/EngineCore.h"

namespace USS
{
    static FInventoryManager g_LocalInventoryManager;

    FInventoryManager& GetLocalInventoryManager()
    {
        return g_LocalInventoryManager;
    }

    FInventoryManager::FInventoryManager()
        : m_MaxSlots(200)
        , m_MaxStackSize(999)
        , m_ItemIdCounter(0)
    {
    }

    FInventoryManager::~FInventoryManager()
    {
        Shutdown();
    }

    EResult FInventoryManager::Initialize(void* PlayerController)
    {
        USS_LOG("Initializing Inventory Manager...");

        m_PlayerController = UObjectWrapper(PlayerController);

        // Initialize quickbars
        for (int32 i = 0; i < 2; ++i)
        {
            m_Quickbars[i].QuickbarIndex = i;
            m_Quickbars[i].CurrentSlot = 0;
            m_Quickbars[i].Slots.resize(i == 0 ? 6 : 4);  // 6 weapon slots, 4 build slots

            for (size_t j = 0; j < m_Quickbars[i].Slots.size(); ++j)
            {
                m_Quickbars[i].Slots[j].SlotIndex = static_cast<int32>(j);
                m_Quickbars[i].Slots[j].bIsEmpty = true;
            }
        }

        // Find inventory components
        // TODO: Traverse to find UFortInventory on controller

        USS_LOG("Inventory Manager initialized");
        return EResult::Success;
    }

    void FInventoryManager::Shutdown()
    {
        m_Items.clear();
        m_SlotToItem.clear();
        m_EventCallbacks.clear();

        m_PlayerController = UObjectWrapper();
        m_InventoryComponent = UObjectWrapper();
        m_QuickbarComponent = UObjectWrapper();
    }

    void FInventoryManager::Update()
    {
        // Sync inventory state from engine if needed
    }

    const FInventoryItem* FInventoryManager::GetItem(const std::string& ItemId) const
    {
        auto It = m_Items.find(ItemId);
        return (It != m_Items.end()) ? &It->second : nullptr;
    }

    const FInventoryItem* FInventoryManager::GetItemBySlot(int32 SlotIndex) const
    {
        auto It = m_SlotToItem.find(SlotIndex);
        if (It != m_SlotToItem.end())
        {
            return GetItem(It->second);
        }
        return nullptr;
    }

    std::vector<const FInventoryItem*> FInventoryManager::GetItemsByCategory(EItemCategory Category) const
    {
        std::vector<const FInventoryItem*> Result;

        for (const auto& Pair : m_Items)
        {
            if (Pair.second.Category == Category)
            {
                Result.push_back(&Pair.second);
            }
        }

        return Result;
    }

    int32 FInventoryManager::GetItemCount(const std::string& TemplateId) const
    {
        int32 Total = 0;

        for (const auto& Pair : m_Items)
        {
            if (Pair.second.TemplateId == TemplateId)
            {
                Total += Pair.second.Count;
            }
        }

        return Total;
    }

    bool FInventoryManager::HasItem(const std::string& TemplateId, int32 MinCount) const
    {
        return GetItemCount(TemplateId) >= MinCount;
    }

    EResult FInventoryManager::AddItem(const FInventoryItem& Item)
    {
        // Check if we can stack with existing
        for (auto& Pair : m_Items)
        {
            if (Pair.second.TemplateId == Item.TemplateId &&
                Pair.second.Count < Pair.second.MaxStackSize)
            {
                int32 CanAdd = Pair.second.MaxStackSize - Pair.second.Count;
                int32 ToAdd = (Item.Count < CanAdd) ? Item.Count : CanAdd;

                Pair.second.Count += ToAdd;

                FInventoryChangeEvent Event;
                Event.Type = FInventoryChangeEvent::EChangeType::Modified;
                Event.ItemId = Pair.first;
                Event.OldCount = Pair.second.Count - ToAdd;
                Event.NewCount = Pair.second.Count;
                NotifyChange(Event);

                USS_LOG("Stacked %d %s (total: %d)", ToAdd, Item.ItemName.c_str(), Pair.second.Count);

                // All added by stacking
                if (ToAdd >= Item.Count)
                    return EResult::Success;

                // Need to create new stack for remainder
                FInventoryItem Remainder = Item;
                Remainder.Count = Item.Count - ToAdd;
                return AddItem(Remainder);
            }
        }

        // Need new slot
        int32 FreeSlot = FindFreeSlot();
        if (FreeSlot < 0)
        {
            USS_WARN("Inventory full, cannot add item: %s", Item.ItemName.c_str());
            return EResult::InventoryFull;
        }

        // Create new item
        FInventoryItem NewItem = Item;
        NewItem.ItemId = GenerateItemId();
        NewItem.SlotIndex = FreeSlot;

        m_Items[NewItem.ItemId] = NewItem;
        m_SlotToItem[FreeSlot] = NewItem.ItemId;

        FInventoryChangeEvent Event;
        Event.Type = FInventoryChangeEvent::EChangeType::Added;
        Event.ItemId = NewItem.ItemId;
        Event.OldCount = 0;
        Event.NewCount = NewItem.Count;
        Event.SlotIndex = FreeSlot;
        NotifyChange(Event);

        USS_LOG("Added item: %s x%d (slot %d)", NewItem.ItemName.c_str(), NewItem.Count, FreeSlot);

        return EResult::Success;
    }

    EResult FInventoryManager::RemoveItem(const std::string& ItemId, int32 Count)
    {
        auto It = m_Items.find(ItemId);
        if (It == m_Items.end())
        {
            return EResult::ItemNotFound;
        }

        FInventoryItem& Item = It->second;
        int32 OldCount = Item.Count;

        if (Count >= Item.Count)
        {
            // Remove entirely
            int32 Slot = Item.SlotIndex;
            m_SlotToItem.erase(Slot);
            m_Items.erase(It);

            FInventoryChangeEvent Event;
            Event.Type = FInventoryChangeEvent::EChangeType::Removed;
            Event.ItemId = ItemId;
            Event.OldCount = OldCount;
            Event.NewCount = 0;
            Event.SlotIndex = Slot;
            NotifyChange(Event);

            USS_LOG("Removed item: %s", ItemId.c_str());
        }
        else
        {
            // Reduce count
            Item.Count -= Count;

            FInventoryChangeEvent Event;
            Event.Type = FInventoryChangeEvent::EChangeType::Modified;
            Event.ItemId = ItemId;
            Event.OldCount = OldCount;
            Event.NewCount = Item.Count;
            NotifyChange(Event);

            USS_LOG("Reduced item %s: %d -> %d", ItemId.c_str(), OldCount, Item.Count);
        }

        return EResult::Success;
    }

    EResult FInventoryManager::MoveItem(const std::string& ItemId, int32 NewSlot)
    {
        auto It = m_Items.find(ItemId);
        if (It == m_Items.end())
            return EResult::ItemNotFound;

        if (NewSlot < 0 || NewSlot >= m_MaxSlots)
            return EResult::InvalidParameter;

        FInventoryItem& Item = It->second;
        int32 OldSlot = Item.SlotIndex;

        // Check if target slot is occupied
        auto TargetIt = m_SlotToItem.find(NewSlot);
        if (TargetIt != m_SlotToItem.end())
        {
            // Swap items
            const std::string& TargetItemId = TargetIt->second;
            auto TargetItemIt = m_Items.find(TargetItemId);

            if (TargetItemIt != m_Items.end())
            {
                TargetItemIt->second.SlotIndex = OldSlot;
                m_SlotToItem[OldSlot] = TargetItemId;
            }
        }
        else
        {
            m_SlotToItem.erase(OldSlot);
        }

        Item.SlotIndex = NewSlot;
        m_SlotToItem[NewSlot] = ItemId;

        USS_LOG("Moved item %s: slot %d -> %d", ItemId.c_str(), OldSlot, NewSlot);

        return EResult::Success;
    }

    EResult FInventoryManager::StackItems(const std::string& SourceId, const std::string& TargetId)
    {
        auto SourceIt = m_Items.find(SourceId);
        auto TargetIt = m_Items.find(TargetId);

        if (SourceIt == m_Items.end() || TargetIt == m_Items.end())
            return EResult::ItemNotFound;

        FInventoryItem& Source = SourceIt->second;
        FInventoryItem& Target = TargetIt->second;

        if (Source.TemplateId != Target.TemplateId)
            return EResult::InvalidParameter;

        int32 CanAdd = Target.MaxStackSize - Target.Count;
        int32 ToAdd = (Source.Count < CanAdd) ? Source.Count : CanAdd;

        Target.Count += ToAdd;
        Source.Count -= ToAdd;

        if (Source.Count <= 0)
        {
            m_SlotToItem.erase(Source.SlotIndex);
            m_Items.erase(SourceIt);
        }

        return EResult::Success;
    }

    int32 FInventoryManager::GetWoodCount() const
    {
        const FInventoryItem* Item = FindResourceItem(EResourceType::Wood);
        return Item ? Item->Count : 0;
    }

    int32 FInventoryManager::GetStoneCount() const
    {
        const FInventoryItem* Item = FindResourceItem(EResourceType::Stone);
        return Item ? Item->Count : 0;
    }

    int32 FInventoryManager::GetMetalCount() const
    {
        const FInventoryItem* Item = FindResourceItem(EResourceType::Metal);
        return Item ? Item->Count : 0;
    }

    EResult FInventoryManager::ConsumeResources(EResourceType Type, int32 Amount)
    {
        // Find resource item
        for (auto& Pair : m_Items)
        {
            if (Pair.second.Category == EItemCategory::Resource)
            {
                // TODO: Check actual resource type matches
                if (Pair.second.Count >= Amount)
                {
                    return RemoveItem(Pair.first, Amount);
                }
            }
        }

        return EResult::InsufficientResources;
    }

    const FQuickbar* FInventoryManager::GetQuickbar(int32 Index) const
    {
        if (Index >= 0 && Index < 2)
        {
            return &m_Quickbars[Index];
        }
        return nullptr;
    }

    int32 FInventoryManager::GetCurrentQuickbarSlot(int32 QuickbarIndex) const
    {
        if (QuickbarIndex >= 0 && QuickbarIndex < 2)
        {
            return m_Quickbars[QuickbarIndex].CurrentSlot;
        }
        return -1;
    }

    EResult FInventoryManager::SetQuickbarSlot(int32 QuickbarIndex, int32 SlotIndex, const std::string& ItemId)
    {
        if (QuickbarIndex < 0 || QuickbarIndex >= 2)
            return EResult::InvalidParameter;

        FQuickbar& Quickbar = m_Quickbars[QuickbarIndex];

        if (SlotIndex < 0 || SlotIndex >= static_cast<int32>(Quickbar.Slots.size()))
            return EResult::InvalidParameter;

        Quickbar.Slots[SlotIndex].ItemId = ItemId;
        Quickbar.Slots[SlotIndex].bIsEmpty = ItemId.empty();

        return EResult::Success;
    }

    EResult FInventoryManager::ClearQuickbarSlot(int32 QuickbarIndex, int32 SlotIndex)
    {
        return SetQuickbarSlot(QuickbarIndex, SlotIndex, "");
    }

    EResult FInventoryManager::SelectQuickbarSlot(int32 QuickbarIndex, int32 SlotIndex)
    {
        if (QuickbarIndex < 0 || QuickbarIndex >= 2)
            return EResult::InvalidParameter;

        FQuickbar& Quickbar = m_Quickbars[QuickbarIndex];

        if (SlotIndex < 0 || SlotIndex >= static_cast<int32>(Quickbar.Slots.size()))
            return EResult::InvalidParameter;

        Quickbar.CurrentSlot = SlotIndex;

        USS_LOG("Selected quickbar %d slot %d", QuickbarIndex, SlotIndex);

        return EResult::Success;
    }

    const FInventoryItem* FInventoryManager::GetEquippedWeapon() const
    {
        return GetItem(m_EquippedWeaponId);
    }

    const FInventoryItem* FInventoryManager::GetEquippedPickaxe() const
    {
        return GetItem(m_EquippedPickaxeId);
    }

    EResult FInventoryManager::EquipItem(const std::string& ItemId)
    {
        auto It = m_Items.find(ItemId);
        if (It == m_Items.end())
            return EResult::ItemNotFound;

        FInventoryItem& Item = It->second;

        if (Item.Category == EItemCategory::Weapon || Item.Category == EItemCategory::Melee)
        {
            m_EquippedWeaponId = ItemId;
            Item.bIsEquipped = true;

            FInventoryChangeEvent Event;
            Event.Type = FInventoryChangeEvent::EChangeType::Equipped;
            Event.ItemId = ItemId;
            NotifyChange(Event);

            USS_LOG("Equipped weapon: %s", Item.ItemName.c_str());
        }

        return EResult::Success;
    }

    EResult FInventoryManager::UnequipItem(const std::string& ItemId)
    {
        auto It = m_Items.find(ItemId);
        if (It == m_Items.end())
            return EResult::ItemNotFound;

        FInventoryItem& Item = It->second;
        Item.bIsEquipped = false;

        if (m_EquippedWeaponId == ItemId)
        {
            m_EquippedWeaponId.clear();
        }

        FInventoryChangeEvent Event;
        Event.Type = FInventoryChangeEvent::EChangeType::Unequipped;
        Event.ItemId = ItemId;
        NotifyChange(Event);

        return EResult::Success;
    }

    EResult FInventoryManager::SwapWeaponSlots(int32 SlotA, int32 SlotB)
    {
        // Swap in primary quickbar
        if (SlotA < 0 || SlotB < 0)
            return EResult::InvalidParameter;

        FQuickbar& Primary = m_Quickbars[0];

        if (SlotA >= static_cast<int32>(Primary.Slots.size()) ||
            SlotB >= static_cast<int32>(Primary.Slots.size()))
            return EResult::InvalidParameter;

        std::swap(Primary.Slots[SlotA], Primary.Slots[SlotB]);
        Primary.Slots[SlotA].SlotIndex = SlotA;
        Primary.Slots[SlotB].SlotIndex = SlotB;

        return EResult::Success;
    }

    float FInventoryManager::GetItemDurability(const std::string& ItemId) const
    {
        const FInventoryItem* Item = GetItem(ItemId);
        return Item ? Item->Durability : 0.0f;
    }

    EResult FInventoryManager::UseItemDurability(const std::string& ItemId, float Amount)
    {
        auto It = m_Items.find(ItemId);
        if (It == m_Items.end())
            return EResult::ItemNotFound;

        FInventoryItem& Item = It->second;
        Item.Durability -= Amount;

        if (Item.Durability < 0.0f)
            Item.Durability = 0.0f;

        if (Item.Durability <= 0.0f)
        {
            USS_LOG("Item broken: %s", Item.ItemName.c_str());
        }

        return EResult::Success;
    }

    EResult FInventoryManager::RepairItem(const std::string& ItemId)
    {
        auto It = m_Items.find(ItemId);
        if (It == m_Items.end())
            return EResult::ItemNotFound;

        FInventoryItem& Item = It->second;
        Item.Durability = Item.MaxDurability;

        USS_LOG("Repaired item: %s", Item.ItemName.c_str());

        return EResult::Success;
    }

    bool FInventoryManager::IsItemBroken(const std::string& ItemId) const
    {
        const FInventoryItem* Item = GetItem(ItemId);
        return Item ? (Item->Durability <= 0.0f) : true;
    }

    bool FInventoryManager::CanCraftItem(const std::string& SchematicId) const
    {
        // TODO: Look up recipe and check if we have materials
        return false;
    }

    EResult FInventoryManager::CraftItem(const std::string& SchematicId, int32 Count)
    {
        if (!CanCraftItem(SchematicId))
        {
            return EResult::InsufficientResources;
        }

        // TODO: Consume materials and create item
        USS_LOG("Crafted item from schematic: %s x%d", SchematicId.c_str(), Count);

        return EResult::Success;
    }

    std::vector<FCraftingRecipe> FInventoryManager::GetAvailableRecipes() const
    {
        // TODO: Return recipes player can craft
        return {};
    }

    int32 FInventoryManager::GetAmmoCount(const std::string& AmmoType) const
    {
        return GetItemCount(AmmoType);
    }

    EResult FInventoryManager::ConsumeAmmo(const std::string& AmmoType, int32 Amount)
    {
        for (auto& Pair : m_Items)
        {
            if (Pair.second.Category == EItemCategory::Ammo &&
                Pair.second.TemplateId == AmmoType)
            {
                return RemoveItem(Pair.first, Amount);
            }
        }

        return EResult::ItemNotFound;
    }

    EResult FInventoryManager::ReloadWeapon(const std::string& WeaponId)
    {
        auto It = m_Items.find(WeaponId);
        if (It == m_Items.end())
            return EResult::ItemNotFound;

        FInventoryItem& Weapon = It->second;

        // TODO: Determine ammo type for weapon and consume from inventory
        Weapon.AmmoCount = Weapon.MaxAmmo;

        USS_LOG("Reloaded weapon: %s", Weapon.ItemName.c_str());

        return EResult::Success;
    }

    void FInventoryManager::RegisterEventCallback(FInventoryEventCallback Callback)
    {
        if (Callback)
        {
            m_EventCallbacks.push_back(std::move(Callback));
        }
    }

    void FInventoryManager::OnProcessEvent(void* Object, void* Function, void* Params)
    {
        // Handle inventory-related ProcessEvents
    }

    void FInventoryManager::SyncFromEngine()
    {
        // TODO: Read inventory state from engine UFortInventory
    }

    void FInventoryManager::SyncToEngine()
    {
        // TODO: Write inventory changes to engine UFortInventory
    }

    void FInventoryManager::NotifyChange(const FInventoryChangeEvent& Event)
    {
        for (const auto& Callback : m_EventCallbacks)
        {
            if (Callback)
            {
                Callback(Event);
            }
        }
    }

    int32 FInventoryManager::FindFreeSlot() const
    {
        for (int32 i = 0; i < m_MaxSlots; ++i)
        {
            if (m_SlotToItem.find(i) == m_SlotToItem.end())
            {
                return i;
            }
        }
        return -1;
    }

    std::string FInventoryManager::GenerateItemId() const
    {
        char Buffer[64];
        snprintf(Buffer, sizeof(Buffer), "item_%u", ++m_ItemIdCounter);
        return Buffer;
    }

    const FInventoryItem* FInventoryManager::FindResourceItem(EResourceType Type) const
    {
        // TODO: would check actual resource type
        for (const auto& Pair : m_Items)
        {
            if (Pair.second.Category == EItemCategory::Resource)
            {
                return &Pair.second;
            }
        }
        return nullptr;
    }

}