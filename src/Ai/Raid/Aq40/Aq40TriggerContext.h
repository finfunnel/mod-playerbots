/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AQ40TRIGGERCONTEXT_H
#define PLAYERBOTS_AQ40TRIGGERCONTEXT_H

#include "Aq40Triggers.h"
#include "NamedObjectContext.h"

class RaidAq40TriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidAq40TriggerContext()
    {
        // The Prophet Skeram
        creators["skeram member mind controlled"] = &RaidAq40TriggerContext::skeram_member_mind_controlled;
        creators["skeram image up"] = &RaidAq40TriggerContext::skeram_image_up;
        creators["skeram untanked"] = &RaidAq40TriggerContext::skeram_untanked;

        // Battleguard Sartura
        creators["sartura whirlwind active"] = &RaidAq40TriggerContext::sartura_whirlwind_active;
        creators["sartura royal guard up"] = &RaidAq40TriggerContext::sartura_royal_guard_up;

        // Bug Trio
        creators["bug trio fear prep"] = &RaidAq40TriggerContext::bug_trio_fear_prep;
        creators["bug trio poison cloud"] = &RaidAq40TriggerContext::bug_trio_poison_cloud;
        creators["bug trio kill order target"] = &RaidAq40TriggerContext::bug_trio_kill_order_target;

        // Fankriss the Unyielding
        creators["fankriss mortal wound stacks"] = &RaidAq40TriggerContext::fankriss_mortal_wound_stacks;
        creators["fankriss spawn up"] = &RaidAq40TriggerContext::fankriss_spawn_up;

        // Viscidus
        creators["viscidus needs frost"] = &RaidAq40TriggerContext::viscidus_needs_frost;
        creators["viscidus frozen"] = &RaidAq40TriggerContext::viscidus_frozen;
        creators["viscidus glob up"] = &RaidAq40TriggerContext::viscidus_glob_up;
        creators["viscidus toxin near"] = &RaidAq40TriggerContext::viscidus_toxin_near;

        // Princess Huhuran
        creators["huhuran berserk"] = &RaidAq40TriggerContext::huhuran_berserk;

        // Twin Emperors
        creators["twin emp veklor untanked"] = &RaidAq40TriggerContext::twin_emp_veklor_untanked;
        creators["twin emp tank swap"] = &RaidAq40TriggerContext::twin_emp_tank_swap;
        creators["twin emp wrong damage type"] = &RaidAq40TriggerContext::twin_emp_wrong_damage_type;

        // Ouro
        creators["ouro submerged"] = &RaidAq40TriggerContext::ouro_submerged;
        creators["ouro scarabs up"] = &RaidAq40TriggerContext::ouro_scarabs_up;

        // C'Thun
        creators["cthun dark glare"] = &RaidAq40TriggerContext::cthun_dark_glare;
        creators["cthun tentacles up"] = &RaidAq40TriggerContext::cthun_tentacles_up;
        creators["cthun giant tentacle up"] = &RaidAq40TriggerContext::cthun_giant_tentacle_up;
        creators["cthun in stomach"] = &RaidAq40TriggerContext::cthun_in_stomach;
    }

private:
    static Trigger* skeram_member_mind_controlled(PlayerbotAI* ai)
    {
        return new SkeramMemberMindControlledTrigger(ai);
    }
    static Trigger* skeram_image_up(PlayerbotAI* ai) { return new SkeramImageUpTrigger(ai); }
    static Trigger* skeram_untanked(PlayerbotAI* ai) { return new SkeramUntankedTrigger(ai); }
    static Trigger* sartura_whirlwind_active(PlayerbotAI* ai) { return new SarturaWhirlwindActiveTrigger(ai); }
    static Trigger* sartura_royal_guard_up(PlayerbotAI* ai) { return new SarturaRoyalGuardUpTrigger(ai); }
    static Trigger* bug_trio_fear_prep(PlayerbotAI* ai) { return new BugTrioFearPrepTrigger(ai); }
    static Trigger* bug_trio_poison_cloud(PlayerbotAI* ai) { return new BugTrioPoisonCloudTrigger(ai); }
    static Trigger* bug_trio_kill_order_target(PlayerbotAI* ai) { return new BugTrioKillOrderTargetTrigger(ai); }
    static Trigger* fankriss_mortal_wound_stacks(PlayerbotAI* ai) { return new FankrissMortalWoundStacksTrigger(ai); }
    static Trigger* fankriss_spawn_up(PlayerbotAI* ai) { return new FankrissSpawnUpTrigger(ai); }
    static Trigger* viscidus_needs_frost(PlayerbotAI* ai) { return new ViscidusNeedsFrostTrigger(ai); }
    static Trigger* viscidus_frozen(PlayerbotAI* ai) { return new ViscidusFrozenTrigger(ai); }
    static Trigger* viscidus_glob_up(PlayerbotAI* ai) { return new ViscidusGlobUpTrigger(ai); }
    static Trigger* viscidus_toxin_near(PlayerbotAI* ai) { return new ViscidusToxinNearTrigger(ai); }
    static Trigger* huhuran_berserk(PlayerbotAI* ai) { return new HuhuranBerserkTrigger(ai); }
    static Trigger* twin_emp_veklor_untanked(PlayerbotAI* ai) { return new TwinEmpVeklorUntankedTrigger(ai); }
    static Trigger* twin_emp_tank_swap(PlayerbotAI* ai) { return new TwinEmpTankSwapTrigger(ai); }
    static Trigger* twin_emp_wrong_damage_type(PlayerbotAI* ai) { return new TwinEmpWrongDamageTypeTrigger(ai); }
    static Trigger* ouro_submerged(PlayerbotAI* ai) { return new OuroSubmergedTrigger(ai); }
    static Trigger* ouro_scarabs_up(PlayerbotAI* ai) { return new OuroScarabsUpTrigger(ai); }
    static Trigger* cthun_dark_glare(PlayerbotAI* ai) { return new CthunDarkGlareTrigger(ai); }
    static Trigger* cthun_tentacles_up(PlayerbotAI* ai) { return new CthunTentaclesUpTrigger(ai); }
    static Trigger* cthun_giant_tentacle_up(PlayerbotAI* ai) { return new CthunGiantTentacleUpTrigger(ai); }
    static Trigger* cthun_in_stomach(PlayerbotAI* ai) { return new CthunInStomachTrigger(ai); }
};

#endif
