/**
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "nordic_common.h"

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_spi.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(0);

#define UART_TX 39

#define LCD_RST 0x2D
#define LCD_A0  0x1D
#define LCD_CS  0x02
#define LCD_SCL 0x2F
#define LCD_W   0x1F
#define LCD_BACKLIGHT 0x1A
#define LCD_POWER 0x2B // ???


#define U8X8_MSG_CAD_INIT 20


#define U8X8_MSG_CAD_SEND_CMD 21
/*  arg_int: cmd byte */
#define U8X8_MSG_CAD_SEND_ARG 22
/*  arg_int: arg byte */
#define U8X8_MSG_CAD_SEND_DATA 23
/* arg_int: expected cs level after processing this msg */
#define U8X8_MSG_CAD_START_TRANSFER 24
/* arg_int: expected cs level after processing this msg */
#define U8X8_MSG_CAD_END_TRANSFER 25


#define U8X8_C(c0)                              (U8X8_MSG_CAD_SEND_CMD), (c0)
#define U8X8_A(a0)                              (U8X8_MSG_CAD_SEND_ARG), (a0)
#define U8X8_CA(c0,a0)                  (U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0)
#define U8X8_CAA(c0,a0,a1)              (U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0), (U8X8_MSG_CAD_SEND_ARG), (a1)
#define U8X8_CAAA(c0,a0,a1, a2) (U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0), (U8X8_MSG_CAD_SEND_ARG), (a1), (U8X8_MSG_CAD_SEND_ARG), (a2)
#define U8X8_CAAAA(c0,a0,a1,a2,a3)              (U8X8_MSG_CAD_SEND_CMD), (c0), (U8X8_MSG_CAD_SEND_ARG), (a0), (U8X8_MSG_CAD_SEND_ARG), (a1), (U8X8_MSG_CAD_SEND_ARG), (a2), (U8X8_MSG_CAD_SEND_ARG), (a3)
#define U8X8_AAC(a0,a1,c0)              (U8X8_MSG_CAD_SEND_ARG), (a0), (U8X8_MSG_CAD_SEND_ARG), (a1), (U8X8_MSG_CAD_SEND_CMD), (c0)
#define U8X8_D1(d0)                     (U8X8_MSG_CAD_SEND_DATA), (d0)

#define U8X8_A4(a0,a1,a2,a3)            U8X8_A(a0), U8X8_A(a1), U8X8_A(a2), U8X8_A(a3)
#define U8X8_A8(a0,a1,a2,a3,a4,a5,a6,a7)        U8X8_A4((a0), (a1), (a2), (a3)), U8X8_A4((a4), (a5), (a6), (a7))


#define U8X8_START_TRANSFER()   (U8X8_MSG_CAD_START_TRANSFER)
#define U8X8_END_TRANSFER()     (U8X8_MSG_CAD_END_TRANSFER)
#define U8X8_DLY(m)                     (0xfe),(m)              /* delay in milli seconds */
#define U8X8_END()     

uint8_t st75320_btg160240_init_seq[] = {
    
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  U8X8_C(0xAE), 				// Display OFF
  U8X8_CA(0xEA, 0x00), 			// Power Discharge Control, Discharge OFF
  U8X8_C(0xA8), 				// sleep out
  U8X8_C(0xAB), 				// OSC ON
  U8X8_C(0x69), 				// Temperature Detection ON
  U8X8_C(0x4E), 				// TC Setting
  U8X8_A8(0xAF, 0x56, 0x32, 0x00, 0x40, 0x66, 0xB7, 0xFF),

  U8X8_CAA(0x39, 0x00, 0x00), 	//TC Flag
  
  
  U8X8_CA(0x2B, 0x00), 			// Frame Rate Level X1
  U8X8_CAA(0x5F, 0x44, 0x44), 	// Set Frame Frequency, fFR=??Hz in all temperature range
  U8X8_CAAA(0xEC, 0x19, 0x2D, 0x55), // FR Compensation Temp. Range, TA = -?? degree, TB = ?? degree, TC = ?? degree
  U8X8_CAA(0xED, 0x04, 0x04), 	// Temp. Hysteresis Value (thermal sensitivity)
  U8X8_C(0xA6), 				// Display Inverse OFF
  U8X8_C(0xA4), 				// Disable Display All Pixel ON

  U8X8_CA(0xC4, 0x02), 			// COM Output Status  
  U8X8_C(0xA1), 				// Column Address Direction: MX=0
  
  U8X8_CAA(0x6D, 0x07, 0x00), 	// Display Area, Duty = 1/240 duty, Start Group = 1
  U8X8_C(0x84), 				// Display Data Input Direction: Column
  U8X8_CA(0x36, 0x0D), 			// Set N-Line
  U8X8_C(0xE5), 				// N-Line On
  U8X8_CA(0xE7, 0x19), 			// LCD Drive Method //NLFR=1//

  U8X8_CAA(0x81, /*mem & 0xff*/0x11, /*(mem & 0xffff)>>8*/0x01), 	// mem = 0x10111 OX81: Set EV=64h, 0..255, 0..3
  U8X8_CA(0xA2, 0x0a), 			// BIAS //1/16 BIAS

  U8X8_C(0xFE),
  U8X8_CAAA(0xF0, 0x35, 0xC7, 0x10),
  U8X8_C(0xFC),
  
  U8X8_CA(0x25, 0x40),
  U8X8_DLY(100),
  U8X8_CA(0x25, 0x70), 			// Power Control //AVDD, MV3, NAVDD & V3 ON
  U8X8_DLY(100),
  U8X8_CA(0x25, 0x78), 			// Power Control//AVDD, MV3, NAVDD, V3 & VPF ON
  U8X8_DLY(100),
  U8X8_CA(0x25, 0x7c), 			// Power Control//AVDD, MV3, NAVDD, V3, VPF & VNF ON
  U8X8_DLY(100),
  U8X8_CA(0x25, 0x7e), 			// Power Control//VOUT, AVDD, MV3, NAVDD, V3, VPF & VNF ON
  U8X8_DLY(100),
  U8X8_CA(0x25, 0x7f), 			// Power Control/VOUT, AVDD, MV3, NAVDD, V3, VPF & VNF ON
  U8X8_DLY(100),

  U8X8_C(0xFE),
  U8X8_CAAA(0xF0, 0x35, 0x92, 0x50),
  U8X8_C(0xFC),

  U8X8_C(0xAF), //Display ON  

  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()           			/* end of sequence */
};

