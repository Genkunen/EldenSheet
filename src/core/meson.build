incs += include_directories('.')

c_srcs = files(
    'context.c',
    'display.c',
    'graphics.c',
    'window.c',
)

libs += static_library(
    'core', 
    c_srcs, 
    include_directories: incs,
    dependencies: deps,
)
