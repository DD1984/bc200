if (CONFIG_ST75320)
	zephyr_include_directories(.)
	zephyr_library()
	zephyr_library_sources(
		display_st75320.c
	)
endif()
