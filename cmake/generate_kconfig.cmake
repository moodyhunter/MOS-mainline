# SPDX-License-Identifier: GPL-3.0-or-later

function(generate_kconfig)
    message(STATUS "Generating kconfig.c according to configuration...")

    execute_process(
        COMMAND git describe --long --tags --all --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MOS_KERNEL_REVISION_STRING
        ERROR_VARIABLE _DROP_
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    set(MOS_KERNEL_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}")
    message(STATUS " -- Kernel Version      ${MOS_KERNEL_VERSION_STRING}")
    message(STATUS " -- Kernel Revision     ${MOS_KERNEL_REVISION_STRING}")
    message(STATUS " -- Kernel Boot Method  ${MOS_BOOT_METHOD}")

    make_directory(${CMAKE_BINARY_DIR}/include)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/kconfig.c.in ${CMAKE_BINARY_DIR}/include/kconfig.c)
    target_sources(mos_kernel_object PRIVATE ${CMAKE_BINARY_DIR}/include/kconfig.c)
endfunction()