# tokenizer

tokenizer的入口有四个：
```c
struct tok_state *_PyTokenizer_FromString(const char *, int, int);
struct tok_state *_PyTokenizer_FromUTF8(const char *, int, int);
struct tok_state *_PyTokenizer_FromReadline(PyObject*, const char*, int, int);
struct tok_state *_PyTokenizer_FromFile(FILE *, const char*,
                                              const char *, const char *);
```

如果执行`python xxx.py`命令的话，则从`cpython/Parser/tokenizer/file_tokenizer.c`中的`_PyTokenizer_FromFile`进入。

分析该函数：

```c
/* Set up tokenizer for file */
struct tok_state *
_PyTokenizer_FromFile(FILE *fp, const char* enc,
                      const char *ps1, const char *ps2)
{
    struct tok_state *tok = _PyTokenizer_tok_new();
    if (tok == NULL)
        return NULL;
    if ((tok->buf = (char *)PyMem_Malloc(BUFSIZ)) == NULL) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->cur = tok->inp = tok->buf;
    tok->end = tok->buf + BUFSIZ;
    tok->fp = fp;
    tok->prompt = ps1;
    tok->nextprompt = ps2;
    if (ps1 || ps2) {
        tok->underflow = &tok_underflow_interactive;
    } else {
        tok->underflow = &tok_underflow_file;
    }
    if (enc != NULL) {
        /* Must copy encoding declaration since it
           gets copied into the parse tree. */
        tok->encoding = _PyTokenizer_new_string(enc, strlen(enc), tok);
        if (!tok->encoding) {
            _PyTokenizer_Free(tok);
            return NULL;
        }
        tok->decoding_state = STATE_NORMAL;
    }
    return tok;
}
```

这个函数`_PyTokenizer_FromFile`是用来初始化一个文件输入的分词器（tokenizer）的。它配置了一个`tok_state`结构体，以便从给定的文件流`fp`中读取和分词Python源代码。下面是函数的详细解释和参数的含义：

- `FILE *fp`: 这是一个指向C标准库中文件流的指针，分词器将从这个文件流中读取输入。

- `const char* enc`: 这是一个指向字符数组的指针，表示输入文件的字符编码。如果非空，分词器将使用这个编码来解码输入。

- `const char *ps1`, `const char *ps2`: 这两个参数是用于交互式提示的字符串。`ps1`通常是主提示符（例如Python的`>>>`），而`ps2`通常是次级提示符（例如Python的`...`），用于多行输入。

函数的步骤如下：

1. `_PyTokenizer_tok_new()`: 创建一个新的`tok_state`结构体实例。

2. 如果`tok`为`NULL`，说明内存分配失败，函数返回`NULL`。

3. 为`tok->buf`分配`BUFSIZ`大小的内存。`BUFSIZ`是标准C库中定义的一个常量，通常是一个合理的缓冲区大小，用于I/O操作。

4. 如果内存分配失败，释放`tok`并返回`NULL`。

5. 初始化`tok->cur`、`tok->inp`和`tok->end`指针，分别指向缓冲区的开始、当前输入位置和缓冲区的末尾。

6. 设置`tok->fp`为传入的文件指针`fp`。

7. 设置`tok->prompt`和`tok->nextprompt`为传入的提示符字符串`ps1`和`ps2`。

8. 根据是否有提示符字符串，设置`tok->underflow`函数指针。如果`ps1`或`ps2`非空，使用交互式下溢函数`tok_underflow_interactive`；否则，使用文件下溢函数`tok_underflow_file`。

9. 如果提供了编码`enc`，则复制这个编码字符串到分词器状态中，并设置`tok->decoding_state`为`STATE_NORMAL`。

10. 返回初始化好的`tok_state`结构体指针。

这个函数主要用于Python解释器内部，用于从文件中读取和分词Python代码。通过这个函数的配置，分词器可以正确地处理文件输入，包括编码和交互式提示符。


关于tok_state结构体:

结构体`tok_state`是C语言中对于解析Python源代码的分词器（tokenizer）的内部状态的定义。这种状态类型通常在分析语言时使用，比如构建编译器或解释器时。下面是每个字段的简要说明：

