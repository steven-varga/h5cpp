## Attributes[^1]

An HDF5 attribute is a small metadata object describing the nature and/or intended usage of a primary data object. A primary data object may be a dataset `h5::ds_t`, group `h5::gr_t`, or committed datatype `h5::dt_t` and in this documentation will be referred as `parent` or `location`.

Attributes are assumed to be very small as data objects go, so storing them as standard HDF5 datasets would be quite inefficient. HDF5 attributes are therefore managed through a special attributes interface, H5A, which is designed to easily attach attributes to primary data objects as small datasets containing metadata information and to minimize storage requirements.

Consider, as examples of the simplest case, a set of laboratory readings taken under known temperature and pressure conditions of 18.0 degrees celsius and 0.5 atmospheres, respectively. The temperature and pressure stored as attributes of the dataset could be described as the following name/value pairs:

* temp=18.0
* pressure=0.5

**example:**

```cpp
#include <h5cpp/all>
...
auto ds = h5::open("my-cntainer.h5","my-dataset");
ds["temperature"] = 18.0;
ds["pressure"] = 0.5;
...
```


**While HDF5 attributes are not standard HDF5 datasets, they have much in common:**

* An attribute has a user-defined dataspace and the included metadata has a user-assigned datatype
* Metadata can be of any valid HDF5 datatype
* Attributes are addressed by name

**But there are some very important differences:**

* There is no provision for special storage such as compression or chunking
* There is no partial I/O or sub-setting capability for attribute data
* Attributes cannot be shared
* Attributes cannot have attributes
* Being small, an attribute is stored in the object header of the object it describes and is thus attached directly to that object

### Objects
```yacc
pod             := arbitrary plain old dataype - consequtive memory region:
				   automatic compiler assisted reflection or manual handling
integral        := [ unsigned | signed ] [int_8 | int_16 | int_32 | int_64 | float | double ] 
string          := std::string | char*
scalar          := integral | pod_struct | string
vector          := std::vector<scalar>
initializer_list:= std::initializer_list<scalar>{}
linalg          := armadillo | eigen | ... 

T ::= scalar | vector | initializer_list | linalg
```



### IO Templates

#### Single Objects
```cpp
parent ::= h5::gr_t | h5::ds_t | h5::dt_t | h5::at_t;

[open]
h5::at_t h5::aopen(parent, const std::string& name [, const & acpl] );

[create]
h5::at_t acreate<T>( parent, const std::string& name 
	[, const h5::current_dims{...} ] [, const h5::acpl_t& acpl]);

[read]
T aread( parent, const std::string& name [, const h5::acpl_t& acpl]) const;

[write]
void awrite( parent, const std::string &name, const T& obj  [, const h5::acpl_t& acpl]);
```

#### Multiple Objects

Passing `std::tuple<std::string 0, T0 V_0, std::string 1, T1 V1, ..., std::string n, Tn Vn>` a sequence of name - value pairs, a meta template 
breaks up the call into a sequence of `awrite` calls at compile time.
``` cpp
void awrite( parent, const std::string &name, const std::tuple<T...>& objects  [, const h5::acpl_t& acpl]);
```
This mechanism comes handy when dealing with many attributes. Typing IO operators is boring repetitive process killing the flow, bundling multiple calls into a single operations with `std::tuple<...>` type seems to be natural:

```cpp
...
h5::gr_t gr = h5::gcreate(fd, "my-group" );

arma::mat matrix = arma::zeros(3,4); 
std::vector<sn::example::Record> vector = h5::utils::get_test_data<sn::example::Record>(40);
sn::example::Record& record = vector[0];

h5::awrite(gr, std::make_tuple(
	"temperature", 18.0,
	"pressure",    0.5,
	"matrix",      matrix, // linear algebra object
	"vector",      vector, // std::vector of POD type
	"pod struct",  record  // single POD type
	));
```



### Examples:
The examples are to demonstrate how to use HDF5 attribute with various object types

* writing attributes to dataset with `ds_t['attribute_name'] = attribute;`
* using: `h5::awrite`, `h5::aread`, `h5::create`
* compiler assisted reflection with `h5cpp` code transformation tool
* bundling objects for single shot write with `std::tuple<T...>`

[^1]: Lifted from HDF5 CAPI documentation

