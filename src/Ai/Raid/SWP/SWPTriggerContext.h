/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_SWPTRIGGERCONTEXT_H
#define PLAYERBOTS_SWPTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "SWPTriggers.h"

class RaidSunwellPlateauTriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidSunwellPlateauTriggerContext()
    {
        // Kalecgos
        creators["kalecgos too close to rift"] = &RaidSunwellPlateauTriggerContext::kalecgos_too_close_to_rift;
        creators["kalecgos spectral rift available"] =
            &RaidSunwellPlateauTriggerContext::kalecgos_spectral_rift_available;
        creators["kalecgos in spectral realm"] = &RaidSunwellPlateauTriggerContext::kalecgos_in_spectral_realm;
        creators["kalecgos sathrovarr lagging"] =
            &RaidSunwellPlateauTriggerContext::kalecgos_realm_health_imbalance;
        creators["kalecgos dragon racing ahead"] =
            &RaidSunwellPlateauTriggerContext::kalecgos_dragon_racing_ahead;

        // Brutallus
        creators["brutallus burn on self"] = &RaidSunwellPlateauTriggerContext::brutallus_burn_on_self;
        creators["brutallus burn nearby"] = &RaidSunwellPlateauTriggerContext::brutallus_burn_nearby;
        creators["brutallus tank swap"] = &RaidSunwellPlateauTriggerContext::brutallus_tank_swap;
        creators["brutallus soak position"] = &RaidSunwellPlateauTriggerContext::brutallus_soak_position;

        // Felmyst
        creators["felmyst encapsulate nearby"] = &RaidSunwellPlateauTriggerContext::felmyst_encapsulate_nearby;
        creators["felmyst vapor chasing"] = &RaidSunwellPlateauTriggerContext::felmyst_vapor_chasing;
        creators["felmyst vapor trail nearby"] = &RaidSunwellPlateauTriggerContext::felmyst_vapor_trail_nearby;
        creators["felmyst corrosion"] = &RaidSunwellPlateauTriggerContext::felmyst_corrosion;
        creators["felmyst gas nova on self"] = &RaidSunwellPlateauTriggerContext::felmyst_gas_nova_on_self;
        creators["felmyst deep breath"] = &RaidSunwellPlateauTriggerContext::felmyst_deep_breath;
        creators["felmyst blazing dead nearby"] = &RaidSunwellPlateauTriggerContext::felmyst_blazing_dead_nearby;
        creators["felmyst charmed ally nearby"] = &RaidSunwellPlateauTriggerContext::felmyst_charmed_ally_nearby;

        // Eredar Twins
        creators["twins conflag nearby"] = &RaidSunwellPlateauTriggerContext::twins_conflag_nearby;
        creators["twins pyrogenics"] = &RaidSunwellPlateauTriggerContext::twins_pyrogenics;
        creators["twins tank swap"] = &RaidSunwellPlateauTriggerContext::twins_tank_swap;
        creators["twins blaze nearby"] = &RaidSunwellPlateauTriggerContext::twins_blaze_nearby;

        // M'uru
        creators["muru dark fiend nearby"] = &RaidSunwellPlateauTriggerContext::muru_dark_fiend_nearby;
        creators["muru darkness"] = &RaidSunwellPlateauTriggerContext::muru_darkness;
        creators["muru singularity nearby"] = &RaidSunwellPlateauTriggerContext::muru_singularity_nearby;
        creators["muru void sentinel spawn"] = &RaidSunwellPlateauTriggerContext::muru_void_sentinel_spawn;
        creators["muru fury mage interrupt"] = &RaidSunwellPlateauTriggerContext::muru_fury_mage_interrupt;

        // Kil'jaeden
        creators["kiljaeden fire bloom on self"] = &RaidSunwellPlateauTriggerContext::kiljaeden_fire_bloom_on_self;
        creators["kiljaeden fire bloom nearby"] = &RaidSunwellPlateauTriggerContext::kiljaeden_fire_bloom_nearby;
        creators["kiljaeden shadow spike nearby"] = &RaidSunwellPlateauTriggerContext::kiljaeden_shadow_spike_nearby;
        creators["kiljaeden armageddon nearby"] = &RaidSunwellPlateauTriggerContext::kiljaeden_armageddon_nearby;
        creators["kiljaeden shield orb"] = &RaidSunwellPlateauTriggerContext::kiljaeden_shield_orb;
        creators["kiljaeden felfire fiend"] = &RaidSunwellPlateauTriggerContext::kiljaeden_felfire_fiend;
        creators["kiljaeden blue orb available"] = &RaidSunwellPlateauTriggerContext::kiljaeden_blue_orb_available;
    }

