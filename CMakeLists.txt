cmake_minimum_required(VERSION 3.20)

project(QMcu VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CPM)

find_package(lldb REQUIRED)
find_package(stlink REQUIRED)
find_package(Qt6 COMPONENTS Core Quick Qml Graphs ShaderTools REQUIRED)

CPMAddPackage("gh:Neargye/magic_enum@0.9.7")
CPMAddPackage(
  NAME            fmt
  GIT_REPOSITORY  https://github.com/fmtlib/fmt
  GIT_TAG         12.0.0
  OPTIONS
    CMAKE_POSITION_INDEPENDENT_CODE ON
)
CPMAddPackage(
  NAME            glm
  GIT_REPOSITORY  https://github.com/g-truc/glm
  GIT_TAG         1.0.2
  OPTIONS
    CMAKE_POSITION_INDEPENDENT_CODE ON
)

set(QML_IMPORT_PATH ${CMAKE_CURRENT_BINARY_DIR}/qml)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_QML_GENERATE_QMLLS_INI ON) # Generate QML language server infos

qt_policy(SET QTP0001 NEW) # ':/qt/qml/' is the default resource prefix for QML modules.
qt_policy(SET QTP0004 NEW) # Extra directories with QML files in a QML module need extra qmldir files.

qt_standard_project_setup()

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

