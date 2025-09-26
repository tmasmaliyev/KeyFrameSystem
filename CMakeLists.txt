cmake_minimum_required(VERSION 3.10)
project(KeyframeMotionSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release if not specified
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Add compiler flags
set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_program(GIT_EXECUTABLE git)
if (GIT_EXECUTABLE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
    option(GIT_SUBMODULE "Check submodules during build" ON)

    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
            endif()
    endif()
endif()

find_package(OpenGL REQUIRED)

# Find packages installed by vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Include directories
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${GLFW3_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})

# Add include directory for headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define source files with proper paths
set(SOURCES
    src/main.cpp
    src/motion/Mesh.cpp
    src/motion/MotionController.cpp
    src/motion/Renderer.cpp
    src/motion/Utils.cpp
)

# Define header files (for IDE organization)
set(HEADERS
    include/motion/Mesh.h
    include/motion/MotionController.h
    include/motion/Renderer.h
    include/motion/Utils.h
)

# Copy assets directory to build directory
file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets 
    DESTINATION ${CMAKE_BINARY_DIR}
)

# Add the executable
add_executable(keyframe_system ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(keyframe_system PRIVATE
    glfw
    GLEW::GLEW
    OpenGL::GL
)

# Add compiler flags for GLFW3
target_compile_options(keyframe_system PRIVATE ${GLFW3_CFLAGS_OTHER})

# Path to the directory you want to copy
set(MY_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

add_custom_command(TARGET keyframe_system POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MY_ASSETS_DIR}"
            "$<TARGET_FILE_DIR:keyframe_system>/assets"
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenGL found: ${OPENGL_FOUND}")
message(STATUS "GLFW3 found: ${GLFW3_FOUND}")
message(STATUS "GLEW found: ${GLEW_FOUND}")

# Optional: Print build information
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")