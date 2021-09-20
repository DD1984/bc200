# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND ZEPHYR_EXTRA_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/st75320
	${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/poll_btns
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fw)

target_sources(app PRIVATE
	src/main.c
	src/draw_menu.c
	src/demo.c
	src/disk.c
)

add_custom_target(decode ALL
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../usbfw_decode
	COMMENT "Compiling usbfw_decode"
)

add_custom_target(jump ALL
	COMMAND CROSS_PREFIX=${CROSS_COMPILE} make
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../jump
	COMMENT "Compiling jump"
)

add_custom_target(usb_fw ALL
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../usbfw_decode/decode -e -i ${CMAKE_CURRENT_BINARY_DIR}/zephyr/zephyr.bin -o ${CMAKE_CURRENT_BINARY_DIR}/zephyr/zephyr.bin
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zephyr/zephyr.bin
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../usbfw_decode/decode
	COMMENT "Making usb fw"
)
