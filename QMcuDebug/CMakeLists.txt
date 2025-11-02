set(SRC 
  src/Debugger.cpp
  src/StLinkProbe.cpp
  src/Variable.cpp
  src/Type.cpp
  src/VariableProxy.cpp
  src/VariableProxyGroup.cpp
  src/AbstractVariableRecorder.cpp
  src/ScrollRecorder.cpp
  src/AbstractVariablePlotDataProvider.cpp
  src/ScrollPlotProvider.cpp
  src/BufferRecorder.cpp
  src/AutoScale.cpp
)
set(PUBLIC_HEADERS 
  include/QMcu/Debug/Debugger.hpp
  include/QMcu/Debug/StLinkProbe.hpp
  include/QMcu/Debug/Variable.hpp
  include/QMcu/Debug/Type.hpp
  include/QMcu/Debug/VariableProxy.hpp
  include/QMcu/Debug/VariableProxyGroup.hpp
  include/QMcu/Debug/AbstractVariableRecorder.hpp
  include/QMcu/Debug/ScrollRecorder.hpp
  include/QMcu/Debug/AbstractVariablePlotDataProvider.hpp
  include/QMcu/Debug/ScrollPlotProvider.hpp
  include/QMcu/Debug/BufferRecorder.hpp
  include/QMcu/Debug/AutoScale.hpp
)
add_library(QMcuDebug SHARED ${SRC} ${PUBLIC_HEADERS})
add_library(QMcu::Debug ALIAS QMcuDebug)
set_target_properties(QMcuDebug PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
target_include_directories(QMcuDebug
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/private>
    # MOC does not use this path
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/QMcu/Debug>
)
target_link_libraries(QMcuDebug
  PUBLIC
    Qt6::Quick Qt6::Qml Qt6::Charts
    QMcuPlot
    lldb
  PRIVATE
    stlink
    magic_enum::magic_enum
)

qt_add_qml_module(QMcuDebug
  PLUGIN_TARGET QMcuDebugPlugin
  URI QMcuDebug
  VERSION 1.0
  QML_FILES
    src/qml/RubberBand.qml
    src/qml/VariableComboBox.qml
    src/qml/ScrollRecorderSeries.qml
    src/qml/BufferRecorderSeries.qml
    src/qml/SimplePlot.qml
  OUTPUT_DIRECTORY ${QML_IMPORT_PATH}/QMcuDebug
)

qt_finalize_target(QMcuDebug)

install(TARGETS QMcuDebug
  EXPORT QMcuTargets
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QMcu/Debug
)

install(DIRECTORY
  "${QML_IMPORT_PATH}/QMcuDebug"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml"
)
