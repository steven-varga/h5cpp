// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Driver TU.  This file knows nothing about the record types — only that the
// other TUs export their entry points.  All it needs from h5cpp is h5::fd_t,
// which lives in the file-handle slice of <h5cpp/all>.
//
// No `generated.h` include here: this TU never names `sn::example::record_t`
// directly, so the compound registration would just be dead weight.

#include <h5cpp/all>

void tu_01_linalg_and_create(const h5::fd_t& fd);
void tu_02_pod_vector_round_trip(const h5::fd_t& fd);

int main() {
    h5::fd_t fd = h5::create("multi-tu.h5", H5F_ACC_TRUNC);
    tu_01_linalg_and_create(fd);
    tu_02_pod_vector_round_trip(fd);
    return 0;
}
