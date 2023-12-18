cmake_minimum_required(VERSION 3.22.0)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pill_dispenser VERSION 0.1.0 LANGUAGES C CXX ASM)

pico_sdk_init()

set(source_location ${CMAKE_CURRENT_LIST_DIR}/src)

include_directories(${CMAKE_CURRENT_LIST_DIR}/lib)


add_executable(${PROJECT_NAME} main.c)
add_library(debounce ${source_location}/debounce.c)
add_library(stepper  ${source_location}/stepper.c)
add_library(lora     ${source_location}/lora.c)
add_library(eeprom   ${source_location}/eeprom.c)
add_library(magnusFuncs ${source_location}/magnusFuncs.c)

pico_generate_pio_header(stepper ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_i2c stepper lora eeprom debounce magnusFuncs)
target_link_libraries(stepper         pico_stdlib hardware_pio)
target_link_libraries(lora            pico_stdlib hardware_uart)
target_link_libraries(eeprom          pico_stdlib hardware_i2c)
target_link_libraries(debounce        pico_stdlib)
target_link_libraries(magnusFuncs     hardware_watchdog eeprom)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)


