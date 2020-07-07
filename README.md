# Ephem

| ![Ephem logo](media/Ephem-logo.png) | Dynamic s-expression programming language for PC, with delusions of grandeur. |
| - | - |


## Examples

```clj
> (println "Hello, world!")
Hello, world!
N
> (map #(* % 2) (range 3))
[0 2 4]
> ((if T + /) 12 3)
15
```

## Rationale

(NYA - Not Yet Achieved)

**Concepts:**  
- S-expressions
- Dynamic
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
Run `./build.sh`. If needing to subsequently recompile use `./make.sh`.  
Execute the `build/ephem` binary.

## Syntax and native operations

### Literals

### Functions

### Native operations

## Design and characteristics

### Enumerables and laziness

**Examples**

```clj
(range 5 10)                  => [5 6 7 8 9]
(map + [0 1 2] [4 5 6])       => [4 6 8]
(map + [3 1 4] (range))       => [3 2 6]
(map * (range 4) (range))     => [0 1 4 9]
(map + [0 1 2] 3)             => [3 4 5]
(map + (range) (range))       => infinite, nil on immediate evaluation
(map #(str % \!) [1 2 3])     => ["1!" "2!" "3!"]
(map #(% 12 3) [+ - * /])     => [15 9 36 4]
(map + (cycle 2 1) (range 6)) => [2 2 4 4 6 6]
```

### Immutable vectors

O(n)
https://sinusoid.es/immer/index.html