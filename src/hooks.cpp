#include "hooks.hpp"

#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"
#include "mods/service.hpp"

#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "m_Do/m_Do_controller_pad.h"

// Helpers

namespace {

/// Fork equivalent: daAlink_c::checkShieldCrouch()
bool checkShieldCrouch(daAlink_c* link) {
    u8 s_getSelectEquipShield = dComIfGs_getSelectEquipShield(); // checkShieldGet() might need this reference?

    // checkShieldGet() is a static member of daPy_py_c, so it needs no instance
    return daPy_py_c::checkShieldGet() && !link->checkAttentionLock() && !link->checkGrabAnime() &&
           !link->checkUpperReadyThrowAnime() && !link->wallGrabTrigger() &&
           !link->checkFmChainGrabAnime() && mDoCPd_c::getHoldLockR(PAD_1);
}

constexpr int CUT_NM_PARAM_COMBO_STAB = 3; // enum not accessible, declared here as value for clarity

/// Fork equivalent: daAlink_c::setShieldCrouch()
void setShieldCrouch(daAlink_c* link) {
    if (link->mNormalSpeed >= 10.0f) {
        link->seStartMapInfoLevel(Z2SE_FN_LINK_SLIP);
        link->setFootEffectProcType(1);
    }

    cLib_chaseF(&link->mNormalSpeed, 0.0f, 1.75f);
    const int cutDir = link->getCutDirection();
    link->setSingleAnimeBaseSpeed(daAlink_c::ANM_CROUCH_DEF,
                                  link->mpHIO->mGuard.m.mCrouchGuardAnmSpeed,
                                  link->mpHIO->mGuard.m.mCrouchGuardInterpolation);
    link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);

    if (link->checkItemSwordEquip() && mDoCPd_c::getTrigB(PAD_1)) {
        if (cutDir == daAlink_c::DIR_FORWARD) {
            link->procCutNormalInit(CUT_NM_PARAM_COMBO_STAB);
            link->resetCombo(TRUE);
        } else {
            link->setCutDash(0, 0);
        }
    }
}

}

// Manual shielding: hold R to shield, shield bash is now R+B

DEFINE_HOOK(&daAlink_c::setShieldGuard, SetShieldGuard);

void on_link_guard_post(ModContext*, void* args, void*, void*) {
    auto link = mods::arg<daAlink_c*>(args, 0);

    if ((link->mProcID == daAlink_c::PROC_GUARD_SLIP &&
         link->mEquipItem != dItemNo_IRONBALL_e) ||
        link->checkSmallUpperGuardAnime() ||
        (link->checkGuardAccept() && !link->checkGrabAnime() &&
         !link->checkUpperReadyThrowAnime() && !link->checkDkCaught2Anime() &&
         !link->checkKandelaarSwingAnime() && !link->checkCutDashAnime() &&
         !link->checkCutDashChargeAnime() &&
         (!link->checkEquipAnime() || link->checkUpperGuardAnime()) && !link->checkRideOn() &&
         link->checkGuardActionChange() && mDoCPd_c::getHoldLockR(PAD_1)))
    {
        link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    } else {
        link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    }
}

// Prevent sword from swinging while holding R

DEFINE_HOOK(&daAlink_c::swordSwingTrigger, SwordSwingTrigger);
DEFINE_HOOK(&daAlink_c::setStickData, SetStickData);

void on_sword_swing_trigger_post(ModContext*, void*, void* retval, void*) {
    if (mDoCPd_c::getHoldLockR(PAD_1)) {
        *static_cast<BOOL*>(retval) = FALSE;
    }
}

void on_set_stick_data_post(ModContext*, void* args, void*, void*) {
    if (!mDoCPd_c::getHoldLockR(PAD_1)) {
        return;
    }

    auto link = mods::arg<daAlink_c*>(args, 0);
    if ((link->mItemTrigger & daAlink_c::BTN_B) != 0) {
        link->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_B);
    }
}

// Shield attack handling

