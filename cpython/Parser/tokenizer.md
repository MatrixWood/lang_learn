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