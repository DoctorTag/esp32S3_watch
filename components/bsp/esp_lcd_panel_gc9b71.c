/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "gc9b71";

static esp_err_t panel_gc9b71_del(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9b71_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9b71_init(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9b71_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_gc9b71_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_gc9b71_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_gc9b71_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_gc9b71_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_gc9b71_disp_on_off(esp_lcd_panel_t *panel, bool off);

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    unsigned int bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_cal; // save surrent value of LCD_CMD_COLMOD register
} gc9b71_panel_t;

esp_err_t esp_lcd_new_panel_gc9b71(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = ESP_OK;
    gc9b71_panel_t *gc9b71 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    gc9b71 = calloc(1, sizeof(gc9b71_panel_t));
    ESP_GOTO_ON_FALSE(gc9b71, ESP_ERR_NO_MEM, err, TAG, "no mem for gc9b71 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->color_space) {
    case ESP_LCD_COLOR_SPACE_RGB:
        gc9b71->madctl_val = 0;
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        gc9b71->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    switch (panel_dev_config->bits_per_pixel) {
    case 16:
        gc9b71->colmod_cal = 0x55;
        break;
    case 18:
        gc9b71->colmod_cal = 0x66;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    gc9b71->io = io;
    gc9b71->bits_per_pixel = panel_dev_config->bits_per_pixel;
    gc9b71->reset_gpio_num = panel_dev_config->reset_gpio_num;
    gc9b71->reset_level = panel_dev_config->flags.reset_active_high;
    gc9b71->base.del = panel_gc9b71_del;
    gc9b71->base.reset = panel_gc9b71_reset;
    gc9b71->base.init = panel_gc9b71_init;
    gc9b71->base.draw_bitmap = panel_gc9b71_draw_bitmap;
    gc9b71->base.invert_color = panel_gc9b71_invert_color;
    gc9b71->base.set_gap = panel_gc9b71_set_gap;
    gc9b71->base.mirror = panel_gc9b71_mirror;
    gc9b71->base.swap_xy = panel_gc9b71_swap_xy;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    gc9b71->base.disp_off = panel_gc9b71_disp_on_off;
#else
    gc9b71->base.disp_on_off = panel_gc9b71_disp_on_off;
#endif
    *ret_panel = &(gc9b71->base);
    ESP_LOGI(TAG, "new gc9b71 panel @%p", gc9b71);

    return ESP_OK;

err:
    if (gc9b71) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(gc9b71);
    }
    return ret;
}

static esp_err_t panel_gc9b71_del(esp_lcd_panel_t *panel)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);

    if (gc9b71->reset_gpio_num >= 0) {
        gpio_reset_pin(gc9b71->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del gc9b71 panel @%p", gc9b71);
    free(gc9b71);
    return ESP_OK;
}

static esp_err_t panel_gc9b71_reset(esp_lcd_panel_t *panel)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9b71->io;

    // perform hardware reset
    if (gc9b71->reset_gpio_num >= 0) {
        gpio_set_level(gc9b71->reset_gpio_num, gc9b71->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(gc9b71->reset_gpio_num, !gc9b71->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else { // perform software reset
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5ms before sending new command
    }

    return ESP_OK;
}

typedef struct {
    uint8_t cmd;
    uint8_t data[32];
    uint8_t data_bytes; // Length of data in above data array; 0xFF = end of cmds.
} lcd_init_cmd_t;



static const lcd_init_cmd_t vendor_specific_init[] = {
    // Enable Inter Register
    {0xfe, {0}, 0},
    {0xef, {0}, 0},
    {0x80, {0x11}, 1},
    {0x81, {0x70}, 1},
    {0x82, {0x09}, 1},
    {0x83, {0x03}, 1},
    
    {0x84, {0x62}, 1},
    {0x89, {0x18}, 1},
    {0x8a, {0x40}, 1},
    {0x8b, {0x0a}, 1},
    
    {0x3a, {0x05}, 1},
    {0x36, {0x40}, 1},    
    {0xec, {0x07}, 1},
    {0x74, {0x01, 0x80, 0x00, 0x00, 0x00, 0x00}, 6},
    {0x98, {0x3e}, 1},
    {0x99, {0x3e}, 1},
    
    {0xa1, {0x01, 0x04}, 2},    
    {0xa2, {0x01, 0x04}, 2},
    {0xcb, {0x02}, 1},
    {0x7c, {0xb6, 0x24}, 2},
    {0xac, {0x74}, 1},
    {0xf6, {0x80}, 1},
    {0xb5, {0x09, 0x09}, 2},
    {0xeb, {0x01, 0x81}, 2},
    {0x60, {0x38, 0x06, 0x13, 0x56}, 4},    
    {0x63, {0x38, 0x08, 0x13, 0x56}, 4},
    
    {0x61, {0x3b, 0x1b, 0x58, 0x38}, 4},    
    {0x62, {0x3b, 0x1b, 0x58, 0x38}, 4},

	
    {0x64, {0x38, 0x0a, 0x73, 0x16, 0x13, 0x56}, 6},
    {0x66, {0x38, 0x0b, 0x73, 0x17, 0x13, 0x56}, 6},

	
    {0x68, {0x00, 0x0b, 0x22, 0x0b, 0x22, 0x1c, 0x1c}, 7},
    {0x69, {0x00, 0x0b, 0x26, 0x0b, 0x26, 0x1c, 0x1c}, 7},
    {0x6a, {0x15, 0x00}, 2}, 
    
    {0x6E,{0x08,0x02,0x1a, 0x00, 0x12, 0x12, 0x11, 0x11, 0x14, 0x14, 0x13, 0x13,0x04,0x19, 0x1e, 0x1d,0x1d, 0x1e, 0x19, 0x04,0x0b, 0x0b, 0x0c, 0x0c, 
           0x09, 0x09, 0x0a,0x0a, 0x00, 0x1a,0x01, 0x07},32},    
    {0x6c, {0xcc, 0x0c, 0xcc, 0x84, 0xcc, 0x04, 0x50}, 7},
    {0x7d, {0x72}, 1},
    {0x70, {0x02, 0x03, 0x09, 0x07, 0x09, 0x03, 0x09, 0x07, 0x09, 0x03}, 10},

    {0x90, {0x06, 0x06, 0x05, 0x06}, 4},    
    {0x93, {0x45, 0xff, 0x00}, 3},
    
    {0xc3, {0x15}, 1},    
	{0xc4, {0x36}, 1},
	{0xc9, {0x3d}, 1},
 
    {0xF0, {0x47, 0x07, 0x0a, 0x0a, 0x00, 0x29}, 6},    
    {0xF2, {0x47, 0x07, 0x0a, 0x0a, 0x00, 0x29}, 6},
    
    {0xF1, {0x42, 0x91, 0x10, 0x2d, 0x2f, 0x6f}, 6},
    {0xF3, {0x42, 0x91, 0x10, 0x2d, 0x2f, 0x6f}, 6},
    
    {0xf9, {0x30}, 1},
    {0xbe, {0x11}, 1},
    
    {0xfb, {0x00, 0x00}, 2},

   /* {0xb1, {0x08}, 1},*/
    
    {0x11, {0}, 0},
    {0x29, {0}, 0},
        
    {0x2c, {0x00, 0x00, 0x00, 0x00}, 4},  
    
    {0x2c, {0x00, 0x00, 0x00, 0x00}, 4},  
    {0, {0}, 0xff},
    
};


static esp_err_t panel_gc9b71_init(esp_lcd_panel_t *panel)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9b71->io;

    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
  //  esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
  //  vTaskDelay(pdMS_TO_TICKS(100));
  //  esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
 //       gc9b71->madctl_val,
 //   }, 1);
 //   esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, (uint8_t[]) {
 //       gc9b71->colmod_cal,
 //   }, 1);

    // vendor specific initialization, it can be different between manufacturers
    // should consult the LCD supplier for initialization sequence code
    int cmd = 0;
   while (vendor_specific_init[cmd].data_bytes != 0xff) {
        esp_lcd_panel_io_tx_param(io, vendor_specific_init[cmd].cmd, vendor_specific_init[cmd].data, vendor_specific_init[cmd].data_bytes & 0x3F);
	if((vendor_specific_init[cmd].cmd == 0x11)||(vendor_specific_init[cmd].cmd == 0x29))		
    vTaskDelay(pdMS_TO_TICKS(120));

	cmd++;
    }
    ESP_LOGI(TAG, " panel_gc9b71_init");
    vTaskDelay(pdMS_TO_TICKS(120));

    return ESP_OK;
}

static esp_err_t panel_gc9b71_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = gc9b71->io;

    x_start += gc9b71->x_gap;
    x_end += gc9b71->x_gap;
    y_start += gc9b71->y_gap;
    y_end += gc9b71->y_gap;

    // define an area of frame memory where MCU can access
    esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4);
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * gc9b71->bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_gc9b71_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9b71->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}

static esp_err_t panel_gc9b71_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9b71->io;
    if (mirror_x) {
        gc9b71->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        gc9b71->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        gc9b71->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        gc9b71->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        gc9b71->madctl_val
    }, 1);
    return ESP_OK;
}

static esp_err_t panel_gc9b71_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9b71->io;
    if (swap_axes) {
        gc9b71->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        gc9b71->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        gc9b71->madctl_val
    }, 1);
    return ESP_OK;
}

static esp_err_t panel_gc9b71_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    gc9b71->x_gap = x_gap;
    gc9b71->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_gc9b71_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    gc9b71_panel_t *gc9b71 = __containerof(panel, gc9b71_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9b71->io;
    int command = 0;

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    on_off = !on_off;
#endif

    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}

