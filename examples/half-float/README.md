half float support
--------------------

Obtain Chistian Rau's half float implementation [from this link](http://half.sourceforge.net/)

The IEEE 754 standard specifies a binary16 as having the following format:
- Sign bit: 1 bit
- Exponent width: 5 bits
- Significand precision: 11 bits (10 explicitly stored)

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
