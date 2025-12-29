cmake_minimum_required(VERSION 3.15)
project(SearchEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)


set(SOURCES
    main.cpp
    text_utils.cpp
    compression_utils.cpp
    file_utils.cpp
    search_engine.cpp
)

set(HEADERS
    text_utils.hpp
    compression_utils.hpp
    file_utils.hpp
    search_engine.hpp
)

add_executable(search_engine ${SOURCES} ${HEADERS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(search_engine stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(search_engine c++fs)
endif()


if(ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(search_engine PRIVATE /W4)
    else()
        target_compile_options(search_engine PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()


if(BUILD_TESTS)
    enable_testing()
    
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    
    add_library(search_engine_lib STATIC
        text_utils.cpp
        compression_utils.cpp
        file_utils.cpp
        search_engine.cpp
    )
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(search_engine_lib stdc++fs)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(search_engine_lib c++fs)
    endif()
    
    add_executable(run_tests tests.cpp)
    target_link_libraries(run_tests 
        PRIVATE 
        search_engine_lib
        gtest 
        gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(run_tests)
endif()
