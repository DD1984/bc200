# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/st75320
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fw)

target_sources(app PRIVATE src/main.c)
