cmake_minimum_required(VERSION 3.16)

project(CGVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------
# Compiler options
# --------------------------------------------------

add_compile_options(
    -Wall
    -Wextra
    -fsanitize=address
    -g
)

add_link_options(
    -fsanitize=address
)


# --------------------------------------------------
# Dependencies
# --------------------------------------------------

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)


# --------------------------------------------------
# Shared source files
# Everything except the top-level src/*.cpp samples
# --------------------------------------------------

file(GLOB_RECURSE COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/geom/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer/*.cpp"
)


# --------------------------------------------------
# Shaders
# --------------------------------------------------

set(SHADER_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer/shaders"
)

set(VERT_SPV "${SHADER_DIR}/vert.spv")
set(FRAG_SPV "${SHADER_DIR}/frag.spv")
set(TEXT_FRAG_SPV "${SHADER_DIR}/text_frag.spv")


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
        "${SHADER_DIR}/frag.frag"
        -o ${FRAG_SPV}
    DEPENDS "${SHADER_DIR}/frag.frag"
)

# Comando para construir o novo shader do texto
add_custom_command(
    OUTPUT ${TEXT_FRAG_SPV}
    COMMAND ${GLSLANG_VALIDATOR}
        -V
        "${SHADER_DIR}/text_frag.frag"
        -o ${TEXT_FRAG_SPV}
    DEPENDS "${SHADER_DIR}/text_frag.frag"
)

add_custom_target(shaders
    DEPENDS
        ${VERT_SPV}
        ${FRAG_SPV}
        ${TEXT_FRAG_SPV}
)


# --------------------------------------------------
# Samples
# Every .cpp directly inside src/ becomes an executable
# --------------------------------------------------

file(GLOB SAMPLE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

foreach(SAMPLE ${SAMPLE_SOURCES})

    get_filename_component(SAMPLE_NAME ${SAMPLE} NAME_WE)

    add_executable(${SAMPLE_NAME}
        ${SAMPLE}
        ${COMMON_SOURCES}
    )

    target_include_directories(${SAMPLE_NAME}
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_link_libraries(${SAMPLE_NAME}
        PRIVATE
            glfw
            Vulkan::Vulkan
    )

    add_dependencies(${SAMPLE_NAME} shaders)

endforeach()