# Lambda Calculus Calculator

---

## USAGE

```bash
lambda [INPUT] [OUTPUT]
```
* `[INPUT]` 源文件, 语法见 [GRAMMAR](#grammar).
* `[OUTPUT]` 输出文件, 向其中输出推导过程. 可选, 缺省 `stdout`.

## GRAMMAR

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

## EXAMPLE
```
# ++ := \n.\f.\x. f(n f x)
@ ++ 2
```

将输出
```
++ 2
++ (\f.\x.f (f x))
(\n.\f.\x.f (n f x)) (\f.\x.f (f x))
\f.\x.f ((\f.\x.f (f x)) f x)
\f.\x.f ((\x.f (f x)) x)
\f.\x.f (f (f x))
```