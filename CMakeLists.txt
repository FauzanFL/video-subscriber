cmake_minimum_required(VERSION 3.16)

project(StreamSubscriber VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia)
find_package(CycloneDDS-CXX REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

idlcxx_generate(
    TARGET generated
    FILES idl/VideoMessage.idl
)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appStreamSubscriber
    main.cpp
)

qt_add_qml_module(appStreamSubscriber
    URI StreamSubscriber
    QML_FILES
        Main.qml
        SOURCES qos_profiles.h
        SOURCES ddssubscriber.h ddssubscriber.cpp
        SOURCES videodecoder.h videodecoder.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appStreamSubscriber PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appStreamSubscriber
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appStreamSubscriber PRIVATE ${GST_INCLUDE_DIRS})

target_link_libraries(appStreamSubscriber
    PRIVATE
    Qt6::Quick
    Qt6::Multimedia
    CycloneDDS-CXX::ddscxx
    generated
    ${GST_LIBRARIES}
)

install(TARGETS appStreamSubscriber
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
