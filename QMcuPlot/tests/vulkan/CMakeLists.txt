
qt_add_executable(vulkanunderqml WIN32 MACOSX_BUNDLE
    main.cpp
    vulkansquircle.cpp vulkansquircle.h
    vulkansquirclerenderer.cpp vulkansquirclerenderer.h
)

set_target_properties(vulkanunderqml PROPERTIES
    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME vulkanunderqmlapp
)

target_link_libraries(vulkanunderqml PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    QMcu::Plot
    glm::glm
    vulkan
)

qt_add_qml_module(vulkanunderqml
    URI VulkanUnderQML
    QML_FILES
        main.qml
    RESOURCES
        squircle.frag.spv
        squircle.vert.spv
    RESOURCE_PREFIX /scenegraph/vulkanunderqml
    NO_RESOURCE_TARGET_PATH
)
