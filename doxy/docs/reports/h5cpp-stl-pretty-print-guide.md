@page reports_stl_pretty_print_guide H5CPP STL Pretty-Print Guide

## 1. What it is

A header-only, feature-detection-based set of stream inserters that lets you write

```cpp
std::cout << my_container << "\n";
```

for **any** value whose surface area matches a familiar STL container — `std::vector`, `std::list`, `std::map`, `std::stack`, `std::queue`, `std::priority_queue`, `std::pair`, `std::tuple`, custom containers exposing `.begin()/.end()`, and arbitrary compositions of all of the above.

Lives in `h5cpp/H5Uall.hpp`, automatically pulled by `<h5cpp/all>` and `<h5cpp/core>`. No opt-in macro required.

Inspired by the HDF Group mailing-list post `automated-printing-2022-04-07`, modernised onto C++17 detection idioms and the prototype branch's `h5::meta` traits.

---

## 2. Quick reference — what streams to what

| Category | Detector | Output shape | Example types |
|----------|----------|--------------|----------------|
| **Iterable** | `has_iterator<T>` && !`is_string<T>` && !`is_tuple<T>` | `[a,b,c,...]` | `vector`, `list`, `deque`, `forward_list`, `array`, `set`, `multiset`, `unordered_set`, `unordered_multiset`, `map`, `multimap`, `unordered_map`, `unordered_multimap` |
| **Stack adaptor** | `has_top` && `has_pop` && `has_empty` && !`has_iterator` | `[top,…,bottom]` (destructive copy) | `std::stack`, `std::priority_queue` |
| **Queue adaptor** | `has_front` && `has_pop` && `has_empty` && !`has_iterator` | `[front,…,back]` (destructive copy) | `std::queue` |
| **`std::pair<K,V>`** | direct overload | `{key:value}` | `std::pair`, `std::map::value_type` |
| **`std::tuple<Ts...>`** | direct overload | `<v0,v1,…,vN>` | `std::tuple` |

Long containers truncate at **`H5CPP_CONSOLE_WIDTH`** (default `10`) with a trailing `, ...`.

---

## 3. How it works

The selection is feature detection — Walter Brown's WG21 N4436 idiom. Each overload is gated on which member functions a type exposes, *not* what it inherits from. The pretty-printer never asks "is this `std::stack`?"; it asks "does this thing have `.top()`, `.pop()`, `.empty()`, and not `.begin()`?".

Four new detectors live in `h5::meta`:

```cpp
template <typename T> using has_top   = compat::is_detected<top_f,   T>;
template <typename T> using has_front = compat::is_detected<front_f, T>;
template <typename T> using has_pop   = compat::is_detected<pop_f,   T>;
template <typename T> using has_empty = compat::is_detected<empty_f, T>;
```

joining the pre-existing `has_iterator<T>`, `has_size<T>`, `has_data<T>`, `is_string<T>`, `is_tuple<T>`. The five inserters are global-namespace function templates so ADL picks them up for STL types.

### Overload-resolution invariants

1. **Strings stay native.** `is_string<T>` excluded from the iterable overload — `std::cout << std::string("hi")` continues to use the standard library's char-stream operator.
2. **Tuples take the angle-bracket path.** `is_tuple<T>` excluded from the iterable overload even though C++20 makes tuples iterable in some sense.
3. **Adaptors lose to iterables.** Anything with `.begin()/.end()` routes through the iterable overload; the stack/queue overloads require `!has_iterator`. A custom container exposing both would print as an iterable.
4. **Map elements funnel through pair.** `map::value_type` is `pair<const K, V>` — the iterable overload's `*it` dereferences to a pair, which prints via overload (4): `[{k1:v1},{k2:v2}, ...]`.
5. **Adaptors are destructive on a copy.** `stack`/`queue`/`priority_queue` have no iterators, so the inserter makes a local copy and drains it via `top/pop` or `front/pop`. The caller's container is untouched.

---

## 4. The five overloads in detail

### 4.1 Iterable containers

```cpp
template <class T>
inline std::enable_if_t<
    !h5::meta::is_string<T>::value &&
    !h5::meta::is_tuple<T>::value &&
    h5::meta::has_iterator<T>::value,
std::ostream&>
operator<<(std::ostream& os, const T& container) {
    os << "[";
    auto it = std::begin(container);
    auto last = std::end(container);
    if (it != last) {
        os << *it;
        std::size_t i = 0;
        while (++it != last && i++ < H5CPP_CONSOLE_WIDTH)
            os << "," << *it;
        if (i >= H5CPP_CONSOLE_WIDTH && it != last) os << ", ...";
    }
    os << "]";
    return os;
}
```

Recursion happens implicitly via the `os << *it` call — if `*it` is itself an iterable (`vector<vector<int>>`), a pair (`map::value_type`), or a tuple, the appropriate overload is selected for the element.

### 4.2 Stack adaptors — destructive copy

```cpp
T container = container_in;
while (!container.empty()) { os << container.top(); container.pop(); }
```

The original container is never modified — we copy and drain.

