# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(YD_RP2040_RP2_B2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(YD_RP2040_RP2_B2 
    src/main.cpp
    components/my_mqtt/my_mqtt.cpp
    components/my_mqtt/my_mqtt.h
    components/sim7680/sim7680.cpp
    components/sim7680/sim7680.h
    components/bmi160/bmi160.h
    components/bmi160/bm1160.cpp
    components/buzzer/buzzer.h
    components/buzzer/buzzer.cpp
    components/neo6m/neo6m.h
    components/sdcard/sdcard.h
    components/neo6m/neo6m.cpp
    components/sdcard/sdcard.cpp
    components/tft/tft.h
    components/tft/tft.cpp
    components/algorithm_neo6m/algorithm_neo6m.h
    components/algorithm_neo6m/algorithm_neo6m.cpp
    components/algorithm_bmi160/algorithm_bmi160.h
    components/algorithm_bmi160/algorithm_bmi160.cpp
    components/xpt2046/xpt2046.h
    components/xpt2046/xpt2046.cpp
    components/shared_data/shared_data.h
    components/shared_data/shared_data.cpp
    components/font5x7/font5x7.cpp
    components/font5x7/font5x7.h
    fatfs/diskio.c
    fatfs/diskio.h
    fatfs/ff.c
    fatfs/ff.h
    fatfs/ffconf.h
    fatfs/ffsystem.c
    fatfs/ffunicode.c


)

pico_set_program_name(YD_RP2040_RP2_B2 "YD_RP2040_RP2_B2")
pico_set_program_version(YD_RP2040_RP2_B2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(YD_RP2040_RP2_B2 0)
pico_enable_stdio_usb(YD_RP2040_RP2_B2 1)

# Add the standard library to the build
target_link_libraries(YD_RP2040_RP2_B2
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(YD_RP2040_RP2_B2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(YD_RP2040_RP2_B2 
        hardware_spi
        hardware_i2c
        hardware_uart
        )

pico_add_extra_outputs(YD_RP2040_RP2_B2)