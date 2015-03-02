#ifndef blocksampler_h
#define blocksampler_h

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include "array.h"

#define THREADING

template <size_t L1, const size_t L2>
class BlockDownSampler {
private:
  array orignal;
  std::vector<array> downsampled;
  std::mutex mtx;

public:
  BlockDownSampler(): orignal(L1, L2) {}
  
  void
  insert(uint32_t *in) {
    orignal.insert(in);
  }
  
  void 
  alloc_downsampled_img(std::vector<size_t> sizes, uint32_t resize_factor) {
    downsampled.push_back(array(sizes[0] / resize_factor, 
                                sizes[1] / resize_factor)); 
  }
  
  void 
  calc_mode_for_all_blocks(uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;
    
    for (int row = 0; row < orignal.row_size(); row+=blocksize) {
      for (int col = 0; col < orignal.col_size(); col+=blocksize) {
        uint32_t mode = orignal.get_mode_of_block_2d(row, col, blocksize, blocksize);
        std::unique_lock<std::mutex> lck(mtx);
        (downsampled[downsampled_index])[index] = mode;
        index++;
      }
    }
  }
  
  bool
  check_stop(uint32_t blocksize) {
    if (orignal.get_sizes()[0]/blocksize == 1 || 
        orignal.get_sizes()[1]/blocksize == 1) {
      return true;
    }
    
    return false;
  }
  
  void
  downsample(int l) {
    std::vector<std::thread> workers;
    uint32_t blocksize = 2;
    
    for (int i = 0; i < l; i++) {
#ifdef THREADING
      workers.push_back(std::thread(&BlockDownSampler::thread_downsample,
                                    this,
                                    i,
                                    blocksize)); 
#else
      alloc_downsampled_img(orignal.get_sizes(), blocksize);
      calc_mode_for_all_blocks(blocksize);
#endif

      if (check_stop(blocksize) == true) {
        break;
      }
      
      blocksize *= 2;
    }

#ifdef THREADING
    for (std::thread &t: workers) {
      if (t.joinable()) {
        t.join();
      }
    }
#endif
  }
  
  void
  thread_downsample(uint32_t downsampled_index, uint32_t blocksize) {
    alloc_downsampled_img(orignal.get_sizes(), blocksize);
    calc_mode_for_all_blocks(downsampled_index, blocksize);
  }
  
  void 
  print_original() {
    orignal.print();
  }
  
  void 
  print_downsampled() {
    for (int i = 0; i < downsampled.size(); i++) {
      downsampled[i].print();
    }
  }
};

#endif