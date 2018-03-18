<!---

 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this  software  and associated documentation files (the "Software"), to deal in
 the Software  without   restriction, including without limitation the rights to
 use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 of the Software, and to  permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--->



performance: 
------------
|    experiment                               | time  | trans/sec | Mbyte/sec |
|:--------------------------------------------|------:|----------:|----------:|
|append:  1E6 x 64byte struct                 |  0.06 |   16.46E6 |   1053.87 |
|append: 10E6 x 64byte struct                 |  0.63 |   15.86E6 |   1015.49 |
|append: 50E6 x 64byte struct                 |  8.46 |    5.90E6 |    377.91 |
|append:100E6 x 64byte struct                 | 24.58 |    4.06E6 |    260.91 |
|write:  Matrix<float> [10e6 x  16] no-chunk  |  0.4  |    0.89E6 |   1597.74 |
|write:  Matrix<float> [10e6 x 100] no-chunk  |  7.1  |    1.40E6 |    563.36 |

Lenovo 230 i7 8G ram laptop on Linux Mint 18.1 system



**gprof** directory contains [gperf][1] tools base profiling. `make all` will compile files.
In order to execute install  `google-pprof` and `kcachegrind`.  

```shell
sudo apt install google-perftools
sudo apt install kcachegrind
```

To  invoke google profiler and cachgrind visualizer type `make ????-profile`.



Initial result [c++ struct][40], [stl vector][41] and [armadillo][42] indicate the library adds minimal overhead to
[HDF5][50] calls.

**grid-engine** based detailed test suite is in progress. This is a massive iterative case executed on AWS EC2 based 
Grid-Engine to measure performance. While the tests may be executed on a single process a parallel batch processor 
such as Son of Grid Engine speeds things up.


[1]:  https://github.com/gperftools/gperftools
[40]: http://h5cpp.ca/pix/perf-struct.png 
[41]: http://h5cpp.ca/pix/perf-stl.png
[42]: http://h5cpp.ca/pix/perf-armadillo.png 

[50]: https://www.hdfgroup.org/
