/**
 * UniversalSlashingSimulator - STW Player Pawn Implementation
 */

#include "STWPlayerPawn.h"
#include "../../Core/Logging/Log.h"

namespace USS
{
    FSTWPlayerPawn::FSTWPlayerPawn()
        : m_State(EPawnState::None)
        , m_CurrentHealth(100.0f)
        , m_CurrentShield(0.0f)
        , m_HeroClass(EHeroClass::Soldier)
        , m_PowerLevel(1)
        , m_bIsSprinting(false)
        , m_bIsJumping(false)
        , m_bIsCrouching(false)
        , m_bIsAiming(false)
        , m_bIsFiring(false)
        , m_DBNOTimer(0.0f)
        , m_DBNOMaxTime(20.0f)
        , m_LocationX(0.0f)
        , m_LocationY(0.0f)
        , m_LocationZ(0.0f)
        , m_RotationPitch(0.0f)
        , m_RotationYaw(0.0f)
        , m_RotationRoll(0.0f)
    {
    }

    FSTWPlayerPawn::FSTWPlayerPawn(void* InPawn)
        : FSTWPlayerPawn()
    {
        m_Pawn = UObjectWrapper(InPawn);

        if (IsValid())
        {
            m_State = EPawnState::Alive;
            UpdateFromNative();
        }
    }

    FSTWPlayerPawn::~FSTWPlayerPawn()
    {
    }

    bool FSTWPlayerPawn::IsValid() const
    {
        return m_Pawn.IsValid();
    }

    void FSTWPlayerPawn::Update()
    {
        if (!IsValid())
            return;

        // Update cached values from engine
        UpdateFromNative();

        // Update ability cooldowns
        float DeltaTime = 1.0f / 30.0f;  // Placeholder
        UpdateAbilityCooldowns(DeltaTime);

        // Update DBNO timer
        if (m_State == EPawnState::DBNO)
        {
            m_DBNOTimer -= DeltaTime;
            if (m_DBNOTimer <= 0.0f)
            {
                Die();
            }
        }
    }

    float FSTWPlayerPawn::GetHealthPercent() const
    {
        if (m_Stats.MaxHealth <= 0.0f)
            return 0.0f;
        return m_CurrentHealth / m_Stats.MaxHealth;
    }

    float FSTWPlayerPawn::GetShieldPercent() const
    {
        if (m_Stats.MaxShield <= 0.0f)
            return 0.0f;
        return m_CurrentShield / m_Stats.MaxShield;
    }

    void FSTWPlayerPawn::SetHealth(float Health)
    {
        m_CurrentHealth = Health;

        if (m_CurrentHealth < 0.0f)
            m_CurrentHealth = 0.0f;
        if (m_CurrentHealth > m_Stats.MaxHealth)
            m_CurrentHealth = m_Stats.MaxHealth;

        UpdateState();
    }

    void FSTWPlayerPawn::SetShield(float Shield)
    {
        m_CurrentShield = Shield;

        if (m_CurrentShield < 0.0f)
            m_CurrentShield = 0.0f;
        if (m_CurrentShield > m_Stats.MaxShield)
            m_CurrentShield = m_Stats.MaxShield;
    }

    void FSTWPlayerPawn::ApplyDamage(float Damage, void* DamageCauser)
    {
        if (m_State != EPawnState::Alive)
            return;

        float RemainingDamage = Damage;

        // Shield absorbs first
        if (m_CurrentShield > 0.0f)
        {
            float ShieldDamage = (RemainingDamage < m_CurrentShield) ? RemainingDamage : m_CurrentShield;
            m_CurrentShield -= ShieldDamage;
            RemainingDamage -= ShieldDamage;
        }

        // Then health
        if (RemainingDamage > 0.0f)
        {
            m_CurrentHealth -= RemainingDamage;
        }

        OnDamageReceived(Damage, DamageCauser);
        UpdateState();
    }

