cmake_minimum_required (VERSION 3.8)

project ("DoomVxp")

set(APP_NAME "Doom")
set(DEVELOPER_NAME "Ximik_Boda")
set(RAM "3250" STRING "Ram requirement (in kb)")
set(IMSI "1234567890" CACHE STRING "Imsi of first SIM-card")
set(API "File SIM card ProMng Bitstream Audio")
option(BACKGROUND "App can work background" OFF)

set(MRE_SDK "$ENV{MRE_SDK}" CACHE STRING "MRE SDK dir")
set(TinyMRESDK "$ENV{TinyMRESDK}" CACHE STRING "TinyMRESDK dir")

if(BACKGROUND)
    add_definitions(-DSUPPORT_BG)
endif()

add_definitions(-DFEATURE_SOUND)

add_subdirectory(resourses)
add_subdirectory(core)
add_subdirectory(mreapi)
add_subdirectory(vxp)
add_subdirectory(run)
add_subdirectory(doomgeneric)
add_subdirectory(console)