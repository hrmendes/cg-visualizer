cmake_minimum_required(VERSION 3.16)

project(CGVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall
    -Wextra
    -fsanitize=address
    -g
)

add_link_options(-fsanitize=address)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        Vulkan::Vulkan
)

# Shaders
set(SHADER_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer/shaders"
)

set(VERT_SPV "${SHADER_DIR}/vert.spv")
set(FRAG_SPV "${SHADER_DIR}/frag.spv")

add_custom_command(
    OUTPUT ${VERT_SPV}
    COMMAND ${GLSLANG_VALIDATOR}
        -V
        "${SHADER_DIR}/shader.vert"
        -o ${VERT_SPV}
    DEPENDS "${SHADER_DIR}/shader.vert"
)

add_custom_command(
    OUTPUT ${FRAG_SPV}
    COMMAND ${GLSLANG_VALIDATOR}
        -V
        "${SHADER_DIR}/shader.frag"
        -o ${FRAG_SPV}
    DEPENDS "${SHADER_DIR}/shader.frag"
)

add_custom_target(shaders
    DEPENDS ${VERT_SPV} ${FRAG_SPV}
)

add_dependencies(${PROJECT_NAME} shaders)