    void FSTWPlayerPawn::Heal(float Amount)
    {
        if (m_State != EPawnState::Alive && m_State != EPawnState::DBNO)
            return;

        m_CurrentHealth += Amount;
        if (m_CurrentHealth > m_Stats.MaxHealth)
            m_CurrentHealth = m_Stats.MaxHealth;

        USS_LOG("Healed for %.1f, health: %.1f", Amount, m_CurrentHealth);
    }

    void FSTWPlayerPawn::AddShield(float Amount)
    {
        m_CurrentShield += Amount;
        if (m_CurrentShield > m_Stats.MaxShield)
            m_CurrentShield = m_Stats.MaxShield;

        USS_LOG("Added shield %.1f, total: %.1f", Amount, m_CurrentShield);
    }

    void FSTWPlayerPawn::SetHeroClass(EHeroClass HeroClass)
    {
        m_HeroClass = HeroClass;

        // Apply default stats based on hero class
        switch (HeroClass)
        {
        case EHeroClass::Soldier:
            m_Stats.WeaponDamageMultiplier = 1.1f;
            m_Stats.AbilityDamageMultiplier = 1.0f;
            break;

        case EHeroClass::Constructor:
            m_Stats.BuildingSpeedMultiplier = 1.2f;
            m_Stats.TrapDamageMultiplier = 1.1f;
            m_Stats.MaxHealth = 120.0f;
            break;

        case EHeroClass::Ninja:
            m_Stats.MoveSpeed = 660.0f;
            m_Stats.SprintSpeed = 935.0f;
            m_Stats.AbilityDamageMultiplier = 1.1f;
            m_Stats.MaxHealth = 80.0f;
            break;

        case EHeroClass::Outlander:
            m_Stats.HarvestingEfficiency = 1.24f;
            m_Stats.MoveSpeed = 620.0f;
            break;
        }
    }

    void FSTWPlayerPawn::SetPowerLevel(int32 Level)
    {
        m_PowerLevel = Level;
        // TODO: Scale stats based on power level
    }

    void FSTWPlayerPawn::ApplyHeroStats(const FHeroStats& Stats)
    {
        m_Stats = Stats;
    }

    const FAbilityInfo* FSTWPlayerPawn::GetAbility(int32 Index) const
    {
        if (Index >= 0 && Index < 3)
        {
            return &m_Abilities[Index];
        }
        return nullptr;
    }

    bool FSTWPlayerPawn::CanActivateAbility(int32 Index) const
    {
        if (Index < 0 || Index >= 3)
            return false;

        const FAbilityInfo& Ability = m_Abilities[Index];
        return !Ability.bIsActive && !Ability.bIsOnCooldown && IsAlive();
    }

    void FSTWPlayerPawn::ActivateAbility(int32 Index)
    {
        if (!CanActivateAbility(Index))
            return;

        FAbilityInfo& Ability = m_Abilities[Index];
        Ability.bIsActive = true;

        USS_LOG("Activated ability %d: %s", Index, Ability.AbilityName.c_str());

        // TODO: Actually trigger ability on engine side
    }

    void FSTWPlayerPawn::DeactivateAbility(int32 Index)
    {
        if (Index < 0 || Index >= 3)
            return;

        FAbilityInfo& Ability = m_Abilities[Index];

        if (Ability.bIsActive)
        {
            Ability.bIsActive = false;
            Ability.bIsOnCooldown = true;
            Ability.CurrentCooldown = Ability.Cooldown;

            USS_LOG("Deactivated ability %d, cooldown: %.1fs", Index, Ability.Cooldown);
        }
    }

    void FSTWPlayerPawn::UpdateAbilityCooldowns(float DeltaTime)
    {
        for (int32 i = 0; i < 3; ++i)
        {
            FAbilityInfo& Ability = m_Abilities[i];

            if (Ability.bIsOnCooldown)
            {
                Ability.CurrentCooldown -= DeltaTime;

                if (Ability.CurrentCooldown <= 0.0f)
                {
                    Ability.CurrentCooldown = 0.0f;
                    Ability.bIsOnCooldown = false;
                }
            }
        }
    }

