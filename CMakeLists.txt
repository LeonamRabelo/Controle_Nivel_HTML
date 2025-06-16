# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)


set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Controle_Nivel_HTML C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Controle_Nivel_HTML Controle_Nivel_HTML.c )

pico_set_program_name(Controle_Nivel_HTML "Controle_Nivel_HTML")
pico_set_program_version(Controle_Nivel_HTML "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Controle_Nivel_HTML 0)
pico_enable_stdio_usb(Controle_Nivel_HTML 1)

# Add the standard library to the build
target_link_libraries(Controle_Nivel_HTML
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Controle_Nivel_HTML PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/inc
        ${PICO_SDK_PATH}/lib/lwip/src/include
        ${PICO_SDK_PATH}/lib/lwip/src/include/arch
        ${PICO_SDK_PATH}/lib/lwip/src/include/lwip
)

target_sources(Controle_Nivel_HTML PRIVATE
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/httpd.c
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/fs.c
)

# Add any user requested libraries
target_link_libraries(Controle_Nivel_HTML 
        hardware_pio
        hardware_i2c
        hardware_timer
        hardware_clocks
        hardware_gpio
        hardware_adc
        hardware_pwm
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(Controle_Nivel_HTML)