set(srcs  "usb_cam1.c" "camera_dsi_main.c" )
# set(srcs  "usb_uvc_device.c" "usb_cam1.c"  )
set(embed_files "esp_1280_720.jpg")
if(CONFIG_UVC_SUPPORT_TWO_CAM)
    list(APPEND srcs "usb_cam2.c")
endif()
# REQUIRES esp_mm esp_driver_isp esp_driver_cam esp_driver_i2c dsi_init sensor_init
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "."
                      
                       EMBED_FILES "${embed_files}"
                       )
spiffs_create_partition_image(avi ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs FLASH_IN_PROJECT)