incs += include_directories('.')

core_srcs = files(
    'context.c',
    'display.c',
    'graphics.c',
    'imgui_layer.cpp',
    'renderer.c',
    'window.c',
)

libs += static_library(
    'core', 
    core_srcs, 
    include_directories: incs,
    dependencies: deps,
)
