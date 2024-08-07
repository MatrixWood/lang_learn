# Guide to the parser

> https://devguide.python.org/internals/parser/

## 摘要

CPython中的解析器目前是一个PEG（Parsing Expression Grammar，解析表达式语法）解析器。解析器的第一个版本曾经是一个基于LL(1)的解析器，它是CPython最古老的部分之一，在被PEG解析器取代之前实现。特别是，当前的PEG解析器和旧的LL(1)解析器都是解析器生成器的输出。这意味着编写解析器的方式是将Python语言的语法描述提供给一个特殊的程序（解析器生成器），然后输出解析器。因此，改变Python语言的方式是通过修改语法文件，开发者很少需要与解析器生成器本身互动，除了使用它来生成解析器。

## PEG解析器的工作原理

PEG解析器的语法（如当前所用的）与上下文无关语法的不同之处在于，它的编写方式更紧密地反映了解析器在解析时的操作方式。一个基本的技术差异是选择操作符是有序的。这意味着在编写：

```
rule: A | B | C
```

一个上下文无关语法解析器（如LL(1)解析器）会生成构造，给定一个输入字符串，它将推断出哪个选择（A、B或C）必须被展开，而PEG解析器会检查第一个选择是否成功，只有在失败的情况下，它才会按照它们编写的顺序继续尝试第二个或第三个选择。这使得选择操作符不是可交换的。

与LL(1)解析器不同，基于PEG的解析器不能有歧义：如果一个字符串被解析，它有且只有一个有效的解析树。这意味着基于PEG的解析器不会遭受LL(1)解析器和一般上下文无关语法可能出现的歧义问题。

PEG解析器通常构建为递归下降解析器，语法中的每条规则对应于实现解析器的程序中的一个函数，解析表达式（规则的“展开”或“定义”）代表了该函数中的“代码”。每个解析函数在概念上接受一个输入字符串作为其参数，并产生以下结果之一：

- 一个“成功”结果。这个结果表明该表达式可以被该规则解析，并且该函数可以选择性地向前移动或消耗掉一个或多个提供给它的输入字符串的字符。

- 一个“失败”结果，在这种情况下不消耗任何输入。

请注意，“失败”结果并不意味着程序是不正确的，也不一定意味着解析失败。由于选择操作符是有序的，失败很多时候仅仅表示“尝试下一个选项”。直接实现为递归下降解析器的PEG解析器在最坏情况下将呈现指数时间性能，因为PEG解析器具有无限的前瞻（这意味着它们可以在决定规则之前考虑任意数量的令牌）。通常，PEG解析器通过一种称为“packrat解析”的技术避免了这种指数时间复杂性，这不仅会在解析之前将整个程序加载到内存中，而且还允许解析器任意回溯。通过记忆已经匹配的规则在每个位置上的结果来提高效率。记忆化缓存的代价是解析器将自然使用比简单的LL(1)解析器更多的内存，后者通常是基于表的。

## 核心观点

> 重要：
不要试图以同样的方式去理解PEG语法，就像你对EBNF或上下文无关语法所做的那样。PEG是为了描述输入字符串将如何被解析而优化的，而上下文无关语法是为了生成它们描述的语言的字符串而优化的（在EBNF中，要知道给定的字符串是否属于该语言，你需要做工作来找出答案，因为这并不是从语法中立即显而易见的）。

- 选择是有序的（A | B 与 B | A 不同）。
- 如果规则返回失败，并不意味着解析失败，它只是意味着“尝试其他的”。
- 默认情况下，PEG解析器以指数时间运行，可以通过使用记忆化优化为线性时间。
- 如果解析完全失败（没有规则成功解析所有输入文本），PEG解析器没有“错误在哪里”的概念。

## 有序选择操作符的后果

尽管PEG可能看起来像EBNF，但它的含义却大不相同。PEG解析器中选择是有序的这一事实（这是PEG解析器工作方式的核心）除了消除歧义外，还有深远的后果。

