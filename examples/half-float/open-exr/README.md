Industrial Light and Magick - OpenEXR half float support
----------------------------------------------

Obtain OpenEXR half float implementation: [ilmbase-x.y.z.tar.gz](https://www.openexr.com/downloads.html) and untar into some directory, configure and install
```
tar -xzf ilmbase-2.3.0.tar.gz
./configure --prefix=/usr/local
make -j4 && sudo make install
```

usage and caveats
--------
`-D WITH_OPENXDR_HALF` activates the shim code, since the internal include guards are `_HALF_H` deemed to be inadequate, to make it even less cheerful OpenEXR half precision add-on is **not embedded in namespace**. As a workaround, there is an optional by default empty `OPENEXR_NAMESPACE` macro to control namespaces. In order to make this work **you must hack the OpenEXR library** and recompile it with **namespace added**.

**compiler assisted reflection:** not available, as half is not a POD type

*Contacting listed authors failed, posted on mailing list, waiting for their reply.*


The IEEE 754 standard specifies a binary16 as having the following format:
- Sign bit: 1 bit
- Exponent width: 5 bits
- Significand precision: 11 bits (10 explicitly stored)

Layout:
```
15 (msb)
| 
| 14  10
| |   |
| |   | 9        0 (lsb)
| |   | |        |
X XXXXX XXXXXXXXXX
```

Examples:
```
0 00000 0000000000 = 0.0
0 01110 0000000000 = 0.5
0 01111 0000000000 = 1.0
0 10000 0000000000 = 2.0
0 10000 1000000000 = 3.0
1 10101 1111000001 = -124.0625
0 11111 0000000000 = +infinity
1 11111 0000000000 = -infinity
0 11111 1000000000 = NAN
1 11111 1111111111 = NAN
```



The format is laid out as follows:
`[1 bit sign | 5 bit exponent | 10 bit mantissa]` the exponent encoded using an offset-binary representation, with the zero offset being 15.

```bash
HDF5 "example.h5" {
GROUP "/" {
   DATASET "type" {
      DATATYPE  16-bit little-endian floating-point
      DATASPACE  SIMPLE { ( 20 ) / ( 20 ) }
      DATA {
      (0): 55.7812, 89.375, 90.125, -29.3906, -90.3125, -48.4688, -1.17871,
      (7): 55.1875, -21.6719, 12.7578, -63.7188, 9.22656, 50.625, 37.0938,
      (14): -81.8125, -32.7812, 89.5, -28.3906, -44.125, -41.125
      }
   }
}
}
```
