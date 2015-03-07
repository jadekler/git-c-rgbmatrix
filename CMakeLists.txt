cmake_minimum_required(VERSION 2.8.4)
project(git_c_rgbmatrix)

include_directories(include)

set(HEADER_FILES include/canvas.h include/gpio.h include/graphics.h include/led-matrix.h include/thread.h include/threaded-canvas-manipulator.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    adapter/active/active-rpi-hub75-adapter.kicad_pcb
    adapter/active/active-rpi-hub75-adapter.pro
    adapter/active/active-rpi-hub75-adapter.sch
    adapter/active-3/active3-rpi-hub75-adapter.kicad_pcb
    adapter/active-3/active3-rpi-hub75-adapter.pro
    adapter/active-3/active3-rpi-hub75-adapter.sch
    adapter/passive/passive-rpi-hub75-adapter.kicad_pcb
    adapter/passive/passive-rpi-hub75-adapter.pro
    adapter/passive/passive-rpi-hub75-adapter.sch
    include/canvas.h
    include/gpio.h
    include/graphics.h
    include/led-matrix.h
    include/thread.h
    include/threaded-canvas-manipulator.h
    lib/bdf-font.cc
    lib/framebuffer-internal.h
    lib/framebuffer.cc
    lib/gpio.cc
    lib/graphics.cc
    lib/led-matrix.cc
    lib/thread.cc
    lib/utf8-internal.h
    demo-main.cc
    boom.cc
    led-image-viewer.cc
    minimal-example.cc
    text-example.cc)

add_executable(git_c_rgbmatrix ${SOURCE_FILES})