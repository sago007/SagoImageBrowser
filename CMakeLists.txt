cmake_minimum_required(VERSION 3.21)
project(SagoImageBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIV2 REQUIRED exiv2)

qt_add_executable(SagoImageBrowser
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/imagemodel.cpp
    src/imagemodel.h
    src/fsdirmodel.cpp
    src/fsdirmodel.h
    src/thumbnailworker.cpp
    src/thumbnailworker.h
    src/thumbnailcache.cpp
    src/thumbnailcache.h
    src/imageviewwidget.cpp
    src/imageviewwidget.h
    src/preferencesdialog.cpp
    src/preferencesdialog.h
    src/exifreader.cpp
    src/exifreader.h)

target_include_directories(SagoImageBrowser PRIVATE ${EXIV2_INCLUDE_DIRS})
target_link_libraries(SagoImageBrowser PRIVATE Qt6::Widgets ${EXIV2_LIBRARIES})