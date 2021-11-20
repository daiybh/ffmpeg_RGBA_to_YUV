cmake_minimum_required (VERSION 3.8)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED True)
project ("AjaLibrary")
find_package(ffmpeg)
if(FFMPEG_FOUND)
message(status "ffmpeg found  ${FFMPEG_INCLUDE_DIRS}")
endif()
add_executable(test "main2.cpp")

target_link_libraries(test  PUBLIC ${FFMPEG_LIBRARIES})
target_include_directories(test PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<BUILD_INTERFACE:${FFMPEG_INCLUDE_DIRS}>)


