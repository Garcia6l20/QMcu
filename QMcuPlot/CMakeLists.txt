set(CMAKE_AUTORCC ON)

set(SRC
  src/AbstractPlotDataProvider.cpp
  src/AbstractPlotSeries.cpp
  src/BasicRenderer.cpp
  src/LinePlotSeries.cpp
  src/Plot.cpp
  src/PlotRenderer.cpp
  src/PlotGrid.cpp
  src/Logging.cpp
  src/CurveInterpolator.cpp
  src/FileIO.cpp
)

set(PUBLIC_HEADERS
  include/QMcu/Plot/AbstractPlotDataProvider.hpp
  include/QMcu/Plot/PlotContext.hpp
  include/QMcu/Plot/AbstractPlotSeries.hpp
  include/QMcu/Plot/BasicRenderer.hpp
  include/QMcu/Plot/LinePlotSeries.hpp
  include/QMcu/Plot/Plot.hpp
  include/QMcu/Plot/PlotRenderer.hpp
  include/QMcu/Plot/PlotGrid.hpp
  include/QMcu/Plot/CurveInterpolator.hpp
  include/QMcu/Plot/FileIO.hpp
)

qt_add_library(QMcuPlot SHARED
  ${SRC}
  ${PUBLIC_HEADERS}
  resources/resources.qrc
  MANUAL_FINALIZATION
)

add_library(QMcu::Plot ALIAS QMcuPlot)
set_target_properties(QMcuPlot PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
target_compile_features(QMcuPlot PUBLIC cxx_std_23)

target_include_directories(QMcuPlot
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/private>
    # MOC does not use this path
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/QMcu/Plot>
)
target_link_libraries(QMcuPlot
  PUBLIC
    Qt6::Quick Qt6::Qml Qt6::Charts
  PRIVATE
    fmt::fmt
    glm::glm
    GLU
)

qt_add_qml_module(QMcuPlot
  PLUGIN_TARGET QMcuPlotPlugin
  URI QMcuPlot
  VERSION 1.0
  QML_FILES
    src/qml/PlotView.qml
    src/qml/PlotZoom.qml
    src/qml/PlayButton.qml
  RESOURCE_PREFIX /qmcu/plot
  OUTPUT_DIRECTORY ${QML_IMPORT_PATH}/QMcuPlot
)

qt_finalize_target(QMcuPlot)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()


install(TARGETS QMcuPlot
    EXPORT QMcuTargets
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QMcu/Plot
)

install(DIRECTORY
  "${QML_IMPORT_PATH}/QMcuPlot"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/qt6/qml"
)
