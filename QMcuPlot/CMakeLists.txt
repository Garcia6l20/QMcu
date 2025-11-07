set(CMAKE_AUTORCC ON)

find_package(Vulkan)

macro(vulkan_compile_shaders target)

  set(options)
  set(singleValueArgs OUTPUT_VAR)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(ARGS "${options}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
  
  set(_outputs)
  foreach(shader ${ARGS_SOURCES})
    get_filename_component(name ${shader} NAME)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/shaders/${name}.spv)
    list(APPEND _outputs ${output})
    add_custom_command(OUTPUT ${output}
      COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${shader} -o ${output}
      DEPENDS ${shader}
      COMMENT "Compiling ${shader}...")
    set_source_files_properties(${output} PROPERTIES QT_RESOURCE_ALIAS shaders/${name}.spv)
  endforeach()
  
  add_custom_target(${target} ALL DEPENDS ${_outputs})

  set(${ARGS_OUTPUT_VAR} ${_outputs})
endmacro()

include(PythonUtils)

set(LINE_PLOT_SERIES_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-float.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-double.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-i8.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-u8.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-i16.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-u16.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-i32.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series-u32.vert
)

add_python_generator(qmcu-plot-shaders-gen
  OUTPUT ${LINE_PLOT_SERIES_SHADERS}
  SCRIPT generate_shaders.py
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/templates/shaders/line-plot-series.vert.jinja2
)

vulkan_compile_shaders(qmcu-plot-shaders
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/forward-xy.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/set-color.frag

    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/grid.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/grid.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/grid.frag

    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/stencil.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/stencil.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/stencil.frag

    ${LINE_PLOT_SERIES_SHADERS}

    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series.frag
    # ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series_halo.geom
    # ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line-plot-series_halo.frag

  OUTPUT_VAR COMPILED_SHADERS
)

set(SRC
  src/AbstractPlotDataProvider.cpp
  src/AbstractPlotSeries.cpp
  src/PlotLineSeries.cpp
  src/Plot.cpp
  src/PlotScene.cpp
  src/PlotSceneItem.cpp
  src/PlotGrid.cpp
  src/Logging.cpp
  src/CurveInterpolator.cpp
  src/FileIO.cpp
)

set(PUBLIC_HEADERS
  include/QMcu/Plot/AbstractPlotDataProvider.hpp
  include/QMcu/Plot/PlotContext.hpp
  include/QMcu/Plot/AbstractPlotSeries.hpp
  include/QMcu/Plot/PlotLineSeries.hpp
  include/QMcu/Plot/Plot.hpp
  include/QMcu/Plot/PlotScene.hpp
  include/QMcu/Plot/PlotSceneItem.hpp
  include/QMcu/Plot/PlotGrid.hpp
  include/QMcu/Plot/CurveInterpolator.hpp
  include/QMcu/Plot/FileIO.hpp
)

qt_add_library(QMcuPlot SHARED
  ${SRC}
  ${PUBLIC_HEADERS}
  MANUAL_FINALIZATION
)

qt_add_resources(QMcuPlot "images"
    PREFIX "/qmcu/plot"
    BASE resources
    FILES 
      resources/images/pause.svg
      resources/images/play.svg
      resources/images/stop.svg
)

qt_add_resources(QMcuPlot "shaders"
    PREFIX "/qmcu/plot"
    FILES ${COMPILED_SHADERS}
)

qt_add_shaders(QMcuPlot "qt-shaders"
    PREFIX "/qmcu/plot/qt-shaders"
    FILES 
        "shaders/qt/opacitymask.frag"
    OUTPUTS
        "opacitymask.frag.qsb"
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
    vulkan
    Qt6::GuiPrivate
    magic_enum::magic_enum
)
add_dependencies(QMcuPlot qmcu-plot-shaders)

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