void spi_tx(uint8_t const *buf, uint32_t len)
{
	nrf_drv_spi_transfer(&spi, buf, len, NULL, 0);
}

void cmd_tx(uint8_t cmd)
{
	nrf_gpio_pin_write(LCD_A0, 0);
	spi_tx(&cmd, 1);
}

void data_tx(uint8_t data)
{
	nrf_gpio_pin_write(LCD_A0, 1);
	spi_tx(&data, 1);
}

void SendSequence(uint8_t *data)
{
	uint8_t cmd;
	uint8_t v;

	for(;;) {
		cmd = *data;
		data++;
		switch (cmd) {
			case U8X8_MSG_CAD_SEND_CMD:
				cmd_tx(*data);
				data++;
				break;
			case U8X8_MSG_CAD_SEND_ARG:
				data_tx(*data);
				data++;
				break;
			case U8X8_MSG_CAD_START_TRANSFER:
			case U8X8_MSG_CAD_END_TRANSFER:
				break;
			case 0x0fe:
				v = *data;
				nrf_delay_ms(v);
				data++;
				break;
			default:
			return;
		}
	}
}

void lcd_clr(void)
{
	cmd_tx(0xB1); // Page Address
	data_tx(0); // Page 0

	cmd_tx(0x13); // Column Address
	data_tx(0x00); // Start Column = 0
	data_tx(0x00);

	cmd_tx(0x1D);

	for (uint32_t i = 0; i < 240 * 320 / 8; i ++)
		data_tx(0);
}

void lcd_ptrn(uint8_t start_ptrn)
{
	for (uint32_t y = 0; y < 240 / 8; y++) {
		cmd_tx(0xB1); // Page Address
		data_tx(y);

		cmd_tx(0x13); // Column Address
		data_tx(0); // Start Column = 80
		data_tx(80);

		cmd_tx(0x1D);

		uint8_t p = start_ptrn;
		if (y % 2 == 0)
			p = ~p;

		for (uint32_t x = 0; x < 160; x++) {
			if (x % 8 == 0)
				p = ~p;

			data_tx(p);
		}
	}
}

int main(void)
{
	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();

	NRF_LOG_INFO("hello world");

#if 0
	#define PIN 39
	nrf_gpio_cfg_output(PIN);
	while (1) {
		nrf_gpio_pin_toggle(PIN);
		nrf_delay_ms(500);
	}
#endif

	nrf_gpio_cfg_output(LCD_RST);
	nrf_gpio_cfg_output(LCD_A0);

	nrf_gpio_cfg_output(LCD_POWER);
	nrf_gpio_pin_write(LCD_POWER, 1);

	nrf_gpio_cfg_output(LCD_BACKLIGHT);
	nrf_gpio_pin_write(LCD_BACKLIGHT, 1);


	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
	spi_config.ss_pin   = LCD_CS;
	spi_config.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
	spi_config.mosi_pin = LCD_W;
	spi_config.sck_pin  = LCD_SCL;
	spi_config.frequency = NRF_DRV_SPI_FREQ_4M; //from fw
	APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, NULL, NULL));

	NRF_LOG_INFO("%s[%d]", __func__, __LINE__);


	nrf_gpio_pin_write(LCD_RST, 1);
	nrf_delay_ms(50);
	nrf_gpio_pin_write(LCD_RST, 0);
	nrf_delay_ms(50);
	nrf_gpio_pin_write(LCD_RST, 1);

	nrf_delay_ms(150);

	SendSequence(st75320_btg160240_init_seq);

	lcd_clr();

	NRF_LOG_INFO("%s[%d]", __func__, __LINE__);

    while (1) {
		lcd_ptrn(0xff);
		nrf_delay_ms(2000);
		lcd_ptrn(0x00);
		nrf_delay_ms(2000);
	}
}

/**
 *@}
 **/
