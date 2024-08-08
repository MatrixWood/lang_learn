CPython 分析记录

版本：

Python 3.14.0a0 (heads/main-dirty:d0b92dd5ca, Aug  5 2024, 10:37:42) [GCC 13.2.0] on linux

build 过程：

```sh
mkdir debug
cd debug
bear -- ../configure --with-pydebug
bear -- make -j8
```