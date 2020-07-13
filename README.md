# Ephem

| ![Ephem logo](media/Ephem-logo.png) | Dynamic s-expression programming language for PC, with delusions of grandeur. |
| - | - |

Descendant of the completed Arduino/PC stack-based dynamic s-expression programming language [Chika](https://phunanon.github.com/Chika).

## Examples

```clj
> (println "Hello, world!")
Hello, world!
N
> (map #(* % 2) (range 3))
[0 2 4]
> ((if T + /) 12 3)
15
> (fn double [a] (* a 2))
> (map double (range 10))
[0 2 4 6 8 10 12 14 16 18]
```

## Rationale

(NYA - Not Yet Achieved)

**Concepts:**  
- S-expressions
- Dynamic
- Auto Reference Counting (ARC) memory management
- Has REPL
- Hotloadable (NYA)
- Lazy
- Easy for total beginners (NYA)
- Immutable values & collections (NYA)

**Features:**  
- REPL with [linenoise](https://github.com/antirez/linenoise)
- Destructuring (NYA)
- Keywords (NYA)
- Lazy enumerables
  - Range
  - Vector
  - Map
  - Set (NYA)
  - File stream (NYA)
  - Network stream (NYA)
- Spread forms (NYA)
- Breakpoints (NYA)
- Closures (NYA)
- File system access (NYA)
- Network access (NYA)

**Anti-features:**  
- No JIT; it's slow
- Single-threaded
- No native operation overrides

**Characteristics**
- x86 is ~2x slower but x64 uses ~2x more memory

## Usage

### Ubuntu/Debian  
`git clone --depth=1 https://github.com/phunanon/Ephem.git`  
`cd Ephem && git clone --depth=1 https://github.com/arximboldi/immer.git`  
`mv immer/immer src && rm -r immer`  
Ensure CMake in installed on your system.  
Run `./init.sh`. If needing to subsequently recompile use `./make.sh`.  
Execute the `build/ephem` binary.

## Syntax and native operations

### Literals

| Representations                      | Data type               |
| ------------------------------------ | ----------------------- |
| `T F N`                              | true, false, nil        |
| `\a`                                 | character               |
| `0x1` or `0x12`                      | 8 bit unsigned integer  |
| `1234` or `0x123…`                   | 32 bit unsigned integer |
| `-1234` or `-0x123…`                 | 32 bit signed integer   |
| `3.14` or `.14` or `-3.14` or `-.14` | 32 bit signed float     |
| `[1 2 3]`                            | vector                  |

### Functions and native operations

**Glossary**  
`[…]` representation of a vector.  
`[0..]` zero or more arguments.  
`[1..]` one or more arguments.  
`[arg-name]` optional argument.  
"truthy" is a value that is not `F` or `N`.  
"falsey" is a value that is `F` or `N`.

**Expressions**

An evaluated expression returns a value. An expression is formed as `(operation [0..])` whereby the `operation` can be a native operation, program function, variable containing a function or lambda reference, or another expression evaluated as one of the aforementioned. Arguments can also be expressions.

**Functions**

Function declarations are only accepted at the top-level of a document or REPL interaction - they cannot be contained within expressions.  
They are declared by using `(fn function-name […] [1..])` where `[…]` is a vector of parameters, e.g. `[a b c]`, and `[1..]` is one or more expressions.

Anonymous functions, or lambdas, are defined by the syntax `#(operation [0..])` which constitutes one expression.

**Short-circuited control structures**

`(if cond if-true [if-false])`  
Returns `if-true` if `cond` is truthy, or `if-false` or `N` if `cond` is falsey.  
`if-true` and `if-false` are never mutually evaluated.

`(or [1..])`  
Returns the first truthy argument otherwise `N`.  
Arguments are not evaluated beyond a truthy one.

`(and [1..])`  
Returns `T` if all arguments are truthy, otherwise `F`.  
Arguments are not evaluated beyond a falsey one.

**Arithmetic**

`+ - * / mod **  & | ^ << >>`  
e.g. `(+ 1 2 3)` uses first type, (+ \a 3)  
`~`

**Likeness & Equality**  
Equality compares the 4 bytes of information inside a Value, which may be a primitive type or pointer to a more complex type.  
Likeness intelligently compares strings and lists per character and item respectively. Likeness otherwise decays into equality.  
Infinite lists are only equal to `N`.

`(= [1..])`  
Returns `T` for homogeneously alike arguments.  
Compares list items for sequential likeness.  
```clj
(= 3.14 3.14 3.14)    => T
(= T F 1)             => F
(= [0 1 2] (range 3)) => T
(= 123 [3 4 5])       => F
```

`(!= [1..])`  
Returns `F` for contiguous alike arguments.  
```clj
(!= T T)              => F
(!= T F)              => T
(!= T F T)            => T
(!= T F T T)          => F
```

`(== [1..])`  
Returns `T` for homogeneously equal arguments.  
```clj
(== 3.14 3.14 3.14)   => T
(== T F 1)            => F
(== [1 2 3] [1 2 3])  => F   //As it compares internal pointers
(== 12345678 [1 2 3]) => T~F //… therefore very unlikely, but possible to be T
```

`(!== [1..])`  
Returns `F` for contiguous equal arguments.  
Refer to `!=` for similar characteristics.

**Monotonic comparisons**  
The monotonic comparisons compare each argument in turn with the previous.  
Intelligently compares strings by ASCII order and lists by length.

`(< [1..])`  
Returns `T` for monotonically increasing arguments.  
```clj
(< 1 2)               => T
(< 10 -10)            => F
(< -1 10 56)          => T
(< -10 10 9)          => F
```

`(> [1..])`  
Returns `T` for monotonically decreasing arguments.

`(<= [1..])`  
Returns `T` for monotonically non-decreasing arguments.

`(>= [1..])`  
Returns `T` for monotonically non-increasing arguments.

**Vectors & Lizts**  
A vector is a collection of pre-evaluated values. A lizt is a lazy collection producing values per iteration.  
Vectors have a fixed length. Lizts either have a fixed expected length or infinite length.  

`(vec [1..])`  

O_Skip, O_Take, O_Range, O_Cycle, O_Emit, O_Map, O_Where

**Environment**

`(print [1..])` `(println [0..])`  
Prints the string representation of arguments to the terminal.

`(get-key)`  
Gets the next buffered character from terminal input. Returns `N` if no key was in the buffer.

`(get-str)`  
Gets a string of text from terminal input, blocking Ephem until Enter is pressed.

`(sleep)` `(sleep num-seconds)`  
Blocks Ephem for a duration of `num-seconds` or 1 second. `num-seconds` may be a float.

## Design and characteristics

### Enumerables and laziness

**Examples of lazy enumerables**

```clj
(range 5 10)                  => [5 6 7 8 9]
(map + [0 1 2] [4 5 6 7])     => [4 6 8]
(map + [3 1 4] (range))       => [3 2 6]
(map * (range 4) (range))     => [0 1 4 9]
(map + [0 1 2] 3)             => [3 4 5]
(map + (range) (range))       => infinite, nil as immediate
(map #(str % \!) [1 2 3])     => ["1!" "2!" "3!"]
(map #(% 12 3) [+ - * /])     => [15 9 36 4]
(map + (cycle 2 1) (range 6)) => [2 2 4 4 6 6]
(take 5 (skip 4 (range)))     => [4 5 6 7 8]
(take 5 4 (range))            => [4 5 6 7 8]
(emit 3 5)                    => [3 3 3 3 3]
```

**Examples of immediate collections**

```clj
(where odd? (range -8))       => [-1 -3 -5 -7] immediate
(where even? 4 3 (range))     => [6 8 10 12] immediate
```

### Immutable vectors

O(n)
https://sinusoid.es/immer/index.html