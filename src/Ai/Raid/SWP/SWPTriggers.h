/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_SWPTRIGGERS_H
#define PLAYERBOTS_SWPTRIGGERS_H

#include "EncounterHelpers.h"
#include "Trigger.h"

/*
 * Sunwell Plateau - Kalecgos encounter (dragon realm Kalecgos / spectral realm Sathrovarr).
 * IDs verified against src/server/scripts/EasternKingdoms/SunwellPlateau/boss_kalecgos.cpp.
 * Note: Frost Breath (magic) and Boundless Agony (curse) dispels are already covered by the
 * default class "cure" strategies every bot gets from AiFactory; not duplicated here.
 */
enum SunwellKalecgosIDs
{
    NPC_KALECGOS_DRAGON                = 24850, // Kalecgos (outer realm, Z > 50)
    NPC_SATHROVARR                     = 24892, // Sathrovarr the Corruptor (spectral realm, Z < 50)

    SPELL_SPECTRAL_BLAST               = 44869, // Spectral Blast: teleports a random player, leaves a portal
    SPELL_ARCANE_BUFFET                = 45018, // Arcane Buffet: stacking raid debuff every 8s (outer realm)
    SPELL_CORRUPTION_STRIKE            = 45029, // Corruption Strike: spectral tank killer + knockdown
    SPELL_SPECTRAL_REALM               = 46021, // Spectral Realm: 60s aura while inside
    SPELL_SPECTRAL_EXHAUSTION          = 44867, // Spectral Exhaustion: cannot re-enter after returning

    GO_SPECTRAL_RIFT                   = 187055, // Spectral Rift portal (casts spell 44811 on use)

    SWP_MAP_ID                         = 580,   // Sunwell Plateau

    // User rule: the OUTER dragon burns to KALECGOS_BALANCE_THRESHOLD_PCT then holds
    // (only the tank keeps threat) while the INSIDE burns Sathrovarr down to the
    // banished/kill point (1%) - both die together. Healers/DPS outside stop at 30%.
    KALECGOS_BALANCE_THRESHOLD_PCT     = 30,    // user rule: outer burns to 30% then holds until Sathrovarr dies
    KALECGOS_MAX_INSIDE_FRACTION_NUM   = 1,     // inside >= alive/2
    KALECGOS_MAX_INSIDE_FRACTION_DEN   = 2,
    KALECGOS_MAX_INSIDE_HEALERS        = 3,     // rest stay out healing the dragon tanks
    KALECGOS_BLOODLUST_INSIDE_COUNT    = 10,    // user rule: outer shaman bloodlusts once 10+ are inside
};

/*
 * Brutallus (NPC 24882). IDs verified against boss_brutallus.cpp:
 * - Meteor Slash 45150: frontal share (~20000 split), stacking fire vuln, every 12s on the tank group.
 * - Burn 45141 -> damage aura 46394 on a random player, 60s, spreads within 3 yards.
 * - Stomp 45185: tank only, -50% armor 10s, removes Burn, every 30s.
 * - Berserk 26662 at 6 minutes (pure DPS check, no bot-side handling).
 */
enum SunwellBrutallusIDs
{
    NPC_BRUTALLUS                      = 24882,

    SPELL_METEOR_SLASH_SWP             = 45150,
    SPELL_BURN_DAMAGE                  = 46394,
    SPELL_BURN                         = 45141,
    SPELL_STOMP                        = 45185,

    BRUTALLUS_BURN_SPREAD_RANGE        = 3,    // guide: Burn spreads within 3 yards
    BRUTALLUS_SLASH_SWAP_STACKS        = 3,    // user: swap tanks at 3 slash stacks
    BRUTALLUS_SLASH_TOLERANCE          = 3     // soak position radius around the anchor tank
};

