cmake_minimum_required(VERSION 3.20)
project(QuantaBook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(quantabook
    src/order_book.cpp
    src/matching_engine.cpp
)

target_include_directories(quantabook PUBLIC include)

add_executable(quantabook_demo src/main.cpp)
target_link_libraries(quantabook_demo PRIVATE quantabook)

add_executable(benchmark_matching_engine benchmarks/benchmark_matching_engine.cpp)
target_link_libraries(benchmark_matching_engine PRIVATE quantabook)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
add_executable(order_book_tests tests/order_book_tests.cpp)
target_link_libraries(order_book_tests PRIVATE quantabook Catch2::Catch2WithMain)
add_test(NAME order_book_tests COMMAND order_book_tests)

add_executable(matching_engine_tests tests/matching_engine_tests.cpp)
target_link_libraries(matching_engine_tests PRIVATE quantabook Catch2::Catch2WithMain)
add_test(NAME matching_engine_tests COMMAND matching_engine_tests)
