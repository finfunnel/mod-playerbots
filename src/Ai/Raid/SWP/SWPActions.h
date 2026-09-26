/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_SWPACTIONS_H
#define PLAYERBOTS_SWPACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "MovementActions.h"
#include "SWPTriggers.h"

class Player;

// Move to a usable Spectral Rift and click it to enter the spectral realm.
// Rotation per guide: tanks and healers enter with their group when a rift is available;
// Arcane Buffet stack pressure decides for everyone else (handled by priority, see SWPStrategy).
class KalecgosEnterSpectralRiftAction : public MovementAction
{
public:
    KalecgosEnterSpectralRiftAction(PlayerbotAI* ai) : MovementAction(ai, "kalecgos enter spectral rift") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Outer realm: move away from a nearby Spectral Rift / freshly teleported player so the
// next Spectral Blast teleport (~6000 dmg within 5 yards) does not hit this bot.
class KalecgosSpreadFromRiftAction : public MovementAction
{
public:
    KalecgosSpreadFromRiftAction(PlayerbotAI* ai) : MovementAction(ai, "kalecgos spread from rift") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Spectral realm: attack Sathrovarr. Attack from behind/flank - never stand in front next to
// the tank (Shadow Bolt on victim, Corruption Strike frontal). Set-behind is enough here since
// melee-side positioning is enforced by the engine's normal behaviour once targeted.
class KalecgosAttackSathrovarrAction : public AttackAction
{
public:
    KalecgosAttackSathrovarrAction(PlayerbotAI* ai) : AttackAction(ai, "kalecgos attack sathrovarr") {}
    bool Execute(Event event) override;
};

// Spectral realm healers during Corruption Strike are handled WITHOUT a dedicated action:
// AttackAction::Attack() refuses friendly targets, so a "heal the tank" action would be a no-op.
// The default heal engine (PartyMemberToHeal) already picks the lowest-health member, and
// KalecgosCorruptionStrikeHealMultiplier freezes healer movement for the burst window.

// Inside DPS hold while Sathrovarr lags the dragon by more than the balance gap
// (user rule: keep the gap within 10%). The old version AttackStopped and returned
// true - starving every lower action and leaving bots IDLE. Now: AttackStop then
// return FALSE so the rest of the queue (positioning, heals, moving out of bad)
// still runs; only raw DPS is suppressed until the dragon catches up.
class KalecgosBalanceRealmDpsAction : public Action
{
public:
    KalecgosBalanceRealmDpsAction(PlayerbotAI* ai) : Action(ai, "kalecgos balance realm dps") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- Brutallus ----

// Bot has Burn: self-cleanse if the class can (guide: rogue Cloak of Shadows,
// mage Ice Block, paladin Divine Shield are the reliable outs for a 60s ticking
// stack), then move out of the group pile (spreads within 3 yards, ramps over 60s).
// Reuses the engine's safe-position search from MoveAwayFromPlayerWithDebuffAction.
// TANKS NEVER RUN (user rule): the tank moving breaks the Meteor Slash soak cone and
// drags the whole camp with him - everyone else spreads away from a burning tank instead.
class BrutallusBurnSpreadAction : public MoveAwayFromPlayerWithDebuffAction
{
public:
    BrutallusBurnSpreadAction(PlayerbotAI* ai)
        : MoveAwayFromPlayerWithDebuffAction(ai, "brutallus burn spread", SPELL_BURN_DAMAGE,
              BRUTALLUS_BURN_SPREAD_RANGE) {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

// Off-tank taunt swap: 3 Meteor Slash vulnerability stacks or a Stomp on the current
// tank means the offtank takes over - but only between slash casts (the trigger
// blocks mid-cast swaps; taunting into a resolving cone kills the new tank).
class BrutallusTankSwapAction : public AttackAction
{
public:
    BrutallusTankSwapAction(PlayerbotAI* ai) : AttackAction(ai, "brutallus tank swap") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Two-camp Meteor Slash soaking (user: 队伍分成两个方向分摊). The bot is anchored to a
// tank (odd/even group index): while the anchor tank is Brutallus' victim the bot moves
// into the frontal cone to share the ~20000 slash; otherwise it slides behind the boss
// and waits for its tank's turn. Camp swap follows aggro automatically. Off-duty tanks
// wait at the same rear spot with their camp so the 120-degree cone never stacks the
// fire vulnerability on them (their taunt turn is blocked while the debuff holds).
// User rules on top: the parked tank HOLDS the tick (tank不要乱动 - no chase moves can
// drag him around), and the taunt only fires from the boss's back (tank要注意boss的
// 朝向 - the cone flips with his facing, so it lands on the taunter's own camp).
class BrutallusSoakPositionAction : public MovementAction
{
public:
    BrutallusSoakPositionAction(PlayerbotAI* ai) : MovementAction(ai, "brutallus soak position") {}
    bool Execute(Event event) override;
};

// ---- Felmyst ----

// Run away from the Encapsulate target (channel hits the target and everyone nearby).
class FelmystEncapsulateSpreadAction : public MovementAction
{
public:
    FelmystEncapsulateSpreadAction(PlayerbotAI* ai) : MovementAction(ai, "felmyst encapsulate spread") {}
    bool Execute(Event event) override;
};

// Kite the Demonic Vapor beam away from the raid while it chases this bot (P2).
class FelmystVaporKiteAction : public MovementAction
{
public:
    FelmystVaporKiteAction(PlayerbotAI* ai) : MovementAction(ai, "felmyst vapor kite") {}
    bool Execute(Event event) override;
};

// Move out of a vapor trail cloud / the fogged breath zone.
class FelmystVaporTrailEscapeAction : public MovementAction
{
public:
    FelmystVaporTrailEscapeAction(PlayerbotAI* ai) : MovementAction(ai, "felmyst vapor trail escape") {}
    bool Execute(Event event) override;
};

// Remove Corrosion from the main tank (magic, +100% damage taken). Healers dispel him first;
// only when no dispel is available do they fall back to plain healing.
class FelmystCorrosionAction : public Action
{
public:
    FelmystCorrosionAction(PlayerbotAI* ai) : Action(ai, "felmyst corrosion") {}
    bool Execute(Event event) override;
};

// Dispel Gas Nova off this bot (magic). Dedicated hard node so the fight is not silently
// dependent on the generic class cure strategy achieving the dispel in time.
class FelmystGasNovaSelfDispelAction : public Action
{
public:
    FelmystGasNovaSelfDispelAction(PlayerbotAI* ai) : Action(ai, "felmyst gas nova self dispel") {}
    bool Execute(Event event) override;
};

// Flight-phase dodge (user: 看龙在哪一条 lane 就是喷哪边): DYNAMIC lane dodge. The dragon
// breathes the lane it is flying on; this action runs only when THIS bot stands inside that
// swept lane and shifts it sideways to the farthest non-swept lane. Bots on other lanes hold
// (their trigger is inactive), and when the dragon is repositioning (no active lane) nobody
// moves. Once the dragon lands (P1) the trigger deactivates and normal ground-phase play resumes.
class FelmystDeepBreathAvoidAction : public MovementAction
{
public:
    FelmystDeepBreathAvoidAction(PlayerbotAI* ai) : MovementAction(ai, "felmyst deep breath avoid") {}
    bool Execute(Event event) override;
};

// Blazing Dead skeleton (25268): tank picks it up (taunt / melee) and keeps the pack off the
// raid, mirroring MuruVoidSentinelPickupAction. Works on freshly-summoned skeletons too.
class FelmystBlazingDeadPickupAction : public AttackAction
{
public:
    FelmystBlazingDeadPickupAction(PlayerbotAI* ai) : AttackAction(ai, "felmyst blazing dead pickup") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

// Kill the skeleton pack BEFORE anything else in P2 (user: 出小骷髅, 除了被点名的,其他人优先
// 击杀小骷髅). Every non-tank (melee + ranged) engages the nearest Blazing Dead; the
// eye-beam-chased bot keeps kiting at a higher priority and therefore does not stop to fight.
// Nobody fights while standing inside the lane being swept (the deep-breath check gates this),
// otherwise skeletons are burned from anywhere - ranged hit far ones, melee only what is
// already in reach. Fresh skeletons (not yet in combat) are burned too.
class FelmystBlazingDeadAttackAction : public AttackAction
{
public:
    FelmystBlazingDeadAttackAction(PlayerbotAI* ai) : AttackAction(ai, "felmyst blazing dead attack") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

// A raid ally is mind-controlled (Fog of Corruption, 45717). If the charm turns them hostile
// they must be killed before they hurt us; otherwise stay away from them until the charm ends.
// First try Attack() - the bot only can if the charm flipped faction - and if it cannot,
// move away instead.
class FelmystCharmedAllyAction : public AttackAction
{
public:
    FelmystCharmedAllyAction(PlayerbotAI* ai) : AttackAction(ai, "felmyst charmed ally") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

// ---- Eredar Twins ----

// Conflagration is casting/near: the target runs out of the group, everyone else moves away
// from the target. Reuses MoveAwayFromPlayerWithDebuffAction's safe-direction search when the
// bot itself is not the target; the carrier itself just runs straight away from the raid.
class TwinsConflagSpreadAction : public MovementAction
{
public:
    TwinsConflagSpreadAction(PlayerbotAI* ai) : MovementAction(ai, "twins conflag spread") {}
    bool Execute(Event event) override;
};

// Alythess has Pyrogenics up: mages spellsteal it, priests dispel it, shamans purge it.
class TwinsDispelPyrogenicsAction : public Action
{
public:
    TwinsDispelPyrogenicsAction(PlayerbotAI* ai) : Action(ai, "twins dispel pyrogenics") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Standing on a Blaze fire trap (Alythess' ground fire): move to a spot that is out of every
// nearby Blaze, not just the nearest one - the raid can have several traps down at once.
class TwinsBlazeEscapeAction : public MovementAction
{
public:
    TwinsBlazeEscapeAction(PlayerbotAI* ai) : MovementAction(ai, "twins blaze escape") {}
    bool Execute(Event event) override;
};

// Off-tank taunts Sacrolash back when her current tank is confused by Confounding Blow.
class TwinsTankSwapAction : public AttackAction
{
public:
    TwinsTankSwapAction(PlayerbotAI* ai) : AttackAction(ai, "twins tank swap") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- M'uru ----

// Dispel a Dark Fiend before it reaches its target. Priest: Mass Dispel first (clears
// the whole wave of 8 at once), single Dispel Magic as fallback; shaman Purge.
// Other classes cannot remove it; the guide calls dispelling the only counter.
class MuruDispelDarkFiendAction : public Action
{
public:
    MuruDispelDarkFiendAction(PlayerbotAI* ai) : Action(ai, "muru dispel dark fiend") {}
    bool Execute(Event event) override;
};

// Move out of the Darkness field (3k/s, cannot be healed inside).
class MuruDarknessEscapeAction : public MovementAction
{
public:
    MuruDarknessEscapeAction(PlayerbotAI* ai) : MovementAction(ai, "muru darkness escape") {}
    bool Execute(Event event) override;
};

// Move away from a Singularity (it chases and knocks players back).
class MuruSingularityEscapeAction : public MovementAction
{
public:
    MuruSingularityEscapeAction(PlayerbotAI* ai) : MovementAction(ai, "muru singularity escape") {}
    bool Execute(Event event) override;
};

// A free tank attacks an un-tanked Void Sentinel the moment it spawns at a wall.
class MuruVoidSentinelPickupAction : public AttackAction
{
public:
    MuruVoidSentinelPickupAction(PlayerbotAI* ai) : AttackAction(ai, "muru void sentinel pickup") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Interrupt a Shadowsword Fury Mage that is casting Fel Fireball (2s cast, 5-9k on the
// tank). Uses the class interrupt (kick / pummel / wind shear / mind freeze / counterspell
// / spell lock / shield bash / silencing shot), falling back to a stun/silence when the
// dedicated interrupt is on cooldown. Only bots that can stop a cast react (isUseful).
class MuruInterruptFuryMageAction : public Action
{
public:
    MuruInterruptFuryMageAction(PlayerbotAI* ai) : Action(ai, "muru interrupt fury mage") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- Kil'jaeden ----

// Fire Bloom carriers keep 10 yards away from everyone (debuff on 5 random players, each
// carrier damages everyone within 10 yards). Reuses the engine's safe-position search.
class KiljaedenFireBloomSpreadAction : public MoveAwayFromPlayerWithDebuffAction
{
public:
    KiljaedenFireBloomSpreadAction(PlayerbotAI* ai)
        : MoveAwayFromPlayerWithDebuffAction(ai, "kiljaeden fire bloom spread", SPELL_FIRE_BLOOM,
              KJ_FIRE_BLOOM_SPREAD_RANGE) {}
};

// Run away from the Shadow Spike impact target (8-yard explosion on arrival).
class KiljaedenShadowSpikeSpreadAction : public MovementAction
{
public:
    KiljaedenShadowSpikeSpreadAction(PlayerbotAI* ai) : MovementAction(ai, "kiljaeden shadow spike spread") {}
    bool Execute(Event event) override;
};

// An Armageddon meteor marker is nearby: run out of the ~5-yard blast radius.
class KiljaedenArmageddonEscapeAction : public MovementAction
{
public:
    KiljaedenArmageddonEscapeAction(PlayerbotAI* ai) : MovementAction(ai, "kiljaeden armageddon escape") {}
    bool Execute(Event event) override;
};

// Ranged dps switches to a Shield Orb (melee cannot reach a floating orb).
class KiljaedenShieldOrbAction : public AttackAction
{
public:
    KiljaedenShieldOrbAction(PlayerbotAI* ai) : AttackAction(ai, "kiljaeden shield orb") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Attack a Volatile Felfire Fiend before it reaches someone and explodes.
class KiljaedenFelfireFiendAction : public AttackAction
{
public:
    KiljaedenFelfireFiendAction(PlayerbotAI* ai) : AttackAction(ai, "kiljaeden felfire fiend") {}
    bool Execute(Event event) override;
};

// Dedicated drake carrier: run to the nearest empowered Blue Dragonflight Orb and click it to
// gain 45839 (Blue Dragonflight form) - the only way the raid survives Darkness of a Thousand
// Souls (46605 -> 45657 on everyone without 45839). Modelled on the Os portal-click flow:
// move to interaction distance, then HandleGameObjectUseOpcode (OSActions.cpp).
class KiljaedenClickBlueOrbAction : public MovementAction
{
public:
    KiljaedenClickBlueOrbAction(PlayerbotAI* ai) : MovementAction(ai, "kiljaeden click blue orb") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

#endif
