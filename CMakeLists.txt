cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(CYW43_Bare_Test C CXX ASM)
pico_sdk_init()

add_executable(CYW43_Bare_Test src/main.c)

pico_set_program_name(CYW43_Bare_Test "CYW43_Bare_Test")
pico_set_program_version(CYW43_Bare_Test "0.1")

pico_enable_stdio_uart(CYW43_Bare_Test 0)
pico_enable_stdio_usb(CYW43_Bare_Test 1)

target_link_libraries(CYW43_Bare_Test
    pico_stdlib
    pico_cyw43_arch_lwip_poll   # lwIP polling mode — no FreeRTOS needed
)

target_include_directories(CYW43_Bare_Test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_options(${PROJECT_NAME} PRIVATE -Wl,--print-memory-usage)
pico_add_extra_outputs(CYW43_Bare_Test)
