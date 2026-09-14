idf_component_register(
    SRCS "main.c"
         "stopwatch_core.c"
         "demo_eva_stopwatch.c"
    INCLUDE_DIRS "."
    REQUIRES bsp lvgl esp_timer
)
