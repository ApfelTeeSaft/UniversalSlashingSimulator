/**
 * UniversalSlashingSimulator - STW Player Pawn
 *
 * Wrapper for AFortPlayerPawn with STW-specific functionality.
 * Handles hero abilities, stats, and player-specific game logic.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include <string>

namespace USS
{
    // Hero class types
    enum class EHeroClass : uint8
    {
        Soldier,
        Constructor,
        Ninja,
        Outlander
    };

    // Pawn state
    enum class EPawnState : uint8
    {
        None,
        Alive,
        DBNO,           // Down But Not Out
        Dead,
        Spectating
    };

    // Hero stats structure
    struct FHeroStats
    {
        float MaxHealth;
        float MaxShield;
        float HealthRegenRate;
        float ShieldRegenRate;
        float MoveSpeed;
        float SprintSpeed;
        float JumpHeight;
        float AbilityDamageMultiplier;
        float WeaponDamageMultiplier;
        float BuildingSpeedMultiplier;
        float HarvestingEfficiency;
        float TrapDamageMultiplier;

        FHeroStats()
            : MaxHealth(100.0f)
            , MaxShield(100.0f)
            , HealthRegenRate(0.0f)
            , ShieldRegenRate(5.0f)
            , MoveSpeed(600.0f)
            , SprintSpeed(850.0f)
            , JumpHeight(400.0f)
            , AbilityDamageMultiplier(1.0f)
            , WeaponDamageMultiplier(1.0f)
            , BuildingSpeedMultiplier(1.0f)
            , HarvestingEfficiency(1.0f)
            , TrapDamageMultiplier(1.0f)
        {}
    };

    // Hero ability info
    struct FAbilityInfo
    {
        std::string AbilityName;
        std::string AbilityClass;    // UClass name
        float Cooldown;
        float CurrentCooldown;
        int32 AbilityIndex;
        bool bIsActive;
        bool bIsOnCooldown;

        FAbilityInfo()
            : Cooldown(0.0f)
            , CurrentCooldown(0.0f)
            , AbilityIndex(-1)
            , bIsActive(false)
            , bIsOnCooldown(false)
        {}
    };

    // STW Player Pawn wrapper
    class FSTWPlayerPawn
    {
    public:
        FSTWPlayerPawn();
        explicit FSTWPlayerPawn(void* InPawn);
        ~FSTWPlayerPawn();

        USS_NON_COPYABLE(FSTWPlayerPawn)

        // Validity
        bool IsValid() const;
        void* GetNative() const { return m_Pawn.GetRaw(); }

        // Update
        void Update();

        // State
        EPawnState GetState() const { return m_State; }
        bool IsAlive() const { return m_State == EPawnState::Alive; }
        bool IsDBNO() const { return m_State == EPawnState::DBNO; }
        bool IsDead() const { return m_State == EPawnState::Dead; }

        // Health/Shield
        float GetHealth() const { return m_CurrentHealth; }
        float GetMaxHealth() const { return m_Stats.MaxHealth; }
        float GetHealthPercent() const;
        float GetShield() const { return m_CurrentShield; }
        float GetMaxShield() const { return m_Stats.MaxShield; }
        float GetShieldPercent() const;

        void SetHealth(float Health);
        void SetShield(float Shield);
        void ApplyDamage(float Damage, void* DamageCauser = nullptr);
        void Heal(float Amount);
        void AddShield(float Amount);

        // Hero info
        EHeroClass GetHeroClass() const { return m_HeroClass; }
        const std::string& GetHeroName() const { return m_HeroName; }
        const FHeroStats& GetStats() const { return m_Stats; }
        int32 GetPowerLevel() const { return m_PowerLevel; }

        void SetHeroClass(EHeroClass HeroClass);
        void SetPowerLevel(int32 Level);
        void ApplyHeroStats(const FHeroStats& Stats);

        // Abilities
        int32 GetAbilityCount() const { return 3; }  // STW heroes have 3 abilities
        const FAbilityInfo* GetAbility(int32 Index) const;
        bool CanActivateAbility(int32 Index) const;
        void ActivateAbility(int32 Index);
        void DeactivateAbility(int32 Index);
        void UpdateAbilityCooldowns(float DeltaTime);

        // Position/Movement
        void GetLocation(float& OutX, float& OutY, float& OutZ) const;
        void GetRotation(float& OutPitch, float& OutYaw, float& OutRoll) const;
        void SetLocation(float X, float Y, float Z);
        void SetRotation(float Pitch, float Yaw, float Roll);

        bool IsSprinting() const { return m_bIsSprinting; }
        bool IsJumping() const { return m_bIsJumping; }
        bool IsCrouching() const { return m_bIsCrouching; }

        // Combat
        void* GetCurrentWeapon() const;
        void EquipWeapon(int32 Slot);
        bool IsAiming() const { return m_bIsAiming; }
        bool IsFiring() const { return m_bIsFiring; }

        // DBNO (Down But Not Out) system
        float GetDBNOTimer() const { return m_DBNOTimer; }
        float GetDBNOMaxTime() const { return m_DBNOMaxTime; }
        void EnterDBNO();
        void ReviveFromDBNO(void* Reviver = nullptr);
        void Die();

        // Events
        void OnDamageReceived(float Damage, void* DamageCauser);
        void OnDeath(void* Killer);
        void OnRevived(void* Reviver);
        void OnProcessEvent(void* Function, void* Params);

    private:
        void UpdateFromNative();
        void UpdateState();

        UObjectWrapper m_Pawn;  // AFortPlayerPawn*

        // State
        EPawnState m_State;
        float m_CurrentHealth;
        float m_CurrentShield;

        // Hero info
        EHeroClass m_HeroClass;
        std::string m_HeroName;
        FHeroStats m_Stats;
        int32 m_PowerLevel;

        // Abilities
        FAbilityInfo m_Abilities[3];

        // Movement state
        bool m_bIsSprinting;
        bool m_bIsJumping;
        bool m_bIsCrouching;

        // Combat state
        bool m_bIsAiming;
        bool m_bIsFiring;

        // DBNO
        float m_DBNOTimer;
        float m_DBNOMaxTime;

        // Cached position (updated each frame)
        float m_LocationX, m_LocationY, m_LocationZ;
        float m_RotationPitch, m_RotationYaw, m_RotationRoll;
    };

}