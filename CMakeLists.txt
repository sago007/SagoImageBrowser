cmake_minimum_required(VERSION 3.21)
project(SagoImageBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(SagoImageBrowser
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/imagemodel.cpp
    src/imagemodel.h
    src/thumbnailworker.cpp
    src/thumbnailworker.h
    src/imageviewwidget.cpp
    src/imageviewwidget.h    src/preferencesdialog.cpp
    src/preferencesdialog.h)

target_link_libraries(SagoImageBrowser PRIVATE Qt6::Widgets)