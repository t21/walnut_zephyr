# Re-direct the directory where the 'boards' directory is found from
# $ZEPHYR_BASE to this directory.
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(BOARD walnut)

if(NOT BOARD_VARIANT)
    set(BOARD_VARIANT climate)
endif()

add_definitions(-DBOARD_VARIANT=${BOARD_VARIANT})

macro(set_conf_file)
    if(EXISTS ${APPLICATION_SOURCE_DIR}/boards/arm/walnut/${BOARD}_${BOARD_VARIANT}.conf)
        set(CONF_FILE "prj.conf ${APPLICATION_SOURCE_DIR}/boards/arm/walnut/${BOARD}_${BOARD_VARIANT}.conf")
    else()
        set(CONF_FILE "prj.conf")
    endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

add_subdirectory(drivers)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/fs/nvs)
