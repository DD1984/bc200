/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 * Copyright (c) 2020 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st75320

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <drivers/display.h>
#include <drivers/pwm.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_st75320, CONFIG_DISPLAY_LOG_LEVEL);

#define ST75320_CMD_DISP_OFF 0xAE
#define ST75320_CMD_DISP_ON 0xAF
#define ST75320_CMD_SLEEP_OUT 0xA8
#define ST75320_CMD_SLEEP_IN 0xA9
#define ST75320_CMD_PAGE_ADDR 0xB1
#define ST75320_CMD_COLUMN_ADDR 0x13

#define ST75320_RESET_TIME K_MSEC(50)
#define ST75320_EXIT_SLEEP_TIME K_MSEC(150)

#define U8X8_MSG_CAD_SEND_CMD 21
#define U8X8_MSG_CAD_SEND_ARG 22

#define U8X8_C(c0)							(U8X8_MSG_CAD_SEND_CMD), (c0)
#define U8X8_A(a0)							(U8X8_MSG_CAD_SEND_ARG), (a0)
#define U8X8_CA(c0,a0)						(U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0)
#define U8X8_CAA(c0,a0,a1)					(U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0), (U8X8_MSG_CAD_SEND_ARG), (a1)
#define U8X8_CAAA(c0,a0,a1, a2)				(U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0), (U8X8_MSG_CAD_SEND_ARG), (a1), (U8X8_MSG_CAD_SEND_ARG), (a2)

#define U8X8_A4(a0,a1,a2,a3)				U8X8_A(a0), U8X8_A(a1), U8X8_A(a2), U8X8_A(a3)
#define U8X8_A8(a0,a1,a2,a3,a4,a5,a6,a7)	U8X8_A4((a0), (a1), (a2), (a3)), U8X8_A4((a4), (a5), (a6), (a7))

#define U8X8_DLY(m)							(0xfe),(m) /* delay in milli seconds */
#define U8X8_END()							(0x00)

#define CONTRAST_DEF 0x111

static const uint8_t st75320_btg160240_init_seq[] =
{
	U8X8_C(ST75320_CMD_DISP_OFF),		// Display OFF
	U8X8_CA(0xEA, 0x00), 				// Power Discharge Control, Discharge OFF
	U8X8_C(ST75320_CMD_SLEEP_OUT), 		// sleep out
	U8X8_C(0xAB), 						// OSC ON
	U8X8_C(0x69), 						// Temperature Detection ON
	U8X8_C(0x4E), 						// TC Setting
	U8X8_A8(0xAF, 0x56, 0x32, 0x00, 0x40, 0x66, 0xB7, 0xFF),

	U8X8_CAA(0x39, 0x00, 0x00), 		//TC Flag

	U8X8_CA(0x2B, 0x00), 				// Frame Rate Level X1
	U8X8_CAA(0x5F, 0x44, 0x44), 		// Set Frame Frequency, fFR=??Hz in all temperature range
	U8X8_CAAA(0xEC, 0x19, 0x2D, 0x55), 	// FR Compensation Temp. Range, TA = -?? degree, TB = ?? degree, TC = ?? degree
	U8X8_CAA(0xED, 0x04, 0x04), 		// Temp. Hysteresis Value (thermal sensitivity)
	U8X8_C(0xA6), 						// Display Inverse OFF
	U8X8_C(0xA4), 						// Disable Display All Pixel ON

	U8X8_CA(0xC4, 0x02), 				// COM Output Status  
	U8X8_C(0xA1), 						// Column Address Direction: MX=0

	U8X8_CAA(0x6D, 0x07, 0x00), 		// Display Area, Duty = 1/240 duty, Start Group = 1
	U8X8_C(0x84), 						// Display Data Input Direction: Column
	U8X8_CA(0x36, 0x0D), 				// Set N-Line
	U8X8_C(0xE5), 						// N-Line On
	U8X8_CA(0xE7, 0x19), 				// LCD Drive Method //NLFR=1//

	U8X8_CAA(0x81, CONTRAST_DEF & 0xff, (CONTRAST_DEF >> 8) & 3), 	//OX81: Set EV=64h, 0..255, 0..3
	U8X8_CA(0xA2, 0x0a), 				// BIAS //1/16 BIAS

	U8X8_C(0xFE),
	U8X8_CAAA(0xF0, 0x35, 0xC7, 0x10),
	U8X8_C(0xFC),

	U8X8_CA(0x25, 0x40),
	U8X8_DLY(100),
	U8X8_CA(0x25, 0x70), 				// Power Control //AVDD, MV3, NAVDD & V3 ON
	U8X8_DLY(100),
	U8X8_CA(0x25, 0x78),	 			// Power Control//AVDD, MV3, NAVDD, V3 & VPF ON
	U8X8_DLY(100),
	U8X8_CA(0x25, 0x7c), 				// Power Control//AVDD, MV3, NAVDD, V3, VPF & VNF ON
	U8X8_DLY(100),
	U8X8_CA(0x25, 0x7e), 				// Power Control//VOUT, AVDD, MV3, NAVDD, V3, VPF & VNF ON
	U8X8_DLY(100),
	U8X8_CA(0x25, 0x7f), 				// Power Control/VOUT, AVDD, MV3, NAVDD, V3, VPF & VNF ON
	U8X8_DLY(100),

	U8X8_C(0xFE),
	U8X8_CAAA(0xF0, 0x35, 0x92, 0x50),
	U8X8_C(0xFC),

	//U8X8_C(ST75320_CMD_DISP_ON),		//Display ON

	U8X8_END() 							/* end of sequence */
};

