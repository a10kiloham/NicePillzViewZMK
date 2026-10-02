target_sources(app PRIVATE leds.c)
target_sources_ifdef(CONFIG_NICEPILLZ_STATUS_SCREEN app PRIVATE custom_status_screen.c view_draw.c)

# Stamp the firmware with a hash of the keymap, so a flash with a changed keymap discards the
# keymap stored by ZMK Studio (see keymap_rev.c).
if(CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE AND KEYMAP_FILE)
  # The shield's own keymap only includes config/nicepillz.keymap, so hash that file instead.
  get_filename_component(nicepillz_keymap_src ${KEYMAP_FILE} REALPATH)
  if(nicepillz_keymap_src STREQUAL "${CMAKE_CURRENT_LIST_DIR}/nicepillz.keymap")
    get_filename_component(nicepillz_keymap_src ${CMAKE_CURRENT_LIST_DIR}/../../../config/nicepillz.keymap REALPATH)
  endif()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${nicepillz_keymap_src})
  file(SHA256 ${nicepillz_keymap_src} nicepillz_keymap_hash)
  string(SUBSTRING ${nicepillz_keymap_hash} 0 8 nicepillz_keymap_rev)
  target_sources(app PRIVATE keymap_rev.c)
  target_compile_definitions(app PRIVATE NICEPILLZ_KEYMAP_REV=0x${nicepillz_keymap_rev}U)
endif()