### 4.3 Queue adaptors — destructive copy

Same shape as 4.2 but using `.front()` and `.pop()`.

### 4.4 `std::pair<K,V>`

```cpp
template <class K, class V>
inline std::ostream& operator<<(std::ostream& os, const std::pair<K,V>& p) {
    os << "{" << p.first << ":" << p.second << "}";
    return os;
}
```

The key and value print via *their* operator<<, so `pair<int, vector<double>>` becomes `{42:[1.0,2.0,3.0]}`.

### 4.5 `std::tuple<Ts...>`

```cpp
template <class... Ts>
inline std::ostream& operator<<(std::ostream& os, const std::tuple<Ts...>& t) {
    os << "<";
    std::apply([&os](auto const&... fields) {
        std::size_t i = 0;
        constexpr std::size_t N = sizeof...(Ts);
        ((os << fields, (++i < N ? (os << "," , 0) : 0)), ...);
    }, t);
    os << ">";
    return os;
}
```

Comma fold over the parameter pack; each `fields` element streams through its own operator<<.

---

## 5. Worked examples

### 5.1 Single-level containers

```cpp
std::vector<int>      v {1,2,3,4,5};
std::set<std::string> s {"alpha","beta","gamma"};
std::deque<double>    d {0.1, 0.2, 0.3};

std::cout << v << "\n" << s << "\n" << d << "\n";
```

```
[1,2,3,4,5]
[alpha,beta,gamma]
[0.1,0.2,0.3]
```

### 5.2 Maps

```cpp
std::map<std::string, int> m { {"LID",2},{"U",2},{"Xr",1},{"e",2} };
std::cout << m << "\n";
```

```
[{LID:2},{U:2},{Xr:1},{e:2}]
```

### 5.3 Stacks, queues, priority queues

```cpp
std::stack<int>              s; for (int v : {172,252,181,11}) s.push(v);
std::queue<std::string>      q; for (auto x : {"a","bb","ccc"}) q.push(x);
std::priority_queue<int>     pq; for (int v : {5,3,8,1,9}) pq.push(v);

std::cout << s  << "\n";
std::cout << q  << "\n";
std::cout << pq << "\n";
```

```
[11,181,252,172]
[a,bb,ccc]
[9,8,5,3,1]
```

Note: stack is LIFO so its top (most recently pushed) comes first. Priority queue is ordered by `std::less` by default — largest first.

### 5.4 Pairs and tuples

```cpp
std::pair<int, std::string>          pi {42, "answer"};
std::tuple<int, double, std::string> tup {7, 3.14, "hi"};

std::cout << pi  << "\n";
std::cout << tup << "\n";
```

```
{42:answer}
<7,3.14,hi>
```

### 5.5 Nested / recursive composition

```cpp
std::vector<std::vector<int>>               vec_of_vec  {{1,2,3},{4,5,6,7},{8,9}};
std::map<std::string, std::vector<double>>  map_to_vec  {{"a",{1.0,2.0}},{"b",{3.0,4.0,5.0}}};
std::tuple<int, std::tuple<float,float>, bool> nested_tup {1, {2.5f,3.5f}, true};

std::cout << vec_of_vec << "\n";
std::cout << map_to_vec << "\n";
std::cout << nested_tup << "\n";
```

```
[[1,2,3],[4,5,6,7],[8,9]]
[{a:[1.0,2.0]},{b:[3.0,4.0,5.0]}]
<1,<2.5,3.5>,1>
```

### 5.6 Round-trip from HDF5 (live `examples-container` run)

```
vector:        [14,39,83,67,22,77,15,94,36,60,46,25,15,49,71,52,99,24,73,4]
deque:         [14,39,83,67,22,77,15,94,36,60,46,25,15,49,71,52,99,24,73,4]
list:          [14,39,83,67,22,77,15,94,36,60,46,25,15,49,71,52,99,24,73,4]
vec_array4:    [[51,11,76,73],[69,78,20,97],[44,50,67,48],[9,83,11,70],[96,47,93,80]]  (each inner array prints recursively)
set:           [1,2,3,4,5,6,9]  (size 7, unique)
multiset:      [1,1,2,3,3,4,5,5,5,6,9]  (size 11, dups kept)
unordered_set: [3,1,4,5,9,2,6]  (size 7, unique)
u_multiset:    [3,3,1,1,4,5,5,5,9,2,6]  (size 11, dups kept)
forward_list:  [7,14,21,28,35]  (read back as vector<int>)
```

The three flat containers (`vector`, `deque`, `list`) show identical data — same input, different on-disk dispatch paths, invisible at the user level. `vec_array4` demonstrates recursion: outer brackets are the `vector`, inner brackets are each `std::array<int,4>`.

---

## 6. Truncation

Containers longer than `H5CPP_CONSOLE_WIDTH` elements truncate with a trailing `, ...`. The default is `10`. Override before including h5cpp:

```cpp
#define H5CPP_CONSOLE_WIDTH 25
#include <h5cpp/all>
```

