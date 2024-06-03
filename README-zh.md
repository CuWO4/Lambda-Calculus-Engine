# Lambda Calculus Engine

一个十分快速且允许指定局部求值优先级的 lambda 演算引擎.

---

## INTRODUCTION TO LAMBDA CALCULUS

[Wikipedia: Lambda Calculus](https://en.wikipedia.org/wiki/Lambda_calculus)

## COMPILE

```bash
make
```

编译环境:

* Ubuntu 20.04.6 LTS

* clang 13.0.1

* GNU Make 4.2.1

* flex 2.6.4

* GNU bison 3.5.1

## USAGE

```bash
lambda [INPUT] [OUTPUT]
```
* `[INPUT]` 源文件, 语法见 [GRAMMAR](#grammar).
* `[OUTPUT]` 输出文件, 向其中输出推导过程. 可选, 缺省 `stdout`.

## GRAMMAR

#### 关键字

`\`, `@`, `#`, `.`, `:=`, `(`, `)` `{`, `}`, `$`.

#### 语句

```
# [SYMBOL] := [EXPRESSION]
```
将 `[SYMBOL]` 定义为 `[EXPRESSION]`.

```
@ [EXPRESSION]
```
推导`[EXPRESSION]`.

所有数字会被自动推导为对应丘奇数.

```
import [PATH]
```
推导 `[PATH]` 指示的文件, 并保留其内的定义.

#### 表达式

**变元**

任意长的不含关键字的字符串.


**抽象**
```
\[VAR]. [EXPR]
```

**应用**

```
[EXPR] [EXPR]
```

**优先级**

小括号可以指定优先级.

抽象比应用优先级更高. 例如 `\x. A B` 会被解释为 `\x. (A B)` 而非 `(\x. A) B`.

应用是左结合的. 例如 `A B C` 会被解释为 `(A B) C` 而非 `A (B C)`.

#### 求值顺序

引擎默认采用惰性求值. 例如对 `A B` 来说, 引擎会先尝试在 `A` 上应用 `B`, 然后尝试化简 `A`, 最后尝试化简 `B`.

**大括号**
可以用大括号 `{}` 包裹表达式, 被包裹的表达式将积极求值. 例如对 `{A} B` 来说, 引擎会先尝试化简 `A`, 然后尝试在 `A` 上应用 `B`, 最后尝试化简 `B`.

大括号指定优先级是递归的. 例如对 `A ({B} C)` 来说, 引擎会先尝试化简 `B`, 然后尝试在 `A` 上应用 `(B C)`.

**美元符号**

可以将美元符号 `$` 添加在变元或被小括号 `()` 或大括号 `{}` 包裹的表达式前, 来屏蔽其内部的大括号优先级指定(但并不会移除内部的大括号). 例如对 `A $({B} C)` 来说, 引擎会先尝试在 `A` 上应用 `(B C)`, 然后在剩下的剩下的表达式中先尝试化简`B`.

## EXAMPLE
```
import "nature.lambda"
import "control.lambda"

# list := make_list 1 (>=n 10) ++n
@ fold (filter list prime?) 0 +n
```
先生成了一个 1 到 10 的列表, 然后过滤只取其中的质数项, 然后将其中的项相加. 这段表达式将生成10以内质数和. (相关文件见 `lib/`)

对应输出:
```
...
beta>  \f.\x. f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f ((\x. x) x)))))))))))))))))
beta>  \f.\x. f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f x))))))))))))))))

to be sought:     fold (filter list prime?) 0 +n
result:           \f.\x. f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f x))))))))))))))))
step taken:       10563
character count:  15009827
time cost:        2036ms
```