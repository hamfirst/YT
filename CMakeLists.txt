cmake_minimum_required(VERSION 4.3.1)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
#set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_FLAGS "-fmodules -Wno-c23-extensions") #-Wno-interference-size  -freflection -fexpansion-statements
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(YT LANGUAGES C CXX)

find_package(Threads)

find_package(Vulkan)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator-hpp CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)

find_package(Freetype CONFIG REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n data)

find_package(concurrentqueue CONFIG REQUIRED)
find_package(readerwriterqueue CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_path(PLF_HIVE_INCLUDE_DIRS "plf_hive.h")
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

find_package(ECM CONFIG REQUIRED NO_MODULE)
list(INSERT CMAKE_MODULE_PATH 0 ${ECM_MODULE_PATH})

find_package(WaylandScanner REQUIRED)
find_package(WaylandProtocols REQUIRED)

enable_testing()

find_package(GTest CONFIG REQUIRED)

set(YT_CPP_SOURCES
        ${PROTO_CLIENT_SRC}
        src/External/stb/STBI.cpp
        src/External/vulkan/Vulkan.cpp
        src/External/vma/VMA.cpp
)

set(YT_MODULE_SOURCES
        src/Allocators/BitAllocator.ixx
        src/Allocators/BlockTable.ixx
        src/Allocators/FixedBlockAllocator.ixx
        src/Allocators/ObjectPool.ixx
        src/Allocators/ShelfAllocator2D.ixx

        src/Delegate/Delegate.ixx

        src/Init/Init.ixx
        src/Init/InitImpl.cpp

        src/Job/Coroutine.ixx
        src/Job/CoroutineImpl.cpp
        src/Job/FileMapper.ixx
        src/Job/FileMapperImpl.cpp
        src/Job/JobManager.ixx
        src/Job/JobManagerImpl.cpp
        src/Job/JobTypes.ixx
        src/Job/WorkerThread.ixx
        src/Job/WorkerThreadQueue.ixx
        src/Job/WorkerThreadQueueImpl.cpp

        src/Queues/MultiProducerMultiConsumer.ixx
        src/Queues/MultiProducerSingleConsumer.ixx
        src/Queues/SingleProducerMultiConsumer.ixx
        src/Queues/SingleProducerSingleConsumer.ixx

        src/Render/DeferredDelete.ixx
        src/Render/DeferredImageLoad.ixx
        src/Render/DeferredImageLoadImpl.cpp
        src/Render/Drawer.ixx
        src/Render/DrawerImpl.cpp
        src/Render/ImageBuffer.ixx
        src/Render/ImageReference.ixx
        src/Render/ImageReferenceImpl.cpp
        src/Render/QuadRender.ixx
        src/Render/QuadRenderImpl.cpp
        src/Render/RenderManager.ixx
        src/Render/RenderManagerImpl.cpp
        src/Render/RenderReflect.ixx
        src/Render/RenderReflectImpl.cpp
        src/Render/RenderTypes.ixx
        src/Render/ShaderBuilder.ixx
        src/Render/StagingBuffer.ixx
        src/Render/TransientBuffer.ixx

        src/Widget/Widget.ixx
        src/Widget/WidgetRegistry.ixx

        src/Window/Window.ixx
        src/Window/WindowImpl.cpp
        src/Window/WindowManager.ixx
        src/Window/WindowManagerImpl.cpp
        src/Window/WindowResource.ixx
        src/Window/WindowTable.ixx
        src/Window/WindowTypes.ixx

        src/Types.ixx
        src/YT.ixx
        src/YTImpl.cpp
)

set(YT_EXTERNAL_MODULE_SOURCES
        src/External/glm/GLM.ixx
        src/External/glslang/GLSLang.ixx
)

set(YT_LINK_LIBRARIES
        Microsoft.GSL::GSL
        concurrentqueue::concurrentqueue
        readerwriterqueue::readerwriterqueue
        wayland-client
        Freetype::Freetype
        harfbuzz::harfbuzz
        ICU::uc ICU::i18n ICU::data
        vulkan
        GPUOpen::VulkanMemoryAllocator
        unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        glslang::SPIRV
        glslang::glslang-default-resource-limits
        unofficial::spirv-reflect
        glm::glm
)

function(add_yt_executable TARGET_NAME SOURCES)

    add_executable(${TARGET_NAME} ${SOURCES} ${YT_CPP_SOURCES})
    target_sources(${TARGET_NAME} PUBLIC FILE_SET CXX_MODULES FILES ${YT_MODULE_SOURCES} ${YT_EXTERNAL_MODULE_SOURCES})

    ecm_add_wayland_client_protocol(${TARGET_NAME}
            PROTOCOL ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
            BASENAME xdg-shell
    )

    target_include_directories(${TARGET_NAME} PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/External/wayland
            ${PLF_HIVE_INCLUDE_DIRS}
            ${Stb_INCLUDE_DIR}
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
            ${YT_LINK_LIBRARIES}
    )

    get_target_property(MY_SOURCES ${TARGET_NAME} SOURCES)
    message(STATUS "Source files for ${TARGET_NAME}: ${MY_SOURCES}")

endfunction()

function(add_yt_test_executable TARGET_NAME SOURCES MODULES)
    add_yt_executable(${TARGET_NAME} ${SOURCES})

    target_link_libraries(${TARGET_NAME} PRIVATE
            GTest::gtest
    )

    target_sources(${TARGET_NAME} PUBLIC FILE_SET CXX_MODULES FILES ${MODULES})
    add_test(${TARGET_NAME}Test ${TARGET_NAME} --gtest_break_on_failure)

endfunction()

add_yt_executable(YTTest
        main.cpp
)

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SPV_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders/bin")
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

add_yt_test_executable(YTBlockTableUnitTests
        tests/Empty.cpp tests/BlockTableTests.cpp
)

add_yt_test_executable(YTJobManagerUnitTests
        tests/Empty.cpp tests/JobManagerTests.cpp
)

add_yt_test_executable(YTBitAllocatorUnitTests
        tests/Empty.cpp tests/BitAllocatorTests.cpp
)

add_yt_test_executable(YTFixedBlockAllocatorUnitTests
        tests/Empty.cpp tests/FixedBlockAllocatorTests.cpp
)

add_yt_test_executable(YTShelfAllocator2DUnitTests
        tests/Empty.cpp tests/ShelfAllocator2DTests.cpp
)

add_yt_test_executable(YTDelegateUnitTests
        tests/Empty.cpp tests/DelegateTests.cpp
)

add_yt_test_executable(YTDeferredDeleteUnitTests
        tests/Empty.cpp tests/DeferredDeleteTests.cpp
)

add_yt_test_executable(YTObjectPoolUnitTests
        tests/Empty.cpp tests/ObjectPoolTests.cpp
)

add_yt_test_executable(YTSingleProducerSingleConsumerUnitTests
        tests/Empty.cpp tests/SingleProducerSingleConsumerTests.cpp
)

add_yt_test_executable(YTSingleProducerMultiConsumerUnitTests
        tests/Empty.cpp tests/SingleProducerMultiConsumerTests.cpp
)

add_yt_test_executable(YTMultiProducerMultiConsumerUnitTests
        tests/Empty.cpp tests/MultiProducerMultiConsumerTests.cpp
)

add_yt_test_executable(YTMultiProducerSingleConsumerUnitTests
        tests/Empty.cpp tests/MultiProducerSingleConsumerTests.cpp
)
target_link_libraries(YTMultiProducerSingleConsumerUnitTests PRIVATE GTest::gtest_main)
