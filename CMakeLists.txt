file(GLOB_RECURSE srcs "main.c" "src/*.c" "src/drivers*.c")

idf_component_register(SRCS "${srcs}"
                    PRIV_REQUIRES bt driver nvs_flash esp_driver_gpio esp_adc esp_timer
                    INCLUDE_DIRS "./include" "./include/drivers")