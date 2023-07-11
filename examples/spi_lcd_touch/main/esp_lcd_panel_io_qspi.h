/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "soc/soc_caps.h"
#include "hal/lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create LCD panel IO handle, for SPI interface
 *
 * @param[in] bus SPI bus handle
 * @param[in] io_config IO configuration, for SPI interface
 * @param[out] ret_io Returned IO handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_io_qspi(esp_lcd_spi_bus_handle_t bus, const esp_lcd_panel_io_spi_config_t *io_config, esp_lcd_panel_io_handle_t *ret_io);



#ifdef __cplusplus
}
#endif
