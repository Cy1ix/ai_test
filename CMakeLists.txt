cmake_minimum_required(VERSION 3.20)
project(LearnVulkan_CPP VERSION 1.0.0 LANGUAGES CXX)

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(NOMINMAX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(
    GLM_FORCE_CXX17
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

file(GLOB_RECURSE FRAMEWORK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/framework/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/framework/*.h"
)

file(GLOB_RECURSE SAMPLES_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/samples/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/samples/*.h"
)

file(GLOB_RECURSE UTILS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.h"
)

file(GLOB MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

file(GLOB THIRD_PARTY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan/include/Volk/volk.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/astc/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/astc/Source/*.h"
)

set(ALL_SOURCES
    ${MAIN_SOURCES}
    ${FRAMEWORK_SOURCES}
    ${SAMPLES_SOURCES}
    ${UTILS_SOURCES}
    ${THIRD_PARTY_SOURCES}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES 
    ${MAIN_SOURCES}
    ${FRAMEWORK_SOURCES}
    ${SAMPLES_SOURCES}
    ${UTILS_SOURCES}
)
source_group("Third Party" FILES ${THIRD_PARTY_SOURCES})

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${ALL_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${ALL_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/componment
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/scene
    ${CMAKE_CURRENT_SOURCE_DIR}/src/samples
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/astc/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ktx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tracy/public
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/thread-pool/include
    ${Vulkan_INCLUDE_DIRS}
)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

file(GLOB COMMON_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/*.lib")
file(GLOB DEBUG_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/debug/*.lib")
file(GLOB RELEASE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/release/*.lib")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${COMMON_LIBS}
    $<$<CONFIG:Debug>:${DEBUG_LIBS}>
    $<$<CONFIG:Release>:${RELEASE_LIBS}>
    $<$<CONFIG:RelWithDebInfo>:${RELEASE_LIBS}>
    $<$<CONFIG:MinSizeRel>:${RELEASE_LIBS}>
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        GLFW_STATIC
        GLM_ENABLE_EXPERIMENTAL
        VK_USE_PLATFORM_WIN32_KHR
        ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/assets/\"
        MODEL_DIR=\"${CMAKE_SOURCE_DIR}/assets/models/\"
        GLSL_SHADER_DIR=\"${CMAKE_SOURCE_DIR}/assets/shaders/glsl/\"
        TEXTURE_DIR=\"${CMAKE_SOURCE_DIR}/assets/texture/\"
        OUTPUT_DIR=\"${CMAKE_SOURCE_DIR}/output/\"
    )
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES} pthread dl)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VK_USE_PLATFORM_XCB_KHR
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(METAL_LIBRARY Metal REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA_LIBRARY} ${METAL_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        VK_ENABLE_PORTABILITY
        VK_USE_PLATFORM_MACOS_MVK
    )
endif()

set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSETS_DIR} ${ASSETS_OUTPUT_DIR}
    COMMENT "Copying assets to output directory"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:VK_DEBUG>
    $<$<CONFIG:Debug>:VK_VALIDATION_LAYERS>
    $<$<CONFIG:Debug>:VK_PROFILING>
    #$<$<CONFIG:Debug>:VK_VALIDATION_LAYERS_GPU_ASSISTED>
    #$<$<CONFIG:Debug>:VK_VALIDATION_LAYERS_BEST_PRACTICES>
    #$<$<CONFIG:Debug>:VK_VALIDATION_LAYERS_SYNCHRONIZATION>
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_NO_PROTOTYPES
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    NOMINMAX
    VK_USE_PLATFORM_WIN32_KHR
    _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
)