如果一个规则有两个选择，第一个成功了，即使调用规则未能解析剩余的输入，也不会尝试第二个选择。因此，解析器被认为是“急切的”。为了说明这一点，考虑以下两个规则（在这些例子中，一个token是一个单独的字符）：

```
first_rule:  ('a' | 'aa') 'a'
second_rule: ('aa' | 'a' ) 'a'
```

在常规的EBNF语法中，两个规则都指定了语言{aa, aaa}，但在PEG中，这两个规则中的一个接受字符串aaa但不接受字符串aa。另一个则相反——它接受字符串aa但不接受字符串aaa。规则('a'|'aa')'a'不接受aaa，因为'a'|'aa'消耗了第一个a，让规则中的最后一个a消耗了第二个，留下了第三个a。由于规则已经成功，永远不会尝试回去让'a'|'aa'尝试第二个选择。表达式('aa'|'a')'a'不接受aa，因为'aa'|'a'接受了所有的aa，没有留下什么给最后的a。同样，'aa'|'a'的第二个选择也没有尝试。

> 警告：
有序选择的效果，如上所示，可能被许多层次的规则隐藏。

因此，编写规则时，一个选择包含在下一个选择中，在几乎所有情况下都是一个错误，例如：

```
my_rule:
    | 'if' expression 'then' block
    | 'if' expression 'then' block 'else' block
```

在这个例子中，第二个选择永远不会被尝试，因为第一个选择会首先成功（即使输入字符串后面有一个'else'块）。要正确编写这个规则，你可以简单地改变顺序：

```
my_rule:
    | 'if' expression 'then' block 'else' block
    | 'if' expression 'then' block
```

在这种情况下，如果输入字符串没有'else'块，第一个选择将失败，第二个选择将在没有该部分的情况下被尝试。

## 语法

语法由一系列形式如下的规则组成：

```
rule_name: expression
```

可选地，在规则名称之后可以包括一个类型，它指定了与规则对应的C或Python函数的返回类型：

```
rule_name[return_type]: expression
```

如果省略了返回类型，则在C中返回void *，在Python中返回Any。

## 语法表达式

### ```# comment```

Python风格的注释。

### ```e1 e2```

匹配e1，然后匹配e2。

```
rule_name: first_rule second_rule
```

### ```e1 | e2```

匹配e1或e2。

第一个选择也可以出现在规则名称后的行上，出于格式化的目的。在这种情况下，必须在第一个选择之前使用|，如下所示：

```
rule_name[return_type]:
    | first_alt
    | second_alt
```

### ```( e )```

匹配e。

```
rule_name: (e)
```

一个稍微复杂且有用的例子包括与重复操作符一起使用分组操作符：

```
rule_name: (e1 e2)*
```

### ```[ e ] 或 e?```

可选地匹配e。

```
rule_name: [e]
```

一个更有用的例子包括定义尾随逗号是可选的：

```
rule_name: e (',' e)* [',']
```

### ```e*```

匹配零个或多个e。

```
rule_name: (e1 e2)*
```

### ```e+```

匹配一个或多个e。

```
rule_name: (e1 e2)+
```

### ```s.e+```

匹配一个或多个由s分隔的e。生成的解析树不包括分隔符。这与(e (s e)*)在其他方面是相同的。

```
rule_name: ','.e+
```

### ```&e```

如果e可以被解析，则成功，但不消耗任何输入。

### ```!e```

如果e可以被解析，则失败，但不消耗任何输入。

