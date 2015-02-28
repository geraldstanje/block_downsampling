#include "block_sampler.h"

const int test_example = 4;

void createData(int *data, int size) {
  for (int i = 0; i < size; i++) {
    data[i] = i;
  }
}

int
main() {
  if (test_example == 1) {
    BlockSampler<8,1,1> b;

    int data[] = {
      2,1,1,1,2,2,2,2
    };

    //createData(data, 8);
    b.init(data);
    b.downsample(4);
    b.print_imgs();
  }
  else if (test_example == 2) {
    BlockSampler<4,8,1> b;

    int data[] = {
      1,1,1,1,1,1,1,1,
      1,2,1,2,1,2,1,2,
      1,1,2,2,2,2,2,2,
      1,2,2,2,2,2,2,2
    };

    b.init(data);
    b.downsample(4);
    b.print_imgs();
  }
  else if (test_example == 3) {
    BlockSampler<8,8,1> b;

    int data[] = {
      0,0,0,0,0,0,0,0,
      1,0,1,0,1,0,1,0,
      1,1,0,0,1,1,0,0,
      1,1,1,0,1,1,1,0,
      0,0,0,0,1,1,1,1,
      1,0,1,0,1,1,1,1,
      1,1,1,0,1,1,0,0,
      1,1,0,0,1,1,0,1
    };

    b.init(data);
    b.downsample(4);
    b.print_imgs();
  }
  else if (test_example == 4) {
    BlockSampler<4,4,4> b;

    int data[] = {
      1,1,1,1,
      2,2,2,2,
      1,1,2,2,
      2,2,2,2,

      2,2,1,1,
      1,2,1,2,
      1,2,2,1,
      1,1,2,2,

      1,1,1,1,
      2,2,2,2,
      1,1,2,2,
      2,2,2,2,
      
      2,2,1,1,
      1,2,1,2,
      1,2,2,1,
      1,1,2,2,
    };
    // expected: 2 1 
    //           1 2

    //createData(data, 64);
    b.init(data);
    b.downsample(4);
    b.print_imgs();
  }
}
