cmake_minimum_required(VERSION 3.21)
project(SagoImageBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
include(FetchContent)
# Build exiv2lib as a static library so it is linked into the executable
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(EXIV2_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(EXIV2_BUILD_EXIV2_COMMAND OFF CACHE INTERNAL "")
set(EXIV2_BUILD_UNIT_TESTS OFF CACHE INTERNAL "")
set(EXIV2_BUILD_DOC OFF CACHE INTERNAL "")
set(EXIV2_ENABLE_INIH OFF CACHE INTERNAL "")
FetchContent_Declare(
    exiv2
    GIT_REPOSITORY https://github.com/Exiv2/exiv2.git
    GIT_TAG v0.28.8
)
FetchContent_MakeAvailable(exiv2)

# Exiv2 0.28.8 generates this header in the build root but doesn't add it to include paths
target_include_directories(exiv2lib INTERFACE $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)

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
    src/shortcutmanager.cpp
    src/shortcutmanager.h
    src/exifreader.cpp
    src/exifreader.h
    src/pathcompletermodel.cpp
    src/pathcompletermodel.h
    src/thumbnaildelegate.cpp
    src/thumbnaildelegate.h)

set_source_files_properties(extra/icons/sago_image_browser.svg PROPERTIES QT_RESOURCE_ALIAS sago_image_browser.svg)
qt_add_resources(SagoImageBrowser "icons" PREFIX "/icons" FILES extra/icons/sago_image_browser.svg)

target_link_libraries(SagoImageBrowser PRIVATE Qt6::Widgets exiv2lib)

add_executable(test_exifreader test/test_exifreader.cpp src/exifreader.cpp)
target_include_directories(test_exifreader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_exifreader PRIVATE Qt6::Widgets exiv2lib)

qt_add_translations(SagoImageBrowser
    TS_FILES translations/da.ts
    RESOURCE_PREFIX "/translations")

install(TARGETS SagoImageBrowser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES extra/sago-image-browser.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES extra/icons/sago_image_browser.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)