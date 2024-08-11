# print-alias-sets

"Alias Set Printer" 在 LLVM 中是一个用于调试的Pass，其目的是用来展示有关别名分析（Alias Analysis，AA）的信息。别名分析是一种用来确定两个指针是否指向内存中相同位置的程序分析。通过别名分析，编译器可以进行更精确的优化，比如消除不必要的内存访问、更有效地重新排序指令来提高缓存利用率等。

`print-alias-sets` 并不负责执行任何一种别名分析服务，而是用来输出现存的别名集(alias sets)。别名集是一种数据结构，用于存储在程序分析中可能相互冲突的指针组。如果两个指针被确定为此别名集的一部分，意味着它们在运行时有可能指向相同的内存位置。

这个Pass打印出的信息可以帮助开发人员了解别名分析在编译器内部如何工作，以及各种变量和他们各自指针所在的别名集。这为进行进一步的编译器优化提供了详见的别名关系信息。

若你想在LLVM中使用`print-alias-sets`，你可能需要在你的编译过程中添加相关的Pass，以获取AA的结果并且打印有关信息。 根据你使用的LLVM版本以及具体配置的不同，可能需要进行一些额外的步骤来使pass有效。而且需要记住，因为`print-alias-sets`通常用于调试，它可能对应程序的运行性能产生不良影响。

对应的代码在：`llvm-project/llvm/include/llvm/Analysis/AliasSetTracker.h`

```cpp
class AliasSetsPrinterPass : public PassInfoMixin<AliasSetsPrinterPass> {
  raw_ostream &OS;

public:
  explicit AliasSetsPrinterPass(raw_ostream &OS);
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};
```

## test case

command: `../../../../build/bin/opt -passes=print-alias-sets -S -o - < argmemonly.ll > argmemonly_opt.ll`

input llvm ir:

```llvm
; RUN: opt -passes=print-alias-sets -S -o - < %s 2>&1 | FileCheck %s

@s = global i8 1, align 1
@d = global i8 2, align 1

; CHECK: Alias sets for function 'test_alloca_argmemonly':
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 3 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod       Memory locations: (ptr %a, LocationSize::precise(1))
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 2] may alias, Mod/Ref    Memory locations: (ptr %d, unknown before-or-after), (ptr %s, unknown before-or-after)
define void @test_alloca_argmemonly(ptr %s, ptr %d) {
entry:
  %a = alloca i8, align 1
  store i8 1, ptr %a, align 1
  call void @my_memcpy(ptr %d, ptr %s, i64 1)
  ret void
}

; CHECK: Alias sets for function 'test_readonly_arg'
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 2 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod       Memory locations: (ptr %d, unknown before-or-after)
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Ref       Memory locations: (ptr %s, unknown before-or-after), (ptr %s, LocationSize::precise(1))
define i8 @test_readonly_arg(ptr noalias %s, ptr noalias %d) {
entry:
  call void @my_memcpy(ptr %d, ptr %s, i64 1)
  %ret = load i8, ptr %s
  ret i8 %ret
}

; CHECK: Alias sets for function 'test_noalias_argmemonly':
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 3 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod       Memory locations: (ptr %a, LocationSize::precise(1))
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 2] may alias, Mod/Ref    Memory locations: (ptr %d, unknown before-or-after), (ptr %s, unknown before-or-after)
define void @test_noalias_argmemonly(ptr noalias %a, ptr %s, ptr %d) {
entry:
  store i8 1, ptr %a, align 1
  call void @my_memmove(ptr %d, ptr %s, i64 1)
  ret void
}

; CHECK: Alias sets for function 'test5':
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 2 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod/Ref Memory locations: (ptr %a, LocationSize::precise(1)), (ptr %a, unknown before-or-after)
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod Memory locations: (ptr %b, unknown before-or-after), (ptr %b, LocationSize::precise(1))
define void @test5(ptr noalias %a, ptr noalias %b) {
entry:
  store i8 1, ptr %a, align 1
  call void @my_memcpy(ptr %b, ptr %a, i64 1)
  store i8 1, ptr %b, align 1
  ret void
}

; CHECK: Alias sets for function 'test_argcollapse':
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 2 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod/Ref Memory locations: (ptr %a, LocationSize::precise(1)), (ptr %a, unknown before-or-after)
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod/Ref Memory locations: (ptr %b, unknown before-or-after), (ptr %b, LocationSize::precise(1))
define void @test_argcollapse(ptr noalias %a, ptr noalias %b) {
entry:
  store i8 1, ptr %a, align 1
  call void @my_memmove(ptr %b, ptr %a, i64 1)
  store i8 1, ptr %b, align 1
  ret void
}

; CHECK: Alias sets for function 'test_memcpy1':
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 2 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod/Ref Memory locations: (ptr %b, unknown before-or-after)
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod/Ref Memory locations: (ptr %a, unknown before-or-after)
define void @test_memcpy1(ptr noalias %a, ptr noalias %b) {
entry:
  call void @my_memcpy(ptr %b, ptr %a, i64 1)
  call void @my_memcpy(ptr %a, ptr %b, i64 1)
  ret void
}

; CHECK: Alias sets for function 'test_memset1':
; CHECK-NEXT: Alias Set Tracker: 1 alias sets for 1 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod Memory locations: (ptr %a, unknown before-or-after)
define void @test_memset1() {
entry:
  %a = alloca i8, align 1
  call void @my_memset(ptr %a, i8 0, i64 1)
  ret void
}

; CHECK: Alias sets for function 'test_memset2':
; CHECK-NEXT: Alias Set Tracker: 1 alias sets for 1 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod Memory locations: (ptr %a, unknown before-or-after)
define void @test_memset2(ptr %a) {
entry:
  call void @my_memset(ptr %a, i8 0, i64 1)
  ret void
}

; CHECK: Alias sets for function 'test_memset3':
; CHECK-NEXT: Alias Set Tracker: 1 alias sets for 2 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 2] may alias, Mod Memory locations: (ptr %a, unknown before-or-after), (ptr %b, unknown before-or-after)
define void @test_memset3(ptr %a, ptr %b) {
entry:
  call void @my_memset(ptr %a, i8 0, i64 1)
  call void @my_memset(ptr %b, i8 0, i64 1)
  ret void
}

;; PICKUP HERE

; CHECK: Alias sets for function 'test_memset4':
; CHECK-NEXT: Alias Set Tracker: 2 alias sets for 2 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod Memory locations: (ptr %a, unknown before-or-after)
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Mod Memory locations: (ptr %b, unknown before-or-after)
define void @test_memset4(ptr noalias %a, ptr noalias %b) {
entry:
  call void @my_memset(ptr %a, i8 0, i64 1)
  call void @my_memset(ptr %b, i8 0, i64 1)
  ret void
}

declare void @my_memset(ptr nocapture writeonly, i8, i64) argmemonly
declare void @my_memcpy(ptr nocapture writeonly, ptr nocapture readonly, i64) argmemonly
declare void @my_memmove(ptr nocapture, ptr nocapture readonly, i64) argmemonly


; CHECK: Alias sets for function 'test_attribute_intersect':
; CHECK-NEXT: Alias Set Tracker: 1 alias sets for 1 pointer values.
; CHECK-NEXT: AliasSet[0x{{[0-9a-f]+}}, 1] must alias, Ref       Memory locations: (ptr %a, LocationSize::precise(1))
define i8 @test_attribute_intersect(ptr noalias %a) {
entry:
  ;; This call is effectively readnone since the argument is readonly
  ;; and the function is declared writeonly.  
  call void @attribute_intersect(ptr %a)
  %val = load i8, ptr %a
  ret i8 %val
}

declare void @attribute_intersect(ptr readonly) argmemonly writeonly
```

