cmake_minimum_required(VERSION 3.21)
project(ImageBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(ImageBrowser
    main.cpp
    mainwindow.cpp
    mainwindow.h
    imagemodel.cpp
    imagemodel.h
)

target_link_libraries(ImageBrowser PRIVATE Qt6::Widgets)