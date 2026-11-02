cmake_minimum_required(VERSION 3.14)
project(SmartHospitalSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Portable compiler warnings for MSVC and GCC/Clang
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wconversion)
endif()

include_directories(include)

# Core models and services library
add_library(HospitalCore STATIC
    src/models/Patient.cpp
    src/models/DispatchResource.cpp
    src/services/DataLoader.cpp
)

# Main interactive console application
add_executable(SmartHospitalSystem
    src/main.cpp
)
target_link_libraries(SmartHospitalSystem PRIVATE HospitalCore)

# Test runner executable
add_executable(TestRunner
    tests/TestRunner.cpp
)
target_link_libraries(TestRunner PRIVATE HospitalCore)

# Enable CTest integration with reliable data directory configuration
enable_testing()
add_test(NAME SmartHospitalSystem_CompleteTestSuite COMMAND TestRunner "${CMAKE_CURRENT_SOURCE_DIR}/data/")
set_tests_properties(SmartHospitalSystem_CompleteTestSuite PROPERTIES 
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# REST API server for the web frontend (additive; does not affect the targets above).
# NOTE: this block has not been verified with CMake on the development host (CMake not installed).
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/server/src/server_main.cpp")
    add_executable(HospitalServer
        server/src/server_main.cpp
        server/src/http_server.cpp
        server/src/api.cpp
    )
    target_include_directories(HospitalServer PRIVATE server/third_party server/src)
    target_link_libraries(HospitalServer PRIVATE HospitalCore)
    if (WIN32)
        target_link_libraries(HospitalServer PRIVATE ws2_32)
    endif()
endif()
