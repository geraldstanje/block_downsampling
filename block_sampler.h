#ifndef blocksampler_h
#define blocksampler_h

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include "array.h"

#define THREADING

class BlockDownSampler {
protected:
  array orignal;
  std::vector<array> downsampled;
  std::mutex mtx;

private:
  virtual void alloc_downsampled_img(uint32_t resize_factor) = 0;
  virtual void calc_mode_for_all_blocks(uint32_t downsampled_index, uint32_t blocksize) = 0;
  virtual bool check_stop(uint32_t blocksize) = 0 ;
  
  void
  thread_downsample(uint32_t downsampled_index, uint32_t blocksize) {
    alloc_downsampled_img(blocksize);
    calc_mode_for_all_blocks(downsampled_index, blocksize);
  }

public:
  BlockDownSampler(uint32_t L1): orignal(L1) {}
  BlockDownSampler(uint32_t L1, uint32_t L2): orignal(L1, L2) {}
  BlockDownSampler(uint32_t L1, uint32_t L2, uint32_t L3): orignal(L1, L2, L3) {}
  virtual ~BlockDownSampler() {}

  void
  insert(uint32_t *in) {
    orignal.insert(in);
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
      alloc_downsampled_img(blocksize);
      calc_mode_for_all_blocks(i, blocksize);
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

template <const size_t L1>
class BlockDownSampler1d : public BlockDownSampler {
private:
  void 
  alloc_downsampled_img(uint32_t resize_factor) {
    downsampled.push_back(array(orignal.row_size() / resize_factor)); 
  }

  void 
  calc_mode_for_all_blocks(uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;
    
    for (int row = 0; row < orignal.row_size(); row+=blocksize) {
      uint32_t mode = orignal.get_mode_of_block_1d(row, 0, blocksize);
      std::unique_lock<std::mutex> lck(mtx);
      (downsampled[downsampled_index])[index] = mode;
      index++;
    }
  }

  bool
  check_stop(uint32_t blocksize) {
    if (orignal.row_size()/blocksize == 1) {
      return true;
    }
    
    return false;
  }

public:
  BlockDownSampler1d(): BlockDownSampler(L1) {}
};

template <const size_t L1, const size_t L2>
class BlockDownSampler2d : public BlockDownSampler {
private:
  void 
  alloc_downsampled_img(uint32_t resize_factor) {
    downsampled.push_back(array(orignal.row_size() / resize_factor, 
                                orignal.col_size() / resize_factor)); 
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
    if (orignal.row_size()/blocksize == 1 || 
        orignal.col_size()/blocksize == 1) {
      return true;
    }
    
    return false;
  }

public:
  BlockDownSampler2d(): BlockDownSampler(L1, L2) {}
};

template <const size_t L1, const size_t L2, const size_t L3>
class BlockDownSampler3d : public BlockDownSampler {
private:
  void 
  alloc_downsampled_img(uint32_t resize_factor) {
    downsampled.push_back(array(orignal.row_size() / resize_factor, 
                                orignal.col_size() / resize_factor,
                                orignal.depth_size() / resize_factor)); 
  }

  void 
  calc_mode_for_all_blocks(uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;
    
    for (int row = 0; row < orignal.row_size(); row+=blocksize) {
      for (int col = 0; col < orignal.col_size(); col+=blocksize) {
        for (int depth = 0; depth < orignal.depth_size(); depth+=blocksize) {
          uint32_t mode = orignal.get_mode_of_block_3d(row, col, depth, blocksize, blocksize, blocksize);
          std::unique_lock<std::mutex> lck(mtx);
          (downsampled[downsampled_index])[index] = mode;
          index++;
        }
      }
    }
  }

  bool
  check_stop(uint32_t blocksize) {
    if (orignal.row_size()/blocksize == 1 || 
        orignal.col_size()/blocksize == 1 ||
        orignal.depth_size()/blocksize == 1) {
      return true;
    }
    
    return false;
  }

public:
  BlockDownSampler3d(): BlockDownSampler(L1, L2, L3) {}
};

#endif