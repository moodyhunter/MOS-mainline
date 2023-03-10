# SPDX-License-Identifier: GPL-3.0-or-later

add_library(libuserspace OBJECT
    ${MOS_STANDARD_LIBRARY_SOURCES}
    libs/libuserspace.c
    libs/libuserspace++.cpp
    libs/cxa.c
)

target_include_directories(libuserspace PUBLIC ${CMAKE_SOURCE_DIR}/userspace/include)
target_link_libraries(libuserspace PUBLIC gcc mos::include)
add_library(mos::libuserspace ALIAS libuserspace)
add_dependencies(libuserspace mos_syscall_decl) # needs to be generated before we can compile

# TODO: Remove this once we have a proper userspace libc
target_compile_options(libuserspace PUBLIC "-ffreestanding")
target_link_options(libuserspace PUBLIC "-nostdlib")

include(cmake/userspace.cmake)

add_subdirectory(libs)
add_subdirectory(programs)
add_subdirectory(drivers)
add_subdirectory(tests)

# initrd resources
add_subdirectory(initrd)

# if you want to debug a userspace program, replace the line below with the
# target name of the program you want to debug
add_to_gdbinit(ls)
