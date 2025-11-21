qt_add_executable(QMcuWatch
  src/main.cpp
  src/WatchConfig.cpp
  include/WatchConfig.hpp

  MANUAL_FINALIZATION
)

qt_add_qml_module(QMcuWatch
  URI QMcuWatch
  VERSION 1.0
  QML_FILES
    qml/simple-watch.qml
    qml/configured-watch.qml
  OUTPUT_DIRECTORY ${QML_IMPORT_PATH}/QMcuWatch
)

set(QT_LIBS
  Qt6::Core
  Qt6::Gui
)

target_include_directories(QMcuWatch PRIVATE include src)
target_link_libraries(QMcuWatch PRIVATE
  QMcuDebug
  ${QT_LIBS}
)

add_executable(QMcuWatchCounterExample test/counter.cpp)
add_dependencies(QMcuWatch QMcuWatchCounterExample)

qt_finalize_target(QMcuWatch)

install(TARGETS QMcuWatch QMcuWatchCounterExample
  BUNDLE  DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (QMCU_FORCE_QML_DEV_PLUGINS)
  message(STATUS "QMcuWatch: devolvement plugins enforced")
  target_compile_definitions(QMcuWatch PRIVATE QMCU_FORCE_QML_DEV_PLUGINS="${CMAKE_BINARY_DIR}")
endif()

option(QMCUWATCH_FULL_DEPLOY "Use full reusable deployment script (take care this will install a lot of stuff)" OFF)
if(QMCUWATCH_FULL_DEPLOY)
  qt_generate_deploy_qml_app_script(
    TARGET QMcuWatch
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
  )
  install(SCRIPT ${deploy_script})
endif()

install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/CounterExample.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/test/CounterBuffersExample.qml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/QMcuWatch)