/*
 * Felmyst (NPC 25038). IDs verified against boss_felmyst.cpp:
 * - Ground phase: Corrosion 45866 (tank +100% dmg taken), Gas Nova 45855 (magic, dispellable),
 *   Encapsulate 45661 (random target + nearby players, target lifted up), Cleave 19983.
 * - Noxious Fumes 47002: room-wide aura, nothing to dodge.
 * - Flight phase: Demonic Vapor beams (45391) chase two players leaving trails (45402 dot);
 *   trails spawn skeletons (45400); Fog of Corruption breath (45582 -> charm 45717) covers
 *   one third of the field per pass.
 * - Berserk 45078 at 10 minutes.
 */
enum SunwellFelmystIDs
{
    NPC_FELMYST                        = 25038,
    NPC_DEMONIC_VAPOR                  = 25265, // green beam orb (AI npc_demonic_vapor), verified in creature_template.sql
    NPC_DEMONIC_VAPOR_TRAIL            = 25267, // trail cloud (AI npc_demonic_vapor_trail)
    NPC_BLAZING_DEAD                   = 25268, // skeleton (AI npc_demonic_vapor_trail spawns via 45400), boss_felmyst.cpp

    SPELL_FELMYST_CORROSION            = 45866, // tank debuff: +100% damage taken, magic (boss_felmyst.cpp)
    SPELL_FELMYST_GAS_NOVA             = 45855, // magic debuff: default cure strategies dispel it
    SPELL_FELMYST_ENCAPSULATE_CHANNEL  = 45661, // channeled on a random player
    SPELL_DEMONIC_VAPOR_DOT            = 45402, // standing in a trail applies this
    SPELL_FELMYST_SPEED_BURST          = 45495, // flight: cast on self WHILE sweeping the lane (the breath is active)
    SPELL_FELMYST_TRIGGER_TOP_STRAFE   = 45586, // flight: top-lane breath trigger (CorruptTriggers)
    SPELL_FELMYST_TRIGGER_MID_STRAFE   = 45622, // flight: middle-lane breath trigger
    SPELL_FELMYST_TRIGGER_BOT_STRAFE   = 45623, // flight: bottom-lane breath trigger
    SPELL_FELMYST_FOG_CORRUPTION       = 45582, // landing in the breath = charmAREA kill on removal
    SPELL_FELMYST_FOG_CHARM            = 45717, // charm aura applied to anyone inside the ground fog (SpellScript)

    FELMYST_ENCAPSULATE_SPREAD_RANGE   = 12,   // guide: players near the Encapsulate target must run out
    FELMYST_CORROSION_DISPEL_RANGE     = 40,   // healer/stealer range to the tank to remove Corrosion
    FELMYST_BLAZING_DEAD_PICKUP_RANGE  = 30,   // tank pickup radius for Blazing Dead skeletons
    FELMYST_POST_BREATH_FREEZE_MS      = 10000 // user: after a breath, stand still 10s (fog lingers)
};

/*
 * Eredar Twins: Lady Sacrolash (dark, tanked) / Grand Warlock Alythess (fire, tanked by warlock).
 * IDs verified against boss_eredar_twins.cpp:
 * - Conflagration 45342: 3s cast on a player, AoE around the target; the target must run out
 *   and everyone within 8 yards must move away (also the main way to clear Dark Touched).
 * - Pyrogenics 45230: +35% fire damage self buff on the fire sister - must be stolen/dispelled.
 * - Confounding Blow 45256: Sacrolash's tank killer, confuses the victim ~6s -> tank swap.
 * - No shared health: when one sister dies the other heals full and inherits her abilities
 *   (boss_eredar_twins.cpp JustDied/DoAction), so no balance logic is needed.
 */
enum SunwellTwinsIDs
{
    NPC_SACROLASH                      = 25165,
    NPC_ALYTHESS                       = 25166,

    SPELL_CONFLAGRATION_TWINS          = 45342,
    SPELL_PYROGENICS                   = 45230,
    SPELL_CONFOUNDING_BLOW             = 45256,

