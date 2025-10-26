cmake_minimum_required(VERSION 3.22)

zephyr_library()
zephyr_library_sources(
    src/wifi.c
    src/wifi_events.c
)
zephyr_include_directories(include)
