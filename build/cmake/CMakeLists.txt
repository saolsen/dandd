cmake_minimum_required(VERSION 3.30)
project(dandd C)
set(CMAKE_C_STANDARD 23)

add_executable(dandd ../../dandd.c)
target_compile_options(dandd PRIVATE -Wall -Werror -Wextra -Wconversion)

# note(steve): On MacOS need MallocNanoZone=0 or you'll get a warning with asan in stdlib code.
# target_compile_options(dandd PUBLIC -fsanitize=address -fno-omit-frame-pointer)
# target_link_options(dandd PUBLIC -fsanitize=address -fno-omit-frame-pointer)