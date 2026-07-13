cmake_minimum_required(VERSION 3.21)
project(SagoImageBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

find_package(Iconv REQUIRED)

# Use the system-provided exiv2 (both 0.27 and 0.28 are supported)
find_package(exiv2 REQUIRED CONFIG)

# 0.28 exports Exiv2::exiv2lib; 0.27 configs may export the unnamespaced target
if(TARGET Exiv2::exiv2lib)
    set(EXIV2_TARGET Exiv2::exiv2lib)
else()
    set(EXIV2_TARGET exiv2lib)
endif()

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

target_link_libraries(SagoImageBrowser PRIVATE Qt6::Widgets ${EXIV2_TARGET})

add_executable(test_exifreader test/test_exifreader.cpp src/exifreader.cpp)
target_include_directories(test_exifreader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_exifreader PRIVATE Qt6::Widgets ${EXIV2_TARGET})

qt_add_translations(SagoImageBrowser
    TS_FILES translations/da.ts
    RESOURCE_PREFIX "/translations")

install(TARGETS SagoImageBrowser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES extra/sago-image-browser.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES extra/icons/sago_image_browser.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)