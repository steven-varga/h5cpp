# Dispatch Mechanism


* POD struct
* integral_types: [bool, char, char16_t, char32_t, wchar_t, short, int, long, long long]
* floating point: float, double, long double

### Templates

- `h5::rank<T>::value` returns the number of dimensions of all objects recognized by H5CPP.

- `h5::decay<T>type` recusrsively fiinds the element type of an object

- `std::array<size_t,N> h5::size()` runtime call to find out the `rank` and `extents` of an object 

void, std::nullptr: 

integral types: ( signed | unsigned) char, short, long, long long
floating point: float, double, long double

## Contiguous Memory Layouts

std::array<T,N>, std::vector<T>, 

## Other
container concept: itarators
std::deque<T> ,std::list, std::forward_list<T>, std::set<T>, std::multi_set<T>, std::undordered_set<T>, std::unordered_multiset<T>, 
std::map<K,V>, std::multi_map<K,V>, std::unordered_map<K,V>, std::unordered_multimap<K,V>

adapters:
std::stack<T>, std::queue<T>, std::priority_queue<T> 

code: 6487

## Type System
`h5::is_numerical<T>` returns `true` for the following types
```
numerical ::= [signed | unsigned](char | short | int | long | long int, long long int) | (float | double | long double)
scalar ::= numerical | pod
```