private:
    static Trigger* kalecgos_too_close_to_rift(PlayerbotAI* ai) { return new KalecgosTooCloseToRiftTrigger(ai); }
    static Trigger* kalecgos_spectral_rift_available(PlayerbotAI* ai)
    {
        return new KalecgosSpectralRiftAvailableTrigger(ai);
    }
    static Trigger* kalecgos_in_spectral_realm(PlayerbotAI* ai) { return new KalecgosInSpectralRealmTrigger(ai); }
    static Trigger* kalecgos_realm_health_imbalance(PlayerbotAI* ai)
    {
        return new KalecgosSathrovarrLaggingTrigger(ai);
    }
    static Trigger* kalecgos_dragon_racing_ahead(PlayerbotAI* ai)
    {
        return new KalecgosDragonRacingAheadTrigger(ai);
    }
    static Trigger* brutallus_burn_on_self(PlayerbotAI* ai) { return new BrutallusBurnOnSelfTrigger(ai); }
    static Trigger* brutallus_burn_nearby(PlayerbotAI* ai) { return new BrutallusBurnNearbyTrigger(ai); }
    static Trigger* brutallus_tank_swap(PlayerbotAI* ai) { return new BrutallusTankSwapTrigger(ai); }
    static Trigger* brutallus_soak_position(PlayerbotAI* ai) { return new BrutallusSoakPositionTrigger(ai); }
    static Trigger* felmyst_encapsulate_nearby(PlayerbotAI* ai) { return new FelmystEncapsulateNearbyTrigger(ai); }
    static Trigger* felmyst_vapor_chasing(PlayerbotAI* ai) { return new FelmystVaporChasingTrigger(ai); }
    static Trigger* felmyst_vapor_trail_nearby(PlayerbotAI* ai) { return new FelmystVaporTrailNearbyTrigger(ai); }
    static Trigger* felmyst_corrosion(PlayerbotAI* ai) { return new FelmystCorrosionTrigger(ai); }
    static Trigger* felmyst_gas_nova_on_self(PlayerbotAI* ai) { return new FelmystGasNovaOnSelfTrigger(ai); }
    static Trigger* felmyst_deep_breath(PlayerbotAI* ai) { return new FelmystDeepBreathTrigger(ai); }
    static Trigger* felmyst_blazing_dead_nearby(PlayerbotAI* ai) { return new FelmystBlazingDeadNearbyTrigger(ai); }
    static Trigger* felmyst_charmed_ally_nearby(PlayerbotAI* ai) { return new FelmystCharmedAllyNearbyTrigger(ai); }
    static Trigger* twins_conflag_nearby(PlayerbotAI* ai) { return new TwinsConflagNearbyTrigger(ai); }
    static Trigger* twins_pyrogenics(PlayerbotAI* ai) { return new TwinsPyrogenicsTrigger(ai); }
    static Trigger* twins_tank_swap(PlayerbotAI* ai) { return new TwinsTankSwapTrigger(ai); }
    static Trigger* twins_blaze_nearby(PlayerbotAI* ai) { return new TwinsBlazeNearbyTrigger(ai); }
    static Trigger* muru_dark_fiend_nearby(PlayerbotAI* ai) { return new MuruDarkFiendNearbyTrigger(ai); }
    static Trigger* muru_darkness(PlayerbotAI* ai) { return new MuruDarknessTrigger(ai); }
    static Trigger* muru_singularity_nearby(PlayerbotAI* ai) { return new MuruSingularityNearbyTrigger(ai); }
    static Trigger* muru_void_sentinel_spawn(PlayerbotAI* ai) { return new MuruVoidSentinelSpawnTrigger(ai); }
    static Trigger* muru_fury_mage_interrupt(PlayerbotAI* ai) { return new MuruFuryMageInterruptTrigger(ai); }
    static Trigger* kiljaeden_fire_bloom_on_self(PlayerbotAI* ai) { return new KiljaedenFireBloomOnSelfTrigger(ai); }
    static Trigger* kiljaeden_fire_bloom_nearby(PlayerbotAI* ai) { return new KiljaedenFireBloomNearbyTrigger(ai); }
    static Trigger* kiljaeden_shadow_spike_nearby(PlayerbotAI* ai) { return new KiljaedenShadowSpikeNearbyTrigger(ai); }
    static Trigger* kiljaeden_armageddon_nearby(PlayerbotAI* ai) { return new KiljaedenArmageddonNearbyTrigger(ai); }
    static Trigger* kiljaeden_shield_orb(PlayerbotAI* ai) { return new KiljaedenShieldOrbTrigger(ai); }
    static Trigger* kiljaeden_felfire_fiend(PlayerbotAI* ai) { return new KiljaedenFelfireFiendTrigger(ai); }
    static Trigger* kiljaeden_blue_orb_available(PlayerbotAI* ai) { return new KiljaedenBlueOrbAvailableTrigger(ai); }
};

#endif
