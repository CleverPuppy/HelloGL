cmake_minimum_required(VERSION 3.2)
project(learn_opengl)

#required libraries

find_package(OpenGL REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# GLM
#find_package(GLM REQUIRED)
#message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

# GLFW
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

add_definitions(
    -DUSE_GLAD
    -DUSE_GLFW
)

include_directories(
    "${PROJECT_SOURCE_DIR}/thirdparty/include"
#    ${GLM_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR})

add_library(GLAD "thirdparty/src/glad.c")

set(LIBS  ${GLFW3_LIBRARY} ${OpenGL_LIBRARY} GLAD)
add_executable(${PROJECT_NAME} test.c )
target_link_libraries(${PROJECT_NAME} ${LIBS})

add_executable(triangle triangle.cpp)
target_link_libraries(triangle ${LIBS})
