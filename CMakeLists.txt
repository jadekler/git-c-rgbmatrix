# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8)
project (rest_test)

add_executable (rest_test rest_test.cc)
target_link_libraries(rest_test curl)