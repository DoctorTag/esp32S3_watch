set(priv_requires "esp_lcd" )

idf_component_register(SRCS "."   
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES ${priv_requires})

