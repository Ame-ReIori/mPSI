cmake_minimum_required(VERSION 3.21)

project(mpsi)

include("cmake/mpsiPreamble.cmake")
include("cmake/mpsiBuildOptions.cmake")
include("cmake/mpsiDepHelper.cmake")

add_subdirectory(src)
