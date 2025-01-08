cmake_minimum_required (VERSION 3.8)

set(PROJECT_NAME "console")

add_library(${PROJECT_NAME} STATIC 
	Profont6x11.h
	console.c console.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ".")

target_link_libraries(${PROJECT_NAME} mreapi)