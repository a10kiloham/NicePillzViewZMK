target_sources(app PRIVATE leds.c)
target_sources_ifdef(CONFIG_NICEPILLZ_STATUS_SCREEN app PRIVATE custom_status_screen.c view_draw.c)
