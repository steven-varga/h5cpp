

Few years ago, I was looking for a data format with low latency block and stream support. While protocol buffers offered streams, it was lacking of indexed block access. Soon I realized I was looking for a container with file system like properties. When examined HDF5 I got very close to what I needed to store massive financial engineering datasets. In 2011 HDF5 had good support for full and partial read write operations for high dimensional extendable datasets with optional compression. Also scientific platforms such as Python, Julia, R and Matlab supported HDF5 and most importantly worked across operating systems.

<img src="../pix/financial_eng.jpg" alt="drawing" style="width:400px; float:right;"/>


Machine learning / data science is an emerging field where data-storage is necessary part but not the main attraction. Data science requires a general fast, block and sequential access, capable of storing the observations used for model building. HDF5 does provide basic building blocks for the role, but there is a gap between what it offers and what's provided.

<!--
number of functions
pt: 15, tbale: 10  data: 27 file: 36 error: 30  group: 27 ident: 30
--> 

Researchers working directly with popular linear algebra libraries, the STL or time series can benefit of H5CPP template library's [CRUD like][23] low latency operations. While engineers who need fast storage solution for arbitrary complex POD struct types -- often already available in C/C++ header files -- benefit from H5CPP clang based compiler technology.

The current [HDF5 C++][30] approach considers C++ as a different language from C, and reproduces the CAPI calls, adding only marginal value. Also  existing C/C++ library is lacking of high performance packet writing capability, seamless POD structure transformation to HDF5 compound types and has no support for popular matrix algebra libraries and STL containers. In fact HDF5 C++ doesn't consider  C++ templates at all; whereas modern C++ is about templates, and template meta-programming paradigms.

The original design criteria was to implement an intuitive, easy to use template based library that supports most major linear algebra systems,  with create, read, write and append operations. This work may be freely downloaded from [ this h5cpp11 page][1].
However in the in the past few months, in co-operation with [Gerd Heber, HDFGroup][3],  I've been  engaged with a design and [implementation of new, unique interface][2]: a mixture of Gerd's idea of having something python-ishly flexible, but instead of using dictionary based named argument passing mechanism, I proposed a sexy [EBNF grammar][6], implemented in C++ template meta programming.
This unique C++ API allows you to start coding without any knowledge of HDF5 API, yet it provides ample of room for the details when you need them.

The type system is hidden behind templates, and IO calls will do the right thing. In addition to templates, an [optional clang based compiler][5] scans your project source files, detects all C/C++ POD structures being referenced by H5CPP calls, then from the topologically sorted nodes produces the HDF5 Compound type transformations. The HDF5 DDL is required to do operations with HDF5 Compound datatypes, and can be a tedious process to do [the old fashioned way][40] when you have a large complex project. DDL to source code transformation has been around for decades: [protocol buffers][40] or [apache thrift][50] are good examples -- however what H5CPP compiler does is the exact opposite:
	it takes arbitrary C/C++ source code and produces HDF5 Compound type DDL. [DDL stands for Data Description Language]
The above mechanism works with arbitrary depth of fundamental, array types and POD struct types.

Other design considerations:
- RAII idiom for [resource management][10] to prevent leakage 
- [conversion policy][11] how software writers can reach CAPI from seamless integration to restricted explicit conversion
- error handling policy: exceptions or no-excpetions
- [static polymorphism][20] (CRTP idiom) instead of [runtime polymorphism][21], no virtual table lookups
- [compile time expressions][23], [copy elision][22] and return value optimization
- polished design

H5cpp aims to solve real life problems you as a software writer, engineer, scientist or project manager meet and worth solving.  [Try the live demo][100] and [download the project][4].


[1]: http://github.com/steven-varga/h5cpp11
[2]: http://h5cpp.ca
[3]: https://github.com/gheber
[4]: http://github.com/steven-varga/h5cpp
[5]: http://h5cpp.ca/compiler
[6]: https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form
[10]: http://sandbox.h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_conversion.html#link_raii_idiom
[11]: http://sandbox.h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_conversion.html#link_conversion_policy
[20]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
[21]: https://en.wikipedia.org/wiki/Virtual_method_table
[22]: https://en.wikipedia.org/wiki/Copy_elision
[23]: https://en.wikipedia.org/wiki/Create,_read,_update_and_delete
[23]: http://en.cppreference.com/w/cpp/language/constant_expression
[30]:https://support.hdfgroup.org/HDF5/doc/cpplus_RM/index.html
[40]: https://thrift.apache.org/
[41]: https://en.wikipedia.org/wiki/Protocol_Buffers
[50]: http://h5cpp.ca/compound_8c-example.html
[100]: http://h5cpp.ca/cgi/redirect.py


