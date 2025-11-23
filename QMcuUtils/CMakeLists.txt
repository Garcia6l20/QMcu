
set(SRC
  src/LogInterceptor.cpp
)

set(PUBLIC_HEADERS
  include/QMcu/Utils/LogInterceptor.hpp
)

qt_add_library(QMcuUtils SHARED
  ${SRC}
  ${PUBLIC_HEADERS}
)

add_library(QMcu::Utils ALIAS QMcuUtils)
set_target_properties(QMcuUtils PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
target_compile_features(QMcuUtils PUBLIC cxx_std_23)

target_include_directories(QMcuUtils
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/private>
    # MOC does not use this path
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/QMcu/Utils>
)
target_link_libraries(QMcuUtils
  PUBLIC
    Qt6::Quick
)

qt_add_resources(QMcuUtils "icons"
    PREFIX "/qmcu/utils"
    BASE resources
    FILES 
      resources/icons/circle-exclamation-solid.svg
      resources/icons/circle-info-solid.svg
      resources/icons/circle-xmark-solid.svg
      resources/icons/comment-solid.svg
      resources/icons/lightbulb-solid.svg
)

qt_add_qml_module(QMcuUtils
  PLUGIN_TARGET QMcuUtilsPlugin
  URI QMcuUtils
  VERSION 1.0
  QML_FILES
    src/qml/LoggingErrorDialog.qml
  RESOURCE_PREFIX /qmcu/utils
  OUTPUT_DIRECTORY ${QML_IMPORT_PATH}/QMcuUtils
)


install(TARGETS QMcuUtils
  EXPORT QMcuTargets
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QMcu/Utils
)

install(DIRECTORY
  "${QML_IMPORT_PATH}/QMcuUtils"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml"
)
