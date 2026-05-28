// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
#pragma once

// POD struct declarations shared by every translation unit in this example.
// No h5cpp dependency here on purpose — this header is pure data layout.

typedef unsigned long long int my_uint_t;

namespace sn {
    namespace typecheck {
        struct record_t {                  // all natively-mapped scalar types
            char  _char;  unsigned char  _uchar;
            short _short; unsigned short _ushort;
            int   _int;   unsigned int   _uint;
            long  _long;  unsigned long  _ulong;
            long long _llong; unsigned long long _ullong;
            float _float; double _double; long double _ldouble;
            bool  _bool;
        };
    }
    namespace other {
        struct record_t {                  // typedef + arrays + nested compound
            my_uint_t           idx;
            my_uint_t           aa;
            double              field_02[3];
            typecheck::record_t field_03[4];
        };
    }
    namespace example {
        struct record_t {                  // arrays of arrays + repeated nested type
            my_uint_t            idx;
            float                field_02[7];
            sn::other::record_t  field_03[5];
            sn::other::record_t  field_04[5];  // dedup test: same shape as field_03
            sn::other::record_t  field_05[3][8];
        };
    }
}
