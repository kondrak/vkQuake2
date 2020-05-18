#
# vkQuake2 for MacOS
#

CC=gcc
BASE_CFLAGS=-Wno-unused-result -Wno-dangling-else -Wno-pointer-sign -Wno-pointer-bool-conversion -Wno-nullability-completeness
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O3
DEBUG_CFLAGS=$(BASE_CFLAGS) -g -D_DEBUG
LDFLAGS=-framework CoreAudio

VKLDFLAGS=-L$(VULKAN_SDK)/macOS/lib -Xlinker -rpath -Xlinker $(VULKAN_SDK)/macOS/lib -lvulkan -lstdc++ -framework Cocoa -framework QuartzCore
VKCFLAGS=-I$(VULKAN_SDK)/macOS/include

SHLIBCFLAGS=-fPIC -std=c99
SHLIBCPPFLAGS=-fPIC -std=c++11
SHLIBLDFLAGS=-shared

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_DED_CC=$(CC) $(CFLAGS) -DDEDICATED_ONLY -o $@ -c $<
DO_O_CC=$(CC) $(CFLAGS) -O -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
DO_SHLIB_O_CC=$(CC) $(CFLAGS) -O $(SHLIBCFLAGS) -o $@ -c $<
DO_VK_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) $(VKCFLAGS) -o $@ -c $<
DO_VK_SHLIB_CPP=$(CC) $(CFLAGS) $(SHLIBCPPFLAGS) $(VKCFLAGS) -o $@ -c $<
DO_VK_SHLIB_O_CC=$(CC) $(CFLAGS) -O $(SHLIBCFLAGS) $(VKCFLAGS) -o $@ -c $<

help:
	@echo "* To use Developer Command Line Tools:"
	@echo "build with: make release/debug/all"
	@echo "clean with: make clean/clean-debug/clean-release"
	@echo " "
	@echo "* To use XCode:"
	@echo "build with: make release-xcode/debug-xcode"
	@echo "clean with: make clean-xcode"

#############################################################################
# SETUP AND BUILD
#############################################################################

TARGETS=$(BUILDDIR)/quake2 \
	$(BUILDDIR)/q2ded \
	$(BUILDDIR)/baseq2/game.dylib \
	$(BUILDDIR)/ctf/game.dylib \
	$(BUILDDIR)/ref_vk.dylib \
	$(BUILDDIR)/xatrix/game.dylib \
	$(BUILDDIR)/rogue/game.dylib \
	$(BUILDDIR)/zaero/game.dylib

debug:
	@-mkdir -p debug \
		debug/baseq2 \
		debug/client \
		debug/ded \
		debug/ref_vk \
		debug/game \
		debug/ctf \
		debug/xatrix \
		debug/rogue \
		debug/zaero
	$(MAKE) targets BUILDDIR=debug CFLAGS="$(DEBUG_CFLAGS)"

release:
	@-mkdir -p release \
		release/baseq2 \
		release/client \
		release/ded \
		release/ref_vk \
		release/game \
		release/ctf \
		release/xatrix \
		release/rogue \
		release/zaero
	$(MAKE) targets BUILDDIR=release CFLAGS="$(RELEASE_CFLAGS)"

all: debug release

targets: $(TARGETS)

#############################################################################
# CLIENT/SERVER
#############################################################################

QUAKE2_OBJS = \
	$(BUILDDIR)/client/cd_null.o \
	$(BUILDDIR)/client/cl_cin.o \
	$(BUILDDIR)/client/cl_ents.o \
	$(BUILDDIR)/client/cl_fx.o \
	$(BUILDDIR)/client/cl_input.o \
	$(BUILDDIR)/client/cl_inv.o \
	$(BUILDDIR)/client/cl_main.o \
	$(BUILDDIR)/client/cl_newfx.o \
	$(BUILDDIR)/client/cl_parse.o \
	$(BUILDDIR)/client/cl_pred.o \
	$(BUILDDIR)/client/cl_scrn.o \
	$(BUILDDIR)/client/cl_tent.o \
	$(BUILDDIR)/client/cl_view.o \
	$(BUILDDIR)/client/cmd.o \
	$(BUILDDIR)/client/cmodel.o \
	$(BUILDDIR)/client/common.o \
	$(BUILDDIR)/client/console.o \
	$(BUILDDIR)/client/crc.o \
	$(BUILDDIR)/client/cvar.o \
	$(BUILDDIR)/client/files.o \
	$(BUILDDIR)/client/glob.o \
	$(BUILDDIR)/client/keys.o \
	$(BUILDDIR)/client/m_flash.o \
	$(BUILDDIR)/client/md4.o \
	$(BUILDDIR)/client/menu.o \
	$(BUILDDIR)/client/net_chan.o \
	$(BUILDDIR)/client/net_udp.o \
	$(BUILDDIR)/client/pmove.o \
	$(BUILDDIR)/client/q_shared.o \
	$(BUILDDIR)/client/q_shmacos.o \
	$(BUILDDIR)/client/qmenu.o \
	$(BUILDDIR)/client/snd_dma.o \
	$(BUILDDIR)/client/snd_macos.o \
	$(BUILDDIR)/client/snd_miniaudio.o \
	$(BUILDDIR)/client/snd_mem.o \
	$(BUILDDIR)/client/snd_mix.o \
	$(BUILDDIR)/client/sv_ccmds.o \
	$(BUILDDIR)/client/sv_ents.o \
	$(BUILDDIR)/client/sv_game.o \
	$(BUILDDIR)/client/sv_init.o \
	$(BUILDDIR)/client/sv_main.o \
	$(BUILDDIR)/client/sv_send.o \
	$(BUILDDIR)/client/sv_user.o \
	$(BUILDDIR)/client/sv_world.o \
	$(BUILDDIR)/client/sys_macos.o \
	$(BUILDDIR)/client/vid_dsym.o \
	$(BUILDDIR)/client/vid_menu.o

