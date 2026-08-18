cmake_minimum_required(VERSION 3.22)

project(actions_stub VERSION 0.1.0 LANGUAGES CXX)

add_library(actions_stub_lib src/app.cpp)
target_include_directories(actions_stub_lib PUBLIC include)
target_compile_features(actions_stub_lib PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(actions_stub_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(actions-stub src/main.cpp)
target_link_libraries(actions-stub PRIVATE actions_stub_lib)

enable_testing()

add_executable(actions_stub_tests tests/app_tests.cpp)
target_link_libraries(actions_stub_tests PRIVATE actions_stub_lib)
add_test(NAME actions_stub_tests COMMAND actions_stub_tests)

install(TARGETS actions-stub RUNTIME DESTINATION bin)
