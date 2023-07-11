set(priv_requires "esp_lcd" )

idf_component_register(SRCS "spi_lcd_touch_example_main.c"  "esp_lcd_panel_gc9b71.c"  "lvgl_demo_ui.c"  "esp_lcd_panel_io_qspi.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES ${priv_requires})
