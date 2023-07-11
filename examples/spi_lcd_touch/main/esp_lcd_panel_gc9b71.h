/*
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define GC9B71_CMD_READID			0x04
#define GC9B71_CMD_READSTATUS		0x09
#define GC9B71_CMD_SLEEP_IN			0x10
#define GC9B71_CMD_SLEEP_OUT		0x11
#define GC9B71_CMD_PARTIALMOD		0x12
#define GC9B71_CMD_NORMALMOD		0x13
#define GC9B71_CMD_INV_OFF			0x20
#define GC9B71_CMD_INV_ON			0x21
#define GC9B71_CMD_DISP_OFF			0x28
#define GC9B71_CMD_DISP_ON			0x29
#define GC9B71_CMD_CASET			0x2a
#define GC9B71_CMD_PASET			0x2b
#define GC9B71_CMD_MEMWR			0x2c
#define GC9B71_CMD_PARTIALAREA		0x30
#define GC9B71_CMD_VERTSCROLL		0x33
#define GC9B71_CMD_TE_OFF			0x34
#define GC9B71_CMD_TE_ON			0x35

#define GC9B71_CMD_MEMACC			0x36
/*
#define GC9B71_RAMACC_HREFR_INV		(0x1 << 2)
#define GC9B71_RAMACC_VREFR_INV		(0x1 << 4)
#define GC9B71_RAMACC_RGB_ORDER		(0x0 << 3)
#define GC9B71_RAMACC_BGR_ORDER		(0x1 << 3)
#define GC9B71_RAMACC_ROWCOL_SWAP	(0x1 << 5)
#define GC9B71_RAMACC_COL_INV		(0x1 << 6)
#define GC9B71_RAMACC_ROW_INV		(0x1 << 7)
*/
#define GC9B71_CMD_VERTSCROLL_ADDR	0x37
#define GC9B71_CMD_IDLEMOD_OFF		0x38
#define GC9B71_CMD_IDLEMOD_ON		0x39

#define GC9B71_CMD_PIXELFSET		0x3a
/*
#define GC9B71_COLMOD_RGB_16bit		(0x5 << 4)
#define GC9B71_COLMOD_RGB_18bit		(0x6 << 4)
#define GC9B71_COLMOD_MCU_gray		(0)
#define GC9B71_COLMOD_MCU_3bit		(1)
#define GC9B71_COLMOD_MCU_8bit		(2)
#define GC9B71_COLMOD_MCU_12bit		(3)
#define GC9B71_COLMOD_MCU_16bit		(5)
#define GC9B71_COLMOD_MCU_18bit		(6)
#define GC9B71_COLMOD_MCU_24bit		(7)
*/
#define GC9B71_CMD_MEMWR_CONTINUE	0x3c
#define GC9B71_CMD_TESETSCANLINE	0x44
#define GC9B71_CMD_GETSCANLINE		0x45
#define GC9B71_CMD_BRIGHTNESS		0x51
#define GC9B71_CMD_DISPCTRL			0x53

#define GC9B71_CMD_RDID1			0xd1
#define GC9B71_CMD_RDID2			0xd2
#define GC9B71_CMD_RDID3			0xd3

//#define GC9B71_CMD_RGBCTRL			0xb0
#define GC9B71_CMD_SPI2CTRL			0xb1
//#define GC9B71_CMD_PORCHCTRL		0xb5
#define GC9B71_CMD_DISPFUNCTRL		0xb6
#define GC9B71_CMD_TECTRL			0xb4
//#define GC9B71_CMD_IFCTRL			0xf6

#define GC9B71_CMD_INVERSION		0xec
#define GC9B71_CMD_POWCTRL1			0xc1
#define GC9B71_CMD_POWCTRL2			0xc3
#define GC9B71_CMD_POWCTRL3			0xc4
#define GC9B71_CMD_POWCTRL4			0xc9
#define GC9B71_CMD_GAMSET1      	0xf0
#define GC9B71_CMD_GAMSET2      	0xf1
#define GC9B71_CMD_GAMSET3      	0xf2
#define GC9B71_CMD_GAMSET4      	0xf3
#define GC9B71_CMD_INTERREG_EN1		0xfe
#define GC9B71_CMD_INTERREG_EN2		0xef

esp_err_t esp_lcd_new_panel_gc9b71(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