一个从Python语法中取的例子指定了一个主体由一个原子组成，它不跟随一个.或(或[：

```
primary: atom !'.' !'(' !'['
```

### ```~```

即使解析失败，也要坚持当前的选择（这称为“剪切”）。

```
rule_name: '(' ~ some_rule ')' | some_alt
```

在这个例子中，如果解析了一个左括号，即使some_rule或)未能被解析，也不会考虑其他选择。

## 左递归

PEG解析器通常不支持左递归，但CPython的解析器生成器实现了一种类似于Medeiros等人描述的技术，但使用记忆化缓存而不是静态变量。这种方法更接近Warth等人描述的方法。这使我们能够编写不仅仅是简单的左递归规则，还包括涉及间接左递归的更复杂的规则，如：

```
rule1: rule2 | 'a'
rule2: rule3 | 'b'
rule3: rule1 | 'c'
```

以及“隐藏的左递归”，如：

```
rule: 'optional'? rule ' some_other_rule
```

## 语法中的变量

可以通过在它前面加上标识符和等号来命名子表达式。然后可以在动作中使用该名称（见下文），如下所示：

```
rule_name[return_type]: '(' a=some_other_rule ')' { a }
```

## 语法动作

为了避免中间步骤使语法与AST生成之间的关系变得模糊，PEG解析器允许直接为规则生成AST节点通过语法动作。语法动作是在成功解析语法规则时评估的特定于语言的表达式。这些表达式可以用Python或C编写，这取决于解析器生成器的期望输出。这意味着，如果想要分别在Python和C中生成解析器，应该编写两个语法文件，每个文件都有一组不同的动作，除了这些动作之外，其他所有内容在两个文件中都是相同的。作为一个带有Python动作的语法的例子，解析器生成器的一部分，它解析语法文件是从一个带有Python动作的元语法文件引导的，这些动作生成解析结果的语法树。

在Python的PEG语法的特定情况下，有动作允许直接在语法本身中描述AST是如何组成的，使其更清晰和可维护。这个AST生成过程得到了一些帮助函数的支持，这些函数抽象出常见的AST对象操作和一些其他不直接与语法相关的必需操作。

为了指示这些动作，每个选择后面可以跟上花括号内的动作代码，它指定了选择的返回值：

```
rule_name[return_type]:
    | first_alt1 first_alt2 { first_alt1 }
    | second_alt1 second_alt2 { second_alt1 }
```

如果省略了动作，则会生成一个默认动作：

- 如果规则中只有一个名称，则返回它。
- 如果规则中有多个名称，则返回一个包含所有解析表达式的集合（集合的类型在C和Python中会不同）。

这种默认行为主要是为了非常简单的情况和调试目的而设定的。

> 警告：
重要的是，动作不要改变通过变量引用其他规则传入的任何AST节点。不允许变异的原因是AST节点被记忆化缓存，并可能在不同的上下文中重用，在那里变异将是无效的。如果动作需要改变AST节点，它应该代替制作节点的新副本并改变那个。

PEG生成器支持的语法的完整元语法是：

```
start[Grammar]: grammar ENDMARKER { grammar }

grammar[Grammar]:
    | metas rules { Grammar(rules, metas) }
    | rules { Grammar(rules, []) }

metas[MetaList]:
    | meta metas { [meta] + metas }
    | meta { [meta] }

meta[MetaTuple]:
    | "@" NAME NEWLINE { (name.string, None) }
    | "@" a=NAME b=NAME NEWLINE { (a.string, b.string) }
    | "@" NAME STRING NEWLINE { (name.string, literal_eval(string.string)) }

rules[RuleList]:
    | rule rules { [rule] + rules }
    | rule { [rule] }

rule[Rule]:
    | rulename ":" alts NEWLINE INDENT more_alts DEDENT {
            Rule(rulename[0], rulename[1], Rhs(alts.alts + more_alts.alts)) }
    | rulename ":" NEWLINE INDENT more_alts DEDENT { Rule(rulename[0], rulename[1], more_alts) }
    | rulename ":" alts NEWLINE { Rule(rulename[0], rulename[1], alts) }

rulename[RuleName]:
    | NAME '[' type=NAME '*' ']' {(name.string, type.string+"*")}
    | NAME '[' type=NAME ']' {(name.string, type.string)}
    | NAME {(name.string, None)}

alts[Rhs]:
    | alt "|" alts { Rhs([alt] + alts.alts)}
    | alt { Rhs([alt]) }

more_alts[Rhs]:
    | "|" alts NEWLINE more_alts { Rhs(alts.alts + more_alts.alts) }
    | "|" alts NEWLINE { Rhs(alts.alts) }

alt[Alt]:
    | items '$' action { Alt(items + [NamedItem(None, NameLeaf('ENDMARKER'))], action=action) }
    | items '$' { Alt(items + [NamedItem(None, NameLeaf('ENDMARKER'))], action=None) }
    | items action { Alt(items, action=action) }
    | items { Alt(items, action=None) }

items[NamedItemList]:
    | named_item items { [named_item] + items }
    | named_item { [named_item] }

named_item[NamedItem]:
    | NAME '=' ~ item {NamedItem(name.string, item)}
    | item {NamedItem(None, item)}
    | it=lookahead {NamedItem(None, it)}

lookahead[LookaheadOrCut]:
    | '&' ~ atom {PositiveLookahead(atom)}
    | '!' ~ atom {NegativeLookahead(atom)}
    | '~' {Cut()}

item[Item]:
    | '[' ~ alts ']' {Opt(alts)}
    |  atom '?' {Opt(atom)}
    |  atom '*' {Repeat0(atom)}
    |  atom '+' {Repeat1(atom)}
    |  sep=atom '.' node=atom '+' {Gather(sep, node)}
    |  atom {atom}

atom[Plain]:
    | '(' ~ alts ')' {Group(alts)}
    | NAME {NameLeaf(name.string) }
    | STRING {StringLeaf(string.string)}

# Mini-grammar for the actions

action[str]: "{" ~ target_atoms "}" { target_atoms }

target_atoms[str]:
    | target_atom target_atoms { target_atom + " " + target_atoms }
    | target_atom { target_atom }

target_atom[str]:
    | "{" ~ target_atoms "}" { "{" + target_atoms + "}" }
    | NAME { name.string }
    | NUMBER { number.string }
    | STRING { string.string }
    | "?" { "?" }
    | ":" { ":" }
```

这个简单的语法文件作为示例，可以直接生成一个完整的解析器，该解析器能够解析简单的算术表达式，并返回一个基于C的有效Python AST：

```c
start[mod_ty]: a=expr_stmt* ENDMARKER { _PyAST_Module(a, NULL, p->arena) }
expr_stmt[stmt_ty]: a=expr NEWLINE { _PyAST_Expr(a, EXTRA) }

expr[expr_ty]:
    | l=expr '+' r=term { _PyAST_BinOp(l, Add, r, EXTRA) }
    | l=expr '-' r=term { _PyAST_BinOp(l, Sub, r, EXTRA) }
    | term

term[expr_ty]:
    | l=term '*' r=factor { _PyAST_BinOp(l, Mult, r, EXTRA) }
    | l=term '/' r=factor { _PyAST_BinOp(l, Div, r, EXTRA) }
    | factor

factor[expr_ty]:
    | '(' e=expr ')' { e }
    | atom

atom[expr_ty]:
    | NAME
    | NUMBER
```

这里的EXTRA是一个宏，它扩展为start_lineno, start_col_offset, end_lineno, end_col_offset, p->arena，这些变量由解析器自动注入；p指向一个保存解析器所有状态的对象。

写成针对Python AST对象的类似语法：

```python
start[ast.Module]: a=expr_stmt* ENDMARKER { ast.Module(body=a or []) }
expr_stmt: a=expr NEWLINE { ast.Expr(value=a, EXTRA) }

expr:
    | l=expr '+' r=term { ast.BinOp(left=l, op=ast.Add(), right=r, EXTRA) }
    | l=expr '-' r=term { ast.BinOp(left=l, op=ast.Sub(), right=r, EXTRA) }
    | term

term:
    | l=term '*' r=factor { ast.BinOp(left=l, op=ast.Mult(), right=r, EXTRA) }
    | l=term '/' r=factor { ast.BinOp(left=l, op=ast.Div(), right=r, EXTRA) }
    | factor

factor:
    | '(' e=expr ')' { e }
    | atom

atom:
    | NAME
    | NUMBER
```










