// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
#pragma once

// POD struct declarations consumed by `generated.h` (which registers them as
// HDF5 compound types via H5CPP_REGISTER_STRUCT). No h5cpp dependency in this
// header — pure layout.

typedef unsigned long long int MyUInt;

namespace sn {
    namespace typecheck {
        struct Record {                    // every natively-mapped scalar
            char _char;  unsigned char  _uchar;
            short _short; unsigned short _ushort;
            int _int;     unsigned int   _uint;
            long _long;   unsigned long  _ulong;
            long long _llong; unsigned long long _ullong;
            float _float; double _double; long double _ldouble;
            bool _bool;
        };
    }
    namespace other {
        struct Record {                    // typedef + arrays + nested compound
            MyUInt              idx;
            MyUInt              aa;
            double              field_02[3];
            typecheck::Record   field_03[4];
        };
    }
    namespace example {
        struct Record {                    // arrays of arrays + repeated nested type
            MyUInt              idx;
            float               field_02[7];
            sn::other::Record   field_03[5];
            sn::other::Record   field_04[5];   // same shape as field_03 — dedup test
            sn::other::Record   field_05[3][8];
        };
    }
}
