# Ephem

Dynamic s-expression programming language for PC, with delusions of grandeur.

## Rationale

(NYA - Not Yet Achieved)  
**Concepts:**  
- S-expressions
- Dynamic
- Has REPL (NYA)
- Hotloadable (NYA)
- Lazy (NYA)
- FaaS ready (NYA)
- Easy for total beginners (NYA)
- Immutable values & collections (NYA)
**Features:**
- REPL with [linenoise](https://github.com/antirez/linenoise)
- Destructuring (NYA)
- Keywords (NYA)
- Lazy enumerables (NYA)
  - Range (NYA)
  - Vector (NYA)
  - Map (NYA)
  - Set (NYA)
  - File stream (NYA)
  - Network stream (NYA)
- Breakpoints (NYA)
- Closures (NYA)
- File system access (NYA)
- Network access (NYA)
**Anti-features:**  
- Compiled for 32bit
- Single-threaded
- No native operation overrides

## Usage

**Ubuntu/Debian**  
`git clone --depth=1 https://github.com/phunanon/Ephem.git`
Ensure CMake in installed on your system.  
Run `./build.sh`. If needing to subsequently recompile use `./make.sh`.  
Execute the `build/ephem` binary.

## Syntax



## Design and characteristics

**Vectors**

https://sinusoid.es/immer/index.html