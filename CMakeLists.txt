cmake_minimum_required(VERSION 3.22)
project(SpatialAcceleration)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3.8 REQUIRED)

set(CMAKE_CXX_FLAGS "-L ../lib -lglfw3 -lpng -framework GLUT -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon")
include_directories(${PROJECT_SOURCE_DIR}/include)
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpng ")

add_subdirectory(src)
