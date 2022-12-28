cmake_minimum_required (VERSION 3.22)
project (Animation)
include(ImportFbxSdk.cmake)
include(ImportQGLViewer.cmake)
find_package(Eigen3 REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(PROJECT_SOURCES
    MainWindow.ui
    MainWindow.cpp
    main.cpp
    Loader.cpp
    Viewer.cpp
    SafeGl.cpp
    MainWindow.h
    Loader.h
    Viewer.h
    Model.h
    Model.cpp
    SafeGl.h
    Skeleton.h
    Skeleton.cpp
    Animation.h
    Animation.cpp
    AnimationListModel.h
    AnimationListModel.cpp
    AnimationTreeModel.h
    AnimationTreeModel.cpp
)
set(PROJECT_SHADERS
    shaders/vert.glsl
    shaders/frag.glsl
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_SOURCES} ${PROJECT_SHADERS})
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_SHADERS})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
target_link_libraries(${PROJECT_NAME} FbxSdk QGLViewer Eigen3::Eigen)

#Copy ressources next to executable
foreach(RES ${PROJECT_SHADERS})
	configure_file(${RES} ${RES} COPYONLY)
endforeach()
