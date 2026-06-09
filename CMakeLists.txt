cmake_minimum_required(VERSION 3.20)
project(SonicFlowPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/assets/SonicFlowPlayer.icns")
set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

qt_add_executable(SonicFlowPlayer
    MACOSX_BUNDLE
    main.cpp
    MainWindow.h MainWindow.cpp
    PlayerController.h PlayerController.cpp
    PlaylistManager.h PlaylistManager.cpp
    MetadataManager.h MetadataManager.cpp
    resources.qrc
    ${APP_ICON_MACOS}
)

target_link_libraries(SonicFlowPlayer PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)

set_target_properties(SonicFlowPlayer PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.waqas.sonicflowplayer"
    MACOSX_BUNDLE_BUNDLE_NAME "SonicFlow Player"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "SonicFlowPlayer.icns"
)
