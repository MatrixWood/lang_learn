# Compile

在前端生成出ast之后，即在`cpython/Python/pythonrun.c`文件中的`pyrun_file`函数里：

```c
static PyObject *
pyrun_file(FILE *fp, PyObject *filename, int start, PyObject *globals,
           PyObject *locals, int closeit, PyCompilerFlags *flags)
{
    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod;
    mod = _PyParser_ASTFromFile(fp, filename, NULL, start, NULL, NULL,
                                flags, NULL, arena);

    if (closeit) {
        fclose(fp);
    }

    PyObject *ret;
    if (mod != NULL) {
        ret = run_mod(mod, filename, globals, locals, flags, arena, NULL, 0);
    }
    else {
        ret = NULL;
    }
    _PyArena_Free(arena);

    return ret;
}
```

生成的ast的top-level是mod_ty，那么得到这个类型的语法树后，在下一步去执行`run_mod`。

在`run_mod`中，代码如下：

```c
static PyObject *
run_mod(mod_ty mod, PyObject *filename, PyObject *globals, PyObject *locals,
            PyCompilerFlags *flags, PyArena *arena, PyObject* interactive_src,
            int generate_new_source)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject* interactive_filename = filename;
    if (interactive_src) {
        PyInterpreterState *interp = tstate->interp;
        if (generate_new_source) {
            interactive_filename = PyUnicode_FromFormat(
                "%U-%d", filename, interp->_interactive_src_count++);
        } else {
            Py_INCREF(interactive_filename);
        }
        if (interactive_filename == NULL) {
            return NULL;
        }
    }

    PyCodeObject *co = _PyAST_Compile(mod, interactive_filename, flags, -1, arena);
    if (co == NULL) {
        if (interactive_src) {
            Py_DECREF(interactive_filename);
        }
        return NULL;
    }

    if (interactive_src) {
        PyObject *linecache_module = PyImport_ImportModule("linecache");

        if (linecache_module == NULL) {
            Py_DECREF(co);
            Py_DECREF(interactive_filename);
            return NULL;
        }

        PyObject *print_tb_func = PyObject_GetAttrString(linecache_module, "_register_code");

        if (print_tb_func == NULL) {
            Py_DECREF(co);
            Py_DECREF(interactive_filename);
            Py_DECREF(linecache_module);
            return NULL;
        }

        if (!PyCallable_Check(print_tb_func)) {
            Py_DECREF(co);
            Py_DECREF(interactive_filename);
            Py_DECREF(linecache_module);
            Py_DECREF(print_tb_func);
            PyErr_SetString(PyExc_ValueError, "linecache._register_code is not callable");
            return NULL;
        }

        PyObject* result = PyObject_CallFunction(
            print_tb_func, "OOO",
            interactive_filename,
            interactive_src,
            filename
        );

        Py_DECREF(interactive_filename);

        Py_DECREF(linecache_module);
        Py_XDECREF(print_tb_func);
        Py_XDECREF(result);
        if (!result) {
            Py_DECREF(co);
            return NULL;
        }
    }

    if (_PySys_Audit(tstate, "exec", "O", co) < 0) {
        Py_DECREF(co);
        return NULL;
    }

    PyObject *v = run_eval_code_obj(tstate, co, globals, locals);
    Py_DECREF(co);
    return v;
}
```

- 获取当前线程的状态对象。
- 如果提供了交互式源码(`interactive_src`不为空)，会根据`generate_new_source`决定是否为文件生成新的名称，用于交互模式下的问题跟踪。
- 使用`_PyAST_Compile`函数将AST编译成PyCodeObject，如果失败，返回`NULL`。
- 在交互式模式下，注册编译后的代码到`linecache`模块，这样在出错时能显示正确的源码行。
- 使用内置的`_PySys_Audit`进行安全审计，确保执行符合安全要求。
- 使用`run_eval_code_obj`函数实际执行编译后的字节码并完成解释执行。
- 执行完成后释放编译的代码对象，并返回执行结果或在错误发生时为`NULL`。

在这个函数中我们关注`_PyAST_Compile`和`run_eval_code_obj`两部分的功能，重点关注`_PyAST_Compile`：

```c
PyCodeObject *
_PyAST_Compile(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
               int optimize, PyArena *arena)
{
    assert(!PyErr_Occurred());
    struct compiler *c = new_compiler(mod, filename, pflags, optimize, arena);
    if (c == NULL) {
        return NULL;
    }

    PyCodeObject *co = compiler_mod(c, mod);
    compiler_free(c);
    assert(co || PyErr_Occurred());
    return co;
}
```
这段代码定义了一个函数 `_PyAST_Compile`，它的作用是将一个抽象语法树（AST）编译成 Python 字节码对象，即 `PyCodeObject`。


