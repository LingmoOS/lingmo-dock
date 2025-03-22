cmake_minimum_required(VERSION 3.18)

set(PROJECT_NAME lingmo-dock)
project(${PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 CONFIG REQUIRED Widgets DBus Gui Concurrent LinguistTools QuickControls2)

find_package(KF6WindowSystem REQUIRED)

find_package(LingmoLogger REQUIRED)

add_subdirectory(src)

file(GLOB TS_FILES translations/*.ts)
foreach(filepath ${TS_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND ts_files_replaced ${filename})
endforeach()
qt_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${ts_files_replaced})

add_custom_target(translations ALL DEPENDS ${QM_FILES} SOURCES ${TS_FILES})

install(FILES ${QM_FILES} DESTINATION /usr/share/${PROJECT_NAME}/translations)
install(FILES lingmo-dock-list.conf DESTINATION /etc)
