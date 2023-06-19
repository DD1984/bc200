/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>



#define LOG_LEVEL /*CONFIG_LOG_DEFAULT_LEVEL*/5
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define PRIORITY 6

K_THREAD_STACK_DEFINE(gui_refresh_thread_stack_area, 8192);
static struct k_thread gui_refresh_thread_data;
K_MUTEX_DEFINE(lvgl_mutex); //lock when lv_ fuction called from other thread

void gui_refresh_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (1) {
		k_mutex_lock(&lvgl_mutex, K_FOREVER);
		lv_task_handler();
		k_mutex_unlock(&lvgl_mutex);

		k_sleep(K_MSEC(10));
	}
}

lv_obj_t *upgr_lbl;
void uprg_progress(const char *text)
{
	k_mutex_lock(&lvgl_mutex, K_FOREVER);
	lv_label_set_text(upgr_lbl, text);
	k_mutex_unlock(&lvgl_mutex);
}

void main(void)
{
	LOG_INF("hello world");

	const struct device *display_dev;
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	display_blanking_off(display_dev);
	display_set_brightness(display_dev, 1 << 4); // 0-7 levels
	display_set_contrast(display_dev, 3 << 4); // 0-7 levels

	k_thread_create(&gui_refresh_thread_data, gui_refresh_thread_stack_area, K_THREAD_STACK_SIZEOF(gui_refresh_thread_stack_area),
				gui_refresh_thread, NULL, NULL, NULL, PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&gui_refresh_thread_data, "gui_refresh");

	k_thread_start(&gui_refresh_thread_data);

	//extern void draw_menu(void);
	//draw_menu();

	// gps power en
	struct gpio_dt_spec gps_pwr = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(gps_pwr), gpios, {0});
	gpio_pin_configure_dt(&gps_pwr, GPIO_OUTPUT_INACTIVE);
	gpio_pin_set_dt(&gps_pwr, 1);

	upgr_lbl = lv_label_create(lv_scr_act());
	lv_label_set_text(upgr_lbl, "hello world");
	//lv_obj_set_pos(upgr_lbl, 0, LV_VER_RES - 10);

	extern void fw_upgrade_task_start(void);
	fw_upgrade_task_start();


	//extern void start_demo(void);
	//start_demo();
}
