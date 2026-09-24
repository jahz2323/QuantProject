set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

# Set policies globally
cmake_policy(SET CMP0079 NEW)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

project(QuantSystem LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CUSTOM_MEMORY_LIBS
    LoggingSystem 
    MemoryPool
)

set(CUSTOM_LIBS
    lob_engine 
    API_Connector
)

#Setting up path to historical datasets 
set(BINANCE_HISTORICAL_DATASET_DIR "$ENV{HOME}/data/binance")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

# Order matters: build API_Connector first, then lob_engine
add_subdirectory(NetOrchestrate)
add_subdirectory(lob_engine)
add_subdirectory(external)


add_executable(COMBINED_STREAMS
    tests/combined_streams.cpp
)
target_link_libraries(COMBINED_STREAMS PRIVATE 
    ${CUSTOM_LIBS}
)

# add_executable(HISTORICAL_READER_TEST
#     tests/historical_reader_test.cpp
# ) 
# target_include_directories(HISTORICAL_READER_TEST PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
# target_link_libraries(HISTORICAL_READER_TEST PRIVATE csvpp  
#     ${CUSTOM_LIBS}
# )

