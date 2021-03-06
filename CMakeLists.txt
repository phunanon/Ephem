cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project("Ephem")

add_executable(ephem src/Env.cpp src/Cell.cpp src/Parser.cpp src/EVM.cpp src/linenoise/linenoise.c src/keypresses.c src/main.cpp)

# mimalloc
add_library(mimalloc STATIC IMPORTED)
set_property(TARGET mimalloc PROPERTY IMPORTED_LOCATION libmimalloc.a)
target_link_libraries(ephem PUBLIC mimalloc)

# immer
target_include_directories(ephem PUBLIC src/immer)

#For x86 (broken)
#set_target_properties(ephem PROPERTIES COMPILE_OPTIONS "-m32" LINK_OPTIONS "-m32")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lmimalloc -m32 -O3 -g -Wall -Wunused-parameter -Wno-switch -Wno-unused-function")

#For x64
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O3 -g -Wall -Wno-switch -Wno-unused-function")