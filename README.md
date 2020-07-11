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
- Automatic Reference Counting (ARC) memory management
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
- Compiled for 32bit
- Single-threaded
- No native operation overrides

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

### Functions

### Native operations

`(if cond if-true) (if cond if-true if-false)`  

`+ - * / mod`  
e.g. `(+ 1 2 3)`

**Likeness vs. Equality**  
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

**Monotonic comparison**  
The monotonic comparisons compare each argument in turn with the previous.  
Strings are intelligently compared for ASCII order.

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

  
  O_Vec, O_Skip, O_Take, O_Range, O_Cycle, O_Emit,
  O_Map, O_Where,
  O_Str, O_Print, O_Priln, O_Val, O_Do

## Design and characteristics

### Enumerables and laziness

**Examples of lazy enumerables**

```clj
(range 5 10)                  => [5 6 7 8 9]
(map + [0 1 2] [4 5 6 7])     => [4 6 8]
(map + [3 1 4] (range))       => [3 2 6]
(map * (range 4) (range))     => [0 1 4 9]
(map + [0 1 2] 3)             => [3 4 5]
(map + (range) (range))       => infinite, nil on immediate evaluation
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