1. 函数参数：
   - `mod`: AST对象，代表要编译的Python模块。
   - `filename`: 编译源代码的文件名，用于错误报告。
   - `pflags`: 编译标志的指针，可能包含影响编译的选项。
   - `optimize`: 优化级别，通常是0、1或2，分别对应不同的优化策略。
   - `arena`: 用于分配内存的区域，编译过程中所有的内存分配都应该在这个区域中进行。

2. 函数主体：
   - 首先，使用 `assert` 确保在函数开始执行时没有未处理的 Python 错误。
   - 调用 `new_compiler` 函数创建一个新的编译器实例 `c`。如果创建失败，返回 `NULL`。
   - 使用 `compiler_mod` 函数将 AST 编译成 `PyCodeObject`。这个函数会处理实际的编译过程。
   - 释放编译器实例 `c` 的资源，因为编译工作已经完成。
   - 使用 `assert` 确保编译结果 `co` 是有效的，或者如果编译失败，确保有一个 Python 错误被设置。

3. 返回值：
   - 如果编译成功，返回指向 `PyCodeObject` 的指针。
   - 如果编译失败，返回 `NULL`。

这个函数是 Python 编译过程的一部分，它将高级的 Python 代码（表示为 AST）转换为低级的字节码，字节码随后可以被 Python 虚拟机执行。这个过程中涉及到错误检查和内存管理，确保编译过程的正确性和效率。

```c
static PyCodeObject *
compiler_mod(struct compiler *c, mod_ty mod)
{
    PyCodeObject *co = NULL;
    int addNone = mod->kind != Expression_kind;
    if (compiler_enter_anonymous_scope(c, mod) < 0) {
        return NULL;
    }
    if (compiler_codegen(c, mod) < 0) {
        goto finally;
    }
    co = optimize_and_assemble(c, addNone);
finally:
    compiler_exit_scope(c);
    return co;
}
```

这段代码定义了一个静态函数 `compiler_mod`，它是 Python 解释器内部用于将一个 AST 模块编译成 Python 字节码对象 `PyCodeObject` 的函数。这个函数是编译过程的一部分，它处理不同类型的 Python 代码块（如整个模块、一个表达式或一个语句）。

1. 函数参数：
   - `c`: 指向 `struct compiler` 的指针，这是编译器的状态和上下文信息。
   - `mod`: 指向 `mod_ty` 的指针，这是一个 AST 模块节点。

2. 函数主体：
   - 初始化 `PyCodeObject` 指针 `co` 为 `NULL`。
   - 判断是否需要在编译的字节码末尾添加一个 `None`。如果模块类型不是 `Expression_kind`（即不是一个单独的表达式），则需要添加 `None`。
   - 调用 `compiler_enter_anonymous_scope` 函数进入一个匿名的编译作用域。如果这个操作失败（返回值小于0），则直接返回 `NULL`。
   - 调用 `compiler_codegen` 函数进行实际的代码生成。如果代码生成失败（返回值小于0），则跳转到 `finally` 标签进行清理。
   - 如果代码生成成功，调用 `optimize_and_assemble` 函数对生成的代码进行优化和汇编，生成最终的 `PyCodeObject`。
   - `finally` 标签用于退出编译作用域，无论代码生成是否成功。这是一个清理步骤，确保所有的资源都被正确地释放。

3. 返回值：
   - 如果编译成功，返回指向 `PyCodeObject` 的指针。
   - 如果编译失败，返回 `NULL`。

在`compiler_enter_anonymous_scope`中，
```
static int
compiler_enter_anonymous_scope(struct compiler* c, mod_ty mod)
{
    _Py_DECLARE_STR(anon_module, "<module>");
    RETURN_IF_ERROR(
        compiler_enter_scope(c, &_Py_STR(anon_module), COMPILER_SCOPE_MODULE,
                             mod, 1, NULL));
    return SUCCESS;
}
```

关注`compiler_enter_scope`：