    GO_BLAZE_TWINS                     = 175566, // Blaze fire trap (spell 45235 -> summon 45236), verified in gameobject.sql

    TWINS_CONFLAG_SPREAD_RANGE         = 8,    // guide: Conflagration hits everyone within 8 yards of the target
    TWINS_BLAZE_ESCAPE_RANGE           = 3,    // guide: Blaze burns a small area, ~2-3 yards; run out when close
    TWINS_BLAZE_SEARCH_RANGE           = 12,   // scan radius around the bot for nearby Blaze traps
    TWINS_SEARCH_RANGE                 = 60
};

/*
 * M'uru / Entropius. IDs verified against boss_muru.cpp + sunwell_plateau.h + creature_template.sql:
 * - Dark Fiend 25744: spawns from Darkness, walks to a player; if it reaches its target it blasts
 *   the whole raid. Guide: the ONLY counter is dispelling it (priest Dispel Magic / shaman Purge).
 * - Darkness 25879: huge dark field, ~3k/s while inside and cannot be healed -> run out.
 * - Singularity 25855 (Entropius phase): chases players and knocks them back -> avoid.
 * - Void Sentinel 25772: spawns at the room walls every 30s, must be picked up by a tank at once.
 */
enum SunwellMuruIDs
{
    NPC_MURU                           = 25741,
    NPC_ENTROPIUS                      = 25840,
    NPC_DARK_FIEND_SWP                 = 25744, // creature_template.sql:19710 'Dark Fiend'
    NPC_VOID_SENTINEL_SWP              = 25772, // creature_template.sql:19738
    NPC_DARKNESS                       = 25879, // creature_template.sql:19844 'Darkness'
    NPC_SINGULARITY                    = 25855, // creature_template.sql:19820
    NPC_FURY_MAGE_SWP                  = 25799, // creature_template.sql:19764 'Shadowsword Fury Mage'
    NPC_SHADOWSWORD_BERSERKER          = 25798, // creature_template.sql:19763 'Shadowsword Berserker'

    SPELL_FEL_FIREBALL                 = 46101, // smart_scripts.sql:28657 'Cast Fel Fireball' (interrupt this)
    SPELL_FURY                         = 46102, // smart_scripts.sql:28658 'Cast Spell Fury' (mage self-buff, not interrupted)

    MURU_DARKNESS_ESCAPE_RANGE         = 15,
    MURU_SINGULARITY_ESCAPE_RANGE      = 10,
    MURU_FIEND_DISPEL_RANGE            = 30,
    MURU_SENTINEL_PICKUP_RANGE         = 30,
    MURU_FURY_MAGE_INTERRUPT_RANGE      = 30
};

/*
 * Kil'jaeden. IDs verified against boss_kiljaeden.cpp + sunwell_plateau.h + creature_template.sql:
 * - Fire Bloom 45641: on 5 random players; each carrier damages everyone within 10 yards -> spread.
 * - Shadow Spike 46680: drifts at a player, 8-yard explosion on arrival -> move away from carriers.
 * - Armageddon (meteor): impact point marked by creature 25735, ~5-yard blast -> move away.
 * - Shield Orb 25502: floating orb spamming shadow bolts, ~20k HP -> ranged burn it down fast.
 * - Volatile Felfire Fiend 25598 (P1 hands phase): rushes players and explodes -> kill on sight.
 *
 * Thousand Souls / Blue Dragon Orb (guidance vs boss_kiljaeden.cpp):
 * - Darkness of a Thousand Souls 46605: ~8s after the cast the whole raid takes the 45657 nuke.
 *   Counter per guide: a raider must use an Orb of the Blue Dragonflight to become a drake and
 *   shield the raid. In this script the damage is applied through a triggered spell; the exact
 *   DBC protection is not inspectable from C++, but clicking the orb is the only counter that
 *   matches the guide.
 * - To gain 45839 a player clicks an orb GO (187869/188114/188115/188116); EmpowerOrb in
 *   boss_kiljaeden.cpp makes one orb selectable at 85%/50% and all four at 25%. The drake form
 *   lasts 2 minutes. The boss script's `remove_if(UnitAuraCheck(true, 45839))` only filters
 *   Sinister Reflection / dragon breath, not the thousand-souls nuke.
 * - P1 hands phase spawns Volatile Felfire Fiends and Hand of the Deceiver adds (see above).
 *
 * Dedicated drake duty: bots whose GUID counter % KJ_DRAKE_DUTY_GROUP_SIZE == 0 click an orb
 * once they see a selectable one. Non-carriers never touch the orbs.
 */
