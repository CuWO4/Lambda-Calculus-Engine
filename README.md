# Lambda Calculus Engine

A very fast lambda calculus engine that allows to specify local evaluation order.

---

## INTRODUCTION TO LAMBDA CALCULUS

[Wikipedia: Lambda Calculus](https://en.wikipedia.org/wiki/Lambda_calculus)

## COMPILE

```bash
make
```

Compile Environment:

* Ubuntu 20.04.6 LTS

* clang 13.0.1

* GNU Make 4.2.1

* flex 2.6.4

* GNU bison 3.5.1

## USAGE

```bash
lambda [INPUT] [-o OUTPUT] [-i]
```
* `[INPUT]` Source file, see [GRAMMAR](#grammar) for syntax.
* `[OUTPUT]` Output file to store the derivation process. Optional, default is `stdout`.
* `-i` display the intermedia process of derivation. Optional.

## GRAMMAR

#### Keywords

`\`, `@`, `#`, `.`, `:=`, `(`, `)` `{`, `}`, `$`.

#### Comments

```
// line comment
```


#### Statements

```
# [SYMBOL] := [EXPRESSION]
```
Define `[SYMBOL]` as `[EXPRESSION]`.

```
@ [EXPRESSION]
```
Derive `[EXPRESSION]`.

All numbers will be automatically derived as corresponding Church numerals.

```
import [PATH]
```
Derive the file indicated by `[PATH]`, preserving its definitions.

#### Expressions

**Variables**

Any string of any length that does not contain keywords.

**Abstraction**
```
\[VAR]. [EXPR]
```

**Application**

```
[EXPR] [EXPR]
```

**Precedence**

Parentheses can specify precedence.

Abstraction has higher precedence than application. For example, `\x. A B` will be interpreted as `\x. (A B)` rather than `(\x. A) B`.

Application is left-associative. For example, `A B C` will be interpreted as `(A B) C` rather than `A (B C)`.

#### Evaluation Order

The engine uses lazy evaluation by default. For example, for `A B`, the engine will first attempt to apply `B` to `A`, then try to simplify `A`, and finally try to simplify `B`.

**Braces**

Expressions can be wrapped in braces `{}` to be eagerly evaluated. For example, for `{A} B`, the engine will first try to simplify `A`, then attempt to apply `B` to `A`, and finally try to simplify `B`.

Braces specifying precedence are recursive. For example, for `A ({B} C)`, the engine will first try to simplify `B`, then attempt to apply `(B C)` to `A`.

**Dollar Sign**

The dollar sign `$` can be added before variables or expressions wrapped in parentheses `()` or braces `{}` to mask the internal precedence specified by the braces (but will not remove the braces). For example, for `A $({B} C)`, the engine will first attempt to apply `(B C)` to `A`, then try to simplify `B` in the remaining expression.

## EXAMPLE
```
import "nature.lambda"
import "control.lambda"

# list := make_list 1 (>=n 10) ++n
@ fold (filter list prime?) 0 +n
```
First, a list from 1 to 10 is generated, then it is filtered to keep only the prime numbers, and finally, the items are summed. This expression will generate the sum of primes within 10. (Relevant files are in the `lib/`)

Corresponding output:
```
fold (filter list prime?) 0 +n


to be sought:     fold (filter list prime?) 0 +n
result:           \f.\x. f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f (f x))))))))))))))))
step taken:       6311
time cost:        16ms
```