$(BUILDDIR)/quake2 : $(QUAKE2_OBJS)
	$(CC) $(CFLAGS) -o $@ $(QUAKE2_OBJS) $(LDFLAGS)

$(BUILDDIR)/client/cd_null.o :    ../null/cd_null.c
	$(DO_CC)

$(BUILDDIR)/client/cl_cin.o :     ../client/cl_cin.c
	$(DO_CC)

$(BUILDDIR)/client/cl_ents.o :    ../client/cl_ents.c
	$(DO_CC)

$(BUILDDIR)/client/cl_fx.o :      ../client/cl_fx.c
	$(DO_CC)

$(BUILDDIR)/client/cl_input.o :   ../client/cl_input.c
	$(DO_CC)

$(BUILDDIR)/client/cl_inv.o :     ../client/cl_inv.c
	$(DO_CC)

$(BUILDDIR)/client/cl_main.o :    ../client/cl_main.c
	$(DO_CC)

$(BUILDDIR)/client/cl_newfx.o :   ../client/cl_newfx.c
	$(DO_CC)

$(BUILDDIR)/client/cl_parse.o :   ../client/cl_parse.c
	$(DO_CC)

$(BUILDDIR)/client/cl_pred.o :    ../client/cl_pred.c
	$(DO_CC)

$(BUILDDIR)/client/cl_scrn.o :    ../client/cl_scrn.c
	$(DO_CC)

$(BUILDDIR)/client/cl_tent.o :    ../client/cl_tent.c
	$(DO_CC)

$(BUILDDIR)/client/cl_view.o :    ../client/cl_view.c
	$(DO_CC)

$(BUILDDIR)/client/cmd.o :        ../qcommon/cmd.c
	$(DO_CC)

$(BUILDDIR)/client/cmodel.o :     ../qcommon/cmodel.c
	$(DO_CC)

$(BUILDDIR)/client/common.o :     ../qcommon/common.c
	$(DO_CC)

$(BUILDDIR)/client/console.o :    ../client/console.c
	$(DO_CC)

$(BUILDDIR)/client/crc.o :        ../qcommon/crc.c
	$(DO_CC)

$(BUILDDIR)/client/cvar.o :       ../qcommon/cvar.c
	$(DO_CC)

$(BUILDDIR)/client/files.o :      ../qcommon/files.c
	$(DO_CC)

$(BUILDDIR)/client/glob.o :       ../linux/glob.c
	$(DO_CC)

$(BUILDDIR)/client/keys.o :       ../client/keys.c
	$(DO_CC)

$(BUILDDIR)/client/m_flash.o :    ../game/m_flash.c
	$(DO_CC)

$(BUILDDIR)/client/md4.o :        ../qcommon/md4.c
	$(DO_CC)

$(BUILDDIR)/client/menu.o :       ../client/menu.c
	$(DO_CC)

$(BUILDDIR)/client/net_chan.o :   ../qcommon/net_chan.c
	$(DO_CC)

$(BUILDDIR)/client/net_udp.o :    ../linux/net_udp.c
	$(DO_CC)

$(BUILDDIR)/client/pmove.o :      ../qcommon/pmove.c
	$(DO_CC)

$(BUILDDIR)/client/q_shared.o :   ../game/q_shared.c
	$(DO_CC)

$(BUILDDIR)/client/q_shmacos.o :  ../macos/q_shmacos.c
	$(DO_O_CC)

$(BUILDDIR)/client/qmenu.o :      ../client/qmenu.c
	$(DO_CC)

$(BUILDDIR)/client/snd_dma.o :    ../client/snd_dma.c
	$(DO_CC)

$(BUILDDIR)/client/snd_macos.o :  ../macos/snd_macos.m
	$(DO_CC)

$(BUILDDIR)/client/snd_miniaudio.o : ../client/snd_miniaudio.c
	$(DO_CC)

$(BUILDDIR)/client/snd_mem.o :    ../client/snd_mem.c
	$(DO_CC)

$(BUILDDIR)/client/snd_mix.o :    ../client/snd_mix.c
	$(DO_CC)

$(BUILDDIR)/client/sv_ccmds.o :   ../server/sv_ccmds.c
	$(DO_CC)

$(BUILDDIR)/client/sv_ents.o :    ../server/sv_ents.c
	$(DO_CC)

$(BUILDDIR)/client/sv_game.o :    ../server/sv_game.c
	$(DO_CC)

$(BUILDDIR)/client/sv_init.o :    ../server/sv_init.c
	$(DO_CC)

$(BUILDDIR)/client/sv_main.o :    ../server/sv_main.c
	$(DO_CC)

$(BUILDDIR)/client/sv_send.o :    ../server/sv_send.c
	$(DO_CC)

$(BUILDDIR)/client/sv_user.o :    ../server/sv_user.c
	$(DO_CC)

$(BUILDDIR)/client/sv_world.o :   ../server/sv_world.c
	$(DO_CC)

$(BUILDDIR)/client/sys_macos.o :  ../macos/sys_macos.c
	$(DO_CC)

$(BUILDDIR)/client/vid_dsym.o :   ../macos/vid_dsym.c
	$(DO_CC)

$(BUILDDIR)/client/vid_menu.o :   ../linux/vid_menu.c
	$(DO_CC)

#############################################################################
# DEDICATED SERVER
#############################################################################

