/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SWPStrategy.h"
#include "SWPMultipliers.h"

void RaidSunwellPlateauStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // ---- Kalecgos <The Betrayer>: Kalecgos (dragon) / Sathrovarr the Corruptor (demon) ----

    // Outer realm, standing in the portal blast radius: spread first, above everything
    triggers.push_back(new TriggerNode("kalecgos too close to rift",
            { NextAction("kalecgos spread from rift", ACTION_EMERGENCY + 5) }));

    // Portal entry: EMERGENCY priority. Live-testing showed RAID-level priority never
    // runs - every outer bot is permanently busy attacking the dragon, so combat actions
    // starve the entry action and only the Spectral Blast victim ever gets portaled in
    // (1 bot inside, dragon dies too fast). Entering must outrank attacking.
    triggers.push_back(new TriggerNode("kalecgos spectral rift available",
            { NextAction("kalecgos enter spectral rift", ACTION_EMERGENCY + 3) }));

    // Spectral realm duty: attack Sathrovarr (tanks also taunt him if un-tanked).
    // High priority: this is the "do something after entering" action - nothing may
    // leave an inside bot standing around.
    triggers.push_back(new TriggerNode("kalecgos in spectral realm",
            { NextAction("kalecgos attack sathrovarr", ACTION_RAID + 3) }));

    // Outer-realm hold (user rule, rewritten): burn the dragon to 30% then STOP.
    // Sathrovarr (inside) has no hold trigger anymore - the inside burns him down
    // while the outside waits, then the scripted kill fires and both die together.
    // Keeping "dragon racing ahead" only; "sathrovarr lagging" is retired.
    triggers.push_back(new TriggerNode("kalecgos dragon racing ahead",
            { NextAction("kalecgos balance realm dps", ACTION_EMERGENCY + 2) }));

    // ---- Brutallus <The Demonic Punisher>: Meteor Slash / Burn / Stomp tank swap ----

    // Burned bot leaves the pile (spreads within 3 yards, ramps over 60s);
    // self-cleanse first when the class can (rogue cloak / mage ice block / pala shield)
    triggers.push_back(new TriggerNode("brutallus burn on self",
            { NextAction("brutallus burn spread", ACTION_EMERGENCY + 3) }));

    // Everyone else keeps clear of any Burned member
    triggers.push_back(new TriggerNode("brutallus burn nearby",
            { NextAction("brutallus burn spread", ACTION_EMERGENCY + 3) }));

    // Off-tank taunts at 3 slash stacks or after a Stomp - never mid-cast (the trigger
    // blocks it; taunting into a resolving cone kills the new tank with ~20000 solo)
    triggers.push_back(new TriggerNode("brutallus tank swap",
            { NextAction("brutallus tank swap", ACTION_RAID + 3) }));

    // Two-camp slash soaking: non-tank non-healer bots alternate camps by group index,
    // each anchored to a tank; anchored camp soaks in the cone, the other waits behind
    // the boss. Runs below emergency burn-spreading but above default positioning.
    triggers.push_back(new TriggerNode("brutallus soak position",
            { NextAction("brutallus soak position", ACTION_RAID + 1) }));

    // ---- Felmyst <The Broodmother>: ground P1 / flight P2 ----

    // P2 priority model (user-confirmed): poison and breath dodging are TOP, eye-beam kiting
    // SECOND, and tank skeleton pickup / DPS damage are the DEFAULT rotation in the safe zone.
    // Attacking the airborne boss is the LOWEST priority (1-hit-locked, not worth GCDs).

    // 1. TOP: poison cloud under/near us - escape instantly. The vapor trail is dragged by a
    // chased player and can land on our shelter, so it outranks everything in the flight phase.
    triggers.push_back(new TriggerNode("felmyst vapor trail nearby",
            { NextAction("felmyst vapor trail escape", ACTION_EMERGENCY + 6) }));

    // 1. TOP: whole-flight shelter from the breath - every non-tank moves to the middle of the
    // half opposite the dragon and holds there (fog lands on any lane and lingers). Same tier as
    // poison; a charm death is unrecoverable, so it must not be beaten by normal rotation.
    triggers.push_back(new TriggerNode("felmyst deep breath",
            { NextAction("felmyst deep breath avoid", ACTION_EMERGENCY + 5) }));

    // 2. SECOND: this bot is chased by the Demonic Vapor beam - kite it away from the raid or
    // it drags poison through our safe zone. Lower than poison/breath but above all defaults.
    triggers.push_back(new TriggerNode("felmyst vapor chasing",
            { NextAction("felmyst vapor kite", ACTION_EMERGENCY + 4) }));

    // Encapsulate channel is up on someone nearby: run out (arcane burst hits neighbors too)
    triggers.push_back(new TriggerNode("felmyst encapsulate nearby",
            { NextAction("felmyst encapsulate spread", ACTION_EMERGENCY + 3) }));

    // Gas Nova magic debuff on this bot: dispel ourselves (backstop for default class cure).
    triggers.push_back(new TriggerNode("felmyst gas nova on self",
            { NextAction("felmyst gas nova self dispel", ACTION_EMERGENCY + 2) }));

    // Corrosion +100% damage taken on the main tank (magic): dispeller removes it.
    // Above default healing so it is not waited on; heals are frowned but the tank swap
    // trigger is separate - this is purely the debuff.
    triggers.push_back(new TriggerNode("felmyst corrosion",
            { NextAction("felmyst corrosion", ACTION_RAID + 2) }));

    // P2 Blazing Dead skeletons, from the safe half: tanks pick them up and collect the pack;
    // non-tank DPS thin the ones that are reachable without leaving the safe zone.
    triggers.push_back(new TriggerNode("felmyst blazing dead nearby",
            { NextAction("felmyst blazing dead pickup", ACTION_RAID + 2),
              NextAction("felmyst blazing dead attack", ACTION_RAID + 1) }));

    // A raid ally is mind-controlled (Fog of Corruption): kill them if the charm made them
    // hostile, otherwise stay away. Above normal DPS so we do not stand next to a charmed ally.
    triggers.push_back(new TriggerNode("felmyst charmed ally nearby",
            { NextAction("felmyst charmed ally", ACTION_EMERGENCY + 4) }));

    // ---- Eredar Twins <The Holy and the Unholy>: Sacrolash (dark) / Alythess (fire) ----

    // Conflagration is being cast on someone nearby (or this bot): spread from the target
    triggers.push_back(new TriggerNode("twins conflag nearby",
            { NextAction("twins conflag spread", ACTION_EMERGENCY + 3) }));

    // Pyrogenics (+35% fire damage self buff) on the fire sister: steal / dispel / purge
    triggers.push_back(new TriggerNode("twins pyrogenics",
            { NextAction("twins dispel pyrogenics", ACTION_RAID + 2) }));

    // Confounding Blow confuse on Sacrolash's tank: offtank taunts her back
    triggers.push_back(new TriggerNode("twins tank swap",
            { NextAction("twins tank swap", ACTION_RAID + 3) }));

    // Standing on a Blaze ground-fire trap (Alythess): run to a spot clear of every Blaze
    triggers.push_back(new TriggerNode("twins blaze nearby",
            { NextAction("twins blaze escape", ACTION_EMERGENCY + 2) }));

    // ---- M'uru / Entropius ----

    // Dark Fiend walking at the raid: dispel it before it detonates (only counter)
    triggers.push_back(new TriggerNode("muru dark fiend nearby",
            { NextAction("muru dispel dark fiend", ACTION_EMERGENCY + 2) }));

    // Standing in the Darkness field: get out (damage + no healing inside)
    triggers.push_back(new TriggerNode("muru darkness",
            { NextAction("muru darkness escape", ACTION_EMERGENCY + 3) }));

    // Entropius phase: a Singularity black hole is close
    triggers.push_back(new TriggerNode("muru singularity nearby",
            { NextAction("muru singularity escape", ACTION_EMERGENCY + 3) }));

    // Void Sentinel spawned un-tanked: a free tank picks it up at once
    triggers.push_back(new TriggerNode("muru void sentinel spawn",
            { NextAction("muru void sentinel pickup", ACTION_RAID + 3) }));

    // Fury Mage (blood-elf add) casting Fel Fireball: interrupt before the tank eats 5-9k.
    // Emergency priority - a landed cast is what kills the add-tank (guide: 法系怪需打断).
    triggers.push_back(new TriggerNode("muru fury mage interrupt",
            { NextAction("muru interrupt fury mage", ACTION_EMERGENCY + 1) }));

    // ---- Kil'jaeden <The Deceiver> ----

    // Fire Bloom on this bot: keep 10 yards from everyone
    triggers.push_back(new TriggerNode("kiljaeden fire bloom on self",
            { NextAction("kiljaeden fire bloom spread", ACTION_EMERGENCY + 3) }));

    // Fire Bloom on a nearby member: stay out of the 10-yard radius
    triggers.push_back(new TriggerNode("kiljaeden fire bloom nearby",
            { NextAction("kiljaeden fire bloom spread", ACTION_EMERGENCY + 3) }));

    // Shadow Spike drifting at a player: move away from the impact target
    triggers.push_back(new TriggerNode("kiljaeden shadow spike nearby",
            { NextAction("kiljaeden shadow spike spread", ACTION_EMERGENCY + 2) }));

    // Armageddon meteor marker nearby: run out of the blast radius
    triggers.push_back(new TriggerNode("kiljaeden armageddon nearby",
            { NextAction("kiljaeden armageddon escape", ACTION_EMERGENCY + 4) }));

    // Shield Orb up: ranged dps switch to it
    triggers.push_back(new TriggerNode("kiljaeden shield orb",
            { NextAction("kiljaeden shield orb", ACTION_RAID + 2) }));

    // Volatile Felfire Fiend rushing someone: kill it before it explodes
    triggers.push_back(new TriggerNode("kiljaeden felfire fiend",
            { NextAction("kiljaeden felfire fiend", ACTION_RAID + 1) }));

    // Dedicated drake carrier: an empowered Blue Dragonflight Orb is available. Clicking it
    // grants 45839 (Blue Dragonflight form) - the guide's counter to Darkness of a Thousand
    // Souls. Above generic combat so the carrier actually walks off boss DPS to grab the orb,
    // but below the emergency spreading.
    triggers.push_back(new TriggerNode("kiljaeden blue orb available",
            { NextAction("kiljaeden click blue orb", ACTION_RAID + 2) }));
}

void RaidSunwellPlateauStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    // Kalecgos only
    multipliers.push_back(new KalecgosPortalMovementMultiplier(botAI));
    multipliers.push_back(new KalecgosCorruptionStrikeHealMultiplier(botAI));
    // Outer-realm DPS hold at 30%: zeroes AttackAction so bots can't re-engage the
    // dragon every tick (AttackStop-only would lose to the generic melee loop).
    multipliers.push_back(new KalecgosOuterDpsHoldMultiplier(botAI));
    // User rule: outer shaman opens Bloodlust/Heroism once 10+ members are inside.
    multipliers.push_back(new KalecgosSpectralBloodlustMultiplier(botAI));

    // Kil'jaeden: shaman opens Bloodlust/Heroism once the boss is at 85% or below
    // (P3 - first Thousand Souls / orb pressure peak; held through P1 hands and the
    // easy P2 opener, user rule: P3开嗜血).
    multipliers.push_back(new KiljaedenP3BloodlustMultiplier(botAI));

    // Brutallus: hold Bloodlust/Heroism for the first 15s, then fire it at full
    // priority into the DPS window (the opening is soak setup / Burn spread).
    multipliers.push_back(new BrutallusBloodlustMultiplier(botAI));

    // Felmyst P2: attacking the (1-hit-locked) boss is the LOWEST priority - dodge poison/breath
    // and fight skeletons / charmed allies first; only idle GCDs are spent on the boss.
    multipliers.push_back(new FelmystAirborneDpsHoldMultiplier(botAI));
    // Felmyst P2, post-breath transition: raid stands still (no movement) while the fog
    // settles, but keeps casting (user: 深呼吸后 +10s 内不移动,可以放技能).
    multipliers.push_back(new FelmystPostBreathFreezeMultiplier(botAI));

    // Entropius rush: holds Bloodlust/Heroism during P1 adds, fires it the moment P2 starts.
    multipliers.push_back(new MuruEntropiusRushMultiplier(botAI));
}
