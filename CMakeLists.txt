cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME lingmo-dock)
project(${PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find ECM Modules
find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
message(STATUS "ECM version: ${ECM_VERSION}")

include(FindXCB)

find_package(Qt6 CONFIG REQUIRED Widgets DBus Gui Concurrent LinguistTools QuickControls2)

find_package(KF6WindowSystem REQUIRED)

find_package(KWayland REQUIRED)

# Clear the list of source files
set(sources_files "")
set(qml_files "")
set(resource_files "")

#遍历所有Cpp文件
file(GLOB_RECURSE CPP_FILES src/*.cpp src/*.h)
foreach (filepath ${CPP_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND sources_files ${filename})
endforeach (filepath)

set(SINGLETON_FILES "")
#遍历所有qml文件
file(GLOB_RECURSE QML_PATHS qml/*.qml qmldir)
foreach (filepath ${QML_PATHS})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    if (${filename} MATCHES "qmldir")
        list(APPEND resource_files ${filename})
    else ()
        list(APPEND qml_files ${filename})
        # Check for singleton
        foreach (singletion_name ${SINGLETON_FILES})
            if (${filename} MATCHES ${singletion_name})
                set_source_files_properties(${filename} PROPERTIES QT_QML_SINGLETON_TYPE true)
            endif ()
        endforeach (singletion_name)
    endif ()
endforeach (filepath)

#遍历所有资源文件
set(RES_SUFFIX "")
set(RES_PATHS "")
list(APPEND RES_SUFFIX *.png *.jpg *.svg *.ico *.ttf *.webp *.js *.qm *.qsb)
list(APPEND RES_PATHS qml images src)
foreach (respath ${RES_PATHS})
    foreach (filesuffix ${RES_SUFFIX})
        file(GLOB_RECURSE RES_FOUND ${respath}/${filesuffix})
        foreach (filepath ${RES_FOUND})
            string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
            list(APPEND resource_files ${filename})
        endforeach()
    endforeach ()
endforeach (respath)


qt_add_dbus_adaptor(DBUS_SOURCES
                     src/com.lingmo.Dock.xml
                     src/mainwindow.h MainWindow)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

qt_add_executable(${PROJECT_NAME} ${sources_files} ${DBUS_SOURCES})

qt_add_resources(${PROJECT_NAME} "resources"
    PREFIX "/"
    FILES ${resource_files} ${qml_files}
)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${XCB_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::GuiPrivate
        Qt6::Concurrent
        Qt6::DBus
        KF6::WindowSystem
        Plasma::KWaylandClient
        ${XCB_LIBRARIES}
)

# file(GLOB TS_FILES translations/*.ts)
# foreach(filepath ${TS_FILES})
#     string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
#     list(APPEND ts_files_replaced ${filename})
# endforeach()
# qt_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${ts_files_replaced})

# add_custom_target(translations ALL DEPENDS ${QM_FILES} SOURCES ${TS_FILES})

# install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# install(FILES ${QM_FILES} DESTINATION /usr/share/${PROJECT_NAME}/translations)
# install(FILES lingmo-dock-list.conf DESTINATION /etc)