command 执行后输出：

```sh
Alias sets for function 'test_alloca_argmemonly':
Alias Set Tracker: 2 alias sets for 3 pointer values.
  AliasSet[0x6000017cc5f0, 1] must alias, Mod       Memory locations: (ptr %a, LocationSize::precise(1))
  AliasSet[0x6000017c5900, 2] may alias, Mod/Ref   Memory locations: (ptr %d, unknown before-or-after), (ptr %s, unknown before-or-after)

Alias sets for function 'test_readonly_arg':
Alias Set Tracker: 2 alias sets for 2 pointer values.
  AliasSet[0x6000017c5950, 1] must alias, Mod       Memory locations: (ptr %d, unknown before-or-after)
  AliasSet[0x6000017c59a0, 1] must alias, Ref       Memory locations: (ptr %s, unknown before-or-after), (ptr %s, LocationSize::precise(1))

Alias sets for function 'test_noalias_argmemonly':
Alias Set Tracker: 2 alias sets for 3 pointer values.
  AliasSet[0x6000017c5950, 1] must alias, Mod       Memory locations: (ptr %a, LocationSize::precise(1))
  AliasSet[0x6000017c59f0, 2] may alias, Mod/Ref   Memory locations: (ptr %d, unknown before-or-after), (ptr %s, unknown before-or-after)

Alias sets for function 'test5':
Alias Set Tracker: 2 alias sets for 2 pointer values.
  AliasSet[0x6000017c3840, 1] must alias, Mod/Ref   Memory locations: (ptr %a, LocationSize::precise(1)), (ptr %a, unknown before-or-after)
  AliasSet[0x6000017c3890, 1] must alias, Mod       Memory locations: (ptr %b, unknown before-or-after), (ptr %b, LocationSize::precise(1))

Alias sets for function 'test_argcollapse':
Alias Set Tracker: 2 alias sets for 2 pointer values.
  AliasSet[0x6000017c3840, 1] must alias, Mod/Ref   Memory locations: (ptr %a, LocationSize::precise(1)), (ptr %a, unknown before-or-after)
  AliasSet[0x6000017c38e0, 1] must alias, Mod/Ref   Memory locations: (ptr %b, unknown before-or-after), (ptr %b, LocationSize::precise(1))

Alias sets for function 'test_memcpy1':
Alias Set Tracker: 2 alias sets for 2 pointer values.
  AliasSet[0x6000017c3840, 1] must alias, Mod/Ref   Memory locations: (ptr %b, unknown before-or-after)
  AliasSet[0x6000017c3930, 1] must alias, Mod/Ref   Memory locations: (ptr %a, unknown before-or-after)

Alias sets for function 'test_memset1':
Alias Set Tracker: 1 alias sets for 1 pointer values.
  AliasSet[0x6000017c3840, 1] must alias, Mod       Memory locations: (ptr %a, unknown before-or-after)

Alias sets for function 'test_memset2':
Alias Set Tracker: 1 alias sets for 1 pointer values.
  AliasSet[0x6000017c8140, 1] must alias, Mod       Memory locations: (ptr %a, unknown before-or-after)

Alias sets for function 'test_memset3':
Alias Set Tracker: 1 alias sets for 2 pointer values.
  AliasSet[0x6000017c8190, 2] may alias, Mod       Memory locations: (ptr %a, unknown before-or-after), (ptr %b, unknown before-or-after)

Alias sets for function 'test_memset4':
Alias Set Tracker: 2 alias sets for 2 pointer values.
  AliasSet[0x6000017c81e0, 1] must alias, Mod       Memory locations: (ptr %a, unknown before-or-after)
  AliasSet[0x6000017c8230, 1] must alias, Mod       Memory locations: (ptr %b, unknown before-or-after)

Alias sets for function 'test_attribute_intersect':
Alias Set Tracker: 1 alias sets for 1 pointer values.
  AliasSet[0x6000017d0050, 1] must alias, Ref       Memory locations: (ptr %a, LocationSize::precise(1))
```