struct st75320_gpio_data {
	const char *const name;
	gpio_dt_flags_t flags;
	gpio_pin_t pin;
};

#define BKL_PWM_PERIOD 100

struct st75320_bkl_data {
	const struct device *dev;
	uint32_t channel;
};

struct st75320_config {
	const char *spi_name;
	const char *cs_name;
	struct spi_config spi_config;
	struct st75320_gpio_data cmd_data;
	struct st75320_gpio_data reset;
	struct st75320_gpio_data power;
	struct st75320_bkl_data backlight;
	uint16_t height;
	uint16_t width;
};

struct st75320_data {
	const struct st75320_config *config;
	const struct device *spi_dev;
	struct spi_cs_control cs_ctrl;
	const struct device *cmd_data_dev;
	const struct device *reset_dev;
	const struct device *power_dev;
	uint16_t x_offset;
	uint16_t y_offset;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t pm_state;
#endif
};

static void st75320_set_cmd(struct st75320_data *data, int is_cmd)
{
	gpio_pin_set(data->cmd_data_dev, data->config->cmd_data.pin, is_cmd);
}

static void st75320_cmd_tx(struct st75320_data *data, uint8_t cmd)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	st75320_set_cmd(data, 1);
	spi_write(data->spi_dev, &data->config->spi_config, &tx_bufs);
}