enum SunwellKiljaedenIDs
{
    NPC_KILJAEDEN_SWP                  = 25315,
    NPC_SHIELD_ORB                     = 25502,
    NPC_VOLATILE_FELFIRE_FIEND         = 25598,
    NPC_ARMAGEDDON_TARGET              = 25735,
    NPC_HAND_OF_THE_DECEIVER_SWP       = 25588,

    GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_1  = 187869,
    GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_2  = 188114,
    GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_3  = 188115,
    GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_4  = 188116,

    SPELL_FIRE_BLOOM                   = 45641,
    SPELL_SHADOW_SPIKE                 = 46680,
    SPELL_DARKNESS_OF_A_THOUSAND_SOULS = 46605, // boss cast; 8s later the raid is nuked (45657)
    SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT = 45839, // Blue Dragonflight form from clicking an orb

    KJ_FIRE_BLOOM_SPREAD_RANGE         = 10,
    KJ_SHADOW_SPIKE_SPREAD_RANGE       = 8,
    KJ_ARMAGEDDON_ESCAPE_RANGE         = 6,
    KJ_FIEND_ENGAGE_RANGE              = 25,
    KJ_ORB_ENGAGE_RANGE                = 45,
    KJ_DRAKE_DUTY_GROUP_SIZE           = 2,     // every 2nd bot (by object counter) is a drake carrier
    KJ_DRAKE_DUTY_RANGE                = 120    // search range for the empowered orb GO
};

// Trigger base: only active while an encounter is in progress in Sunwell Plateau
class SunwellEncounterTrigger : public Trigger
{
public:
    SunwellEncounterTrigger(PlayerbotAI* ai, std::string const name, int32 checkInterval = 1)
        : Trigger(ai, name, checkInterval) {}

    bool IsActive() final
    {
        return EncounterHelpers::IsEncounterInProgress(bot, SWP_MAP_ID) && IsActiveInEncounter();
    }

protected:
    virtual bool IsActiveInEncounter() = 0;
};

// ---- Kalecgos ----

// A Spectral Rift portal is up near this bot, bot is outside, not exhausted -> may enter.
// Group rotation (who enters which portal) is decided in the action.
class KalecgosSpectralRiftAvailableTrigger : public SunwellEncounterTrigger
{
public:
    KalecgosSpectralRiftAvailableTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "kalecgos spectral rift available") {}
    bool IsActiveInEncounter() override;
};

// Bot is inside the spectral realm (has Spectral Realm aura): fight Sathrovarr
class KalecgosInSpectralRealmTrigger : public SunwellEncounterTrigger
{
public:
    KalecgosInSpectralRealmTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "kalecgos in spectral realm") {}
    bool IsActiveInEncounter() override;
};

// Outer realm: a Spectral Rift (or its freshly teleported player) is close to this bot.
// Guide: the teleported player deals ~6000 damage within 5 yards on teleport, so spread out.
class KalecgosTooCloseToRiftTrigger : public SunwellEncounterTrigger
{
public:
    KalecgosTooCloseToRiftTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "kalecgos too close to rift") {}
    bool IsActiveInEncounter() override;
};

