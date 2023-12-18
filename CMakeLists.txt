cmake_minimum_required(VERSION 3.22.0)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(lab VERSION 0.1.0 LANGUAGES C CXX ASM)

pico_sdk_init()


add_executable(${PROJECT_NAME} main.c)
add_library(debounce debounce.c)
add_library(stepper stepper.c)
add_library(lora lora.c)

pico_generate_pio_header(stepper ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_i2c stepper lora)
target_link_libraries(debounce pico_stdlib)
target_link_libraries(stepper pico_stdlib hardware_pio)
target_link_libraries(lora pico_stdlib hardware_uart)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)


