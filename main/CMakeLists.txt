
file(GLOB_RECURSE SOURCES ./*.cpp ./*.c
                            )

idf_component_register(SRCS ${SOURCES}
                     INCLUDE_DIRS "" ./opencv ./bsp
                )
  

if(NOT CONFIG_SOC_SDMMC_HOST_SUPPORTED)
    fail_at_build_time(sdmmc ""
                             "SDMMC host is not supported."
                             "Please refer README.md for more details")
endif()


spiffs_create_partition_image(avi ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs FLASH_IN_PROJECT)

# OpenCV的静态库
# add_prebuilt_library(opencv_imgcodecs "opencv/libopencv_imgcodecs.a")
# add_prebuilt_library(libpng "opencv/3rdparty/liblibpng.a")
# add_prebuilt_library(libzlib "opencv/3rdparty/libzlib.a")
# add_prebuilt_library(opencv_imgproc "opencv/libopencv_imgproc.a")
# add_prebuilt_library(opencv_core "opencv/libopencv_core.a")
# add_prebuilt_library(ade "opencv/libade.a")

# set(IMGCODEC_DEP libpng libzlib)
# target_link_libraries(opencv_imgcodecs INTERFACE ${IMGCODEC_DEP})

# set(CORE_DEP libzlib)
# target_link_libraries(opencv_core INTERFACE ${CORE_DEP})

# set(OPENCV_DEP opencv_imgcodecs opencv_imgproc opencv_core)
# target_link_libraries(${COMPONENT_LIB} ${OPENCV_DEP})
 