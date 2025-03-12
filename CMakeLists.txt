cmake_minimum_required(VERSION 3.29)
project(YT)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(YTTest
        Internal/WindowManager.cpp
        Internal/RenderManager.cpp
        main.cpp
        External/XDG/XDGShell.c
        Internal/Drawer.cpp
        Internal/Window.cpp
)

target_sources(YTTest PUBLIC FILE_SET CXX_MODULES FILES
        YT.ixx
        Drawer.ixx
        Init.ixx
        Types.ixx
        Widget.ixx
        Window.ixx
        Internal/RenderManager.ixx
        Internal/RenderTypes.ixx
        Internal/WindowManager.ixx
        Internal/ThreadManager.ixx
        Internal/WindowResource.ixx
        Internal/WindowTable.ixx
        Internal/WindowTypes.ixx
        Internal/BlockTable.ixx
        Internal/TransientBuffer.ixx
)

target_include_directories(YTTest PRIVATE External)
target_include_directories(YTTest PRIVATE External/VulkanMemoryAllocator)
target_link_libraries(YTTest PRIVATE wayland-client vulkan)

target_compile_options(YTTest PRIVATE -Wno-c23-extensions -Wno-invalid-offsetof -Wno-nullability-completeness)

find_package(Vulkan COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
set(SPV_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/bin")
file(MAKE_DIRECTORY ${SPV_OUTPUT_DIR})

file(GLOB_RECURSE SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")
foreach(SHADER_PATH ${SHADERS})
    message("-- Found shader " ${SHADER_PATH})
    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME_WE)
    add_custom_command(
            OUTPUT ${SPV_OUTPUT_DIR}/${SHADER_NAME}.spv
            COMMAND ${glslc_executable} ${SHADER_PATH} -o ${SPV_OUTPUT_DIR}/${SHADER_NAME}.spv
            DEPENDS ${SHADER_PATH}
            VERBATIM
    )
    add_custom_target(${SHADER_NAME} DEPENDS ${SPV_OUTPUT_DIR}/${SHADER_NAME}.spv)
    set_source_files_properties(${SPV_OUTPUT_DIR}/${SHADER_NAME}.spv PROPERTIES HEADER_FILE_ONLY TRUE)

    add_dependencies(YTTest ${SHADER_NAME})
endforeach()