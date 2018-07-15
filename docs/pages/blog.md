

1) 
Few years back then, I was looking for data container with low latency access to stored objects, and possibly support for linear algebra as well as streams. While protocol buffers offered stream support, was lacking of indexed block access. Soon I realized I was looking for something like TIFF, a container with file system like properties. When examined HDF5 I got very close to what I needed to store financial engineering datasets. In 2011 HDF5 had good support for full and partial read write for high dimensional extendable datasets with optional compression. Also scientific platforms such as Matlab and R supported HDF5 and most importantly worked across various operating systems.

Custom storage for each data format has the advantage of focusing on components which is needed; in the past we've seen various image formats, databases for 3d meshes and so on; machine learning / data science is an emerging field where data-storage is a necessary part but not the main attraction.  Which requires a general fast, block and sequential access, capable of storing the observations used for model building. HDF5 does provide basic building blocks for the role, but there is a gap between what offers and what was needed.

Researchers and engineers working directly with popular linear algebra libraries, complex POD structures or time series with tight bounds on latency and throughput benefit from using H5CPP template libraries, while engineers who need fast storage solution for arbitrary complex POD struct types already available in C/C++ header files benefit from H5CPP clang based compiler technology.

2) The current C++ HDF5 approach considers C++ as a different language from C, and reproduces the CAPI calls, adding only marginal value. Also  existing C/C++ library is lacking of high performance packet writing capability, seamless POD structure transformation to HDF5 compound types and has no support for popular matrix algebra libraries and STL containers. In fact HDF5 C++ doesn't consider  C++ templates at all; whereas modern C++ is about templates, and template meta-programming paradigms.

The original design criteria was to implement an intuitive, easy to use template based library that supports most major linear algebra systems,  with read, write and append operations. This work may be freely downloaded from my [GitHub page][1], and doxygen based documentation is  [here][2].
However in the in the past few month, in co-operation with [Gerd Heber, HDFGroup][3],  I've been  engaged with a design and [implementation of new, unique interface][4]: a mixture of Gerd's idea of having something python-ishly flexible, but instead of using dictionary based named argument passing mechanism, I proposed sexy [EBNF grammar][6], implemented in C++ template meta programming.
This unique C++ API  gets you to start coding without any knowledge of HDF5 API, yet it provides extensive support for the details when you're ready for it.

The type system is hidden behind templates, and IO calls will do the right thing. In addition to templates, an [optional clang based compiler][5] scans your project source files, detects all C/C++ POD structs being referenced by H5CPP calls, then from the topologically sorted nodes produces the HDF5 Compound type transformations. The HDF5 DDL is required to do operations with HDF5 Compound datatypes, and can be a tedious process to do the old fashioned way when you have a large complex project. If you know protocolbuffer or thift, which produces source code from DDL -- what you see here is the exact opposite:
	the compiler takes arbitrary C/C++ source code and produces HDF5 Compound type DDL. [DDL stands for Data Description Language]
The above mechanism works with arbitrary depth of fundamental, array types and POD struct types.

Other design considerations: 

- RAII idiom for [resource management][10] prevents leakage 
- [conversion policy][11] how software writers can reach CAPI from seamless integration to restricted explicit conversion
- error handling policy: exceptions or no-excpetions
- [static polymorphism][20] (CRTP idiom) instead of [runtime polymorphism][21]
- [compile time expressions][23], [copy elision][22] and return value optimization
- polished design

3) It would be too early to speak of future of a not yet completed project,  but most likely it will follow the law of economics: increased demand will positively affect production.


[1]: http://github.com/steven-varga/h5cpp
[2]: http://h5cpp.ca
[3]: https://github.com/gheber
[4]: http://sandbox.h5cpp.ca
[5]: http://sandbox.h5cpp.ca/compiler
[6]: https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form
[10]: http://sandbox.h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_conversion.html#link_raii_idiom
[11]: http://sandbox.h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_conversion.html#link_conversion_policy
[20]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
[21]: https://en.wikipedia.org/wiki/Virtual_method_table
[22]: https://en.wikipedia.org/wiki/Copy_elision
[23]: http://en.cppreference.com/w/cpp/language/constant_expression