// RETIRED: inside never holds anymore - it burns Sathrovarr to the end while the
// outside waits at 30%. Kept only so the registered name stays resolvable (always
// inactive). See KalecgosDragonRacingAheadTrigger for the current pace rule.
class KalecgosSathrovarrLaggingTrigger : public SunwellEncounterTrigger
{
public:
    KalecgosSathrovarrLaggingTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "kalecgos sathrovarr lagging") {}
    bool IsActiveInEncounter() override;
};

// User rule: outside fights the dragon down to 30% then holds DPS (tank keeps
// attacking for threat) until Sathrovarr inside is banished (<=1%). Without this the
// outside never slows down and the realms split apart (user report: 门外没停手).
class KalecgosDragonRacingAheadTrigger : public SunwellEncounterTrigger
{
public:
    KalecgosDragonRacingAheadTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "kalecgos dragon racing ahead") {}
    bool IsActiveInEncounter() override;
};

// ---- Brutallus ----

// This bot has the Burn damage aura (46394): move away from everyone (it spreads within 3 yards
// and ramps up over 60s). Tanks keep tanking; everyone else gets out of the pile.
class BrutallusBurnOnSelfTrigger : public SunwellEncounterTrigger
{
public:
    BrutallusBurnOnSelfTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "brutallus burn on self") {}
    bool IsActiveInEncounter() override;
};

// A nearby group member has Burn: keep at least spread range away so it cannot jump.
class BrutallusBurnNearbyTrigger : public SunwellEncounterTrigger
{
public:
    BrutallusBurnNearbyTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "brutallus burn nearby", 2) {}
    bool IsActiveInEncounter() override;
};

// Off-tank taunts when the active tank is Stomped (-50% armor) or reaches 3 Meteor Slash
// vulnerability stacks - but NEVER while a Meteor Slash cast is in progress: the cone
// resolves on the current victim, taunting mid-cast flips the cone onto an unpositioned
// tank who eats ~20000 solo and dies (user: 流星会秒人).
class BrutallusTankSwapTrigger : public SunwellEncounterTrigger
{
public:
    BrutallusTankSwapTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "brutallus tank swap") {}
    bool IsActiveInEncounter() override;
};

// Raid split into two soak camps left/right of Brutallus (user: 队伍分成两个方向分摊).
// Each non-tank non-healer is anchored to a tank; while that tank holds aggro the bot
// stands inside the frontal cone to share Meteor Slash, otherwise it waits behind the boss.
class BrutallusSoakPositionTrigger : public SunwellEncounterTrigger
{
public:
    BrutallusSoakPositionTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "brutallus soak position") {}
    bool IsActiveInEncounter() override;
};

// ---- Felmyst ----

// Felmyst is channeling Encapsulate on someone near this bot: run out (3 ticks of 3500 arcane
// hit the target AND nearby players; the target is lifted and cannot move).
class FelmystEncapsulateNearbyTrigger : public SunwellEncounterTrigger
{
public:
    FelmystEncapsulateNearbyTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst encapsulate nearby") {}
    bool IsActiveInEncounter() override;
};

// A Demonic Vapor orb (P2 beam) is chasing this bot: keep moving away from the raid so the
// trail it drops does not cut through the group. Guide: the beam follows the chosen player.
class FelmystVaporChasingTrigger : public SunwellEncounterTrigger
{
public:
    FelmystVaporChasingTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst vapor chasing") {}
    bool IsActiveInEncounter() override;
};

// A vapor trail cloud (or its dot 45402 on this bot) is too close: move out of the green.
class FelmystVaporTrailNearbyTrigger : public SunwellEncounterTrigger
{
public:
    FelmystVaporTrailNearbyTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst vapor trail nearby") {}
    bool IsActiveInEncounter() override;
};

