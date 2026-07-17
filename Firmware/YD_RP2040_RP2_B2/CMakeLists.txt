# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# ---- NẠP FREERTOS-KERNEL (phải include TRƯỚC project(), giống pico_sdk_import.cmake) ----
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(YD_RP2040_RP2_B2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

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
    components/algorithm_bmi160/algorithm_bmi160.h
    components/algorithm_bmi160/algorithm_bmi160.cpp
    components/xpt2046/xpt2046.h
    components/xpt2046/xpt2046.cpp
    components/ui/ui.h
    components/ui/ui.cpp
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

pico_enable_stdio_uart(YD_RP2040_RP2_B2 0)
pico_enable_stdio_usb(YD_RP2040_RP2_B2 1)

# Thêm đường dẫn include cho dự án chính (nơi chứa file FreeRTOSConfig.h)
# và toàn bộ thư mục con trong components/ để các file .cpp có thể include
# lẫn nhau bằng tên ngắn (vd "bmi160.h") thay vì đường dẫn đầy đủ.
target_include_directories(YD_RP2040_RP2_B2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/components/my_mqtt
    ${CMAKE_CURRENT_LIST_DIR}/components/sim7680
    ${CMAKE_CURRENT_LIST_DIR}/components/bmi160
    ${CMAKE_CURRENT_LIST_DIR}/components/buzzer
    ${CMAKE_CURRENT_LIST_DIR}/components/neo6m
    ${CMAKE_CURRENT_LIST_DIR}/components/sdcard
    ${CMAKE_CURRENT_LIST_DIR}/components/tft
    ${CMAKE_CURRENT_LIST_DIR}/components/algorithm_bmi160
    ${CMAKE_CURRENT_LIST_DIR}/components/xpt2046
    ${CMAKE_CURRENT_LIST_DIR}/components/ui
    ${CMAKE_CURRENT_LIST_DIR}/components/shared_data
    ${CMAKE_CURRENT_LIST_DIR}/components/font5x7
    ${CMAKE_CURRENT_LIST_DIR}/fatfs
)

# QUAN TRỌNG: bản thân các file nguồn của FreeRTOS-Kernel (tasks.c, queue.c...) cũng include
# "FreeRTOS.h" -> "FreeRTOSConfig.h", nên target FreeRTOS-Kernel cũng cần biết đường dẫn tới
# FreeRTOSConfig.h (đang nằm trong src/), nếu không sẽ lỗi khi build chính thư viện FreeRTOS.
target_include_directories(FreeRTOS-Kernel INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Liên kết các thư viện phần cứng và target FreeRTOS-Kernel (được định nghĩa bởi FreeRTOS_Kernel_import.cmake)
target_link_libraries(YD_RP2040_RP2_B2 
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_uart
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4   # heap_4.c - cấp phát động, phù hợp cho phần lớn ứng dụng
)

pico_add_extra_outputs(YD_RP2040_RP2_B2)