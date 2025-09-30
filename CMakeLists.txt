cmake_minimum_required(VERSION 3.16)
project(varser_project LANGUAGES C CXX)

add_subdirectory(user)
# kernel module: built separately with make in kernel/ (kbuild)
message(STATUS "Kernel module built via kernel/Makefile (use `make` inside kernel/)")
