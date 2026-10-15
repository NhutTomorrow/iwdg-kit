cmake_minimum_required(VERSION 3.20.0)

# 1. Dat cau hinh Kconfig truoc find_package
set(OVERLAY_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/../conf/wdt.fragment.conf")

# 2. Goi Zephyr build system
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iwdg_sample)

# 3. Them ma nguon
target_sources(app PRIVATE 
    src/main.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/wdt_token.c
)

# 4. Them thu muc chua header (.h)
target_include_directories(app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 5. Bat co kiem thu de kich hoat ham force_hang
target_compile_definitions(app PRIVATE WDT_TOKEN_TEST=1)