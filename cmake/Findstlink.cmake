include(FindPackageHandleStandardArgs)

find_path(
    stlink_INCLUDE_DIR NAMES stm32.h
    HINTS /usr /usr/local /opt
    PATH_SUFFIXES stlink
    )
set(stlink_NAME stlink)
find_library(
    stlink_LIBRARY NAMES ${stlink_NAME}
    HINTS /usr /usr/local /opt
    )
find_package_handle_standard_args(stlink DEFAULT_MSG stlink_LIBRARY stlink_INCLUDE_DIR)
mark_as_advanced(stlink_INCLUDE_DIR stlink_LIBRARY)

if (NOT stlink_FOUND)
    message(FATAL_ERROR "stlink library not found on your system!")
endif ()

message(STATUS "stlink library: ${stlink_LIBRARY}")


find_package(libusb REQUIRED)

add_library(stlink INTERFACE)
target_link_libraries(stlink INTERFACE ${stlink_LIBRARY} ${LIBUSB_LIBRARY})
target_include_directories(stlink INTERFACE ${stlink_INCLUDE_DIR} ${LIBUSB_INCLUDE_DIR})
