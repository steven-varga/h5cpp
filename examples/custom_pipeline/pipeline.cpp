#include "h5cpp/all"

int main() {
  std::vector<double> v(100, 2.);
  auto fd = h5::create("test.h5", H5F_ACC_TRUNC);
  //FIXME: reference count problems...
  h5::write(fd, "dataset", v, h5::chunk{2}, h5::high_throughput);
}
