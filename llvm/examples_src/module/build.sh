clang++ -w -o HelloModule `llvm-config --cxxflags --ldflags --system-libs --libs core` Module.cpp
