# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD rtl8305_stm32g030)

find_package(Zephyr)
project(my_zephyr_app)

target_sources(app PRIVATE src/main.c)