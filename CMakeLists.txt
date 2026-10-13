cmake_minimum_required(VERSION 3.20)
project(Camera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

set(HIK_SDK_DIR "/opt/MVS")  
include_directories(${HIK_SDK_DIR}/include)

add_executable(demo src/main.cpp src/Camera.cpp)

target_link_directories(demo PRIVATE ${HIK_SDK_DIR}/lib/64)
target_link_libraries(demo PRIVATE
    ${OpenCV_LIBS}
    MvCameraControl      
    pthread  
)           

target_include_directories(demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)