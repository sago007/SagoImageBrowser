cmake_minimum_required(VERSION 3.21)
project(SagoImageBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
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

target_include_directories(SagoImageBrowser PRIVATE ${EXIV2_INCLUDE_DIRS})
target_link_libraries(SagoImageBrowser PRIVATE Qt6::Widgets ${EXIV2_LIBRARIES})

add_executable(test_exifreader test_exifreader.cpp src/exifreader.cpp)
target_include_directories(test_exifreader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${EXIV2_INCLUDE_DIRS})
target_link_libraries(test_exifreader PRIVATE Qt6::Widgets ${EXIV2_LIBRARIES})

qt_add_translations(SagoImageBrowser
    TS_FILES translations/da.ts
    RESOURCE_PREFIX "/translations")

install(TARGETS SagoImageBrowser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES extra/sago-image-browser.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES extra/icons/sago_image_browser.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)