DEFINE_HOOK(&daAlink_c::procGuardAttackInit, ProcGuardAttackInit);
DEFINE_HOOK_SYMBOL("dComIfGp_setRStatus", void(u8, u8), ComIfSetRStatus);
DEFINE_HOOK(&daAlink_c::spActionTrigger, SpActionTrigger);

namespace {
bool shieldAttackRequested() {
    return mDoCPd_c::getHoldLockR(PAD_1) && mDoCPd_c::getTrigB(PAD_1);
}

bool s_shieldAttackPending = false;
}

HookAction on_guard_attack_init_pre(ModContext*, void*, void* retval, void*) {
    if (shieldAttackRequested()) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

// Free function: arg 0 is `status`, arg 1 is `flag`
HookAction on_com_set_r_status_pre(ModContext*, void* args, void*, void*) {
    if (mods::arg<u8>(args, 0) != BUTTON_STATUS_SHIELD_ATTACK) {
        return HOOK_CONTINUE;
    }
    s_shieldAttackPending = true;
    return HOOK_SKIP_ORIGINAL;
}

void on_sp_action_trigger_post(ModContext*, void* args, void* retval, void*) {
    if (!s_shieldAttackPending) {
        return;
    }
    s_shieldAttackPending = false;

    const bool rHeld = mDoCPd_c::getHoldLockR(PAD_1);
    if (rHeld) {
        mods::arg<daAlink_c*>(args, 0)->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
    }

    *static_cast<BOOL*>(retval) = (rHeld && mDoCPd_c::getTrigB(PAD_1)) ? TRUE : FALSE;
}

// Crouch shielding

DEFINE_HOOK(&daAlink_c::procWait, ProcWait);
DEFINE_HOOK(&daAlink_c::procMove, ProcMove);

HookAction on_proc_wait_pre(ModContext*, void* args, void* retval, void*) {
    auto link = mods::arg<daAlink_c*>(args, 0);
    if (!checkShieldCrouch(link)) {
        return HOOK_CONTINUE;
    }

    setShieldCrouch(link);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_proc_move_pre(ModContext*, void* args, void* retval, void*) {
    auto link = mods::arg<daAlink_c*>(args, 0);
    if (!checkShieldCrouch(link)) {
        return HOOK_CONTINUE;
    }

    link->setFootEffectProcType(3);
    setShieldCrouch(link);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

// Crouch shielding hit animation handling

DEFINE_HOOK(&daAlink_c::setSmallGuard, SetSmallGuard);

void on_small_guard_replace(ModContext*, void* args, void*, void*) {
    auto link = mods::arg<daAlink_c*>(args, 0);
    auto objinf = mods::arg<dCcD_GObjInf*>(args, 1);

    link->setUpperAnimeBase(checkShieldCrouch(link) ? dRes_ID_ALANM_BCK_CROUCHDEFS_e
                                                    : dRes_ID_ALANM_BCK_ATDEFS_e);

    cXyz* dmg_vec = link->getDamageVec(objinf);

    link->mBodyAngle.y = (dmg_vec->atan2sX_Z() + 0x8000) - link->shape_angle.y;
    if (abs(link->mBodyAngle.y) > 0x7000) {
        link->mBodyAngle.y = 0;
    } else {
        link->mBodyAngle.y = cLib_minMaxLimit<s16>((link->mBodyAngle.y), -link->mpHIO->mGuard.m.mSmallGuardLRAngleMax,
            link->mpHIO->mGuard.m.mSmallGuardLRAngleMax);
    }

    link->mBodyAngle.x =
        cLib_minMaxLimit<s16>(cM_atan2s(dmg_vec->y, dmg_vec->absXZ()),
                              -link->mpHIO->mGuard.m.mSmallGuardFBAngleMax,
                              link->mpHIO->mGuard.m.mSmallGuardFBAngleMax);
}

DEFINE_HOOK(&daAlink_c::procGuardSlipInit, ProcGuardSlipInit);
DEFINE_HOOK(&daAlink_c::setSingleAnimeParam, SetSingleAnimeParam);
DEFINE_HOOK(&daAlink_c::setSingleAnime, SetSingleAnime);

namespace {
bool s_inGuardSlipInit = false;
bool s_guardSlipCrouch = false;

bool isCrouchedGuardSlipAnime(void* args) {
    if (!s_inGuardSlipInit) {
        return false;
    }

    const auto anm = mods::arg<daAlink_c::daAlink_ANM>(args, 1);
    if (anm != daAlink_c::ANM_GUARD_LEFT && anm != daAlink_c::ANM_GUARD_RIGHT) {
        return false;  // the Ball-and-Chain branch, or a nested guard-break init
    }

    return checkShieldCrouch(mods::arg<daAlink_c*>(args, 0));
}
}

HookAction on_guard_slip_init_pre(ModContext*, void*, void*, void*) {
    s_inGuardSlipInit = true;
    s_guardSlipCrouch = false;
    return HOOK_CONTINUE;
}

HookAction on_single_anime_param_pre(ModContext*, void* args, void*, void*) {
    if (!isCrouchedGuardSlipAnime(args)) {
        return HOOK_CONTINUE;
    }

    // Keep the crouch animation playing instead of standing Link up
    s_guardSlipCrouch = true;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_single_anime_pre(ModContext*, void* args, void*, void*) {
    if (!isCrouchedGuardSlipAnime(args)) {
        return HOOK_CONTINUE;
    }

    s_guardSlipCrouch = true;
    return HOOK_SKIP_ORIGINAL;
}

void on_guard_slip_init_post(ModContext*, void* args, void*, void*) {
    s_inGuardSlipInit = false;
    if (!s_guardSlipCrouch) {
        return;
    }
    s_guardSlipCrouch = false;

    auto link = mods::arg<daAlink_c*>(args, 0);
    link->setUpperAnimeBase(dRes_ID_ALANM_BCK_CROUCHDEFSS_e);
    link->resetUpperAnime(daAlink_c::UPPER_2, 3.0f);
    link->setUpperGuardAnime(-1.0f);
}

// Let quick transform work while crouch shielding

DEFINE_HOOK(&daAlink_c::handleQuickTransform, HandleQuickTransform);

namespace {
dMeter2Draw_c* s_zAlphaPatched = nullptr;
f32 s_zAlphaSaved = 0.0f;
}

HookAction on_quick_transform_pre(ModContext*, void* args, void*, void*) {
    s_zAlphaPatched = nullptr;

    auto link = mods::arg<daAlink_c*>(args, 0);
    if (!checkShieldCrouch(link) || !link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_8000000)) {
        return HOOK_CONTINUE;
    }

    dMeter2_c* meterClass = g_meter2_info.getMeterClass();
    if (meterClass == nullptr) {
        return HOOK_CONTINUE;
    }

    dMeter2Draw_c* meterDraw = meterClass->getMeterDrawPtr();
    if (meterDraw == nullptr || meterDraw->getButtonZAlpha() == 1.0f) {
        return HOOK_CONTINUE;
    }

    s_zAlphaPatched = meterDraw;
    s_zAlphaSaved = meterDraw->mButtonZAlpha;
    meterDraw->mButtonZAlpha = 1.0f;
    return HOOK_CONTINUE;
}

void on_quick_transform_post(ModContext*, void*, void*, void*) {
    if (s_zAlphaPatched != nullptr) {
        s_zAlphaPatched->mButtonZAlpha = s_zAlphaSaved;
        s_zAlphaPatched = nullptr;
    }
}

// Installation

#define ADD_POST_HOOK(defined_hook, function, original)                                            \
    result = mods::hook::add_post<defined_hook>(function);                                         \
    if (result != MOD_OK) {                                                                        \
        failures++;                                                                                \
        mods::log::error(                                                                          \
            "failed to add post hook to " #original ", Result {}", static_cast<int>(result));      \
    }

#define ADD_PRE_HOOK(defined_hook, function, original)                                             \
    result = mods::hook::add_pre<defined_hook>(function);                                          \
    if (result != MOD_OK) {                                                                        \
        failures++;                                                                                \
        mods::log::error(                                                                          \
            "failed to add pre hook to " #original ", Result {}", static_cast<int>(result));       \
    }

#define ADD_REPLACE_HOOK(defined_hook, function, original)                                         \
    result = mods::hook::replace<defined_hook>(function);                                          \
    if (result != MOD_OK) {                                                                        \
        failures++;                                                                                \
        mods::log::error(                                                                          \
            "failed to add replace hook to " #original ", Result {}", static_cast<int>(result));   \
    }

ModResult add_all_hooks() {
    ModResult result{};
    int failures = 0;

    ADD_POST_HOOK(SetShieldGuard, on_link_guard_post, daAlink_c::setShieldGuard);
    ADD_POST_HOOK(SwordSwingTrigger, on_sword_swing_trigger_post, daAlink_c::swordSwingTrigger);
    ADD_POST_HOOK(SetStickData, on_set_stick_data_post, daAlink_c::setStickData);
    ADD_PRE_HOOK(ProcGuardAttackInit, on_guard_attack_init_pre, daAlink_c::procGuardAttackInit);
    ADD_PRE_HOOK(ComIfSetRStatus, on_com_set_r_status_pre, dComIfGp_setRStatus);
    ADD_POST_HOOK(SpActionTrigger, on_sp_action_trigger_post, daAlink_c::spActionTrigger);
    ADD_PRE_HOOK(ProcWait, on_proc_wait_pre, daAlink_c::procWait);
    ADD_PRE_HOOK(ProcMove, on_proc_move_pre, daAlink_c::procMove);
    ADD_REPLACE_HOOK(SetSmallGuard, on_small_guard_replace, daAlink_c::setSmallGuard);
    ADD_PRE_HOOK(ProcGuardSlipInit, on_guard_slip_init_pre, daAlink_c::procGuardSlipInit);
    ADD_POST_HOOK(ProcGuardSlipInit, on_guard_slip_init_post, daAlink_c::procGuardSlipInit);
    ADD_PRE_HOOK(SetSingleAnimeParam, on_single_anime_param_pre, daAlink_c::setSingleAnimeParam);
    ADD_PRE_HOOK(SetSingleAnime, on_single_anime_pre, daAlink_c::setSingleAnime);
    ADD_PRE_HOOK(HandleQuickTransform, on_quick_transform_pre, daAlink_c::handleQuickTransform);
    ADD_POST_HOOK(HandleQuickTransform, on_quick_transform_post, daAlink_c::handleQuickTransform);

    if (failures != 0) {
        mods::log::error("{} of 15 hooks failed to install; those features are inactive",
                         failures);
    } else {
        mods::log::info("all 15 hooks installed");
    }

    return MOD_OK;
}

ModResult remove_all_hooks() {
    s_shieldAttackPending = false;
    s_inGuardSlipInit = false;
    s_guardSlipCrouch = false;
    s_zAlphaPatched = nullptr;

    mods::hook::uninstall<SetShieldGuard>();
    mods::hook::uninstall<SetSmallGuard>();
    mods::hook::uninstall<ProcGuardSlipInit>();
    mods::hook::uninstall<SetSingleAnimeParam>();
    mods::hook::uninstall<SetSingleAnime>();
    mods::hook::uninstall<SwordSwingTrigger>();
    mods::hook::uninstall<ComIfSetRStatus>();
    mods::hook::uninstall<SpActionTrigger>();
    mods::hook::uninstall<SetStickData>();
    mods::hook::uninstall<ProcGuardAttackInit>();
    mods::hook::uninstall<ProcWait>();
    mods::hook::uninstall<ProcMove>();
    mods::hook::uninstall<HandleQuickTransform>();

    return MOD_OK;
}
