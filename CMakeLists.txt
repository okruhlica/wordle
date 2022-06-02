cmake_minimum_required(VERSION 3.21)
project(wordle_solver)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(
        SOURCE_FILES
        main.cpp WordleGame.h WordleStratEntropy.h WordleStrat.h WordleConstants.h ParallelExec.h
        Echo.h WordleSecrets.h)
add_executable(wordle_solver ${SOURCE_FILES})

cmake_minimum_required(VERSION 3.1)
find_package(nlohmann_json 3.10.5 REQUIRED)
target_link_libraries(wordle_solver PRIVATE nlohmann_json::nlohmann_json)
set_target_properties(wordle_solver PROPERTIES LINKER_LANGUAGE CXX)