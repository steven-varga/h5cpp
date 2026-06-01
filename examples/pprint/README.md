@page example_guide_pprint Pretty-Print for STL Containers

h5cpp ships a set of `operator<<` overloads (in `h5cpp/H5Uall.hpp`) that stream any STL-shaped container — and recursively stream its elements — to a `std::ostream`. No HDF5 is involved; this example exists purely to make container debugging painless and to back the diagnostics used throughout the other examples.

```cpp
#include <h5cpp/all>
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::cout << v << "\n";    // → [1,2,3,4,5]
}
```

The whole pretty-print surface arrives by including any of the h5cpp umbrella headers (`<h5cpp/all>`, `<h5cpp/core>`).

## Files

| File          | What it teaches                                                         |
| ------------- | ----------------------------------------------------------------------- |
| `pprint.cpp`  | Sample print of every supported container shape + recursive composition. |

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-pprint
./examples-pprint
```

No HDF5 file is written; output is pure `stdout`.

## Five Overload Categories

The dispatch uses Walter Brown's WG21 N4436 feature-detection idiom — each category is selected by which member functions the type exposes, not by listing types one by one.

| #  | Detected via                          | Catches                                                                |
| -- | ------------------------------------- | ---------------------------------------------------------------------- |
| 1  | `begin() / end()`                     | `vector`, `list`, `forward_list`, `deque`, `set`, `multiset`, `unordered_set`, `unordered_multiset`, `map`, `multimap`, `unordered_map`, `array` |
| 2  | `top() / pop() / empty()` (no iter)   | `std::stack`, `std::priority_queue`                                    |
| 3  | `front() / pop() / empty()` (no iter) | `std::queue`                                                           |
| 4  | `std::pair<K, V>`                     | rendered `{key:value}`                                                 |
| 5  | `std::tuple<Ts...>`                   | rendered `<v0,v1,...,vN>`                                              |

Maps fall into category 1 (they're iterable) — but `*it` returns a `pair<const K, V>`, so the pair overload kicks in for the elements and you get `[{k1:v1},{k2:v2},...]`.

The stack/queue dispatch is non-destructive: a copy of the adaptor is made, then popped element by element into a buffer. The original is untouched.

## Sample Output (with `H5CPP_CONSOLE_WIDTH = 30`)

```
LISTS / VECTORS / SETS
---------------------------------------------------------------------
       array<string,7>: [xSs,wc,gu,Ssi,Sx,pzb,OY]
        vector<string>: [CDi,PUs,zpf,Hm,teO,XG,bu,QZs]
            deque<int>: [256,233,23,89,128,268,69,278,130]
             list<int>: [95,284,24,124,49,40,200,108,281,251,57,12,9]
     forward_list<int>: [147,76,81,193,44]
           set<string>: [V,f,szy,v]

ADAPTORS
---------------------------------------------------------------------
     stack<T,deque<T>>: [11,181,252,172]
    stack<T,vector<T>>: [9,224,149,58,15,121,44,230,70,66,278,54]
        priority_queue: [zdbUzd,tTknDw,qorxgk,mCcEay,gDeJ,FYPOEd,CIhMU]
     queue<T,deque<T>>: [bVG,Bbs,vchuT,FfxEw,CXFrr,JAx,sVlcI]

ASSOCIATIVE CONTAINERS
---------------------------------------------------------------------
       map<string,int>: [{LID:2},{U:2},{Xr:1},{e:2},{esU:1},{kbj:1},{qFc:3}]
multimap<short,list<int>>: [{5:[6,6,8,7,8,5,7,8,5,5,6,7]},{6:[8,5,6]},{7:[7,5,8,8]},{8:[5,8,6,8]}]

PAIRS / TUPLES
---------------------------------------------------------------------
      pair<int,string>: {42:answer}
tuple<int,double,string>: <7,3.14,hi>
tuple<string,vector<int>>: <channels,[10,20,30,40]>
tuple<int,tuple<f,f>,bool>: <1,<2.5,3.5>,1>

DEEP NESTING
---------------------------------------------------------------------
   vector<vector<int>>: [[1,2,3],[4,5,6,7],[8,9]]
map<string,vector<double>>: [{a:[1,2,3]},{b:[4,5]}]
vector<map<int,string>>: [[{1:a},{2:b}],[{10:x},{20:y},{30:z}]]

TRUNCATION (H5CPP_CONSOLE_WIDTH = 30)
---------------------------------------------------------------------
             short (5): [0,1,2,3,4]
        exact (=width): [0,1,2,...,28,29]
            long  (50): [0,1,2,...,28,29, ...]
```

(Set / unordered_set / unordered_map element order varies because the underlying containers are hash-based.)

## Truncation

Long containers truncate at `H5CPP_CONSOLE_WIDTH` (default `10`) with a trailing `", ..."` so single-line output stays readable. Override the limit by `#define`-ing it **before** including the h5cpp header that pulls in `H5Uall.hpp`:

```cpp
#define H5CPP_CONSOLE_WIDTH 25
#include <h5cpp/all>
```

The cap is per inserter call: nested containers each truncate at the same width, so a `map<string, vector<int>>` with 100 outer entries and 100 inner entries shows the first 10 outer keys and within each shown value the first 10 elements.

## Why It's Useful Beyond Debugging

Several other examples in this directory use the pretty-print operators as their verification output:

- **`examples/optimized/`** prints column slices as `std::vector<int>`.
- **`examples/multi-tu/`** prints a vector of POD record indices.
- **`examples/datasets/`** uses it throughout for compact dataset content display.

Anywhere you have a container and want a one-liner `std::cout << c`, the operators work without further setup.

## Limitations

| Limitation                                          | Why it's there                                                          |
| --------------------------------------------------- | ----------------------------------------------------------------------- |
| Single-line output only.                            | A printf-style "[" / "]" / "," scheme — no pretty indenting.            |
| Elements are streamed with their own `operator<<`.  | If your element type has no `operator<<`, the print won't compile.      |
| Eigen / linalg types are **vetoed** via `Scalar`.   | They have `Scalar` member typedefs that would otherwise match container detection — see `H5Uall.hpp`. Use the library's own print (`std::cout << M`) for those. |
| Set/unordered_set ordering is implementation-defined. | Inherited from the container; not something the inserter can control.   |

## Build State (as of HEAD)

| Target           | Status                                              |
| ---------------- | --------------------------------------------------- |
| `examples-pprint`| ✔ ok — every category prints, deep nesting renders. |

No external library dependencies.

## Cross-References

- **`h5cpp/H5Uall.hpp`** — where every `operator<<` lives, along with `H5CPP_CONSOLE_WIDTH` and the Walter Brown detection idiom.
- **`examples/container/`** — the same detection idiom applied to the *I/O* dispatch surface (Walter Brown for both pretty-print and `h5::write` / `h5::read`).
- **`examples/datasets/`** — relies on these inserters to verify readbacks.

## Source

- [`pprint.cpp`](pprint_8cpp-example.html) — rendered with syntax highlighting