Q2DED_OBJS = \
	$(BUILDDIR)/ded/cmd.o \
	$(BUILDDIR)/ded/cmodel.o \
	$(BUILDDIR)/ded/common.o \
	$(BUILDDIR)/ded/crc.o \
	$(BUILDDIR)/ded/cvar.o \
	$(BUILDDIR)/ded/files.o \
	$(BUILDDIR)/ded/md4.o \
	$(BUILDDIR)/ded/net_chan.o \
	$(BUILDDIR)/ded/sv_ccmds.o \
	$(BUILDDIR)/ded/sv_ents.o \
	$(BUILDDIR)/ded/sv_game.o \
	$(BUILDDIR)/ded/sv_init.o \
	$(BUILDDIR)/ded/sv_main.o \
	$(BUILDDIR)/ded/sv_send.o \
	$(BUILDDIR)/ded/sv_user.o \
	$(BUILDDIR)/ded/sv_world.o \
	$(BUILDDIR)/ded/q_shmacos.o \
	$(BUILDDIR)/ded/sys_macos.o \
	$(BUILDDIR)/ded/glob.o \
	$(BUILDDIR)/ded/net_udp.o \
	$(BUILDDIR)/ded/q_shared.o \
	$(BUILDDIR)/ded/pmove.o \
	$(BUILDDIR)/ded/cl_null.o \
	$(BUILDDIR)/ded/cd_null.o

$(BUILDDIR)/q2ded : $(Q2DED_OBJS)
	$(CC) $(CFLAGS) -o $@ $(Q2DED_OBJS) $(LDFLAGS)

$(BUILDDIR)/ded/cmd.o :        ../qcommon/cmd.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cmodel.o :     ../qcommon/cmodel.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/common.o :     ../qcommon/common.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/crc.o :        ../qcommon/crc.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cvar.o :       ../qcommon/cvar.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/files.o :      ../qcommon/files.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/md4.o :        ../qcommon/md4.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/net_chan.o :   ../qcommon/net_chan.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_ccmds.o :   ../server/sv_ccmds.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_ents.o :    ../server/sv_ents.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_game.o :    ../server/sv_game.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_init.o :    ../server/sv_init.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_main.o :    ../server/sv_main.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_send.o :    ../server/sv_send.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_user.o :    ../server/sv_user.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_world.o :   ../server/sv_world.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/q_shmacos.o :  ../macos/q_shmacos.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sys_macos.o :  ../macos/sys_macos.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/glob.o :       ../linux/glob.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/net_udp.o :    ../linux/net_udp.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/q_shared.o :   ../game/q_shared.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/pmove.o :      ../qcommon/pmove.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cl_null.o :    ../null/cl_null.c    
	$(DO_DED_CC)

$(BUILDDIR)/ded/cd_null.o :    ../null/cd_null.c    
	$(DO_DED_CC)

#############################################################################
# GAME
#############################################################################

GAME_OBJS = \
	$(BUILDDIR)/game/g_ai.o \
	$(BUILDDIR)/game/g_chase.o \
	$(BUILDDIR)/game/g_cmds.o \
	$(BUILDDIR)/game/g_combat.o \
	$(BUILDDIR)/game/g_func.o \
	$(BUILDDIR)/game/g_items.o \
	$(BUILDDIR)/game/g_main.o \
	$(BUILDDIR)/game/g_misc.o \
	$(BUILDDIR)/game/g_monster.o \
	$(BUILDDIR)/game/g_phys.o \
	$(BUILDDIR)/game/g_save.o \
	$(BUILDDIR)/game/g_spawn.o \
	$(BUILDDIR)/game/g_svcmds.o \
	$(BUILDDIR)/game/g_target.o \
	$(BUILDDIR)/game/g_trigger.o \
	$(BUILDDIR)/game/g_turret.o \
	$(BUILDDIR)/game/g_utils.o \
	$(BUILDDIR)/game/g_weapon.o \
	$(BUILDDIR)/game/m_actor.o \
	$(BUILDDIR)/game/m_berserk.o \
	$(BUILDDIR)/game/m_boss2.o \
	$(BUILDDIR)/game/m_boss3.o \
	$(BUILDDIR)/game/m_boss31.o \
	$(BUILDDIR)/game/m_boss32.o \
	$(BUILDDIR)/game/m_brain.o \
	$(BUILDDIR)/game/m_chick.o \
	$(BUILDDIR)/game/m_flash.o \
	$(BUILDDIR)/game/m_flipper.o \
	$(BUILDDIR)/game/m_float.o \
	$(BUILDDIR)/game/m_flyer.o \
	$(BUILDDIR)/game/m_gladiator.o \
	$(BUILDDIR)/game/m_gunner.o \
	$(BUILDDIR)/game/m_hover.o \
	$(BUILDDIR)/game/m_infantry.o \
	$(BUILDDIR)/game/m_insane.o \
	$(BUILDDIR)/game/m_medic.o \
	$(BUILDDIR)/game/m_move.o \
	$(BUILDDIR)/game/m_mutant.o \
	$(BUILDDIR)/game/m_parasite.o \
	$(BUILDDIR)/game/m_soldier.o \
	$(BUILDDIR)/game/m_supertank.o \
	$(BUILDDIR)/game/m_tank.o \
	$(BUILDDIR)/game/p_client.o \
	$(BUILDDIR)/game/p_hud.o \
	$(BUILDDIR)/game/p_trail.o \
	$(BUILDDIR)/game/p_view.o \
	$(BUILDDIR)/game/p_weapon.o \
	$(BUILDDIR)/game/q_shared.o

