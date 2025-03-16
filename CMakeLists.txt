cmake_minimum_required(VERSION 3.29)
project(YT)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator-hpp CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

find_package(Freetype)
find_path(PLF_HIVE_INCLUDE_DIRS "plf_hive.h")

find_package(ECM CONFIG REQUIRED NO_MODULE)
list(INSERT CMAKE_MODULE_PATH 0 ${ECM_MODULE_PATH})

find_package(WaylandScanner REQUIRED)
find_package(WaylandProtocols REQUIRED)

add_executable(YTTest
        ${PROTO_CLIENT_SRC}
        Internal/WindowManager.cpp
        Internal/RenderManager.cpp
        main.cpp
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
        Delegate.ixx
        Internal/RenderManager.ixx
        Internal/RenderTypes.ixx
        Internal/WindowManager.ixx
        Internal/ThreadManager.ixx
        Internal/WindowResource.ixx
        Internal/WindowTable.ixx
        Internal/WindowTypes.ixx
        Internal/WidgetRegistry.ixx
        Internal/BlockTable.ixx
        Internal/TransientBuffer.ixx
        Internal/ShaderBuilder.ixx
)


ecm_add_wayland_client_protocol(YTTest
        PROTOCOL ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
        BASENAME xdg-shell
)

target_include_directories(YTTest PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PLF_HIVE_INCLUDE_DIRS}
)

target_link_libraries(YTTest PRIVATE
        wayland-client
        vulkan
        GPUOpen::VulkanMemoryAllocator
        unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        glslang::SPIRV
        glslang::glslang-default-resource-limits
)
target_compile_options(YTTest PRIVATE -Wno-c23-extensions -Wno-invalid-offsetof -Wno-nullability-completeness)

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
