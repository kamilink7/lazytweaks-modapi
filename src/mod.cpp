#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/config.h"

// Game includes
#include "f_op/f_op_actor_mng.h"
#include "d/d_cc_uty.h"
#include "d/actor/d_a_player.h"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(ConfigService, svc_config);

ConfigVarHandle sword_multiplier = 0;

DEFINE_HOOK(cc_at_check, AttackCheck);

static HookAction on_at_check_pre(ModContext*, void* args, void*, void*) {
    if (sword_multiplier == 100 || daPy_py_c::checkNowWolf()) return HOOK_CONTINUE;

    auto* atInfo = mods::arg<dCcU_AtInfo*>(args, 0);
    if (atInfo->mHitType != HIT_TYPE_LINK_NORMAL_ATTACK) return HOOK_CONTINUE;

    atInfo->mAttackPower *= sword_multiplier * 0.01;
    return HOOK_CONTINUE;
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    ModResult result = mods::hook_add_pre<AttackCheck>(svc_hook, on_at_check_pre);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install on_at_check_pre");
        return result;
    }

    svc_log->info(mod_ctx, "lazytweaks initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
