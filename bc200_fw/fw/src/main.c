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

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

void main(void)
{
	const struct device *dev;

	dev = device_get_binding(LED0);
	gpio_pin_configure(dev, 0x1A, GPIO_OUTPUT_ACTIVE);


	uint32_t count = 0U;
	char count_str[11] = {0};
	const struct device *display_dev;
	lv_obj_t *count_label;

	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
		return;
	}

	lv_obj_t *checkbox = lv_checkbox_create(lv_scr_act(), NULL);
	lv_checkbox_set_text(checkbox, "hello");

	count_label = lv_label_create(lv_scr_act(), NULL);
	lv_obj_align(count_label, NULL, /*LV_ALIGN_IN_BOTTOM_MID*/LV_ALIGN_CENTER, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	while (1) {
		if ((count % 20) == 0U) {
			sprintf(count_str, "CooSpo-%03d", count/20U);
			lv_label_set_text(count_label, count_str);
		}
		if ((count % 100) == 0U) {
			lv_checkbox_set_checked(checkbox, !lv_checkbox_is_checked(checkbox));
		}

		lv_task_handler();
		k_sleep(K_MSEC(10));
		++count;
	}
}
