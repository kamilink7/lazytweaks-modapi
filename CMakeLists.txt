cmake_minimum_required(VERSION 3.26)
project(lazytweaks CXX)

# The Dusklight version to build against: a release tag (e.g. "v1.5.0") or a commit SHA.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "a775418c667f748724c24283b9bad62ebe2dc48b")
endif ()
# Fetches the Dusklight sources into ./dusklight and provides DUSKLIGHT_DIR.
# Use -DDUSKLIGHT_DIR= to point at an existing Dusklight checkout.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

add_mod(lazytweaks
        FEATURES game fmt      # remove for service/asset-only mods; add webgpu for GfxService
        SOURCES src/mod.cpp src/hooks.cpp src/hooks.hpp
        MOD_JSON mod.json
        RES_DIR res            # mod resources, including icon.png and banner.png
)

# Automatically copy the built .dusk archive to the user mods directory after every build (Linux only)
if (UNIX AND NOT APPLE)
    set(DUSKLIGHT_USER_MODS_DIR "$ENV{HOME}/.local/share/TwilitRealm/Dusklight/mods")

    add_custom_command(TARGET lazytweaks_package POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DUSKLIGHT_USER_MODS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/mods/lazytweaks.dusk"
            "${DUSKLIGHT_USER_MODS_DIR}/lazytweaks.dusk"
            COMMENT "Copying .dusk package to ${DUSKLIGHT_USER_MODS_DIR}"
    )
endif ()