static void st75320_data_tx(struct st75320_data *data, uint8_t d)
{
	struct spi_buf tx_buf = { .buf = &d, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	st75320_set_cmd(data, 0);
	spi_write(data->spi_dev, &data->config->spi_config, &tx_bufs);
}

static void st75320_send_sequence(struct st75320_data *data, const uint8_t *d)
{
	uint8_t cmd;

	for(;;) {
		cmd = *d;
		d++;
		switch (cmd) {
			case U8X8_MSG_CAD_SEND_CMD:
				st75320_cmd_tx(data, *d);
				d++;
				break;
			case U8X8_MSG_CAD_SEND_ARG:
				st75320_data_tx(data, *d);
				d++;
				break;
			case 0x0fe:
				k_sleep(K_MSEC(*d));
				d++;
				break;
			default:
			return;
		}
	}
}

static int st75320_reset_display(struct st75320_data *data)
{
	LOG_DBG("Resetting display");
	if (!data->config->reset.name)
		return -1;

	gpio_pin_set(data->reset_dev, data->config->reset.pin, 1);
	k_sleep(ST75320_RESET_TIME);
	gpio_pin_set(data->reset_dev, data->config->reset.pin, 0);

	k_sleep(ST75320_EXIT_SLEEP_TIME);

	return 0;
}

static int st75320_blanking_on(const struct device *dev)
{
	struct st75320_data *data = (struct st75320_data *)dev->data;

	st75320_cmd_tx(data, ST75320_CMD_DISP_OFF);

	return 0;
}

static int st75320_blanking_off(const struct device *dev)
{
	struct st75320_data *data = (struct st75320_data *)dev->data;

	st75320_cmd_tx(data, ST75320_CMD_DISP_ON);

	return 0;
}

static int st75320_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void st75320_set_column_addr(struct st75320_data *data, uint16_t x)
{
	st75320_cmd_tx(data, ST75320_CMD_COLUMN_ADDR);
	st75320_data_tx(data, x >> 8);
	st75320_data_tx(data, x & 0xff);
}

static void st75320_set_page_addr(struct st75320_data *data, uint16_t y)
{
	st75320_cmd_tx(data, ST75320_CMD_PAGE_ADDR);
	st75320_data_tx(data, ((y + data->y_offset) / 8) & 0xff);
}

static int st75320_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct st75320_data *data = (struct st75320_data *)dev->data;
	size_t buf_len;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller then width");
		return -1;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -1;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -1;
	}

	if (((y + data->y_offset) & 0x7) != 0U) {
		LOG_ERR("Unsupported origin");
		return -1;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u",
		x, y, desc->pitch, desc->width, desc->height, buf_len);

	for (uint16_t i = 0; i < desc->height; i += 8) {
		st75320_set_column_addr(data, x + data->x_offset);
		st75320_set_page_addr(data, y + data->y_offset + i);

		st75320_cmd_tx(data, 0x1D);
		st75320_set_cmd(data, 0);

		struct spi_buf tx_buf = { .buf = (uint8_t *)buf + desc->pitch * (i / 8), .len = desc->pitch };
		struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };
		spi_write(data->spi_dev, &data->config->spi_config, &tx_bufs);
	}

	return 0;
}

static void *st75320_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int st75320_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	const uint8_t levels[8] = {0, 14, 28, 42, 56, 70, 84, 100};
	struct st75320_config *config = (struct st75320_config *)dev->config;

	if (!device_is_ready(config->backlight.dev))
		return -ENOTSUP;

	uint32_t pulse = BKL_PWM_PERIOD * levels[brightness >> 4] / 100;

	LOG_INF("brightness: in-%d -> pwm-%d/%d", brightness, pulse, BKL_PWM_PERIOD);

	pwm_pin_set_cycles(config->backlight.dev, config->backlight.channel, BKL_PWM_PERIOD, pulse, PWM_POLARITY_NORMAL);

	return 0;
}

static int st75320_set_contrast(const struct device *dev,
				const uint8_t contrast)
{
	const uint16_t levels[8] = {
		CONTRAST_DEF - 18, CONTRAST_DEF - 12, CONTRAST_DEF - 6,
		CONTRAST_DEF,
		CONTRAST_DEF + 6, CONTRAST_DEF + 12, CONTRAST_DEF + 18, CONTRAST_DEF + 24,
	};
	struct st75320_data *data = (struct st75320_data *)dev->data;

	LOG_INF("contrast: in-%d -> %d", contrast, levels[contrast >> 4]);

	st75320_cmd_tx(data, 0x81);
	st75320_data_tx(data, levels[contrast >> 4] & 0xff);
	st75320_data_tx(data, (levels[contrast >> 4] >> 8) & 3);

	return 0;
}

static void st75320_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct st75320_config *config = (struct st75320_config *)dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;

	capabilities->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	capabilities->current_pixel_format = PIXEL_FORMAT_MONO10;

	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;

	capabilities->screen_info = SCREEN_INFO_MONO_VTILED;
}

static int st75320_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_MONO10)
		return 0;

	LOG_ERR("Pixel format change not implemented");

	return -ENOTSUP;
}

static int st75320_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	LOG_ERR("Changing display orientation not implemented");

	return -ENOTSUP;
}

