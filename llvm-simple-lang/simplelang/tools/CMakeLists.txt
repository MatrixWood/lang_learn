create_subdirectory_options(SIMPLELANG TOOL)

set(LLVM_LINK_COMPONENTS
  Support
)

add_simplelang_tool(simplelangtools
  driver.cpp
)

target_link_libraries(simplelangtools
  PRIVATE
  simplelang
)