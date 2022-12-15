find_package(Qt5 REQUIRED COMPONENTS Gui OpenGL Widgets Xml)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_library(QGLVIEWER_PATH NAMES QGLViewer-qt5)
find_path(QGLVIEWER_INCLUDE_DIR NAMES QGLViewer/config.h)

add_library(QGLViewer IMPORTED UNKNOWN)
target_include_directories(QGLViewer INTERFACE ${QGLVIEWER_INCLUDE_DIR})
set_target_properties(QGLViewer PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION ${QGLVIEWER_PATH}
)
target_link_libraries(QGLViewer INTERFACE GL Qt5::Gui Qt5::OpenGL Qt5::Widgets Qt5::Xml)
