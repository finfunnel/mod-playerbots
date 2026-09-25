/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_SWPACTIONCONTEXT_H
#define PLAYERBOTS_SWPACTIONCONTEXT_H

#include "NamedObjectContext.h"
#include "SWPActions.h"

class RaidSunwellPlateauActionContext : public NamedObjectContext<Action>
{
public:
    RaidSunwellPlateauActionContext()
    {
        // Kalecgos
        creators["kalecgos spread from rift"] = &RaidSunwellPlateauActionContext::kalecgos_spread_from_rift;
        creators["kalecgos enter spectral rift"] = &RaidSunwellPlateauActionContext::kalecgos_enter_spectral_rift;
        creators["kalecgos attack sathrovarr"] = &RaidSunwellPlateauActionContext::kalecgos_attack_sathrovarr;
        creators["kalecgos balance realm dps"] = &RaidSunwellPlateauActionContext::kalecgos_balance_realm_dps;

        // Brutallus
        creators["brutallus burn spread"] = &RaidSunwellPlateauActionContext::brutallus_burn_spread;
        creators["brutallus tank swap"] = &RaidSunwellPlateauActionContext::brutallus_tank_swap;
        creators["brutallus soak position"] = &RaidSunwellPlateauActionContext::brutallus_soak_position;

        // Felmyst
        creators["felmyst encapsulate spread"] = &RaidSunwellPlateauActionContext::felmyst_encapsulate_spread;
        creators["felmyst vapor kite"] = &RaidSunwellPlateauActionContext::felmyst_vapor_kite;
        creators["felmyst vapor trail escape"] = &RaidSunwellPlateauActionContext::felmyst_vapor_trail_escape;
        creators["felmyst corrosion"] = &RaidSunwellPlateauActionContext::felmyst_corrosion;
        creators["felmyst gas nova self dispel"] = &RaidSunwellPlateauActionContext::felmyst_gas_nova_self_dispel;
        creators["felmyst deep breath avoid"] = &RaidSunwellPlateauActionContext::felmyst_deep_breath_avoid;
        creators["felmyst blazing dead pickup"] = &RaidSunwellPlateauActionContext::felmyst_blazing_dead_pickup;
        creators["felmyst blazing dead attack"] = &RaidSunwellPlateauActionContext::felmyst_blazing_dead_attack;
        creators["felmyst charmed ally"] = &RaidSunwellPlateauActionContext::felmyst_charmed_ally;

        // Eredar Twins
        creators["twins conflag spread"] = &RaidSunwellPlateauActionContext::twins_conflag_spread;
        creators["twins dispel pyrogenics"] = &RaidSunwellPlateauActionContext::twins_dispel_pyrogenics;
        creators["twins tank swap"] = &RaidSunwellPlateauActionContext::twins_tank_swap;
        creators["twins blaze escape"] = &RaidSunwellPlateauActionContext::twins_blaze_escape;

        // M'uru
        creators["muru dispel dark fiend"] = &RaidSunwellPlateauActionContext::muru_dispel_dark_fiend;
        creators["muru darkness escape"] = &RaidSunwellPlateauActionContext::muru_darkness_escape;
        creators["muru singularity escape"] = &RaidSunwellPlateauActionContext::muru_singularity_escape;
        creators["muru void sentinel pickup"] = &RaidSunwellPlateauActionContext::muru_void_sentinel_pickup;
        creators["muru interrupt fury mage"] = &RaidSunwellPlateauActionContext::muru_interrupt_fury_mage;

        // Kil'jaeden
        creators["kiljaeden fire bloom spread"] = &RaidSunwellPlateauActionContext::kiljaeden_fire_bloom_spread;
        creators["kiljaeden shadow spike spread"] = &RaidSunwellPlateauActionContext::kiljaeden_shadow_spike_spread;
        creators["kiljaeden armageddon escape"] = &RaidSunwellPlateauActionContext::kiljaeden_armageddon_escape;
        creators["kiljaeden shield orb"] = &RaidSunwellPlateauActionContext::kiljaeden_shield_orb;
        creators["kiljaeden felfire fiend"] = &RaidSunwellPlateauActionContext::kiljaeden_felfire_fiend;
        creators["kiljaeden click blue orb"] = &RaidSunwellPlateauActionContext::kiljaeden_click_blue_orb;
    }

private:
    static Action* kalecgos_spread_from_rift(PlayerbotAI* ai) { return new KalecgosSpreadFromRiftAction(ai); }
    static Action* kalecgos_enter_spectral_rift(PlayerbotAI* ai) { return new KalecgosEnterSpectralRiftAction(ai); }
    static Action* kalecgos_attack_sathrovarr(PlayerbotAI* ai) { return new KalecgosAttackSathrovarrAction(ai); }
    static Action* kalecgos_balance_realm_dps(PlayerbotAI* ai) { return new KalecgosBalanceRealmDpsAction(ai); }
    static Action* brutallus_burn_spread(PlayerbotAI* ai) { return new BrutallusBurnSpreadAction(ai); }
    static Action* brutallus_tank_swap(PlayerbotAI* ai) { return new BrutallusTankSwapAction(ai); }
    static Action* brutallus_soak_position(PlayerbotAI* ai) { return new BrutallusSoakPositionAction(ai); }
    static Action* felmyst_encapsulate_spread(PlayerbotAI* ai) { return new FelmystEncapsulateSpreadAction(ai); }
    static Action* felmyst_vapor_kite(PlayerbotAI* ai) { return new FelmystVaporKiteAction(ai); }
    static Action* felmyst_vapor_trail_escape(PlayerbotAI* ai) { return new FelmystVaporTrailEscapeAction(ai); }
    static Action* felmyst_corrosion(PlayerbotAI* ai) { return new FelmystCorrosionAction(ai); }
    static Action* felmyst_gas_nova_self_dispel(PlayerbotAI* ai) { return new FelmystGasNovaSelfDispelAction(ai); }
    static Action* felmyst_deep_breath_avoid(PlayerbotAI* ai) { return new FelmystDeepBreathAvoidAction(ai); }
    static Action* felmyst_blazing_dead_pickup(PlayerbotAI* ai) { return new FelmystBlazingDeadPickupAction(ai); }
    static Action* felmyst_blazing_dead_attack(PlayerbotAI* ai) { return new FelmystBlazingDeadAttackAction(ai); }
    static Action* felmyst_charmed_ally(PlayerbotAI* ai) { return new FelmystCharmedAllyAction(ai); }
    static Action* twins_conflag_spread(PlayerbotAI* ai) { return new TwinsConflagSpreadAction(ai); }
    static Action* twins_dispel_pyrogenics(PlayerbotAI* ai) { return new TwinsDispelPyrogenicsAction(ai); }
    static Action* twins_tank_swap(PlayerbotAI* ai) { return new TwinsTankSwapAction(ai); }
    static Action* twins_blaze_escape(PlayerbotAI* ai) { return new TwinsBlazeEscapeAction(ai); }
    static Action* muru_dispel_dark_fiend(PlayerbotAI* ai) { return new MuruDispelDarkFiendAction(ai); }
    static Action* muru_darkness_escape(PlayerbotAI* ai) { return new MuruDarknessEscapeAction(ai); }
    static Action* muru_singularity_escape(PlayerbotAI* ai) { return new MuruSingularityEscapeAction(ai); }
    static Action* muru_void_sentinel_pickup(PlayerbotAI* ai) { return new MuruVoidSentinelPickupAction(ai); }
    static Action* muru_interrupt_fury_mage(PlayerbotAI* ai) { return new MuruInterruptFuryMageAction(ai); }
    static Action* kiljaeden_fire_bloom_spread(PlayerbotAI* ai) { return new KiljaedenFireBloomSpreadAction(ai); }
    static Action* kiljaeden_shadow_spike_spread(PlayerbotAI* ai) { return new KiljaedenShadowSpikeSpreadAction(ai); }
    static Action* kiljaeden_armageddon_escape(PlayerbotAI* ai) { return new KiljaedenArmageddonEscapeAction(ai); }
    static Action* kiljaeden_shield_orb(PlayerbotAI* ai) { return new KiljaedenShieldOrbAction(ai); }
    static Action* kiljaeden_felfire_fiend(PlayerbotAI* ai) { return new KiljaedenFelfireFiendAction(ai); }
    static Action* kiljaeden_click_blue_orb(PlayerbotAI* ai) { return new KiljaedenClickBlueOrbAction(ai); }
};

#endif
