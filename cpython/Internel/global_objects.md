# global objects

这段代码不多，来自`cpython/Include/internal/pycore_global_objects.h`


```c
#ifndef Py_INTERNAL_GLOBAL_OBJECTS_H
#define Py_INTERNAL_GLOBAL_OBJECTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_context.h"         // _PyContextTokenMissing
#include "pycore_gc.h"              // _PyGC_Head_UNUSED
#include "pycore_global_strings.h"  // struct _Py_global_strings
#include "pycore_hamt.h"            // PyHamtNode_Bitmap
#include "pycore_hashtable.h"       // _Py_hashtable_t
#include "pycore_typeobject.h"      // pytype_slotdef


// These would be in pycore_long.h if it weren't for an include cycle.
#define _PY_NSMALLPOSINTS           257
#define _PY_NSMALLNEGINTS           5


// Only immutable objects should be considered runtime-global.
// All others must be per-interpreter.

#define _Py_GLOBAL_OBJECT(NAME) \
    _PyRuntime.static_objects.NAME
#define _Py_SINGLETON(NAME) \
    _Py_GLOBAL_OBJECT(singletons.NAME)

struct _Py_cached_objects {
    // XXX We could statically allocate the hashtable.
    _Py_hashtable_t *interned_strings;
};

struct _Py_static_objects {
    struct {
        /* Small integers are preallocated in this array so that they
         * can be shared.
         * The integers that are preallocated are those in the range
         * -_PY_NSMALLNEGINTS (inclusive) to _PY_NSMALLPOSINTS (exclusive).
         */
        PyLongObject small_ints[_PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS];

        PyBytesObject bytes_empty;
        struct {
            PyBytesObject ob;
            char eos;
        } bytes_characters[256];

        struct _Py_global_strings strings;

        _PyGC_Head_UNUSED _tuple_empty_gc_not_used;
        PyTupleObject tuple_empty;

        _PyGC_Head_UNUSED _hamt_bitmap_node_empty_gc_not_used;
        PyHamtNode_Bitmap hamt_bitmap_node_empty;
        _PyContextTokenMissing context_token_missing;
    } singletons;
};

#define _Py_INTERP_CACHED_OBJECT(interp, NAME) \
    (interp)->cached_objects.NAME

struct _Py_interp_cached_objects {
    PyObject *interned_strings;

    /* AST */
    PyObject *str_replace_inf;

    /* object.__reduce__ */
    PyObject *objreduce;
    PyObject *type_slots_pname;
    pytype_slotdef *type_slots_ptrs[MAX_EQUIV];

    /* TypeVar and related types */
    PyTypeObject *generic_type;
    PyTypeObject *typevar_type;
    PyTypeObject *typevartuple_type;
    PyTypeObject *paramspec_type;
    PyTypeObject *paramspecargs_type;
    PyTypeObject *paramspeckwargs_type;
    PyTypeObject *constevaluator_type;
};

#define _Py_INTERP_STATIC_OBJECT(interp, NAME) \
    (interp)->static_objects.NAME
#define _Py_INTERP_SINGLETON(interp, NAME) \
    _Py_INTERP_STATIC_OBJECT(interp, singletons.NAME)

struct _Py_interp_static_objects {
    struct {
        int _not_used;
        // hamt_empty is here instead of global because of its weakreflist.
        _PyGC_Head_UNUSED _hamt_empty_gc_not_used;
        PyHamtObject hamt_empty;
        PyBaseExceptionObject last_resort_memory_error;
    } singletons;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_H */
```

这段代码是 Python 解释器内部使用的头文件，定义了一些全局和解释器级别的缓存对象。它的主要功能是声明和组织这些对象，以便在解释器的不同部分中使用。以下是对代码中关键部分的分析：

1. **宏定义和条件编译**：
   - `#ifndef Py_INTERNAL_GLOBAL_OBJECTS_H` 和对应的 `#endif`：防止头文件被多次包含。
   - `#ifdef __cplusplus` 和 `extern "C"`：确保 C++ 编译器按照 C 语言的链接规则处理这些声明。
   - `#ifndef Py_BUILD_CORE`：确保这个头文件只在构建 Python 核心时被包含，防止在扩展模块中被错误地包含。

2. **包含其他头文件**：
   - 包含了其他一些内部使用的头文件，如 `pycore_context.h`、`pycore_gc.h` 等，这些文件定义了 Python 核心功能所需的类型和函数。

3. **小整数缓存**：
   - `_PY_NSMALLPOSINTS` 和 `_PY_NSMALLNEGINTS` 定义了小整数缓存的大小。
   - Python 预先创建并缓存了一定范围内的小整数对象，以便重复使用，减少内存分配的开销。

4. **全局对象宏定义**：
   - `_Py_GLOBAL_OBJECT` 和 `_Py_SINGLETON` 宏用于访问全局对象和单例。