在LLVM中，`print-alias-sets` pass的输出帮助我们理解了函数中指针的别名关系。这些信息对于编译器优化非常重要，因为它们影响了编译器能够安全执行的变换。例如，如果编译器知道两个指针不会别名（即，它们不会指向相同的内存位置），那么它可以并行化访问这两个指针的操作，或者重新排序这些操作以提高性能。

例子中，我们可以看到几个不同的函数，每个函数都有一些指针操作和对外部函数的调用。`print-alias-sets` pass输出了每个函数的别名集信息，这些信息表明了哪些指针可能会别名。

例如，对于`test_alloca_argmemonly`函数，输出显示了两个别名集：

- 第一个别名集包含指针`%a`，它被标记为`must alias, Mod`，意味着它必定别名并且会被修改。
- 第二个别名集包含指针`%d`和`%s`，它们被标记为`may alias, Mod/Ref`，意味着它们可能别名，并且可能被修改或引用。

这些信息表明编译器在处理这些指针时需要小心，特别是在进行内存访问相关的优化时。

对于其他函数，输出也提供了类似的信息。例如，`test_readonly_arg`函数的输出表明指针`%d`和`%s`都在一个`must alias, Mod`的别名集中，这意味着它们必定别名并且会被修改。

在优化方面，这些别名集信息可以用来：

- 确定哪些变量可以被存储在寄存器中，以减少内存访问。
- 重排指令以减少潜在的冲突和提高并行性。
- 决定哪些循环可以安全地向量化或并行化。
- 应用更激进的内存访问优化，如预取和缓存行大小对齐。

LVM使用一系列的别名分析（Alias Analysis, AA）算法来判断指针之间的别名关系。别名分析的目的是确定两个指针是否可能指向内存中的同一位置。LLVM的别名分析包括但不限于以下几种：

1. 基本别名分析（Basic AA）：检查指针类型、常量偏移量和其他简单的编译时信息。
2. NoAlias分析：检查特定的指针对，这些指针由于语言或其他保证不可能别名。
3. 高级别名分析（Advanced AA）：使用更复杂的逻辑，如查看函数调用的效果、循环依赖关系等。

`test_alloca_argmemonly`中，`%a` 是通过 `alloca` 在栈上分配的，它是一个局部的、新的内存位置，因此它不太可能与其他指针别名。这就是为什么它被标记为 `must alias`，因为它只与自己别名，并且被标记为 `Mod`，因为它被存储操作修改。

另一方面，`%d` 和 `%s` 是函数的参数，它们的来源不是在函数内部明确的，因此它们可能与函数外部的指针别名。这就是为什么它们被放在同一个 `may alias` 集合中，并且被标记为 `Mod/Ref`，因为它们可能被修改或引用。

别名集中的 `unknown before-or-after` 表示在分析时，不能确定这些指针的别名关系是在函数调用之前还是之后发生的，因此需要假设它们可能在任何时间别名。

总结来说，LLVM通过分析指针的来源、使用和函数调用的上下文来判断别名。在你的例子中，栈上新分配的内存（通过 `alloca`）通常不会与其他指针别名，而函数参数的别名关系则不确定，因为它们可能与函数外部的内存位置相关联。