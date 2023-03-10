find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)

set(FBX_ROOT "~/fbxsdk" CACHE PATH "The root of the FBX SDK")

IF(NOT EXISTS "${FBX_ROOT}/lib")
    MESSAGE(FATAL_ERROR "The specified FBX SDK folder has no lib subfolder")
ENDIF()

IF(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    IF(MSVC_VERSION GREATER 1899 AND MSVC_VERSION LESS 1911)
        SET(FBX_COMPILER "vs2015")
    ELSEIF(MSVC_VERSION GREATER 1910 AND MSVC_VERSION LESS 1920)
        SET(FBX_COMPILER "vs2017")
    ELSEIF(MSVC_VERSION GREATER 1919)
        SET(FBX_COMPILER "vs2019")
    ENDIF()
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    SET(FBX_COMPILER "clang")
ELSE(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    SET(FBX_COMPILER "gcc")
ENDIF()

IF(NOT DEFINED FBX_COMPILER OR NOT EXISTS "${FBX_ROOT}/lib/${FBX_COMPILER}")
    MESSAGE(FATAL_ERROR  "Compiler not supported")
ENDIF()

set(FBX_ARCH "x64" CACHE STRING "architecture to use with the FBX SDK") #TODO : detect x86

add_library(FbxSdk STATIC IMPORTED)
target_include_directories(FbxSdk INTERFACE "${FBX_ROOT}/include")
set_property(
    TARGET FbxSdk APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE
)
set_target_properties(FbxSdk PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
    IMPORTED_LOCATION_RELEASE "${FBX_ROOT}/lib/${FBX_COMPILER}/${FBX_ARCH}/release/libfbxsdk.a"
)
set_property(
    TARGET FbxSdk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG
)
set_target_properties(FbxSdk PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
    IMPORTED_LOCATION_DEBUG "${FBX_ROOT}/lib/${FBX_COMPILER}/${FBX_ARCH}/debug/libfbxsdk.a"
)

set_target_properties(FbxSdk PROPERTIES
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

target_link_libraries(FbxSdk INTERFACE xml2 z)
