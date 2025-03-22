qt_standard_project_setup()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Clear the list of source files
set(sources_files "")
set(qml_files "")
set(resource_files "")

#遍历所有Cpp文件
file(GLOB_RECURSE CPP_FILES *.cpp)
foreach (filepath ${CPP_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND sources_files ${filename})
endforeach (filepath)

#遍历所有h文件
file(GLOB_RECURSE H_FILES *.h)
foreach (filepath ${H_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND sources_files ${filename})
endforeach (filepath)

set(SINGLETON_FILES "")
#遍历所有qml文件
file(GLOB_RECURSE QML_PATHS *.qml qmldir)
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
list(APPEND RES_PATHS images)
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
                     com.lingmo.Dock.xml
                     view/mainwindow.h MainWindow)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

add_executable(${PROJECT_NAME} ${sources_files} ${DBUS_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::GuiPrivate
        Qt6::Concurrent
        Qt6::DBus
        KF6::WindowSystem
        LingmoLogger
)

qt_add_resources(${PROJECT_NAME} "${PROJECT_NAME}_res"
    PREFIX "/"
    FILES ${resource_files}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
