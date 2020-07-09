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
- FaaS ready (NYA)
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

`(= )`
  O_Equit, O_Nequi, O_Equal, O_Nqual, O_GThan, O_LThan, O_GETo, O_LETo,
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