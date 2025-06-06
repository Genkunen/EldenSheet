incs += include_directories('.')

imgui_srcs = files(
    'imgui.cpp',
    'imgui_draw.cpp',
    'imgui_impl_glfw.cpp',
    'imgui_tables.cpp',
    'imgui_widgets.cpp',
)

libs += static_library(
    'imgui', 
    imgui_srcs, 
    dependencies: deps,
    include_directories: incs
)
