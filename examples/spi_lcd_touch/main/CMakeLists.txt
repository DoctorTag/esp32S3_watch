set(priv_requires "esp_lcd" "bsp" )

idf_component_register(SRCS "spi_lcd_touch_example_main.c"  "lvgl_demo_ui.c"  
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES ${priv_requires})
