cmake_minimum_required(VERSION 3.20)
project(QuantSystem LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Common dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS headers json)
find_package(pybind11 CONFIG REQUIRED)

# Tools setup
set(TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../GraphicsEngineSDK/Tools")
cmake_path(SET TOOLS_DIR NORMALIZE "${TOOLS_DIR}")

if(EXISTS "${TOOLS_DIR}")
    if(NOT TARGET LoggingSystem)
        add_library(LoggingSystem STATIC "${TOOLS_DIR}/LoggingSystem/src/LoggingSystem.cpp")
        target_include_directories(LoggingSystem PUBLIC "${TOOLS_DIR}/LoggingSystem/include")
    endif()
    
    if(NOT TARGET MemoryPool)
        add_library(MemoryPool INTERFACE)
        target_include_directories(MemoryPool INTERFACE "${TOOLS_DIR}/MemoryPool/include")
    endif()
endif()

# Order matters: build API_Connector first, then lob_engine
add_subdirectory(NetOrchestrate)
add_subdirectory(lob_engine)