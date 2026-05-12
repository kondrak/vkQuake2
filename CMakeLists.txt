cmake_minimum_required(VERSION 3.16)
project(vkQuake2 C CXX ASM)

# Game library arch suffix (mirrors the Makefile ARCH variable)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(GAME_ARCH "x64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(GAME_ARCH "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64le")
    set(GAME_ARCH "ppc64le")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
    set(GAME_ARCH "armv7l")
else()
    set(GAME_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)

find_path(X11_Xxf86dga_INCLUDE_PATH X11/extensions/Xxf86dga.h)
find_library(X11_Xxf86dga_LIB Xxf86dga)
if(NOT X11_Xxf86dga_INCLUDE_PATH OR NOT X11_Xxf86dga_LIB)
    message(FATAL_ERROR "libXxf86dga development files not found")
endif()

if(NOT X11_Xxf86vm_FOUND)
    message(FATAL_ERROR "libXxf86vm development files not found")
endif()

add_compile_definitions(stricmp=strcasecmp _GNU_SOURCE)
add_compile_options(-Wno-format-truncation -Wno-unused-result -Wno-format-overflow)

option(ENABLE_DEBUG_LABELS "Attach Vulkan debug labels to GPU objects (requires VK_EXT_debug_utils)" OFF)

# ── quake2 executable ─────────────────────────────────────────────────────────

set(QUAKE2_SOURCES
    client/cl_cin.c
    client/cl_ents.c
    client/cl_fx.c
    client/cl_input.c
    client/cl_inv.c
    client/cl_main.c
    client/cl_newfx.c
    client/cl_parse.c
    client/cl_pred.c
    client/cl_scrn.c
    client/cl_tent.c
    client/cl_view.c
    client/console.c
    client/keys.c
    client/menu.c
    client/qmenu.c
    client/snd_dma.c
    client/snd_mem.c
    client/snd_miniaudio.c
    client/snd_mix.c
    game/m_flash.c
    game/q_shared.c
    qcommon/cmd.c
    qcommon/cmodel.c
    qcommon/common.c
    qcommon/crc.c
    qcommon/cvar.c
    qcommon/files.c
    qcommon/md4.c
    qcommon/net_chan.c
    qcommon/pmove.c
    server/sv_ccmds.c
    server/sv_ents.c
    server/sv_game.c
    server/sv_init.c
    server/sv_main.c
    server/sv_send.c
    server/sv_user.c
    server/sv_world.c
    linux/cd_linux.c
    linux/glob.c
    linux/net_udp.c
    linux/q_shlinux.c
    linux/snd_alsa.c
    linux/sys_linux.c
    linux/vid_menu.c
    linux/vid_so.c
    linux/snd_mixa.s
)

set_source_files_properties(linux/snd_mixa.s PROPERTIES
    COMPILE_FLAGS "-x assembler-with-cpp -DELF"
)

add_executable(quake2 ${QUAKE2_SOURCES})
target_include_directories(quake2 PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(quake2 PRIVATE -std=gnu11)
target_link_libraries(quake2 PRIVATE dl m asound pthread)

# ── ref_vk shared library ─────────────────────────────────────────────────────

add_library(ref_vk SHARED
    ref_vk/vk_buffer.c
    ref_vk/vk_cmd.c
    ref_vk/vk_common.c
    ref_vk/vk_device.c
    ref_vk/vk_draw.c
    ref_vk/vk_image.c
    ref_vk/vk_light.c
    ref_vk/vk_mem_alloc.cpp
    ref_vk/vk_mesh.c
    ref_vk/vk_model.c
    ref_vk/vk_pipeline.c
    ref_vk/vk_rmain.c
    ref_vk/vk_rmisc.c
    ref_vk/vk_rsurf.c
    ref_vk/vk_shaders.c
    ref_vk/vk_swapchain.c
    ref_vk/vk_validation.c
    ref_vk/vk_warp.c
    linux/vk_imp.c
    game/q_shared.c
    linux/q_shlinux.c
    linux/glob.c
)

target_include_directories(ref_vk PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
    ${X11_Xxf86dga_INCLUDE_PATH}
    ${X11_Xxf86vm_INCLUDE_PATH}
)
target_compile_options(ref_vk PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-fPIC -std=c99>
    $<$<COMPILE_LANGUAGE:CXX>:-fPIC -std=c++17>
)
set_target_properties(ref_vk PROPERTIES PREFIX "" OUTPUT_NAME "ref_vk")
if(ENABLE_DEBUG_LABELS)
    target_compile_definitions(ref_vk PRIVATE ENABLE_DEBUG_LABELS)
endif()
target_link_libraries(ref_vk PRIVATE
    ${Vulkan_LIBRARIES}
    ${X11_LIBRARIES}
    ${X11_Xext_LIB}
    ${X11_Xxf86dga_LIB}
    ${X11_Xxf86vm_LIB}
    m stdc++
)

# ── baseq2 game shared library ────────────────────────────────────────────────

add_library(baseq2_game SHARED
    game/g_ai.c
    game/g_chase.c
    game/g_cmds.c
    game/g_combat.c
    game/g_func.c
    game/g_items.c
    game/g_main.c
    game/g_misc.c
    game/g_monster.c
    game/g_phys.c
    game/g_save.c
    game/g_spawn.c
    game/g_svcmds.c
    game/g_target.c
    game/g_trigger.c
    game/g_turret.c
    game/g_utils.c
    game/g_weapon.c
    game/m_actor.c
    game/m_berserk.c
    game/m_boss2.c
    game/m_boss3.c
    game/m_boss31.c
    game/m_boss32.c
    game/m_brain.c
    game/m_chick.c
    game/m_flash.c
    game/m_flipper.c
    game/m_float.c
    game/m_flyer.c
    game/m_gladiator.c
    game/m_gunner.c
    game/m_hover.c
    game/m_infantry.c
    game/m_insane.c
    game/m_medic.c
    game/m_move.c
    game/m_mutant.c
    game/m_parasite.c
    game/m_soldier.c
    game/m_supertank.c
    game/m_tank.c
    game/p_client.c
    game/p_hud.c
    game/p_trail.c
    game/p_view.c
    game/p_weapon.c
    game/q_shared.c
)

target_include_directories(baseq2_game PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(baseq2_game PRIVATE -fPIC -std=c99)
set_target_properties(baseq2_game PROPERTIES
    PREFIX ""
    OUTPUT_NAME "game${GAME_ARCH}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/baseq2"
)
