create_subdirectory_options(SIMPLELANG TOOL)

set(LLVM_LINK_COMPONENTS
  Support
)

add_simplelang_tool(simplelanglexer
  driver.cpp
)

target_link_libraries(simplelanglexer
  PRIVATE
  simplelang
)