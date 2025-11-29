if (NOT TARGET Qt6::GuiPrivate)
  set(QT_NO_PRIVATE_MODULE_WARNING ON)
  find_package(Qt6 COMPONENTS GuiPrivate REQUIRED)
endif()

link_libraries(QMcuPlot Qt6::GuiPrivate)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

add_library(QPlotTest
  TestCounter.hpp
  TestSignal.hpp
  rhiwindow.cpp
  rhiwindow.h
)

qt_add_qml_module(QPlotTest
  PLUGIN_TARGET QPlotTestPlugin
  URI QPlotTest
  VERSION 1.0
  QML_FILES
    SplineEditor.qml
    TestDial.qml
    TestCounterApp.qml
    TestSignalApp.qml
    TestGridApp.qml
  OUTPUT_DIRECTORY ${QML_IMPORT_PATH}/QPlotTest
)

link_libraries(QPlotTestPlugin)

macro(add_test_app target qml_main)
  
  qt_add_executable(${target}
    test-runner.cpp
  )
  target_compile_definitions(${target} PRIVATE -DQML_MAIN="${qml_main}")

  if (QMCU_FORCE_QML_DEV_PLUGINS)
    target_compile_definitions(${target} PRIVATE QMCU_FORCE_QML_DEV_PLUGINS="${CMAKE_BINARY_DIR}")
  endif()

endmacro()

add_test_app(spline-editor SplineEditor)
add_test_app(test-counter TestCounterApp)
add_test_app(test-signal TestSignalApp)
add_test_app(test-grid TestGridApp)


find_package(Qt6 REQUIRED COMPONENTS Gui Test)

add_executable(plot-test-scaling test-scaling.cpp)
target_link_libraries(plot-test-scaling PRIVATE Qt6::Gui Qt6::Test)

add_subdirectory(vulkan)