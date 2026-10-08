# settings_reset has no Bluetooth profile table, so it must not compile these behaviors.
if(CONFIG_BT_MAX_PAIRED)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/boards/shields/totem/behaviors/os_mode.c
    ${CMAKE_CURRENT_LIST_DIR}/boards/shields/totem/behaviors/smart_modifier.c
    ${CMAKE_CURRENT_LIST_DIR}/boards/shields/totem/behaviors/smart_shift.c
  )
endif()
