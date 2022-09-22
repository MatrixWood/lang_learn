cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(SimpleLang)

  find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
  list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})

  include(AddLLVM)
  include(HandleLLVMOptions)

  include_directories("${LLVM_BINARY_DIR}/include" "${LLVM_INCLUDE_DIR}")
  link_directories("${LLVM_LIBRARY_DIR}")

  set(SIMPLELANG_BUILT_STANDALONE 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(SIMPLELANG_VERSION_STRING "0.1")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/simplelang/include/version.inc.in
  ${CMAKE_CURRENT_BINARY_DIR}/simplelang/include/version.inc
)

include(AddSimpleLang)

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/simplelang/include
  ${CMAKE_CURRENT_SOURCE_DIR}/simplelang/include
)

add_subdirectory(simplelang/src)
add_subdirectory(simplelang/tools)
