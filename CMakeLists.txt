cmake_minimum_required(VERSION 3.20)

project(ch32v003_chal 
	VERSION 1.0.0
	LANGUAGES C
    DESCRIPTION "MISRA C:2012 bare-metal HAL for the WCH CH32V003 (RISC-V RV32EC)"
	HOMEPAGE_URL "https://github.com/i-Sage/ch32v003_chal"
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(STATUS "ch32v003_chal: building as top-level project (no targets will be built)")
endif()


add_library(ch32v003_chal INTERFACE)
add_library(ch32v003::chal ALIAS ch32v003_chal)

target_include_directories(ch32v003_chal
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ch32v003_chal INTERFACE c_std_11)

target_compile_options(ch32v003_chal INTERFACE
    $<$<C_COMPILER_ID:GNU>:
        -Wall
        -Wextra
        -Wconversion
        -Wsign-conversion
        -Wpedantic
    >
)