include_directories(
	${ZEPHYR_LVGL_MODULE_DIR}
	${ZEPHYR_BASE}/lib/gui/lvgl
)

if (CONFIG_POLL_BTNS)
	zephyr_include_directories(.)
	zephyr_library()
	zephyr_library_sources(
		poll_btns.c
	)
endif()
