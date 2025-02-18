cmake_minimum_required (VERSION 3.8)

set(PROJECT_NAME "doomgeneric")

add_library(${PROJECT_NAME} STATIC 
    am_map.c
    am_map.h
    config.h
    d_englsh.h
    d_event.c
    d_event.h
    d_items.c
    d_items.h
    d_iwad.c
    d_iwad.h
    d_loop.c
    d_loop.h
    d_main.c
    d_main.h
    d_mode.c
    d_mode.h
    d_net.c
    d_player.h
    d_textur.h
    d_think.h
    d_ticcmd.h
    deh_main.h
    deh_misc.h
    deh_str.h
    doom.h
    doomdata.h
    doomdef.c
    doomdef.h
    doomfeatures.h
    doomgeneric.c
    doomgeneric_mre.c
    doomgeneric.h
    doomkeys.h
    doomstat.c
    doomstat.h
    doomtype.h
    dstrings.c
    dstrings.h
    dummy.c
    f_finale.c
    f_finale.h
    f_wipe.c
    f_wipe.h
    g_game.c
    g_game.h
    gusconf.c
    gusconf.h
    hu_lib.c
    hu_lib.h
    hu_stuff.c
    hu_stuff.h
    i_cdmus.c
    i_cdmus.h
    i_endoom.c
    i_endoom.h
    i_input.c
    i_joystick.c
    i_joystick.h
    i_scale.c
    i_scale.h
    i_sound.c
    i_sound.h
    i_mresound.c
    i_swap.h
    i_system.c
    i_system.h
    i_timer.c
    i_timer.h
    i_video.c
    i_video.h
    icon.c
    info.c
    info.h
    m_argv.c
    m_argv.h
    m_bbox.c
    m_bbox.h
    m_cheat.c
    m_cheat.h
    m_config.c
    m_config.h
    m_controls.c
    m_controls.h
    m_fixed.c
    m_fixed.h
    m_menu.c
    m_menu.h
    m_misc.c
    m_misc.h
    m_random.c
    m_random.h
    memio.c
    memio.h
    mus2mid.c
    mus2mid.h
    net_client.h
    net_dedicated.h
    net_defs.h
    net_gui.h
    net_io.h
    net_loop.h
    net_packet.h
    net_query.h
    net_sdl.h
    net_server.h
    p_ceilng.c
    p_doors.c
    p_enemy.c
    p_floor.c
    p_inter.c
    p_inter.h
    p_lights.c
    p_local.h
    p_map.c
    p_maputl.c
    p_mobj.c
    p_mobj.h
    p_plats.c
    p_pspr.c
    p_pspr.h
    p_saveg.c
    p_saveg.h
    p_setup.c
    p_setup.h
    p_sight.c
    p_spec.c
    p_spec.h
    p_switch.c
    p_telept.c
    p_tick.c
    p_tick.h
    p_user.c
    r_bsp.c
    r_bsp.h
    r_data.c
    r_data.h
    r_defs.h
    r_draw.c
    r_draw.h
    r_local.h
    r_main.c
    r_main.h
    r_plane.c
    r_plane.h
    r_segs.c
    r_segs.h
    r_sky.c
    r_sky.h
    r_state.h
    r_things.c
    r_things.h
    s_sound.c
    s_sound.h
    sha1.c
    sha1.h
    sounds.c
    sounds.h
    st_lib.c
    st_lib.h
    st_stuff.c
    st_stuff.h
    statdump.c
    statdump.h
    tables.c
    tables.h
    v_patch.h
    v_video.c
    v_video.h
    w_checksum.c
    w_checksum.h
    w_file.c
    w_file.h
    w_file_stdc.c
    w_main.c
    w_main.h
    w_merge.h
    w_wad.c
    w_wad.h
    wi_stuff.c
    wi_stuff.h
    z_zone.c
    z_zone.h
)

target_link_libraries(${PROJECT_NAME} mreapi console mixer)