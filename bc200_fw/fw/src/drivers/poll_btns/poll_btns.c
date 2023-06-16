/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>

#include "poll_btn.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(poll_btns, CONFIG_POLL_BTNS_LOG_LEVEL);

#define PRIORITY 7

#define POLL_BTN_TIME 20

static const struct gpio_dt_spec btns[] = {
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(btn0), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(btn1), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(btn2), gpios, {0}),
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(btn3), gpios, {0}),
};

#define BTN_MAX_NUM ARRAY_SIZE(btns)

typedef struct {
	const struct gpio_dt_spec *btns;
} poll_btns_config_t ;

typedef struct {
	lv_indev_t *indev;
	bool states[BTN_MAX_NUM];
} poll_btns_data_t;

static const poll_btns_config_t config = {
	.btns = btns,
};

static poll_btns_data_t data; 

static uint32_t keypad_get_key(void)
{
	for (unsigned int i = 0; i < BTN_MAX_NUM; i++)
		if (data.states[i])
			return i + 1;

	return 0;
}

static bool keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	static uint32_t last_key = 0;

	/*Get whether the a key is pressed and save the pressed key*/
	uint32_t act_key = keypad_get_key();
	if(act_key != 0) {
		data->state = LV_INDEV_STATE_PR;

		/*Translate the keys to LVGL control characters according to your key definitions*/
		switch(act_key) {
		case 1:
			act_key = LV_KEY_UP;;
			break;
		case 2:
			act_key = LV_KEY_DOWN;
			break;
		case 3:
			act_key = LV_KEY_ESC;
			break;
		case 4:
			act_key = LV_KEY_ENTER;
			break;
		}

		last_key = act_key;
	} else {
		data->state = LV_INDEV_STATE_REL;
	}

	data->key = last_key;

	/*Return `false` because we are not buffering and no more data to read*/
	return false;
}

static int poll_btns_init(const struct device *dev)
{
	for (unsigned int i = 0; i < BTN_MAX_NUM; i++) {
		if (!config.btns[i].port) {
			LOG_ERR("button #%d - not inited", i);
			continue;
		}

		gpio_pin_configure_dt(&config.btns[i], GPIO_INPUT | config.btns[i].dt_flags);
	}

	memset(&data, 0, sizeof(poll_btns_data_t));

	lv_indev_drv_t indev_drv;

	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_KEYPAD;
	indev_drv.read_cb = keypad_read;
	data.indev = lv_indev_drv_register(&indev_drv);

	LOG_INF("indev init done %x", (uint32_t)data.indev);

	return 0;
}

DEVICE_DEFINE(poll_btns, CONFIG_POLL_BTNS_DEV_NAME,
			&poll_btns_init, NULL,
			&data, NULL,
			APPLICATION, CONFIG_POLL_BTNS_INIT_PRIORITY,
			NULL);

static void poll_btn_func(void)
{
	bool inited = false;
	static bool last_states[BTN_MAX_NUM];

	while (1) {
		for (unsigned int i = 0; i < BTN_MAX_NUM; i++) {
			bool cur_state = gpio_pin_get_dt(&config.btns[i]);

			if (inited)
				if (cur_state == last_states[i] && cur_state != data.states[i])
					data.states[i] = cur_state;

			last_states[i] = cur_state;
		}

		inited = true;

		k_msleep(POLL_BTN_TIME);
	}
}

K_THREAD_DEFINE(poll_btns_thread, 512, poll_btn_func, NULL, NULL, NULL, PRIORITY, 0, 0);

