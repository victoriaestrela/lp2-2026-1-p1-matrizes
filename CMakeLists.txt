cmake_minimum_required(VERSION 3.10)
project(matrix_multiply C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(matrix_multiply
    src/main.c
    src/matrix_multiply.c
)

target_compile_options(matrix_multiply PRIVATE -O2 -Wall -Wextra)
target_link_libraries(matrix_multiply PRIVATE Threads::Threads m)