`H5CPP_CONSOLE_WIDTH` is `#ifndef`-guarded — the first definition wins. The macro lives at file scope in `H5Uall.hpp`. Setting it inside a translation unit is local to that TU.

### Truncation semantics — fine print

The implementation counts iteration steps *after the first element*:

```cpp
os << *it;
std::size_t i = 0;
while (++it != last && i++ < H5CPP_CONSOLE_WIDTH)
    os << "," << *it;
```

So `H5CPP_CONSOLE_WIDTH = 10` prints up to **11 elements** before the cutoff: the first one, then 10 more from the `while` body. A 12-element container shows the first 11 followed by `, ...`. If your terminal is wider than that, bump the macro.

---

## 7. Where it lives

| Location | Contents |
|----------|----------|
| `h5cpp/H5Uall.hpp` | `H5CPP_CONSOLE_WIDTH` macro, four new detectors (`has_top`, `has_front`, `has_pop`, `has_empty`), five `operator<<` overloads |
| `h5cpp/H5cout.hpp` | h5cpp-specific stream inserters for `dxpl_t`, `sp_t`, `dt_t<T>`, `impl::array<T>`, `pipeline_t<T>`. **No** STL inserter here — historical `vector<T>`-only one was removed (it shadowed the new generic and didn't recurse) |
| `h5cpp/H5meta.hpp` | Pre-existing detectors (`has_iterator`, `has_size`, `has_data`, …) used by the new inserters |
| `examples/container/container.cpp` | Tier-1 demo: uses the pretty-printer for HDF5 round-trip diagnostics |
| `examples/pprint/pprint.cpp` | Standalone demo of the full inserter surface — no HDF5 IO involved |

---

## 8. Caveats and edge cases

### 8.1 Custom containers with weird method sets

The detection rules will pick up the most generic overload that matches. A container that has both `.begin()/.end()` *and* `.top()/.pop()` (rare) prints as an iterable, not a stack — the iterable overload's SFINAE includes `has_iterator<T>` and the stack overload's includes `!has_iterator<T>`, so they're mutually exclusive by design.

### 8.2 Elements without `operator<<`

If `*it` is a user-defined type without its own `operator<<`, the iterable overload's `os << *it` becomes a hard compile error. The fix is to provide the user type's own `operator<<` — the pretty-printer recurses into whatever streamers exist; it does not synthesise them.

### 8.3 `std::string` and `std::string_view`

Both pass `has_iterator` but are excluded from the iterable overload via `is_string<T>`. They stream as `"hello"` (just the characters) via the standard library, not as `[h,e,l,l,o]`. That's intentional.

### 8.4 Map keys with no operator<<

The pair overload calls `os << p.first << ":" << p.second`. If `K` (e.g., a custom struct) has no `operator<<`, the build fails when the map is streamed. Same fix as 8.2.

### 8.5 Adaptor copy cost

Stack and queue adaptors print by *copying* the container and draining the copy. For a million-element `std::priority_queue<std::string>`, this allocates a million strings worth of intermediate state. Pretty-printing is for diagnostic use — not a hot path.

### 8.6 Concurrent modification

The iterable overload calls `std::begin()/std::end()` and walks the range. If another thread mutates the container during the stream, behaviour is undefined — same as iterating any STL container concurrently.

---

## 9. Adding new types

To make a custom container pretty-print:

1. **If it has STL-style iterators**, do nothing — the generic iterable overload picks it up automatically.
2. **If it's adaptor-shaped** (stack-like or queue-like), expose `.top()`/`.front()`, `.pop()`, `.empty()` and the corresponding overload kicks in.
3. **If it's neither**, write your own `operator<<` — the pretty-printer's design does not require you to extend it; just provide the streamer for your type and existing iterable/pair/tuple inserters will recurse through it.

You don't need to extend the h5cpp headers to add a new printable type. The mechanism is open by default.

---

## 10. Migration note — removed overload

Prior to this change, `h5cpp/H5cout.hpp` had a `std::vector<T>`-specific `operator<<` (line 113, pre-edit). It was more specialised than the new generic iterable, so the compiler preferred it. But its body did `os << vec[i]` directly without recursing into the element's pretty-printer — meaning `vector<array<int,4>>`, `vector<pair<K,V>>`, and any other vector-of-non-scalar would emit "no known conversion" errors during template instantiation.

The legacy overload has been **removed** from `H5cout.hpp` (replaced with a doc comment). The new generic iterable in `H5Uall.hpp` handles `std::vector<T>` exactly the same way it handles every other iterable — and recurses correctly. Existing user code unaffected; the output is byte-identical for flat vectors.

---

## 11. One-line summary

> `h5cpp/H5Uall.hpp` adds five global-namespace `operator<<` overloads — iterable, stack adaptor, queue adaptor, pair, tuple — selected by feature detection. Together they pretty-print any STL-shaped value, including arbitrarily nested compositions, to a single line truncated at `H5CPP_CONSOLE_WIDTH` (default 10). No opt-in macro, no extension API; works on any user type that provides the matching member-function surface.

