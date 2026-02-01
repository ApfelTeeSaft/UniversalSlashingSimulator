/**
 * UniversalSlashingSimulator - Inventory Types
 *
 * Type definitions for STW inventory system.
 * Handles items, weapons, resources, schematics, etc.
 */

#pragma once

#include "../../Core/Common.h"
#include <string>
#include <vector>

// Forward declare types from BuildingTypes.h to avoid circular dependency
// Full definitions are in BuildingTypes.h - include that for ETrapType and FTrapStats

namespace USS
{
    // Item rarity (STW uses different names than BR)
    enum class EItemRarity : uint8
    {
        Common,         // Gray
        Uncommon,       // Green
        Rare,           // Blue
        Epic,           // Purple
        Legendary,      // Orange
        Mythic          // Gold (only for certain heroes/schematics)
    };

    // Item category
    enum class EItemCategory : uint8
    {
        None,
        Weapon,         // Ranged weapons
        Melee,          // Melee weapons
        Trap,           // Traps (floor, wall, ceiling)
        Resource,       // Wood, Stone, Metal
        Crafting,       // Crafting materials
        Ammo,           // Ammunition
        Consumable,     // Healing items, etc.
        Gadget,         // Adrenaline Rush, Turrets, etc.
        Hero,           // Hero characters
        Schematic,      // Weapon/trap schematics
        Survivor,       // Survivor squad members
        Defender,       // Defenders
        LootDrop        // Loot llamas, etc.
    };

    // Weapon type
    enum class EWeaponType : uint8
    {
        AssaultRifle,
        Shotgun,
        SMG,
        Pistol,
        Sniper,
        ExplosiveLauncher,
        Bow,
        Sword,
        Axe,
        Hammer,
        Spear,
        Scythe,
        Club,
        Hardware
    };

    // NOTE: ETrapType is defined in BuildingTypes.h
    // Include "../Building/BuildingTypes.h" if we need trap type definitions

    // Resource type
    enum class EResourceType : uint8
    {
        Wood,
        Stone,
        Metal,

        // Crafting tiers
        Copper,             // Tier 1
        Silver,             // Tier 2
        Malachite,          // Tier 3
        Obsidian,           // Tier 4
        Shadowshard,        // Tier 4 (alt)
        Brightcore,         // Tier 5
        Sunbeam,            // Tier 5 (alt)

        // Other crafting
        Twine,
        Rough,              // Rough Ore
        Mineral,            // Mineral Powder
        Mechanical,         // Mechanical Parts
        Duct,               // Duct Tape
        Bacon,              // Batteries, etc.
        Herb,
        Flower,
        Resin
    };

    // Item instance (runtime item in inventory)
    struct FInventoryItem
    {
        std::string ItemId;             // Unique ID
        std::string TemplateId;         // Item definition template
        std::string ItemName;           // Display name

        EItemCategory Category;
        EItemRarity Rarity;

        int32 Count;                    // Stack count
        int32 MaxStackSize;
        int32 Level;                    // Item level (power level)

        // Weapon-specific
        float Durability;               // Current durability
        float MaxDurability;
        int32 AmmoCount;                // Current ammo in clip
        int32 MaxAmmo;

        // Schematic-specific
        int32 SchematicLevel;           // Evolution level
        int32 SchematicTier;            // Material tier

        // Slot info
        int32 SlotIndex;
        bool bIsEquipped;
        bool bIsFavorite;

        FInventoryItem()
            : Category(EItemCategory::None)
            , Rarity(EItemRarity::Common)
            , Count(1)
            , MaxStackSize(1)
            , Level(1)
            , Durability(100.0f)
            , MaxDurability(100.0f)
            , AmmoCount(0)
            , MaxAmmo(0)
            , SchematicLevel(1)
            , SchematicTier(1)
            , SlotIndex(-1)
            , bIsEquipped(false)
            , bIsFavorite(false)
        {}
    };

    // Quickbar slot (weapon/ability slots)
    struct FQuickbarSlot
    {
        int32 SlotIndex;
        std::string ItemId;             // Linked inventory item
        bool bIsEmpty;
        bool bIsEnabled;

        FQuickbarSlot()
            : SlotIndex(-1)
            , bIsEmpty(true)
            , bIsEnabled(true)
        {}
    };

    // Quickbar (primary = weapons, secondary = build, etc.)
    struct FQuickbar
    {
        int32 QuickbarIndex;
        int32 CurrentSlot;
        std::vector<FQuickbarSlot> Slots;

        FQuickbar()
            : QuickbarIndex(-1)
            , CurrentSlot(0)
        {}
    };

    // Weapon stats (from schematic)
    struct FWeaponStats
    {
        float Damage;
        float FireRate;
        float ReloadTime;
        float MagazineSize;
        float Range;
        float CritChance;
        float CritDamage;
        float Impact;                   // Stagger/knockback
        float DurabilityPerUse;

        // Element
        std::string ElementType;        // Fire, Water, Nature, Physical, Energy
        float ElementDamagePercent;

        FWeaponStats()
            : Damage(0.0f)
            , FireRate(1.0f)
            , ReloadTime(1.0f)
            , MagazineSize(30)
            , Range(1000.0f)
            , CritChance(0.05f)
            , CritDamage(0.5f)
            , Impact(0.0f)
            , DurabilityPerUse(0.01f)
            , ElementDamagePercent(0.0f)
        {}
    };

    // NOTE: FTrapStats is defined in BuildingTypes.h
    // Include "../Building/BuildingTypes.h" if we need trap stats definitions

    // Crafting recipe
    struct FCraftingRecipe
    {
        std::string ResultTemplateId;
        int32 ResultCount;

        struct FIngredient
        {
            std::string TemplateId;
            int32 Count;
        };

        std::vector<FIngredient> Ingredients;

        FCraftingRecipe()
            : ResultCount(1)
        {}
    };

    // Loot drop definition
    struct FLootDrop
    {
        std::string LootId;
        std::string LootTableId;
        float DropChance;
        int32 MinCount;
        int32 MaxCount;
        EItemRarity MinRarity;
        EItemRarity MaxRarity;

        FLootDrop()
            : DropChance(1.0f)
            , MinCount(1)
            , MaxCount(1)
            , MinRarity(EItemRarity::Common)
            , MaxRarity(EItemRarity::Legendary)
        {}
    };

    // Inventory change event
    struct FInventoryChangeEvent
    {
        enum class EChangeType : uint8
        {
            Added,
            Removed,
            Modified,
            Equipped,
            Unequipped
        };

        EChangeType Type;
        std::string ItemId;
        int32 OldCount;
        int32 NewCount;
        int32 SlotIndex;

        FInventoryChangeEvent()
            : Type(EChangeType::Modified)
            , OldCount(0)
            , NewCount(0)
            , SlotIndex(-1)
        {}
    };

}