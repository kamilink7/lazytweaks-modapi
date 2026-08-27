#include "hooks.hpp"

#include "mods/hook.hpp"
#include "mods/svc/log.h"
#include "mods/svc/log.hpp"
#include "mods/service.hpp"

#include "d/actor/d_a_alink.h"

DEFINE_HOOK(&daAlink_c::setShieldGuard, SetShieldGuard);
void on_link_guard_post(ModContext*, void* args, void*, void*) {
    auto link = mods::arg<daAlink_c*>(args, 0);

    if ((link->mProcID == daAlink_c::PROC_GUARD_SLIP && link->mEquipItem != dItemNo_IRONBALL_e) || link->checkSmallUpperGuardAnime() ||
        (link->checkGuardAccept() && !link->checkGrabAnime() && !link->checkUpperReadyThrowAnime() &&
         !link->checkDkCaught2Anime() && !link->checkKandelaarSwingAnime() && !link->checkCutDashAnime() &&
         !link->checkCutDashChargeAnime() && (!link->checkEquipAnime() || link->checkUpperGuardAnime()) &&
         !link->checkRideOn() && link->checkGuardActionChange() && mDoCPd_c::getHoldLockR(PAD_1)))
    {
        link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    } else {
       link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    }
}

#define ADD_POST_HOOK(defined_hook, function, original)                                            \
    result = mods::hook::add_post<defined_hook>(function);                                         \
    if (result != MOD_OK) {                                                                        \
        mods::log::debug(                                                                          \
            "failed to add post hook to" #original ", Result {}", static_cast<int>(result));       \
        return result;                                                                             \
    }

#define ADD_PRE_HOOK(defined_hook, function, original)                                             \
    result = mods::hook::add_pre<defined_hook>(function);                                          \
    if (result != MOD_OK) {                                                                        \
        mods::log::debug(                                                                          \
            "failed to add pre hook to" #original ", Result {}", static_cast<int>(result));        \
        return result;                                                                             \
    }

ModResult add_all_hooks() {
    ModResult result{};

    ADD_POST_HOOK(SetShieldGuard, on_link_guard_post, daAlink_c::setShieldGuard);

    return MOD_OK;
}