5. **缓存对象结构体**：
   - `struct _Py_cached_objects`：定义了一个结构体，用于存储解释器级别的缓存对象，如字符串的缓存。
   - `struct _Py_static_objects`：定义了一个结构体，用于存储全局静态对象，如预分配的小整数数组、空字节对象、字符字节对象、全局字符串对象等。

6. **解释器级别的缓存对象**：
   - `_Py_INTERP_CACHED_OBJECT` 宏用于访问解释器级别的缓存对象。
   - `struct _Py_interp_cached_objects`：定义了一个结构体，包含了解释器级别的缓存对象，如内部字符串、AST 相关对象、类型槽指针等。

7. **解释器级别的静态对象**：
   - `_Py_INTERP_STATIC_OBJECT` 和 `_Py_INTERP_SINGLETON` 宏用于访问解释器级别的静态对象和单例。
   - `struct _Py_interp_static_objects`：定义了一个结构体，包含了解释器级别的静态单例对象，如空的哈希数组映射表（HAMT）、最后的内存错误异常对象等。

总的来说，这段代码是 Python 解释器内部使用的，用于定义和管理全局和解释器级别的缓存对象，以提高解释器的性能和效率。这些对象通常是不可变的，以确保它们可以安全地在多个上下文中共享。

这里我们根据`cpython/Include/internal/pycore_global_strings.h`中的

```c
#define _Py_ID(NAME) \
     (_Py_SINGLETON(strings.identifiers._py_ ## NAME._ascii.ob_base))
#define _Py_STR(NAME) \
     (_Py_SINGLETON(strings.literals._py_ ## NAME._ascii.ob_base))
```

来具体的看一下代码。

`_Py_ID`在`cpython/Objects/boolobject.c`中有用到：

```c
/* We define bool_repr to return "False" or "True" */

static PyObject *
bool_repr(PyObject *self)
{
    return self == Py_True ? &_Py_ID(True) : &_Py_ID(False);
}
```

`_Py_ID` 是一个宏，它用于获取 Python 解释器中预先创建和缓存的标识符对象。这些标识符对象是 Python 字符串对象，通常用于核心解释器代码中的标识符和关键字，以避免重复创建相同的字符串对象。

让我们逐步分解 `_Py_ID` 宏的调用链和它使用的数据：

1. `_Py_SINGLETON` 宏：
   - `_Py_SINGLETON` 宏用于访问 `_PyRuntime` 结构体中的 `singletons` 字段，该字段包含了一系列的单例对象。
   - `_Py_SINGLETON` 宏本身使用 `_Py_GLOBAL_OBJECT` 宏来访问全局对象。

2. `strings.identifiers` 字段：
   - 在 `_Py_SINGLETON` 宏中，`NAME` 被指定为 `strings.identifiers`，这是 `struct _Py_global_strings` 结构体的一个字段，它包含了所有预先创建的标识符字符串对象。

3. `_py_ ## NAME`：
   - 这是宏连接操作，它将 `_py_` 前缀和传递给 `_Py_ID` 宏的 `NAME` 参数连接起来。例如，如果 `NAME` 是 `True`，则结果是 `_py_True`。

4. `_ascii.ob_base` 字段：
   - `_py_ ## NAME` 是 `struct _Py_global_strings` 中的一个字段，它是一个 `_Py_Identifier` 结构体，该结构体包含了一个 ASCII 编码的字符串对象。
   - `_ascii` 字段是 `_Py_Identifier` 结构体中的一个字段，它是一个 `PyASCIIObject` 结构体，表示 ASCII 编码的字符串。
   - `ob_base` 是 `PyASCIIObject` 结构体中的一个字段，它是一个 `PyObject` 结构体，表示 Python 对象的基础结构。

5. `_Py_SINGLETON(strings.identifiers._py_ ## NAME._ascii.ob_base)`：
   - 这个宏最终展开为访问 `_PyRuntime` 结构体中的 `singletons` 字段，然后进一步访问 `strings.identifiers` 中的特定标识符对象，最后获取该对象的基础 `PyObject` 结构体。

这里
```c
#define _Py_ID(NAME) \
     (_Py_SINGLETON(strings.identifiers._py_ ## NAME._ascii.ob_base))
```
的ob_base是一个PyObject
但是
```c
#define _Py_GLOBAL_OBJECT(NAME) \
    _PyRuntime.static_objects.NAME
#define _Py_SINGLETON(NAME) \
    _Py_GLOBAL_OBJECT(singletons.NAME)
```
_Py_SINGLETON里传入的是NAME, 实际上是_PyRuntime.static_objects.singletons.strings.identifiers._py_ ## NAME._ascii.ob_base，所以最后获取该对象的基础 `PyObject` 结构体。

而这个结构体在`cpython/Include/internal/pycore_runtime_init.h`里面由`_Py_str_identifiers_INIT`全局初始化。

总结来说，`_Py_ID` 宏通过一系列的结构体字段访问和获取一个预先创建的标识符字符串对象。这个对象是一个 Python 字符串对象，它已经被创建并缓存在全局单例结构体中，以便在解释器的核心代码中高效地使用。