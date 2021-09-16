/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <usb/usb_device.h>




#define LOG_LEVEL /*CONFIG_LOG_DEFAULT_LEVEL*/5
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

#define PRIORITY 7

K_THREAD_STACK_DEFINE(gui_refresh_thread_stack_area, 8192);
static struct k_thread gui_refresh_thread_data;

void gui_refresh_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}

void main(void)
{
	LOG_INF("hello world");

	extern void setup_disk(void);
	setup_disk();

	int ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("The device is put in USB mass storage mode.\n");


	const struct device *display_dev;
	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
		return;
	}

	display_blanking_off(display_dev);
	display_set_brightness(display_dev, 4 << 4); // 0-7 levels
	display_set_contrast(display_dev, 3 << 4); // 0-7 levels

	k_thread_create(&gui_refresh_thread_data, gui_refresh_thread_stack_area, K_THREAD_STACK_SIZEOF(gui_refresh_thread_stack_area),
				gui_refresh_thread, NULL, NULL, NULL, PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&gui_refresh_thread_data, "gui_refresh");

	k_thread_start(&gui_refresh_thread_data);

	extern void draw_menu(void);
	draw_menu();

	//extern void start_demo(void);
	//start_demo();
}
