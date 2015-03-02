#include "block_sampler.h"

int
main() {
  BlockDownSampler<8,4> b;
  uint32_t data[] = {
      1,1,1,1,1,1,1,1,
      1,2,1,2,1,2,1,2,
      1,1,2,2,2,2,2,2,
      1,2,2,2,2,2,2,2
  };
  b.insert(data);
  b.downsample(4);
  b.print_original();
  b.print_downsampled();

  return 0;
}
