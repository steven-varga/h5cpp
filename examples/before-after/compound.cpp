#include "compound.h"
#include <h5cpp/core>
#include "generated.h"
#include <h5cpp/io>
#include <iostream>
#include <algorithm>
int
main()
{
  std::vector<s1_t> s1(LENGTH);



  std::generate( std::begin(s1), std::end(s1), [i=-1]() mutable {
	return s1_t{++i,static_cast<float>(i*i), 1.0/(i+1)};
  });















  auto fd = h5::create(H5FILE_NAME, H5F_ACC_TRUNC, h5::default_fcpl, h5::default_fapl);




  h5::write(fd, DATASETNAME, s1);











  auto data = h5::read< std::vector<s2_t> >(fd, DATASETNAME);

  std::cout << "reading back data previously written:\n\t";
  for (auto r:data)
      std::cout << r.c << " ";
  std::cout << std::endl;





  return 0;
}