- `char *buf`: 指向输入缓冲区的指针，如果`fp`或者`readline`非空，则由`malloc`分配内存。
- `char *cur`: 指向缓冲区中下一个要处理的字符。
- `char *inp`: 指向缓冲区中数据结束的位置。
- `int fp_interactive`: 表示相关的文件描述符是否为交互式。
- `char *interactive_src_start`: 在交互式模式下，已经解析的源代码的开始位置。
- `char *interactive_src_end`: 在交互式模式下，已经解析的源代码的结束位置。
- `const char *end`: 如果`buf`非空，则是输入缓冲区的结束指针。
- `const char *start`: 如果非空，是当前标记（token）的开始位置。
- `int done`: 状态码，通常是`E_OK`，在文件结束时是`E_EOF`，其他值表示错误代码。如果`done`不等于`E_OK`，那么`cur`必须等于`inp`。
- `FILE *fp`: 代表除了缓冲区之外剩余的输入的文件指针，如果正在对字符串进行分词，则为`NULL`。
- `int tabsize`: 表示制表符的大小（空格数）。
- `int indent`: 当前缩进的级别。
- `int indstack[MAXINDENT]`: 缩进级别的堆栈。
- `int atbol`: 如果在新行的开头，则非零。
- `int pendin`: 待处理的缩进数，如果大于0，则表示增加的缩进；如果小于0，则表示减少的缩进。
- `const char *prompt`, `*nextprompt`: 用于交互式提示时的提示字符串。
- `int lineno`: 当前的行号。
- `int first_lineno`: 单行或多行字符串表达式的第一行（参见issue 16806）。
- `int starting_col_offset`: 标记（token）开始位置的列偏移。
- `int col_offset`: 当前列偏移。
- `int level`: 圆括号、方括号和花括号的嵌套级别。用于在它们内部允许自由续行。
- `char parenstack[MAXLEVEL]`: 用于保持圆括号、方括号、花括号开闭状态的栈。
- `int parenlinenostack[MAXLEVEL]`, `parencolstack[MAXLEVEL]`: 与`parenstack`同步的行号栈和列偏移栈。
- `PyObject *filename`: 代表被分词内容的源文件名。
- `enum decoding_state`: 定义了解码状态的枚举类型。
- `int decoding_erred`: 标记解码过程是否出错。
- `char *encoding`: 源文件的编码。
- `int cont_line`: 表示是否处于续行中。
- `const char *line_start`: 指向当前行开始的位置。
- `const char *multi_line_start`: 指向单行或多行字符串表达式的第一行的开始位置。
- `PyObject *decoding_readline`: 用于解码的`open(...).readline`函数。
- `PyObject *decoding_buffer`: 解码缓冲区。
- `PyObject *readline`: `readline()`函数。
- `const char *enc`: 当前字符串的编码。
- `char *str`: 正在被分词的源字符串（如果使用字符串进行分词）。
- `char *input`: 分词器转换换行符后的字符串副本。
- `int type_comments`: 表示是否查找类型注释。
- `enum interactive_underflow_t`: 在交互模式下，在需要更多标记时如何处理。
- `int (*)(struct tok_state *)`: 当缓冲区为空且需要重新填充时，应调用的函数。
- `int report_warnings`: 是否报告警告。
- `tokenizer_mode tok_mode_stack[MAXFSTRINGLEVEL]`: 处理Python形式字符串时的模式栈。
- `int tok_mode_stack_index`: 模式栈的当前索引。
- `int tok_extra_tokens`: 额外的标记数量。
- `int comment_newline`: 表示注释后是否加了新行。
- `int implicit_newline`: 表示是否在隐式新行（例如，一条语句后，例如`print(x)`之后，虽然没有显式换行，但逻辑上应认为新的一行开始）。

在宏启用调试模式时，可能会存在一些额外的调试字段：

- `int debug`: 如果定义了Py_DEBUG，则为一个调试标志。 

每个字段都是在分词过程中会用到的，分词器使用这些状态信息分析源代码，例如确定下一个标记，处理缩进，父级深度（用于括号匹配）和行列位置等。