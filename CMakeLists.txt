cmake_minimum_required(VERSION 3.16)

project(opcvtest VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(OpenCV REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appopcvtest
    main.cpp
)

qt_add_qml_module(appopcvtest
    URI opcvtest
    QML_FILES
        Main.qml
        SOURCES cvitem.h cvitem.cpp
        SOURCES camerathread.h camerathread.cpp
)

set_target_properties(appopcvtest PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 👈 修改点 2：把 OpenCV 的库 (${OpenCV_LIBS}) 链接到你的程序 (appopcvtest) 上
target_link_libraries(appopcvtest
    PRIVATE Qt6::Quick ${OpenCV_LIBS}
)

include(GNUInstallDirs)
install(TARGETS appopcvtest
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
