cmake_minimum_required(VERSION 3.21)
project(tiresias)

add_subdirectory(3rdparty)

if ( WIN32 )
    add_definitions(-DGLEW_STATIC)
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/App.cpp
    src/image_viewer/ImageViewer.cpp
    src/image_viewer/ImageViewerLDR.cpp
    src/image_viewer/ImageViewerSpectral.cpp
    src/image_viewer/ImageViewerSpectralEXR.cpp
    src/Shader.cpp
    src/image_format/artraw.cpp
    )

target_link_libraries(${PROJECT_NAME} 3rdparty)

target_include_directories(${PROJECT_NAME} PRIVATE src/)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/src/glsl $<TARGET_FILE_DIR:${PROJECT_NAME}>/glsl)