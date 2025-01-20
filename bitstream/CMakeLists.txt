cmake_minimum_required (VERSION 3.8)

set(PROJECT_NAME "bitstream")

add_library(${PROJECT_NAME} STATIC 
	bitstream.c bitstream.h
	injected_bitstream.c injected_bitstream.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ".")

target_link_libraries(${PROJECT_NAME} mreapi)