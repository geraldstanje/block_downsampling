#include "block_sampler.h"

int
main() {
  BlockDownSampler2d<8,4> b1;
  uint32_t data1[] = {
      1,1,1,1,1,1,1,1,
      1,2,1,2,1,2,1,2,
      1,1,2,2,2,2,2,2,
      1,2,2,2,2,2,2,2
  };
  b1.insert(data1);
  b1.downsample(4);
  b1.print_original();
  b1.print_downsampled();

  BlockDownSampler2d<8,8> b2;
  uint32_t data2[] = {
      0,0,0,0,0,0,0,0,
      1,0,1,0,1,0,1,0,
      1,1,0,0,1,1,0,0,
      1,1,1,0,1,1,1,0,
      0,0,0,0,1,1,1,1,
      1,0,1,0,1,1,1,1,
      1,1,1,0,1,1,0,0,
      1,1,0,0,1,1,0,1
  };
  b2.insert(data2);
  b2.downsample(4);
  b2.print_original();
  b2.print_downsampled();

  return 0;
}
