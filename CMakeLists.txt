cmake_minimum_required(VERSION 3.20)

project(Kaleidoscope LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "LLVM_SOURCE_DIR = ${LLVM_SOURCE_DIR}")
message(STATUS "LLVM_INCLUDE_DIRS = ${LLVM_INCLUDE_DIRS}")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR = ${LLVM_DIR}")

add_executable(kaleidoscope
    main.cpp
    lexer.cpp
    parser.cpp
    codegen.cpp
)

set(LLVM_PROJECT_SOURCE_DIR "$ENV{HOME}/Developer/llvm-project")

target_include_directories(kaleidoscope
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LLVM_PROJECT_SOURCE_DIR}/llvm/examples/Kaleidoscope/include
    SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(kaleidoscope
    PRIVATE
        ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    ExecutionEngine
    InstCombine
    OrcJIT
    RuntimeDyld
    ScalarOpts
    Support
    Target
    native
)

target_link_libraries(kaleidoscope
    PRIVATE
        ${LLVM_LIBS}
)
