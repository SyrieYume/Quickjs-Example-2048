cmake_minimum_required(VERSION 3.10)

project(quickjs-example LANGUAGES C)

# 设置编译的输出目录
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})


# 设置默认的构建类型
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No Build Type selected, Default to MinSizeRel")
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()


# 获取quickjs的版本号
file(STRINGS "./quickjs/VERSION" QUICKJS_VERSION_CONTENT)
list(GET QUICKJS_VERSION_CONTENT 0 QUICKJS_VERSION)
message(STATUS "Quickjs Version: ${QUICKJS_VERSION}")


# 编译 quickjs
# 根据 DYNAMIC_LINK_QUICKJS 参数决定程序是动态链接quickjs or 静态链接quickjs
if(DYNAMIC_LINK_QUICKJS)
    set(QUICKJS_LINK_TYPE "SHARED")
else()
    set(QUICKJS_LINK_TYPE "STATIC")
endif()

message(STATUS "Quickjs Link Type: ${QUICKJS_LINK_TYPE}")


add_library(quickjs ${QUICKJS_LINK_TYPE}
    "./quickjs/cutils.c"
    "./quickjs/libregexp.c"
    "./quickjs/libunicode.c"
    "./quickjs/dtoa.c"
    "./quickjs/quickjs.c"
    "./quickjs/quickjs-libc.c"
)

target_link_libraries(quickjs m)

target_compile_definitions(quickjs PRIVATE CONFIG_VERSION="${QUICKJS_VERSION}")

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_link_options(quickjs PRIVATE "-static-libgcc")
endif()

if(WIN32)
    add_library(pthread-for-win32 STATIC "./pthread/pthread-for-win32.c")

    target_link_libraries(quickjs pthread-for-win32)

    target_compile_definitions(quickjs PRIVATE 
        CONFIG_WIN32 
        __USE_MINGW_ANSI_STDIO
    )

    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
else()
    target_link_libraries(quickjs pthread)
endif()


# 编译程序 quickjs-example.exe
add_executable(quickjs-example)

target_sources(quickjs-example PRIVATE "./example.c")

target_link_libraries(quickjs-example quickjs)


# 将 example.js 复制到release目录下
configure_file("./example.js" "${OUTPUT_DIR}/example.js" COPYONLY)