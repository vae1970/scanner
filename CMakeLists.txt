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
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
#判断编译器类型
if(MSVC)
    #添加一个名为WITH_CRT_DLL的开关选项，
    #这样就可以在cmake-gui中或在命令行随时修改该开关选项。
    option(WITH_CRT_DLL
            "Link all libjpeg-turbo libraries and executables with the C run-time DLL (msvcr*.dll) instead of the static C run-time library (libcmt*.lib.)  The default is to use the C run-time DLL only with the libraries and executables that need it."
            FALSE)
    if(NOT WITH_CRT_DLL)
        # for循环修改所有CMAKE_<LANG>_FLAGS开关的编译选项变量，用正则表达式将/MD替换成/MT
        # Use the static C library for all build types
        foreach(var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${var} MATCHES "/MD")
                #正则表达式替换
                string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
            endif()
        endforeach()
    endif()
    add_definitions(-W3 -wd4996)
endif()