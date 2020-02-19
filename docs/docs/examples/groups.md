## Groups[^1]

An HDF5 group is a structure containing zero or more HDF5 objects. A group has two parts:

1. A group header, which contains a group name and a list of group attributes.
2. A group symbol table, which is a list of the HDF5 objects that belong to the group.

```cpp
parent ::= fd_t | gr_t;

[open]
h5::gr_t gopen(parent, const std::string& path [, const h5::gapl_t& gapl]);

[create]
h5::gr_t  gcreate(const L& parent, const std::string& path,
	[, h5::lcpl_t lcpl] [, h5::gcpl_t gcpl] [, h5::gapl_t gapl]);
```

**Example:** adding attributes to a group
```cpp
auto gr = h5::gopen(fd, "/mygroup");
std::initializer_list list = {1,2,3,4,5};
h5::awrite(gr, std::make_tuple(
	"temperature", 42.0,
	"unit", "C",
	"vector of ints", std::vector<int>({1,2,3,4,5}),
	"initializer list", list,
	"strings", std::initializer_list({"first", "second", "third","..."})
));
```

### Examples:
The examples are to demonstrate how to use HDF5 groups as well as how to add attributes to them

* creating groups with an `h5::fd_t` file handle and `h5::gr_t` group handle as parents
* error handling:
	* catching specific `h5::error::io::group::create`
	* using umbrella exception `h5::error::any`
* how to open a group and add complex set of attributes with a single call 

[^1]: Lifted from HDF5 CAPI documentation

