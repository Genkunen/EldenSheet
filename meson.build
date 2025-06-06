project('EldenSheet', 
    'c','cpp', 
    meson_version: '>=0.51.0',
    default_options: [
        'cpp_std=c++14', 
        'c_std=c11',
        'warning_level=3',
    ],
)

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

cc_warnings = [
    '-Wpedantic',
]

add_project_arguments(cc.get_supported_arguments(cc_warnings), language: 'c')

conf = configuration_data()

conf.set10('ERROR_CALLBACK', get_option('error-callback'))
conf.set10('VERBOSE', get_option('verbose'))

incs = [include_directories('libs/glfw/include')]
libs = []
deps = [dependency('vulkan')]

deps += cc.find_library(
    'glfw3_mt',
    dirs: [meson.current_source_dir() / 'libs/glfw'],
    static: true
)

app_srcs = []

# creates static library imgui
subdir('libs/imgui')
# creates static library core
subdir('src/core')
# gathers app source files
subdir('src')

configure_file(input: 'config.in', output: 'config.h', configuration: conf)

executable(
    meson.project_name(), 
    app_srcs, 
    dependencies: deps,
    link_with: libs,
    include_directories: incs,
)
