#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/config.h"

// Game includes
#include "hooks.hpp"
#include "f_op/f_op_actor_mng.h"
#include "d/actor/d_a_alink.h"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    svc_log->info(mod_ctx, "manual shield mod initialized");

    ModResult result{};

    result = add_all_hooks();
    if (result != MOD_OK) {
        return result;
    }

    return MOD_OK;
}
}