// The main tank is debuffed with Corrosion (+100% damage taken, magic): a dispeller must
// remove it at once. Guide: "腐蚀 加大治疗" - removing it is strictly better than healing more.
class FelmystCorrosionTrigger : public SunwellEncounterTrigger
{
public:
    FelmystCorrosionTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst corrosion") {}
    bool IsActiveInEncounter() override;
};

// This bot is carrying the Gas Nova magic debuff (2s/3000 nature + mana burn). The default
// class cure strategies already handle it; this trigger exists so a dedicated raid dispeller
// has a hard node with a clear name and so the fix is not silently dependent on class defaults.
class FelmystGasNovaOnSelfTrigger : public SunwellEncounterTrigger
{
public:
    FelmystGasNovaOnSelfTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst gas nova on self") {}
    bool IsActiveInEncounter() override;
};

// Flight phase: Felmyst is airborne and sweeps lanes - the breath (Fog of Corruption, 45717
// charm on contact = death on removal) covers one third of the field each pass, and the lane
// is random each breath (urand 0-2). Strategy: for the WHOLE airborne phase every non-tank
// stays in the safe middle of the half opposite the dragon's current half; return to normal
// only when it lands. Reacting only at breath time leaves bots walking back into lingering fog.
class FelmystDeepBreathTrigger : public SunwellEncounterTrigger
{
public:
    FelmystDeepBreathTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst deep breath") {}
    bool IsActiveInEncounter() override;
};

// A Blazing Dead skeleton (summoned by vapor trails, 25268) is close to this bot.
// Tanks pick it up; non-tanks in the safe zone attack it (thin the pack), non-tanks outside
// the safe zone give it a berth so the tank can collect them.
class FelmystBlazingDeadNearbyTrigger : public SunwellEncounterTrigger
{
public:
    FelmystBlazingDeadNearbyTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst blazing dead nearby") {}
    bool IsActiveInEncounter() override;
};

// A raid ally is under Fog of Corruption (45717 charm) - they are mind-controlled and will
// be killed when the charm drops. If the charm flips hostile, kill them before they hurt us;
// otherwise keep clear of them. Trigger scans the group for anyone carrying 45717 (not self).
class FelmystCharmedAllyNearbyTrigger : public SunwellEncounterTrigger
{
public:
    FelmystCharmedAllyNearbyTrigger(PlayerbotAI* ai)
        : SunwellEncounterTrigger(ai, "felmyst charmed ally nearby") {}
    bool IsActiveInEncounter() override;
};

// ---- Eredar Twins ----

// Alythess (fire sister) is casting Conflagration (3s cast) on someone within spread range:
// the target itself and everyone nearby must run out. Also fires for the target itself.
class TwinsConflagNearbyTrigger : public SunwellEncounterTrigger
{
public:
    TwinsConflagNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "twins conflag nearby") {}
    bool IsActiveInEncounter() override;
};

// Alythess has the Pyrogenics self buff (+35% fire damage): mages spellsteal, others dispel/purge.
class TwinsPyrogenicsTrigger : public SunwellEncounterTrigger
{
public:
    TwinsPyrogenicsTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "twins pyrogenics", 2) {}
    bool IsActiveInEncounter() override;
};

// Sacrolash's current tank got hit by Confounding Blow (confuse ~6s, threat drop): the offtank
// without the debuff must taunt her back.
class TwinsTankSwapTrigger : public SunwellEncounterTrigger
{
public:
    TwinsTankSwapTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "twins tank swap") {}
    bool IsActiveInEncounter() override;
};

// Alythess' Blaze fire trap (GO 175566) is burning under/near this bot: get out.
class TwinsBlazeNearbyTrigger : public SunwellEncounterTrigger
{
public:
    TwinsBlazeNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "twins blaze nearby") {}
    bool IsActiveInEncounter() override;
};

// ---- M'uru ----