    void FSTWPlayerPawn::GetLocation(float& OutX, float& OutY, float& OutZ) const
    {
        OutX = m_LocationX;
        OutY = m_LocationY;
        OutZ = m_LocationZ;
    }

    void FSTWPlayerPawn::GetRotation(float& OutPitch, float& OutYaw, float& OutRoll) const
    {
        OutPitch = m_RotationPitch;
        OutYaw = m_RotationYaw;
        OutRoll = m_RotationRoll;
    }

    void FSTWPlayerPawn::SetLocation(float X, float Y, float Z)
    {
        m_LocationX = X;
        m_LocationY = Y;
        m_LocationZ = Z;

        // TODO: Actually set location on engine pawn
    }

    void FSTWPlayerPawn::SetRotation(float Pitch, float Yaw, float Roll)
    {
        m_RotationPitch = Pitch;
        m_RotationYaw = Yaw;
        m_RotationRoll = Roll;

        // TODO: Actually set rotation on engine pawn
    }

    void* FSTWPlayerPawn::GetCurrentWeapon() const
    {
        // TODO: Read from pawn's CurrentWeapon
        return nullptr;
    }

    void FSTWPlayerPawn::EquipWeapon(int32 Slot)
    {
        USS_LOG("Equipping weapon slot %d", Slot);
        // TODO: Trigger weapon equip
    }

    void FSTWPlayerPawn::EnterDBNO()
    {
        if (m_State != EPawnState::Alive)
            return;

        m_State = EPawnState::DBNO;
        m_DBNOTimer = m_DBNOMaxTime;

        USS_LOG("Player entered DBNO state");
    }

    void FSTWPlayerPawn::ReviveFromDBNO(void* Reviver)
    {
        if (m_State != EPawnState::DBNO)
            return;

        m_State = EPawnState::Alive;
        m_CurrentHealth = m_Stats.MaxHealth * 0.3f;  // Revive with 30% health

        OnRevived(Reviver);
        USS_LOG("Player revived from DBNO");
    }

    void FSTWPlayerPawn::Die()
    {
        if (m_State == EPawnState::Dead)
            return;

        EPawnState OldState = m_State;
        m_State = EPawnState::Dead;
        m_CurrentHealth = 0.0f;

        OnDeath(nullptr);
        USS_LOG("Player died (was in state: %d)", static_cast<int>(OldState));
    }

    void FSTWPlayerPawn::OnDamageReceived(float Damage, void* DamageCauser)
    {
        USS_LOG("Damage received: %.1f, health: %.1f, shield: %.1f",
            Damage, m_CurrentHealth, m_CurrentShield);
    }

    void FSTWPlayerPawn::OnDeath(void* Killer)
    {
        USS_LOG("Player death event");
    }

    void FSTWPlayerPawn::OnRevived(void* Reviver)
    {
        USS_LOG("Player revived event");
    }

    void FSTWPlayerPawn::OnProcessEvent(void* Function, void* Params)
    {
        // Handle pawn-specific events
    }

    void FSTWPlayerPawn::UpdateFromNative()
    {
        if (!IsValid())
            return;

        // TODO: Read actual values from engine pawn
        // m_CurrentHealth = Read health from pawn
        // m_CurrentShield = Read shield from pawn
        // Read location/rotation
        // Read movement state
    }

    void FSTWPlayerPawn::UpdateState()
    {
        if (m_State == EPawnState::Dead)
            return;

        if (m_CurrentHealth <= 0.0f)
        {
            if (m_State == EPawnState::Alive)
            {
                // In STW, players go DBNO first
                EnterDBNO();
            }
            else if (m_State == EPawnState::DBNO)
            {
                Die();
            }
        }
    }

}