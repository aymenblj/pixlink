cmake_minimum_required(VERSION 3.20)
project(ImageProcessor CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only pipeline library
add_library(pipeline INTERFACE)
target_include_directories(pipeline INTERFACE
    ${PROJECT_SOURCE_DIR}/include
)

# Try to find OpenCV and set up OpenCV-specific header-only pipeline
find_package(OpenCV)

if(OpenCV_FOUND)
    add_library(pipeline_opencv INTERFACE)
    target_include_directories(pipeline_opencv INTERFACE
        ${PROJECT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(pipeline_opencv INTERFACE pipeline ${OpenCV_LIBS})
    target_compile_definitions(pipeline_opencv INTERFACE HAVE_OPENCV_CORE)
endif()

# Main app
add_executable(pixlink src/main.cpp)

# Link pipeline (and opencv if found) to the app
if(OpenCV_FOUND)
    target_link_libraries(pixlink PRIVATE pipeline_opencv)
else()
    target_link_libraries(pixlink PRIVATE pipeline)
endif()