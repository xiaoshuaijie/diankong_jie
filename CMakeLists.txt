cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME diankong_jie)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

set(USER_SOURCES
    arithmetic/pid/pid_incremental/pid_incremental.c
    arithmetic/pid/pid_location/pid_location.c
    arithmetic/referee/crc_ref.c
    bsp/dwt/bsp_dwt.c
    calucate/chassis_control/chassis_control.c
    calucate/joint_control/joint_control.c
    modules/dr16/dev_dr16.c
    modules/game/game.c
    modules/lx824/lx824.c
    modules/motor/dj_motor/dj_motor_ctrl.c
    modules/motor/dj_motor/dj_motor_drv.c
    modules/motor/dm_motor/dm_motor_ctrl.c
    modules/motor/dm_motor/dm_motor_drv.c
    modules/vt13/vt13.c
    modules/vofa/vofa.c
    modules/vofa_usb/vofa_usb.c
    task/chassis_task/chassis_task.c
    task/dr16_task/dr16_task.c
    task/game_task/game_task.c
    task/joint_task/joint_task.c
    task/lx824_task/lx824_task.c
    task/vofa_task/vofa_task.c
    task/vofa_usb_task/vofa_usb_task.c
    task/vt13_task/vt13_task.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
     bsp/bsp_uart/bsp_uart.c
     bsp/bsp_can/bsp_can.c
     bsp/bsp_usb/bsp_usb.c
     bsp/bsp_pwm/bsp_pwm.c
     bsp/bsp_iic/bsp_iic.c
     component/comp_utils.c
     ${USER_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    bsp
    bsp/bsp_uart
    bsp/bsp_can
    bsp/bsp_usb
    bsp/bsp_pwm
    bsp/bsp_iic
    bsp/dwt
    component
    arithmetic/pid/pid_incremental
    arithmetic/pid/pid_location
    arithmetic/referee
    calucate/chassis_control
    calucate/joint_control
    modules
    modules/dr16
    modules/game
    modules/lx824
    modules/motor/dj_motor
    modules/motor/dm_motor
    modules/motor/FHU_motor
    modules/vt13
    modules/vofa
    modules/vofa_usb
    task
    task/chassis_task
    task/dr16_task
    task/game_task
    task/joint_task
    task/lx824_task
    task/vofa_task
    task/vofa_usb_task
    task/vt13_task
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
