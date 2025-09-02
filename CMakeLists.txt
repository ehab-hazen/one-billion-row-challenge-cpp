cmake_minimum_required(VERSION 3.10)
project(1brc)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
    main.cc
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

