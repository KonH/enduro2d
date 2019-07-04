cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(enduro2d)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(E2D_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(E2D_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(E2D_ALL_C_CONFIGS "CMAKE_C_FLAGS")
set(E2D_ALL_CXX_CONFIGS "CMAKE_CXX_FLAGS")

if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} build_type)
    set(E2D_ALL_C_CONFIGS "${E2D_ALL_C_CONFIGS}" "CMAKE_C_FLAGS_${build_type}")
    set(E2D_ALL_CXX_CONFIGS "${E2D_ALL_CXX_CONFIGS}" "CMAKE_CXX_FLAGS_${build_type}")
endif()

foreach(type ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${type} build_type)
    set(E2D_ALL_C_CONFIGS "${E2D_ALL_C_CONFIGS}" "CMAKE_C_FLAGS_${build_type}")
    set(E2D_ALL_CXX_CONFIGS "${E2D_ALL_CXX_CONFIGS}" "CMAKE_CXX_FLAGS_${build_type}")
endforeach()

#
# linking mode
#

if(MSVC)
    option(E2D_BUILD_WITH_STATIC_CRT "Use static C runtime library" ON)
    if(E2D_BUILD_WITH_STATIC_CRT)
        foreach(flag ${E2D_ALL_C_CONFIGS} ${E2D_ALL_CXX_CONFIGS})
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
            if(${flag} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
            endif()
        endforeach()
    endif()
endif(MSVC)

#
# compilation mode
#

set(E2D_SHARED_CXX_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(E2D_SHARED_CXX_FLAGS -Wloop-analysis
                             -Wpointer-arith
                             -Wpessimizing-move
                             -Wuninitialized
                             -Werror=init-self
                             -Werror=self-assign
                             -Werror=self-move
                             -Werror=infinite-recursion
                             -Werror=dangling-else
                             -Werror=large-by-value-copy
                             -Werror=instantiation-after-specialization
                             -Werror=array-bounds
                             -Werror=missing-field-initializers
                             -Werror=user-defined-literals
                             -Werror=parentheses
                             -Werror=address
                             -Werror=return-stack-address)
    string(REPLACE ";" " " E2D_SHARED_CXX_FLAGS "${E2D_SHARED_CXX_FLAGS}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

foreach(config ${E2D_ALL_CXX_CONFIGS})
    set(${config} "${${config}} ${E2D_SHARED_CXX_FLAGS}")
endforeach()

#
# coverage mode
#

option(E2D_BUILD_WITH_COVERAGE "Build with coverage" OFF)
if(E2D_BUILD_WITH_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    add_definitions(-DE2D_BUILD_WITH_COVERAGE)
    set(E2D_COVERAGE_FLAGS "--coverage")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${E2D_COVERAGE_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${E2D_COVERAGE_FLAGS}")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} ${E2D_COVERAGE_FLAGS}")
endif()

#
# sanitizer modes
#

option(E2D_BUILD_WITH_ASAN "Build with address sanitizer" OFF)
if(E2D_BUILD_WITH_ASAN AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    add_definitions(-DE2D_BUILD_WITH_ASAN)
    set(E2D_SANITIZER_FLAGS "-fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${E2D_SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${E2D_SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} ${E2D_SANITIZER_FLAGS}")
endif()

option(E2D_BUILD_WITH_UBSAN "Build with undefined sanitizer" OFF)
if(E2D_BUILD_WITH_UBSAN AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    add_definitions(-DE2D_BUILD_WITH_UBSAN)
    set(E2D_SANITIZER_FLAGS "-fsanitize=undefined")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${E2D_SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${E2D_SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} ${E2D_SANITIZER_FLAGS}")
endif()

#
# e2d config
#

# detect compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(E2D_COMPILER "E2D_COMPILER_MSVC")
    set(E2D_COMPILER_FLAGS /W3 /MP)
    set(E2D_DEFINITIONS -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(E2D_COMPILER "E2D_COMPILER_GCC")
    set(E2D_COMPILER_FLAGS -Wall -Wextra -Wpedantic)
    set(E2D_DEFINITIONS "")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(E2D_COMPILER "E2D_COMPILER_CLANG")
    set(E2D_COMPILER_FLAGS -Wall -Wextra -Wpedantic -Werror=return-type -Werror=multichar -Werror=odr)
    set(E2D_DEFINITIONS "")
else()
    message(FATAL_ERROR "unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# detect target platform
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(E2D_PLATFORM "E2D_PLATFORM_LINUX")
    set(E2D_PLATFORM_LIBS glfw glew)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(E2D_PLATFORM "E2D_PLATFORM_ANDROID")
    set(E2D_PLATFORM_LIBS GLESv2 EGL android log)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(E2D_PLATFORM "E2D_PLATFORM_MACOSX")
    set(E2D_PLATFORM_LIBS glfw glew)
    set(E2D_COMPILER_FLAGS ${E2D_COMPILER_FLAGS} -Wno-deprecated-declarations)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(E2D_PLATFORM "E2D_PLATFORM_IOS")
    set(E2D_PLATFORM_LIBS "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(E2D_PLATFORM "E2D_PLATFORM_WINDOWS")
    set(E2D_PLATFORM_LIBS glfw glew)
else()
    message(FATAL_ERROR "unsupported platform ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
endif()

#
# e2d sources
#

file(GLOB_RECURSE E2D_HEADERS
    headers/enduro2d/*.hpp
    headers/enduro2d/*.inl)

file(GLOB_RECURSE E2D_SOURCES
    sources/enduro2d/*.cpp
    sources/enduro2d/*.hpp
    headers/enduro2d/*.inl)

if(APPLE)
    file(GLOB_RECURSE E2D_SOURCES_MM
        sources/enduro2d/*.mm)
    list(APPEND E2D_SOURCES
        ${E2D_SOURCES_MM})
endif()

#
# e2d internal 3rd party
#

file(GLOB_RECURSE E2D_3RDPARTY
    headers/3rdparty/*.*
    sources/3rdparty/*.*)

#
# e2d external 3rd party
#

add_subdirectory(modules/enduro2d-external)
include(modules/enduro2d-external/engine-integration.cmake)

function(e2d_add_executable target_name source_list)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        add_library(${target_name} SHARED ${${source_list}})
    else()
        add_executable(${target_name} ${${source_list}})
    endif()
endfunction(e2d_add_executable)

#
# e2d library target
#

add_library(${PROJECT_NAME} STATIC
    ${E2D_HEADERS}
    ${E2D_SOURCES}
    ${E2D_3RDPARTY})

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${E2D_PLATFORM_LIBS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${E2D_HEADERS}
    ${E2D_SOURCES}
    ${E2D_3RDPARTY})

target_link_libraries(${PROJECT_NAME}
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:winmm.lib>)

if(${E2D_ENABLE_BASS})
    target_link_libraries(${PROJECT_NAME} PRIVATE bass)
endif()

if(${E2D_ENABLE_CURL})
    target_link_libraries(${PROJECT_NAME} PRIVATE curl)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC headers
    PRIVATE sources)

target_compile_options(${PROJECT_NAME}
    PRIVATE ${E2D_COMPILER_FLAGS})

target_compile_features(${PROJECT_NAME}
    PUBLIC cxx_std_17)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC ${E2D_DEFINITIONS}
    PUBLIC -DE2D_PLATFORM=${E2D_PLATFORM}
    PUBLIC -DE2D_COMPILER=${E2D_COMPILER})

#
# subdirectories
#

option(E2D_BUILD_SAMPLES "Build samples" ON)
if(E2D_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
option(E2D_BUILD_UNTESTS "Build untests" ON)
if(E2D_BUILD_UNTESTS)
    enable_testing()
    add_subdirectory(untests)
endif()
