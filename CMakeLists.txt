cmake_minimum_required(VERSION 3.20)

project(QMcu VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CPM)

find_package(lldb REQUIRED)
find_package(stlink REQUIRED)
find_package(Qt6 COMPONENTS Core Quick Qml Graphs ShaderTools REQUIRED)
find_package(glm REQUIRED)

CPMAddPackage(
  NAME      magic_enum
  URL       https://github.com/Neargye/magic_enum/releases/download/v0.9.7/magic_enum-v0.9.7.tar.gz
  URL_HASH  SHA256=c047bc7ca0b76752168140e7ae9a4a30d72bf6530c196fdfbf5105a39d40cc46
)

set(QML_IMPORT_PATH ${CMAKE_CURRENT_BINARY_DIR}/qml)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_QML_GENERATE_QMLLS_INI ON) # Generate QML language server infos

qt_policy(SET QTP0001 NEW) # ':/qt/qml/' is the default resource prefix for QML modules.
qt_policy(SET QTP0004 NEW) # Extra directories with QML files in a QML module need extra qmldir files.

qt_standard_project_setup()

option(QMCU_FORCE_QML_DEV_PLUGINS "Enforce using qml devolvement plugins" OFF)

add_subdirectory(QMcuUtils)
add_subdirectory(QMcuPlot)
add_subdirectory(QMcuDebug)
add_subdirectory(QMcuWatch)


# install(TARGETS QMcuPlot QMcuDebug
#   EXPORT QMcuTargets
# )

install(EXPORT QMcuTargets
    FILE QMcuTargets.cmake
    NAMESPACE QMcu::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QMcu
)

install(DIRECTORY
  "${QML_IMPORT_PATH}/QMcu"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml"
)


# Generate config file for find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QMcuConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/QMcuConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QMcuConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QMcu"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QMcuConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QMcuConfigVersion.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findlibusb.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findlldb.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findstlink.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QMcu"
)



if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake)
endif()


if(NOT WIN32)
  # Enable .tar.gz and .zip
  list(APPEND CPACK_GENERATOR "STGZ;DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sylvain Garcia (TPL Systems)")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base, qt6-declarative, qt6-graphs, lldb, stlink")
else()
  set(APPEND CPACK_GENERATOR "NSIS")
endif()

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})

set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_PACKAGE_CONTACT "garcia.6l20@gmail.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README  "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)

