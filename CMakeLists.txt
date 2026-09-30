cmake_minimum_required(VERSION 3.20)
project(Camera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(HIK_SDK_DIR "/opt/MVS")  
include_directories(${HIK_SDK_DIR}/include)
link_directories(${HIK_SDK_DIR}/lib/64)

add_executable(demo src/main.cpp src/Camera.cpp)

target_link_libraries(demo PRIVATE
    ${OpenCV_LIBS}
    MvCameraControl      
    pthread  
)           

target_include_directories(demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)