$(BUILDDIR)/baseq2/game.dylib : $(GAME_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(GAME_OBJS)

$(BUILDDIR)/game/g_ai.o :        ../game/g_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_chase.o :     ../game/g_chase.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_cmds.o :      ../game/g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_combat.o :    ../game/g_combat.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_func.o :      ../game/g_func.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_items.o :     ../game/g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_main.o :      ../game/g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_misc.o :      ../game/g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_monster.o :   ../game/g_monster.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_phys.o :      ../game/g_phys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_save.o :      ../game/g_save.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_spawn.o :     ../game/g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_svcmds.o :    ../game/g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_target.o :    ../game/g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_trigger.o :   ../game/g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_turret.o :    ../game/g_turret.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_utils.o :     ../game/g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_weapon.o :    ../game/g_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_actor.o :     ../game/m_actor.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_berserk.o :   ../game/m_berserk.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_boss2.o :     ../game/m_boss2.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_boss3.o :     ../game/m_boss3.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_boss31.o :    ../game/m_boss31.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_boss32.o :    ../game/m_boss32.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_brain.o :     ../game/m_brain.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_chick.o :     ../game/m_chick.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_flash.o :     ../game/m_flash.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_flipper.o :   ../game/m_flipper.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_float.o :     ../game/m_float.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_flyer.o :     ../game/m_flyer.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_gladiator.o : ../game/m_gladiator.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_gunner.o :    ../game/m_gunner.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_hover.o :     ../game/m_hover.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_infantry.o :  ../game/m_infantry.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_insane.o :    ../game/m_insane.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_medic.o :     ../game/m_medic.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_move.o :      ../game/m_move.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_mutant.o :    ../game/m_mutant.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_parasite.o :  ../game/m_parasite.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_soldier.o :   ../game/m_soldier.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_supertank.o : ../game/m_supertank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/m_tank.o :      ../game/m_tank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/p_client.o :    ../game/p_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/p_hud.o :       ../game/p_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/p_trail.o :     ../game/p_trail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/p_view.o :      ../game/p_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/p_weapon.o :    ../game/p_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/q_shared.o :    ../game/q_shared.c
	$(DO_SHLIB_CC)

#############################################################################
# CTF
#############################################################################

CTF_OBJS = \
	$(BUILDDIR)/ctf/g_ai.o \
	$(BUILDDIR)/ctf/g_chase.o \
	$(BUILDDIR)/ctf/g_cmds.o \
	$(BUILDDIR)/ctf/g_combat.o \
	$(BUILDDIR)/ctf/g_ctf.o \
	$(BUILDDIR)/ctf/g_func.o \
	$(BUILDDIR)/ctf/g_items.o \
	$(BUILDDIR)/ctf/g_main.o \
	$(BUILDDIR)/ctf/g_misc.o \
	$(BUILDDIR)/ctf/g_monster.o \
	$(BUILDDIR)/ctf/g_phys.o \
	$(BUILDDIR)/ctf/g_save.o \
	$(BUILDDIR)/ctf/g_spawn.o \
	$(BUILDDIR)/ctf/g_svcmds.o \
	$(BUILDDIR)/ctf/g_target.o \
	$(BUILDDIR)/ctf/g_trigger.o \
	$(BUILDDIR)/ctf/g_utils.o \
	$(BUILDDIR)/ctf/g_weapon.o \
	$(BUILDDIR)/ctf/m_move.o \
	$(BUILDDIR)/ctf/p_client.o \
	$(BUILDDIR)/ctf/p_hud.o \
	$(BUILDDIR)/ctf/p_menu.o \
	$(BUILDDIR)/ctf/p_trail.o \
	$(BUILDDIR)/ctf/p_view.o \
	$(BUILDDIR)/ctf/p_weapon.o \
	$(BUILDDIR)/ctf/q_shared.o

$(BUILDDIR)/ctf/game.dylib : $(CTF_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(CTF_OBJS)

$(BUILDDIR)/ctf/g_ai.o :       ../ctf/g_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_chase.o :    ../ctf/g_chase.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_cmds.o :     ../ctf/g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_combat.o :   ../ctf/g_combat.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_ctf.o :      ../ctf/g_ctf.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_func.o :     ../ctf/g_func.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_items.o :    ../ctf/g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_main.o :     ../ctf/g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_misc.o :     ../ctf/g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_monster.o :  ../ctf/g_monster.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_phys.o :     ../ctf/g_phys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_save.o :     ../ctf/g_save.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_spawn.o :    ../ctf/g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_svcmds.o :   ../ctf/g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_target.o :   ../ctf/g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_trigger.o :  ../ctf/g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_utils.o :    ../ctf/g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/g_weapon.o :   ../ctf/g_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/m_move.o :     ../ctf/m_move.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/p_client.o :   ../ctf/p_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/p_hud.o :      ../ctf/p_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/p_menu.o :     ../ctf/p_menu.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/p_trail.o :    ../ctf/p_trail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/p_view.o :     ../ctf/p_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/p_weapon.o :   ../ctf/p_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ctf/q_shared.o :   ../ctf/q_shared.c
	$(DO_SHLIB_CC)

#############################################################################
# XATRIX
#############################################################################

XATRIX_OBJS = \
	$(BUILDDIR)/xatrix/g_ai.o \
	$(BUILDDIR)/xatrix/g_chase.o \
	$(BUILDDIR)/xatrix/g_cmds.o \
	$(BUILDDIR)/xatrix/g_combat.o \
	$(BUILDDIR)/xatrix/g_func.o \
	$(BUILDDIR)/xatrix/g_items.o \
	$(BUILDDIR)/xatrix/g_main.o \
	$(BUILDDIR)/xatrix/g_misc.o \
	$(BUILDDIR)/xatrix/g_monster.o \
	$(BUILDDIR)/xatrix/g_phys.o \
	$(BUILDDIR)/xatrix/g_save.o \
	$(BUILDDIR)/xatrix/g_spawn.o \
	$(BUILDDIR)/xatrix/g_svcmds.o \
	$(BUILDDIR)/xatrix/g_target.o \
	$(BUILDDIR)/xatrix/g_trigger.o \
	$(BUILDDIR)/xatrix/g_turret.o \
	$(BUILDDIR)/xatrix/g_utils.o \
	$(BUILDDIR)/xatrix/g_weapon.o \
	$(BUILDDIR)/xatrix/m_actor.o \
	$(BUILDDIR)/xatrix/m_berserk.o \
	$(BUILDDIR)/xatrix/m_boss2.o \
	$(BUILDDIR)/xatrix/m_boss3.o \
	$(BUILDDIR)/xatrix/m_boss5.o \
	$(BUILDDIR)/xatrix/m_boss31.o \
	$(BUILDDIR)/xatrix/m_boss32.o \
	$(BUILDDIR)/xatrix/m_brain.o \
	$(BUILDDIR)/xatrix/m_chick.o \
	$(BUILDDIR)/xatrix/m_fixbot.o \
	$(BUILDDIR)/xatrix/m_flash.o \
	$(BUILDDIR)/xatrix/m_flipper.o \
	$(BUILDDIR)/xatrix/m_float.o \
	$(BUILDDIR)/xatrix/m_flyer.o \
	$(BUILDDIR)/xatrix/m_gekk.o \
	$(BUILDDIR)/xatrix/m_gladb.o \
	$(BUILDDIR)/xatrix/m_gladiator.o \
	$(BUILDDIR)/xatrix/m_gunner.o \
	$(BUILDDIR)/xatrix/m_hover.o \
	$(BUILDDIR)/xatrix/m_infantry.o \
	$(BUILDDIR)/xatrix/m_insane.o \
	$(BUILDDIR)/xatrix/m_medic.o \
	$(BUILDDIR)/xatrix/m_move.o \
	$(BUILDDIR)/xatrix/m_mutant.o \
	$(BUILDDIR)/xatrix/m_parasite.o \
	$(BUILDDIR)/xatrix/m_soldier.o \
	$(BUILDDIR)/xatrix/m_supertank.o \
	$(BUILDDIR)/xatrix/m_tank.o \
	$(BUILDDIR)/xatrix/p_client.o \
	$(BUILDDIR)/xatrix/p_hud.o \
	$(BUILDDIR)/xatrix/p_trail.o \
	$(BUILDDIR)/xatrix/p_view.o \
	$(BUILDDIR)/xatrix/p_weapon.o \
	$(BUILDDIR)/xatrix/q_shared.o

$(BUILDDIR)/xatrix/game.dylib : $(XATRIX_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(XATRIX_OBJS)

$(BUILDDIR)/xatrix/g_ai.o :        ../xatrix/g_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_chase.o :     ../xatrix/g_chase.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_cmds.o :      ../xatrix/g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_combat.o :    ../xatrix/g_combat.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_func.o :      ../xatrix/g_func.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_items.o :     ../xatrix/g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_main.o :      ../xatrix/g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_misc.o :      ../xatrix/g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_monster.o :   ../xatrix/g_monster.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_phys.o :      ../xatrix/g_phys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_save.o :      ../xatrix/g_save.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_spawn.o :     ../xatrix/g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_svcmds.o :    ../xatrix/g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_target.o :    ../xatrix/g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_trigger.o :   ../xatrix/g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_turret.o :    ../xatrix/g_turret.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_utils.o :     ../xatrix/g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/g_weapon.o :    ../xatrix/g_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_actor.o :     ../xatrix/m_actor.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_berserk.o :   ../xatrix/m_berserk.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_boss2.o :     ../xatrix/m_boss2.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_boss3.o :     ../xatrix/m_boss3.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_boss5.o :     ../xatrix/m_boss5.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_boss31.o :    ../xatrix/m_boss31.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_boss32.o :    ../xatrix/m_boss32.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_brain.o :     ../xatrix/m_brain.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_chick.o :     ../xatrix/m_chick.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_fixbot.o :    ../xatrix/m_fixbot.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_flash.o :     ../xatrix/m_flash.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_flipper.o :   ../xatrix/m_flipper.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_float.o :     ../xatrix/m_float.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_flyer.o :     ../xatrix/m_flyer.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_gekk.o :      ../xatrix/m_gekk.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_gladb.o :     ../xatrix/m_gladb.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_gladiator.o : ../xatrix/m_gladiator.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_gunner.o :    ../xatrix/m_gunner.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_hover.o :     ../xatrix/m_hover.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_infantry.o :  ../xatrix/m_infantry.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_insane.o :    ../xatrix/m_insane.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_medic.o :     ../xatrix/m_medic.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_move.o :      ../xatrix/m_move.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_mutant.o :    ../xatrix/m_mutant.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_parasite.o :  ../xatrix/m_parasite.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_soldier.o :   ../xatrix/m_soldier.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_supertank.o : ../xatrix/m_supertank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/m_tank.o :      ../xatrix/m_tank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/p_client.o :    ../xatrix/p_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/p_hud.o :       ../xatrix/p_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/p_trail.o :     ../xatrix/p_trail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/p_view.o :      ../xatrix/p_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/p_weapon.o :    ../xatrix/p_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/xatrix/q_shared.o :    ../xatrix/q_shared.c
	$(DO_SHLIB_CC)

#############################################################################
# ROGUE
#############################################################################

ROGUE_OBJS = \
	$(BUILDDIR)/rogue/dm_ball.o \
	$(BUILDDIR)/rogue/dm_tag.o \
	$(BUILDDIR)/rogue/g_ai.o \
	$(BUILDDIR)/rogue/g_chase.o \
	$(BUILDDIR)/rogue/g_cmds.o \
	$(BUILDDIR)/rogue/g_combat.o \
	$(BUILDDIR)/rogue/g_func.o \
	$(BUILDDIR)/rogue/g_items.o \
	$(BUILDDIR)/rogue/g_main.o \
	$(BUILDDIR)/rogue/g_misc.o \
	$(BUILDDIR)/rogue/g_monster.o \
	$(BUILDDIR)/rogue/g_newai.o \
	$(BUILDDIR)/rogue/g_newdm.o \
	$(BUILDDIR)/rogue/g_newfnc.o \
	$(BUILDDIR)/rogue/g_newtarg.o \
	$(BUILDDIR)/rogue/g_newtrig.o \
	$(BUILDDIR)/rogue/g_newweap.o \
	$(BUILDDIR)/rogue/g_phys.o \
	$(BUILDDIR)/rogue/g_save.o \
	$(BUILDDIR)/rogue/g_spawn.o \
	$(BUILDDIR)/rogue/g_sphere.o \
	$(BUILDDIR)/rogue/g_svcmds.o \
	$(BUILDDIR)/rogue/g_target.o \
	$(BUILDDIR)/rogue/g_trigger.o \
	$(BUILDDIR)/rogue/g_turret.o \
	$(BUILDDIR)/rogue/g_utils.o \
	$(BUILDDIR)/rogue/g_weapon.o \
	$(BUILDDIR)/rogue/m_actor.o \
	$(BUILDDIR)/rogue/m_berserk.o \
	$(BUILDDIR)/rogue/m_boss2.o \
	$(BUILDDIR)/rogue/m_boss3.o \
	$(BUILDDIR)/rogue/m_boss31.o \
	$(BUILDDIR)/rogue/m_boss32.o \
	$(BUILDDIR)/rogue/m_brain.o \
	$(BUILDDIR)/rogue/m_carrier.o \
	$(BUILDDIR)/rogue/m_chick.o \
	$(BUILDDIR)/rogue/m_flash.o \
	$(BUILDDIR)/rogue/m_flipper.o \
	$(BUILDDIR)/rogue/m_float.o \
	$(BUILDDIR)/rogue/m_flyer.o \
	$(BUILDDIR)/rogue/m_gladiator.o \
	$(BUILDDIR)/rogue/m_gunner.o \
	$(BUILDDIR)/rogue/m_hover.o \
	$(BUILDDIR)/rogue/m_infantry.o \
	$(BUILDDIR)/rogue/m_insane.o \
	$(BUILDDIR)/rogue/m_medic.o \
	$(BUILDDIR)/rogue/m_move.o \
	$(BUILDDIR)/rogue/m_mutant.o \
	$(BUILDDIR)/rogue/m_parasite.o \
	$(BUILDDIR)/rogue/m_soldier.o \
	$(BUILDDIR)/rogue/m_stalker.o \
	$(BUILDDIR)/rogue/m_supertank.o \
	$(BUILDDIR)/rogue/m_tank.o \
	$(BUILDDIR)/rogue/m_turret.o \
	$(BUILDDIR)/rogue/m_widow.o \
	$(BUILDDIR)/rogue/m_widow2.o \
	$(BUILDDIR)/rogue/p_client.o \
	$(BUILDDIR)/rogue/p_hud.o \
	$(BUILDDIR)/rogue/p_trail.o \
	$(BUILDDIR)/rogue/p_view.o \
	$(BUILDDIR)/rogue/p_weapon.o \
	$(BUILDDIR)/rogue/q_shared.o

$(BUILDDIR)/rogue/game.dylib : $(ROGUE_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(ROGUE_OBJS)

$(BUILDDIR)/rogue/dm_ball.o :      ../rogue/dm_ball.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/dm_tag.o :       ../rogue/dm_tag.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_ai.o :         ../rogue/g_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_chase.o :      ../rogue/g_chase.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_cmds.o :       ../rogue/g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_combat.o :     ../rogue/g_combat.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_func.o :       ../rogue/g_func.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_items.o :      ../rogue/g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_main.o :       ../rogue/g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_misc.o :       ../rogue/g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_monster.o :    ../rogue/g_monster.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_newai.o :      ../rogue/g_newai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_newdm.o :      ../rogue/g_newdm.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_newfnc.o :     ../rogue/g_newfnc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_newtarg.o :    ../rogue/g_newtarg.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_newtrig.o :    ../rogue/g_newtrig.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_newweap.o :    ../rogue/g_newweap.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_phys.o :       ../rogue/g_phys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_save.o :       ../rogue/g_save.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_spawn.o :      ../rogue/g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_sphere.o :     ../rogue/g_sphere.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_svcmds.o :     ../rogue/g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_target.o :     ../rogue/g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_trigger.o :    ../rogue/g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_turret.o :     ../rogue/g_turret.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_utils.o :      ../rogue/g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/g_weapon.o :     ../rogue/g_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_actor.o :      ../rogue/m_actor.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_berserk.o :    ../rogue/m_berserk.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_boss2.o :      ../rogue/m_boss2.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_boss3.o :      ../rogue/m_boss3.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_boss31.o :     ../rogue/m_boss31.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_boss32.o :     ../rogue/m_boss32.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_brain.o :      ../rogue/m_brain.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_carrier.o :    ../rogue/m_carrier.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_chick.o :      ../rogue/m_chick.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_flash.o :      ../rogue/m_flash.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_flipper.o :    ../rogue/m_flipper.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_float.o :      ../rogue/m_float.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_flyer.o :      ../rogue/m_flyer.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_gladiator.o :  ../rogue/m_gladiator.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_gunner.o :     ../rogue/m_gunner.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_hover.o :      ../rogue/m_hover.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_infantry.o :   ../rogue/m_infantry.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_insane.o :     ../rogue/m_insane.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_medic.o :      ../rogue/m_medic.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_move.o :       ../rogue/m_move.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_mutant.o :     ../rogue/m_mutant.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_parasite.o :   ../rogue/m_parasite.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_soldier.o :    ../rogue/m_soldier.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_stalker.o :    ../rogue/m_stalker.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_supertank.o :  ../rogue/m_supertank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_tank.o :       ../rogue/m_tank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_turret.o :     ../rogue/m_turret.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_widow.o :      ../rogue/m_widow.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/m_widow2.o :     ../rogue/m_widow2.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/p_client.o :     ../rogue/p_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/p_hud.o :        ../rogue/p_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/p_trail.o :      ../rogue/p_trail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/p_view.o :       ../rogue/p_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/p_weapon.o :     ../rogue/p_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/rogue/q_shared.o :     ../rogue/q_shared.c
	$(DO_SHLIB_CC)

#############################################################################
# ZAERO
#############################################################################

ZAERO_OBJS = \
	$(BUILDDIR)/zaero/g_ai.o \
	$(BUILDDIR)/zaero/g_cmds.o \
	$(BUILDDIR)/zaero/g_combat.o \
	$(BUILDDIR)/zaero/g_func.o \
	$(BUILDDIR)/zaero/g_items.o \
	$(BUILDDIR)/zaero/g_main.o \
	$(BUILDDIR)/zaero/g_misc.o \
	$(BUILDDIR)/zaero/g_monster.o \
	$(BUILDDIR)/zaero/g_phys.o \
	$(BUILDDIR)/zaero/g_save.o \
	$(BUILDDIR)/zaero/g_spawn.o \
	$(BUILDDIR)/zaero/g_svcmds.o \
	$(BUILDDIR)/zaero/g_target.o \
	$(BUILDDIR)/zaero/g_trigger.o \
	$(BUILDDIR)/zaero/g_turret.o \
	$(BUILDDIR)/zaero/g_utils.o \
	$(BUILDDIR)/zaero/g_weapon.o \
	$(BUILDDIR)/zaero/m_actor.o \
	$(BUILDDIR)/zaero/m_berserk.o \
	$(BUILDDIR)/zaero/m_boss2.o \
	$(BUILDDIR)/zaero/m_boss3.o \
	$(BUILDDIR)/zaero/m_boss31.o \
	$(BUILDDIR)/zaero/m_boss32.o \
	$(BUILDDIR)/zaero/m_brain.o \
	$(BUILDDIR)/zaero/m_chick.o \
	$(BUILDDIR)/zaero/m_flash.o \
	$(BUILDDIR)/zaero/m_flipper.o \
	$(BUILDDIR)/zaero/m_float.o \
	$(BUILDDIR)/zaero/m_flyer.o \
	$(BUILDDIR)/zaero/m_gladiator.o \
	$(BUILDDIR)/zaero/m_gunner.o \
	$(BUILDDIR)/zaero/m_hover.o \
	$(BUILDDIR)/zaero/m_infantry.o \
	$(BUILDDIR)/zaero/m_insane.o \
	$(BUILDDIR)/zaero/m_medic.o \
	$(BUILDDIR)/zaero/m_move.o \
	$(BUILDDIR)/zaero/m_mutant.o \
	$(BUILDDIR)/zaero/m_parasite.o \
	$(BUILDDIR)/zaero/m_soldier.o \
	$(BUILDDIR)/zaero/m_supertank.o \
	$(BUILDDIR)/zaero/m_tank.o \
	$(BUILDDIR)/zaero/p_client.o \
	$(BUILDDIR)/zaero/p_hud.o \
	$(BUILDDIR)/zaero/p_trail.o \
	$(BUILDDIR)/zaero/p_view.o \
	$(BUILDDIR)/zaero/p_weapon.o \
	$(BUILDDIR)/zaero/q_shared.o \
	$(BUILDDIR)/zaero/z_acannon.o \
	$(BUILDDIR)/zaero/z_ai.o \
	$(BUILDDIR)/zaero/z_anim.o \
	$(BUILDDIR)/zaero/z_boss.o \
	$(BUILDDIR)/zaero/z_camera.o \
	$(BUILDDIR)/zaero/z_debug.o \
	$(BUILDDIR)/zaero/z_frames.o \
	$(BUILDDIR)/zaero/z_handler.o \
	$(BUILDDIR)/zaero/z_hound.o \
	$(BUILDDIR)/zaero/z_item.o \
	$(BUILDDIR)/zaero/z_list.o \
	$(BUILDDIR)/zaero/z_mtest.o \
	$(BUILDDIR)/zaero/z_sentien.o \
	$(BUILDDIR)/zaero/z_spawn.o \
	$(BUILDDIR)/zaero/z_trigger.o \
	$(BUILDDIR)/zaero/z_weapon.o

$(BUILDDIR)/zaero/game.dylib : $(ZAERO_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(ZAERO_OBJS)

$(BUILDDIR)/zaero/g_ai.o :         ../zaero/g_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_cmds.o :       ../zaero/g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_combat.o :     ../zaero/g_combat.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_func.o :       ../zaero/g_func.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_items.o :      ../zaero/g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_main.o :       ../zaero/g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_misc.o :       ../zaero/g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_monster.o :    ../zaero/g_monster.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_phys.o :       ../zaero/g_phys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_save.o :       ../zaero/g_save.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_spawn.o :      ../zaero/g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_svcmds.o :     ../zaero/g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_target.o :     ../zaero/g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_trigger.o :    ../zaero/g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_turret.o :     ../zaero/g_turret.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_utils.o :      ../zaero/g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/g_weapon.o :     ../zaero/g_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_actor.o :      ../zaero/m_actor.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_berserk.o :    ../zaero/m_berserk.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_boss2.o :      ../zaero/m_boss2.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_boss3.o :      ../zaero/m_boss3.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_boss31.o :     ../zaero/m_boss31.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_boss32.o :     ../zaero/m_boss32.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_brain.o :      ../zaero/m_brain.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_chick.o :      ../zaero/m_chick.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_flash.o :      ../zaero/m_flash.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_flipper.o :    ../zaero/m_flipper.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_float.o :      ../zaero/m_float.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_flyer.o :      ../zaero/m_flyer.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_gladiator.o :  ../zaero/m_gladiator.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_gunner.o :     ../zaero/m_gunner.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_hover.o :      ../zaero/m_hover.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_infantry.o :   ../zaero/m_infantry.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_insane.o :     ../zaero/m_insane.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_medic.o :      ../zaero/m_medic.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_move.o :       ../zaero/m_move.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_mutant.o :     ../zaero/m_mutant.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_parasite.o :   ../zaero/m_parasite.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_soldier.o :    ../zaero/m_soldier.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_supertank.o :  ../zaero/m_supertank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/m_tank.o :       ../zaero/m_tank.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/p_client.o :     ../zaero/p_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/p_hud.o :        ../zaero/p_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/p_trail.o :      ../zaero/p_trail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/p_view.o :       ../zaero/p_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/p_weapon.o :     ../zaero/p_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/q_shared.o :     ../zaero/q_shared.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_acannon.o :    ../zaero/z_acannon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_ai.o :         ../zaero/z_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_anim.o :       ../zaero/z_anim.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_boss.o :       ../zaero/z_boss.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_camera.o :     ../zaero/z_camera.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_debug.o :      ../zaero/z_debug.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_frames.o :     ../zaero/z_frames.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_handler.o :    ../zaero/z_handler.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_hound.o :      ../zaero/z_hound.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_item.o :       ../zaero/z_item.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_list.o :       ../zaero/z_list.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_mtest.o :      ../zaero/z_mtest.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_sentien.o :    ../zaero/z_sentien.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_spawn.o :      ../zaero/z_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_trigger.o :    ../zaero/z_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/zaero/z_weapon.o :     ../zaero/z_weapon.c
	$(DO_SHLIB_CC)

#############################################################################
# REF_VK
#############################################################################

REF_VK_OBJS = \
	$(BUILDDIR)/ref_vk/glob.o \
	$(BUILDDIR)/ref_vk/q_shared.o \
	$(BUILDDIR)/ref_vk/q_shmacos.o \
	$(BUILDDIR)/ref_vk/vk_buffer.o \
	$(BUILDDIR)/ref_vk/vk_cmd.o \
	$(BUILDDIR)/ref_vk/vk_common.o \
	$(BUILDDIR)/ref_vk/vk_device.o \
	$(BUILDDIR)/ref_vk/vk_draw.o \
	$(BUILDDIR)/ref_vk/vk_image.o \
	$(BUILDDIR)/ref_vk/vk_imp.o \
	$(BUILDDIR)/ref_vk/vk_light.o \
	$(BUILDDIR)/ref_vk/vk_mem_alloc.o \
	$(BUILDDIR)/ref_vk/vk_mesh.o \
	$(BUILDDIR)/ref_vk/vk_model.o \
	$(BUILDDIR)/ref_vk/vk_pipeline.o \
	$(BUILDDIR)/ref_vk/vk_rmain.o \
	$(BUILDDIR)/ref_vk/vk_rmisc.o \
	$(BUILDDIR)/ref_vk/vk_rsurf.o \
	$(BUILDDIR)/ref_vk/vk_shaders.o \
	$(BUILDDIR)/ref_vk/vk_swapchain.o \
	$(BUILDDIR)/ref_vk/vk_validation.o \
	$(BUILDDIR)/ref_vk/vk_warp.o \
	$(BUILDDIR)/ref_vk/win_macos.o

$(BUILDDIR)/ref_vk.dylib : $(REF_VK_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(REF_VK_OBJS) $(VKLDFLAGS)

$(BUILDDIR)/ref_vk/glob.o :         ../linux/glob.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/q_shared.o :     ../game/q_shared.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ref_vk/q_shmacos.o :    ../macos/q_shmacos.c
	$(DO_VK_SHLIB_O_CC)

$(BUILDDIR)/ref_vk/vk_buffer.o :    ../ref_vk/vk_buffer.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_cmd.o :       ../ref_vk/vk_cmd.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_common.o :    ../ref_vk/vk_common.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_device.o :    ../ref_vk/vk_device.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_draw.o :      ../ref_vk/vk_draw.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_image.o :     ../ref_vk/vk_image.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_imp.o :       ../macos/vk_imp.m
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_light.o :     ../ref_vk/vk_light.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_mem_alloc.o : ../ref_vk/vk_mem_alloc.cpp
	$(DO_VK_SHLIB_CPP)

$(BUILDDIR)/ref_vk/vk_mesh.o :      ../ref_vk/vk_mesh.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_model.o :     ../ref_vk/vk_model.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_pipeline.o :  ../ref_vk/vk_pipeline.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_rmain.o :     ../ref_vk/vk_rmain.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_rmisc.o :     ../ref_vk/vk_rmisc.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_rsurf.o :     ../ref_vk/vk_rsurf.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_shaders.o :   ../ref_vk/vk_shaders.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_swapchain.o : ../ref_vk/vk_swapchain.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_validation.o : ../ref_vk/vk_validation.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/vk_warp.o :       ../ref_vk/vk_warp.c
	$(DO_VK_SHLIB_CC)

$(BUILDDIR)/ref_vk/win_macos.o :     ../macos/win_macos.m
	$(DO_VK_SHLIB_CC)

#############################################################################
# XCODE
#############################################################################

debug-xcode:
	xcodebuild -workspace vkQuake2.xcworkspace -scheme vkQuake2 -configuration Debug build
release-xcode:
	xcodebuild -workspace vkQuake2.xcworkspace -scheme vkQuake2 -configuration Release build
clean-xcode:
	xcodebuild -workspace vkQuake2.xcworkspace -scheme vkQuake2 -configuration Release clean
	xcodebuild -workspace vkQuake2.xcworkspace -scheme vkQuake2 -configuration Debug clean

#############################################################################
# MISC
#############################################################################

clean: clean-debug clean-release

clean-debug:
	$(MAKE) clean2 BUILDDIR=debug CFLAGS="$(DEBUG_CFLAGS)"

clean-release:
	$(MAKE) clean2 BUILDDIR=release CFLAGS="$(DEBUG_CFLAGS)"

clean2:
	-rm -f \
	$(QUAKE2_OBJS) \
	$(Q2DED_OBJS) \
	$(GAME_OBJS) \
	$(CTF_OBJS) \
	$(XATRIX_OBJS) \
	$(ROGUE_OBJS) \
	$(ZAERO_OBS) \
	$(REF_VK_OBJS)

.PHONY: help all debug release clean clean-debug clean-release debug-xcode release-xcode clean-xcode