// A Dark Fiend is walking toward the raid: it must be dispelled before it reaches anyone.
// Only bots that can dispel magic (priest dispel / shaman purge) react to this trigger.
class MuruDarkFiendNearbyTrigger : public SunwellEncounterTrigger
{
public:
    MuruDarkFiendNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "muru dark fiend nearby", 2) {}
    bool IsActiveInEncounter() override;
};

// This bot is standing inside the Darkness field (M'uru phase): leave it (3k/s, no healing inside).
class MuruDarknessTrigger : public SunwellEncounterTrigger
{
public:
    MuruDarknessTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "muru darkness") {}
    bool IsActiveInEncounter() override;
};

// A Singularity (Entropius phase black hole) is close: it knocks back everyone it touches.
class MuruSingularityNearbyTrigger : public SunwellEncounterTrigger
{
public:
    MuruSingularityNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "muru singularity nearby", 2) {}
    bool IsActiveInEncounter() override;
};

// A Void Sentinel just spawned at a wall (every 30s): a free tank must pick it up at once.
class MuruVoidSentinelSpawnTrigger : public SunwellEncounterTrigger
{
public:
    MuruVoidSentinelSpawnTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "muru void sentinel spawn", 2) {}
    bool IsActiveInEncounter() override;
};

// A Shadowsword Fury Mage (blood-elf add, P1) is casting Fel Fireball (~2s cast, 5-9k on
// the tank, guide: 魔能火球需打断否则有倒坦危险). Bots with an interrupt/or a silence
// (kick / pummel / wind shear / counterspell / spell lock / shield bash / mind freeze /
// silencing shot, plus control spells like shockwave/hammer) react to stop the cast.
class MuruFuryMageInterruptTrigger : public SunwellEncounterTrigger
{
public:
    MuruFuryMageInterruptTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "muru fury mage interrupt", 2) {}
    bool IsActiveInEncounter() override;
};

// ---- Kil'jaeden ----

// This bot carries Fire Bloom (debuff on 5 random players): keep 10 yards away from everyone.
class KiljaedenFireBloomOnSelfTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenFireBloomOnSelfTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden fire bloom on self") {}
    bool IsActiveInEncounter() override;
};

// A nearby member carries Fire Bloom: stay out of their 10-yard damage radius.
class KiljaedenFireBloomNearbyTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenFireBloomNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden fire bloom nearby", 2) {}
    bool IsActiveInEncounter() override;
};

// Kil'jaeden is channeling Shadow Spike at a player: everyone within 8 yards of that player
// (including the target) must move away from the impact point.
class KiljaedenShadowSpikeNearbyTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenShadowSpikeNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden shadow spike nearby") {}
    bool IsActiveInEncounter() override;
};

// An Armageddon meteor target marker is close to this bot: the impact blasts ~5 yards.
class KiljaedenArmageddonNearbyTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenArmageddonNearbyTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden armageddon nearby") {}
    bool IsActiveInEncounter() override;
};

// A Shield Orb is up: ranged dps must switch to it (melee cannot reach it).
class KiljaedenShieldOrbTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenShieldOrbTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden shield orb", 2) {}
    bool IsActiveInEncounter() override;
};

// A Volatile Felfire Fiend (P1) is rushing someone nearby: kill it before it explodes.
class KiljaedenFelfireFiendTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenFelfireFiendTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden felfire fiend", 2) {}
    bool IsActiveInEncounter() override;
};

// This bot is a dedicated drake carrier (see KJ_DRAKE_DUTY_GROUP_SIZE), it does not yet have
// 45839, and at least one Blue Dragonflight orb is in range and selectable -> click it to gain
// the form that survives Darkness of a Thousand Souls.
class KiljaedenBlueOrbAvailableTrigger : public SunwellEncounterTrigger
{
public:
    KiljaedenBlueOrbAvailableTrigger(PlayerbotAI* ai) : SunwellEncounterTrigger(ai, "kiljaeden blue orb available", 2) {}
    bool IsActiveInEncounter() override;
};

#endif
