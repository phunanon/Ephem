cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project("Ephem")

add_executable(ephem src/Env.cpp src/Cell.cpp src/Parser.cpp src/EVM.cpp src/linenoise/linenoise.c src/keypresses.c src/main.cpp)

set_target_properties(ephem PROPERTIES COMPILE_OPTIONS "-m32" LINK_OPTIONS "-m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -O1 -g -Wall -Wunused-parameter -Wno-switch -Wno-unused-function")

#For x64
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -g -Wall -Wunused-parameter -Wno-switch -Wno-unused-function")