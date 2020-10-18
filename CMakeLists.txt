cmake_minimum_required(VERSION 3.16)
project(scanner)
#add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
#add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
set(CMAKE_CXX_STANDARD 14)
find_package(cpprestsdk REQUIRED)

include_directories(lib/h)
link_directories(lib/lib)

add_executable(scanner main.cpp)

#target_link_libraries(scanner PRIVATE cpprestsdk::cpprest)
#target_link_libraries(scanner )
target_link_libraries(scanner PRIVATE cpprestsdk::cpprest cpprestsdk::cpprestsdk_zlib_internal cpprestsdk::cpprestsdk_brotli_internal ${PROJECT_SOURCE_DIR}/lib/lib/MVIDCodeReader.lib)