```c
static int
compiler_enter_scope(struct compiler *c, identifier name, int scope_type,
                     void *key, int lineno, PyObject *private)
{
    location loc = LOCATION(lineno, lineno, 0, 0);

    struct compiler_unit *u;

    u = (struct compiler_unit *)PyMem_Calloc(1, sizeof(struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return ERROR;
    }
    u->u_scope_type = scope_type;
    u->u_metadata.u_argcount = 0;
    u->u_metadata.u_posonlyargcount = 0;
    u->u_metadata.u_kwonlyargcount = 0;
    u->u_ste = _PySymtable_Lookup(c->c_st, key);
    if (!u->u_ste) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_name = Py_NewRef(name);
    u->u_metadata.u_varnames = list2dict(u->u_ste->ste_varnames);
    if (!u->u_metadata.u_varnames) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_cellvars = dictbytype(u->u_ste->ste_symbols, CELL, DEF_COMP_CELL, 0);
    if (!u->u_metadata.u_cellvars) {
        compiler_unit_free(u);
        return ERROR;
    }
    if (u->u_ste->ste_needs_class_closure) {
        /* Cook up an implicit __class__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        res = dict_add_o(u->u_metadata.u_cellvars, &_Py_ID(__class__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    if (u->u_ste->ste_needs_classdict) {
        /* Cook up an implicit __classdict__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        res = dict_add_o(u->u_metadata.u_cellvars, &_Py_ID(__classdict__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }

    u->u_metadata.u_freevars = dictbytype(u->u_ste->ste_symbols, FREE, DEF_FREE_CLASS,
                               PyDict_GET_SIZE(u->u_metadata.u_cellvars));
    if (!u->u_metadata.u_freevars) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_metadata.u_fasthidden = PyDict_New();
    if (!u->u_metadata.u_fasthidden) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_nfblocks = 0;
    u->u_in_inlined_comp = 0;
    u->u_metadata.u_firstlineno = lineno;
    u->u_metadata.u_consts = PyDict_New();
    if (!u->u_metadata.u_consts) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_names = PyDict_New();
    if (!u->u_metadata.u_names) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_deferred_annotations = NULL;
    if (scope_type == COMPILER_SCOPE_CLASS) {
        u->u_static_attributes = PySet_New(0);
        if (!u->u_static_attributes) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    else {
        u->u_static_attributes = NULL;
    }

    u->u_instr_sequence = (instr_sequence*)_PyInstructionSequence_New();
    if (!u->u_instr_sequence) {
        compiler_unit_free(u);
        return ERROR;
    }

    /* Push the old compiler_unit on the stack. */
    if (c->u) {
        PyObject *capsule = PyCapsule_New(c->u, CAPSULE_NAME, NULL);
        if (!capsule || PyList_Append(c->c_stack, capsule) < 0) {
            Py_XDECREF(capsule);
            compiler_unit_free(u);
            return ERROR;
        }
        Py_DECREF(capsule);
        if (private == NULL) {
            private = c->u->u_private;
        }
    }

    u->u_private = Py_XNewRef(private);

    c->u = u;

    c->c_nestlevel++;

    if (u->u_scope_type == COMPILER_SCOPE_MODULE) {
        loc.lineno = 0;
    }
    else {
        RETURN_IF_ERROR(compiler_set_qualname(c));
    }
    ADDOP_I(c, loc, RESUME, RESUME_AT_FUNC_START);

    return SUCCESS;
}
```

这个函数 `compiler_enter_scope` 是 Python 解释器内部用于进入一个新的编译作用域的函数。它负责设置编译器状态以编译一个新的代码块（如函数、类或模块）。以下是逐行的详细解释：

1. 函数参数：
   - `c`: 当前编译器的上下文。
   - `name`: 当前作用域的名称。
   - `scope_type`: 作用域的类型（如模块、类或函数）。
   - `key`: 用于在符号表中查找作用域实体的键。
   - `lineno`: 代码块开始的行号。
   - `private`: 指示是否有私有变量。