static int st75320_lcd_init(struct st75320_data *data)
{
	st75320_send_sequence(data, st75320_btg160240_init_seq);

	return 0;
}

static int st72320_pwr_pin_init(struct st75320_data *data)
{
	data->power_dev = NULL;

	if (!data->config->power.name)
		return -1;

	data->power_dev = device_get_binding(data->config->power.name);
	if (data->power_dev == NULL)
		return -1;

	int ret = gpio_pin_configure(data->power_dev, data->config->power.pin,
					 GPIO_OUTPUT_INACTIVE | data->config->power.flags);
	if (ret)
		return -1;

	return 0;
}

static void st72320_pwr_en(struct st75320_data *data, bool en)
{
	if (!data->power_dev)
		return;

	if (en)
		gpio_pin_set(data->power_dev, data->config->power.pin, 1);
	else
		gpio_pin_set(data->power_dev, data->config->power.pin, 0);
}

static int st75320_init(const struct device *dev)
{
	struct st75320_data *data = (struct st75320_data *)dev->data;
	struct st75320_config *config = (struct st75320_config *)dev->config;
	int ret;

	data->spi_dev = device_get_binding(config->spi_name);
	if (data->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for LCD");
		return -ENODEV;
	}

	if (config->cs_name) {
		data->cs_ctrl.gpio_dev = device_get_binding(config->cs_name);
		if (data->cs_ctrl.gpio_dev == NULL) {
			LOG_ERR("Could not get device for SPI CS");
			return -ENODEV;
		}
	}

	if (config->reset.name) {
		data->reset_dev = device_get_binding(config->reset.name);
		if (data->reset_dev == NULL) {
			LOG_ERR("Could not get GPIO port for display reset");
			return -ENODEV;
		}

		ret = gpio_pin_configure(data->reset_dev, config->reset.pin,
					 GPIO_OUTPUT_INACTIVE | config->reset.flags);
		if (ret) {
			LOG_ERR("Couldn't configure reset pin");
			return ret;
		}
	}

	st72320_pwr_pin_init(data);
	st72320_pwr_en(data, true);

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	data->pm_state = PM_DEVICE_STATE_ACTIVE;
#endif

	data->cmd_data_dev = device_get_binding(config->cmd_data.name);
	if (data->cmd_data_dev == NULL) {
		LOG_ERR("Could not get GPIO port for cmd/DATA port");
		return -ENODEV;
	}

	ret = gpio_pin_configure(data->cmd_data_dev, config->cmd_data.pin,
				 GPIO_OUTPUT | config->cmd_data.flags);
	if (ret) {
		LOG_ERR("Couldn't configure cmd/DATA pin");
		return ret;
	}

	ret = st75320_reset_display(data);
	if (ret < 0) {
		LOG_ERR("Couldn't reset display");
		return ret;
	}

	ret = st75320_lcd_init(data);
	if (ret < 0) {
		LOG_ERR("Couldn't init LCD");
		return ret;
	}

	LOG_INF("LCD init done");

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int st75320_enter_sleep(struct st75320_data *data)
{
	//return st7735r_transmit(data, ST7735R_CMD_SLEEP_IN, NULL, 0);
}

static int st75320_pm_control(const struct device *dev, uint32_t ctrl_command,
			      void *context, pm_device_cb cb, void *arg)
{
	int ret = 0;
	struct st75320_data *data = (struct st75320_data *)dev->data;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		if (*((uint32_t *)context) == PM_DEVICE_STATE_ACTIVE) {
			ret = st75320_exit_sleep(data);
			if (ret < 0) {
				return ret;
			}
			data->pm_state = PM_DEVICE_STATE_ACTIVE;
		} else {
			ret = st75320_enter_sleep(data);
			if (ret < 0) {
				return ret;
			}
			data->pm_state = PM_DEVICE_STATE_LOW_POWER;
		}

		break;

	case PM_DEVICE_STATE_GET:
		*((uint32_t *)context) = data->pm_state;

		break;

	default:
		ret = -EINVAL;
	}

	if (cb != NULL) {
		cb(dev, ret, context, arg);
	}

	return ret;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static const struct display_driver_api st75320_api = {
	.blanking_on = st75320_blanking_on,
	.blanking_off = st75320_blanking_off,
	.write = st75320_write,
	.read = st75320_read,
	.get_framebuffer = st75320_get_framebuffer,
	.set_brightness = st75320_set_brightness,
	.set_contrast = st75320_set_contrast,
	.get_capabilities = st75320_get_capabilities,
	.set_pixel_format = st75320_set_pixel_format,
	.set_orientation = st75320_set_orientation,
};

#define ST75320_INIT(inst)							\
	static struct st75320_data st75320_data_ ## inst;			\
										\
	const static struct st75320_config st75320_config_ ## inst = {		\
		.spi_name = DT_INST_BUS_LABEL(inst),				\
		.cs_name = UTIL_AND(						\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)),			\
		.spi_config.slave = DT_INST_REG_ADDR(inst),			\
		.spi_config.frequency = UTIL_AND(				\
			DT_HAS_PROP(inst, spi_max_frequency),			\
			DT_INST_PROP(inst, spi_max_frequency)),			\
		.spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),	\
		.spi_config.cs = UTIL_AND(					\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			&(st75320_data_ ## inst.cs_ctrl)),			\
		\
		.cmd_data.name = DT_INST_GPIO_LABEL(inst, cmd_data_gpios),	\
		.cmd_data.pin = DT_INST_GPIO_PIN(inst, cmd_data_gpios),		\
		.cmd_data.flags = DT_INST_GPIO_FLAGS(inst, cmd_data_gpios),	\
		\
		.reset.name = UTIL_AND(						\
			DT_INST_HAS_PROP(inst, reset_gpios),			\
			DT_INST_GPIO_LABEL(inst, reset_gpios)),			\
		.reset.pin = UTIL_AND(						\
			DT_INST_HAS_PROP(inst, reset_gpios),			\
			DT_INST_GPIO_PIN(inst, reset_gpios)),			\
		.reset.flags = UTIL_AND(					\
			DT_INST_HAS_PROP(inst, reset_gpios),			\
			DT_INST_GPIO_FLAGS(inst, reset_gpios)),			\
		\
		.power.name = UTIL_AND(						\
			DT_INST_HAS_PROP(inst, power_gpios),			\
			DT_INST_GPIO_LABEL(inst, power_gpios)),			\
		.power.pin = UTIL_AND(						\
			DT_INST_HAS_PROP(inst, power_gpios),			\
			DT_INST_GPIO_PIN(inst, power_gpios)),			\
		.power.flags = UTIL_AND(					\
			DT_INST_HAS_PROP(inst, power_gpios),			\
			DT_INST_GPIO_FLAGS(inst, power_gpios)),			\
		\
		.backlight.dev = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_CHILD(DT_INST_PHANDLE(inst, backlight), led0), pwms, 0)),	\
		.backlight.channel = DT_PHA_BY_IDX(DT_CHILD(DT_INST_PHANDLE(inst, backlight), led0), pwms, 0, channel),	\
		\
		.width = DT_INST_PROP(inst, width),				\
		.height = DT_INST_PROP(inst, height),				\
	};									\
										\
	static struct st75320_data st75320_data_ ## inst = {			\
		.config = &st75320_config_ ## inst,				\
		.cs_ctrl.gpio_pin = UTIL_AND(					\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			DT_INST_SPI_DEV_CS_GPIOS_PIN(inst)),			\
		.cs_ctrl.gpio_dt_flags = UTIL_AND(				\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst)),			\
		.cs_ctrl.delay = 0U,						\
		.x_offset = DT_INST_PROP(inst, x_offset),			\
		.y_offset = DT_INST_PROP(inst, y_offset),			\
	};									\
	DEVICE_DT_INST_DEFINE(inst, st75320_init, st75320_pm_control,	\
			      &st75320_data_ ## inst, &st75320_config_ ## inst,	\
			      APPLICATION, CONFIG_ST75320_INIT_PRIORITY,	\
			      &st75320_api);



DT_INST_FOREACH_STATUS_OKAY(ST75320_INIT)
