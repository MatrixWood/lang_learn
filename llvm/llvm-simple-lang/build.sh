mkdir build
cd build
cmake .. -G ninja
ninja
cd simplelang/tools
./simplelanglexer "with abc,xyz: (abc+xyz)*3 - 10/abc"