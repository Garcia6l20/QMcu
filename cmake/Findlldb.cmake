include(FindPackageHandleStandardArgs)

find_path(
    lldb_INCLUDE_DIR NAMES lldb/lldb-forward.h
    HINTS /usr /usr/local /opt
    # PATH_SUFFIXES lldb
    )
set(lldb_NAME lldb)
find_library(
    lldb_LIBRARY NAMES ${lldb_NAME}
    HINTS /usr /usr/local /opt
    )
find_package_handle_standard_args(lldb DEFAULT_MSG lldb_LIBRARY lldb_INCLUDE_DIR)
mark_as_advanced(lldb_INCLUDE_DIR lldb_LIBRARY)

if (NOT lldb_FOUND)
    message(FATAL_ERROR "lldb library not found on your system!")
endif ()

message(STATUS "lldb include dir: ${lldb_INCLUDE_DIR}")
message(STATUS "lldb library: ${lldb_LIBRARY}")

add_library(lldb SHARED IMPORTED GLOBAL)
set_target_properties(lldb PROPERTIES
    IMPORTED_LOCATION "${lldb_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${lldb_INCLUDE_DIR}"
)
