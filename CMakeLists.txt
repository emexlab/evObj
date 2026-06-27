cmake_minimum_required(VERSION 3.16)

project(evObj
    VERSION 1.0.0
    LANGUAGES C
)

find_package(Threads REQUIRED)

add_library(evObj STATIC
    src/runtime/EVBase.c
    src/runtime/EVAllocator.c

    src/EVNumber.c
    src/EVString.c
)

target_include_directories(evObj
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(evObj PUBLIC c_std_23)

if (MSVC)
    # Fuck MSVC nobody likes you
    message(FATAL_ERROR "MSVC is not supported. Switch to Linux moron, microslop sheep!")
else()

    # Setting up compiler flags
    set(_EV_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    set(_EV_OPT_FLAGS -fPIC)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND _EV_OPT_FLAGS -O0 -g)
    else()
        list(APPEND _EV_OPT_FLAGS -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -funroll-loops)

        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            list(APPEND _EV_OPT_FLAGS -fgraphite-identity -floop-nest-optimize -fipa-pta)
        elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
            list(APPEND _EV_OPT_FLAGS -fvectorize -fslp-vectorize)
        endif()
    endif()

    target_compile_definitions(evObj PRIVATE $<$<CONFIG:Debug>:DEBUG>)
    target_compile_options(evObj PRIVATE ${_EV_WARN_FLAGS} ${_EV_OPT_FLAGS})
endif()

target_link_libraries(evObj
    PRIVATE
        Threads::Threads
)

install(TARGETS evObj
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.h"
)
