#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.h"

// Game includes
#include "hooks.hpp"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* error) {
    svc_log->info(mod_ctx, "manual shield mod initialized");

    const ModResult result = add_all_hooks();
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install hooks");
    }

    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return remove_all_hooks();
}
}
