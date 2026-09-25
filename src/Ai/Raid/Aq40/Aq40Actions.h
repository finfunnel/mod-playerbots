/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AQ40ACTIONS_H
#define PLAYERBOTS_AQ40ACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "Aq40Triggers.h"
#include "MovementActions.h"

// Polymorph the mind-controlled group member (True Fulfillment 785). Guide: sheep the
// controlled player; the MC buff boosts damage/speed and makes all spells instant.
class SkeramPolymorphMindControlledAction : public Action
{
public:
    SkeramPolymorphMindControlledAction(PlayerbotAI* ai) : Action(ai, "skeram polymorph mind controlled") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Switch DPS to a summoned Skeram image: images deal heavy AoE and must die first (guide).
class SkeramAttackImageAction : public AttackAction
{
public:
    SkeramAttackImageAction(PlayerbotAI* ai) : AttackAction(ai, "skeram attack image") {}
    bool Execute(Event event) override;
};

// A free tank picks up the un-tanked boss/image (taunt if up, else move in and attack).
// Post-split/post-teleport threat wipes mean an unpicked boss spams Earth Shock (26194).
class SkeramPickupUntankedAction : public AttackAction
{
public:
    SkeramPickupUntankedAction(PlayerbotAI* ai) : AttackAction(ai, "skeram pickup untanked") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- Battleguard Sartura ----

// Run away from a whirlwinding Sartura/Royal Guard (threat is wiped on every hop, so the
// "tank holds it" assumption is gone - everyone gets out of the blade storm; guide:
// 旋风斩立刻清除仇恨，玩家停手躲避).
class SarturaFleeWhirlwindAction : public MovementAction
{
public:
    SarturaFleeWhirlwindAction(PlayerbotAI* ai) : MovementAction(ai, "sartura flee whirlwind") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Switch DPS to a living Royal Guard (15984): kill the three guards before the boss
// (guide: 优先击杀三个卫兵，然后杀boss).
class SarturaAttackRoyalGuardAction : public AttackAction
{
public:
    SarturaAttackRoyalGuardAction(PlayerbotAI* ai) : AttackAction(ai, "sartura attack royal guard") {}
    bool Execute(Event event) override;
};

// Rogues keep Kidney Shot on Sartura while she is NOT whirlwinding (she is stunnable -
// guide: 沙尔图拉可以被晕眩，盗贼多打肾击降低难度; whirlwinding boss is immune + hitting it
// during whirlwind is what we are trying to avoid).
class SarturaKidneyShotAction : public Action
{
public:
    SarturaKidneyShotAction(PlayerbotAI* ai) : Action(ai, "sartura kidney shot") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- Bug Trio ----

// Fear protection against Yauj's AoE fear (26580): priests Fear Ward the tank,
// shamans drop a Tremor Totem (guide: 萨满插好图腾，牧师上好防恐).
class BugTrioFearPrepAction : public Action
{
public:
    BugTrioFearPrepAction(PlayerbotAI* ai) : Action(ai, "bug trio fear prep") {}
    bool Execute(Event event) override;
};

// Run out of Kri's corpse poison cloud (26590, ~2000 nature dps - guide: 务必远离).
class BugTrioFleePoisonCloudAction : public MovementAction
{
public:
    BugTrioFleePoisonCloudAction(PlayerbotAI* ai) : MovementAction(ai, "bug trio flee poison cloud") {}
    bool Execute(Event event) override;
};

// Attack the guide's kill-order target: Yauj first (AoE fear + heal), then Vem, Kri last
// (weakened on test realms; his corpse leaves the poison cloud).
class BugTrioAttackKillOrderAction : public AttackAction
{
public:
    BugTrioAttackKillOrderAction(PlayerbotAI* ai) : AttackAction(ai, "bug trio attack kill order") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- Fankriss the Unyielding ----

// Taunt Fankriss off the active tank once Mortal Wound stacks reach the threshold
// (guide: 致命伤口叠加减少治疗效果，坦克换坦).
class FankrissTankSwapAction : public AttackAction
{
public:
    FankrissTankSwapAction(PlayerbotAI* ai) : AttackAction(ai, "fankriss tank swap") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// A free tank picks up an un-tanked sandworm (Spawn of Fankriss) - they hit extremely
// hard and must not run loose on the raid (guide: 坦克玩家拉好).
class FankrissPickupSpawnAction : public AttackAction
{
public:
    FankrissPickupSpawnAction(PlayerbotAI* ai) : AttackAction(ai, "fankriss pickup spawn") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Switch DPS to the sandworm: kills the hard-hitting worm before going back to the
// boss (guide: 沙虫伤害极高，优先击杀).
class FankrissAttackSpawnAction : public AttackAction
{
public:
    FankrissAttackSpawnAction(PlayerbotAI* ai) : AttackAction(ai, "fankriss attack spawn") {}
    bool Execute(Event event) override;
};

// ---- Viscidus ----

// Mages spam Frostbolt to stack the frost-hit counter: 100 slows, 150 slows more, 200
// fully freezes Viscidus (guide: 法师职业直接搓一级寒冰箭).
class ViscidusCastFrostboltAction : public Action
{
public:
    ViscidusCastFrostboltAction(PlayerbotAI* ai) : Action(ai, "viscidus cast frostbolt") {}
    bool Execute(Event event) override;
};

// Viscidus is fully frozen: everyone melee attacks (fast weapons) to crack it - melee
// hits during the freeze build the shatter counter (guide: 换上快速武器直接敲boss).
class ViscidusAttackFrozenAction : public AttackAction
{
public:
    ViscidusAttackFrozenAction(PlayerbotAI* ai) : AttackAction(ai, "viscidus attack frozen") {}
    bool Execute(Event event) override;
};

// Switch to a split glob (15667): each glob killed removes 5% of boss HP; they crawl
// to the room center and rejoin (guide: 打掉尽可能多的分身).
class ViscidusAttackGlobAction : public AttackAction
{
public:
    ViscidusAttackGlobAction(PlayerbotAI* ai) : AttackAction(ai, "viscidus attack glob") {}
    bool Execute(Event event) override;
};

// Run out of the Toxic Slime poison cloud (guide: 躲开毒云).
class ViscidusFleeToxinAction : public MovementAction
{
public:
    ViscidusFleeToxinAction(PlayerbotAI* ai) : MovementAction(ai, "viscidus flee toxin") {}
    bool Execute(Event event) override;
};

// ---- Princess Huhuran ----

// 30% berserk: the 15 players nearest the boss eat the Poison Bolt AoE - designated
// melee "soakers" stack right on Huhuran so the bolts land on high-nature-resistance
// players instead of the ranged raid (guide: 沙包贴boss吃毒镖).
class HuhuranSoakAction : public MovementAction
{
public:
    HuhuranSoakAction(PlayerbotAI* ai) : MovementAction(ai, "huhuran soak") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// ---- Twin Emperors ----

// A warlock grabs the un-tanked caster emperor with searing pain (high threat spell)
// (guide: 术士抗魔皇，传送后拉好怪).
class TwinEmpVeklorPickupAction : public AttackAction
{
public:
    TwinEmpVeklorPickupAction(PlayerbotAI* ai) : AttackAction(ai, "twin emp veklor pickup") {}
    bool Execute(Event event) override;
};

// Warrior taunts Veknilash off the defense-shredded active tank
// (guide: 重压打击减100防御，换坦).
class TwinEmpTankSwapAction : public AttackAction
{
public:
    TwinEmpTankSwapAction(PlayerbotAI* ai) : AttackAction(ai, "twin emp tank swap") {}
    bool Execute(Event event) override;
};

// A melee bot swinging at the magic-immune Veklor switches to the correct twin
// (Veknilash, physical-immune) instead (guide: 注意输出伤害类型).
class TwinEmpAttackCorrectTwinAction : public AttackAction
{
public:
    TwinEmpAttackCorrectTwinAction(PlayerbotAI* ai) : AttackAction(ai, "twin emp attack correct twin") {}
    bool Execute(Event event) override;
};

// ---- Ouro ----

// Run away from a chasing dirt mound (15712): mounds pick a random player every 5-10s,
// reach them and rupture the ground - anyone near the mound keeps moving
// (guide: 土堆追击玩家，被追到会受到伤害).
class OuroFleeMoundAction : public MovementAction
{
public:
    OuroFleeMoundAction(PlayerbotAI* ai) : MovementAction(ai, "ouro flee mound") {}
    bool Execute(Event event) override;
};

// Ranged burn the Ouro Scarabs (15718) spawned on re-emerge / berserk before going back
// to the boss (guide: 场内刷新的小虫需要处理，远程优先清理).
class OuroAttackScarabAction : public AttackAction
{
public:
    OuroAttackScarabAction(PlayerbotAI* ai) : AttackAction(ai, "ouro attack scarab") {}
    bool Execute(Event event) override;
};

// ---- C'Thun ----

// Keep strafing while the Eye sweeps Dark Glare (26029, 35 ticks around the room)
// (guide: 眼睛绿变红的时候，全体跑位).
class CthunFleeDarkGlareAction : public MovementAction
{
public:
    CthunFleeDarkGlareAction(PlayerbotAI* ai) : MovementAction(ai, "cthun flee dark glare") {}
    bool Execute(Event event) override;
};

// Ranged burn small tentacles (claw 15725 / eye 15726) on sight; melee stay on the Eye
// (guide: 近战集火克苏恩，远程优先击杀尖爪触须).
class CthunAttackTentacleAction : public AttackAction
{
public:
    CthunAttackTentacleAction(PlayerbotAI* ai) : AttackAction(ai, "cthun attack tentacle") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// Tanks pick up the hard-hitting giant tentacles (claw 15728 / eye 15334) before anyone
// else attacks them (guide: 巨爪触须伤害极高，坦克没拉住别急着打).
class CthunPickupGiantTentacleAction : public AttackAction
{
public:
    CthunPickupGiantTentacleAction(PlayerbotAI* ai) : AttackAction(ai, "cthun pickup giant tentacle") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// DPS burn a tanked giant tentacle (tanks handle the pickup separately).
class CthunAttackGiantTentacleAction : public AttackAction
{
public:
    CthunAttackGiantTentacleAction(PlayerbotAI* ai) : AttackAction(ai, "cthun attack giant tentacle") {}
    bool Execute(Event event) override;
};

// Inside the stomach: kill the flesh tentacles (15802) on the platforms - two dead
// tentacles weaken C'Thun so the outside raid can burn him (guide: 先上台子杀伤血肉触须).
class CthunAttackFleshTentacleAction : public AttackAction
{
public:
    CthunAttackFleshTentacleAction(PlayerbotAI* ai) : AttackAction(ai, "cthun attack flesh tentacle") {}
    bool Execute(Event event) override;
};

#endif