2. 函数主体：
   - `location loc = LOCATION(lineno, lineno, 0, 0);` 初始化一个位置信息结构体，用于错误报告和字节码注释。
   
   - `struct compiler_unit *u;` 声明一个指向编译器单元的指针。
   
   - 分配内存给新的编译器单元 `u`，并检查内存分配是否成功。如果失败，设置内存错误并返回错误代码。
   
   - 初始化新编译器单元的各个字段，包括作用域类型、参数计数、关键字参数计数等。
   
   - 通过 `_PySymtable_Lookup` 在符号表中查找当前作用域的条目。如果查找失败，释放编译器单元并返回错误代码。
   
   - 将作用域名称的新引用赋给编译器单元，并将变量名列表转换为字典。
   
   - 根据符号表中的信息，创建并填充 `cellvars` 和 `freevars` 字典。
   
   - 如果当前作用域需要一个隐式的 `__class__` 或 `__classdict__` 单元，将其添加到 `cellvars` 字典中。
   
   - 创建一个新的字典 `u->u_metadata.u_fasthidden`，用于存储快速访问的隐藏变量。
   
   - 初始化其他编译器单元字段，如行号、常量字典、名称字典等。
   
   - 如果当前作用域是类作用域，创建一个集合来存储静态属性。
   
   - 创建一个新的指令序列 `u->u_instr_sequence`。
   
   - 如果当前编译器上下文已经有一个编译器单元，将其封装在一个 PyCapsule 中并推入编译器的栈中。
   
   - 设置当前编译器单元的私有变量引用。
   
   - 将新的编译器单元 `u` 设置为当前编译器上下文的单元，并增加嵌套级别。
   
   - 如果当前作用域是模块作用域，设置行号为0；否则，设置合格的名称并添加一个 `RESUME` 操作码。

3. 错误处理和资源管理：
   - 在函数的多个地方，如果遇到错误，会释放已分配的编译器单元 `u` 并返回错误代码。
   - 使用宏 `RETURN_IF_ERROR` 来检查错误并在必要时返回。
   - 使用 `PyMem_Calloc`、`Py_NewRef`、`Py_XNewRef`、`Py_DECREF` 等函数和宏来管理内存和引用计数。

接下来看`compiler_codegen`中逻辑：

```c
static int
compiler_codegen(struct compiler *c, mod_ty mod)
{
    assert(c->u->u_scope_type == COMPILER_SCOPE_MODULE);
    switch (mod->kind) {
    case Module_kind: {
        asdl_stmt_seq *stmts = mod->v.Module.body;
        if (compiler_body(c, start_location(stmts), stmts) < 0) {
            return ERROR;
        }
        break;
    }
    case Interactive_kind: {
        c->c_interactive = 1;
        asdl_stmt_seq *stmts = mod->v.Interactive.body;
        if (compiler_body(c, start_location(stmts), stmts) < 0) {
            return ERROR;
        }
        break;
    }
    case Expression_kind: {
        VISIT(c, expr, mod->v.Expression.body);
        break;
    }
    default: {
        PyErr_Format(PyExc_SystemError,
                     "module kind %d should not be possible",
                     mod->kind);
        return ERROR;
    }}
    return SUCCESS;
}
```

这个函数 `compiler_codegen` 是 Python 解释器内部用于生成字节码的函数。它根据提供的模块类型（`mod_ty`）生成相应的代码。以下是逐行的详细解释：

1. 函数定义和参数：
   - `static int`: 函数的返回类型是整型，`static` 表示这个函数的作用域限定在当前源文件内。
   - `compiler_codegen`: 函数名。
   - `struct compiler *c`: 指向编译器状态的指针。
   - `mod_ty mod`: 指向模块结构体的指针，这个结构体描述了要编译的模块。

2. 函数主体：
   - `assert(c->u->u_scope_type == COMPILER_SCOPE_MODULE);`: 断言当前编译器单元的作用域类型是模块。这是一个安全检查，确保函数不会在错误的上下文中被调用。

   - `switch (mod->kind) { ... }`: 根据模块的类型（`mod->kind`），执行不同的代码生成逻辑。

3. `Module_kind` 分支：
   - 获取模块的语句序列 `stmts`。
   - 调用 `compiler_body` 函数来编译这些语句。如果编译失败，返回错误代码。

4. `Interactive_kind` 分支：
   - 设置编译器状态 `c->c_interactive` 为 1，表示正在编译交互式命令。
   - 获取交互式命令的语句序列 `stmts`。
   - 调用 `compiler_body` 函数来编译这些语句。如果编译失败，返回错误代码。

5. `Expression_kind` 分支：
   - 使用 `VISIT` 宏来访问并编译表达式 `mod->v.Expression.body`。这个宏是一个通用的访问者模式实现，用于遍历和编译抽象语法树（AST）的节点。

6. `default` 分支：
   - 如果 `mod->kind` 不是上述任何一种已知类型，使用 `PyErr_Format` 设置一个系统错误，并返回错误代码。

7. 函数结束：
   - 如果所有的分支都成功执行，函数返回 `SUCCESS`，表示代码生成成功完成。

这个函数的作用是根据提供的模块类型，调用相应的编译函数来生成字节码。它处理了模块、交互式会话和单个表达式的编译，这些都是 Python 程序可能的顶层结构。错误处理在这里也很重要，任何编译步骤的失败都会导致整个函数返回错误代码。

