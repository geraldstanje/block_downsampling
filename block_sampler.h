#ifndef blocksampler_h
#define blocksampler_h

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <queue>
#include "array.h"

const uint32_t NUMBER_OF_THREADS = 2;

class block_description {
public:
  uint32_t downsampled_index;
  uint32_t downsampled_block_index;
  uint32_t row;
  uint32_t col;
  uint32_t depth;
  uint32_t blocksize;
  block_description(uint32_t downsampled_index_,
                    uint32_t downsampled_block_index_,
                    uint32_t row_,
                    uint32_t col_,
                    uint32_t depth_,
                    uint32_t blocksize_):  downsampled_index(downsampled_index_),
                                           downsampled_block_index(downsampled_block_index_),
                                           row(row_),
                                           col(col_),
                                           depth(depth_),
                                           blocksize(blocksize_) {}
  block_description(): downsampled_index(0), 
                       downsampled_block_index(0),
                       row(0),
                       col(0),
                       depth(0),
                       blocksize(0) {}
};

class BlockDownSampler {
protected:
  array orignal;
  std::vector<array> downsampled;
  std::mutex mtx_queue;
  std::mutex mtx_downsampled;
  std::queue<block_description> q;

private:
  virtual void alloc_downsampled_img(uint32_t downsampled_index, uint32_t resize_factor) = 0;
  virtual bool can_stop(uint32_t blocksize) = 0;
  virtual void insert_blocks_to_queue(uint32_t downsampled_index, uint32_t blocksize) = 0;

  void
  thread_downsample(uint32_t num_of_blocks) {
    while (num_of_blocks) {
      block_description block;
      {
        std::unique_lock<std::mutex> lck(mtx_queue);
        block = q.front();
        q.pop();
      }

      uint32_t mode = orignal.get_mode_of_block_2d(block.row, block.col, block.blocksize, block.blocksize);

      {
        std::unique_lock<std::mutex> lck(mtx_downsampled);
        downsampled[block.downsampled_index][block.downsampled_block_index] = mode;
      }

      num_of_blocks--;
    }
  }

  void 
  process_blocks() {
    std::vector<std::thread> workers;
    uint32_t num_of_blocks_pre_thread = q.size() / NUMBER_OF_THREADS;
    uint32_t remaining_blocks = q.size() % NUMBER_OF_THREADS;

    for (uint32_t i = 0; i < NUMBER_OF_THREADS; i++) {
      uint32_t num_of_blocks_to_process = num_of_blocks_pre_thread;
      if (i == NUMBER_OF_THREADS-1) {
        num_of_blocks_to_process += remaining_blocks;
      }

      workers.push_back(std::thread(&BlockDownSampler::thread_downsample,
                                    this,
                                    num_of_blocks_to_process));
    }

    for (std::thread &t: workers) {
      if (t.joinable()) {
        t.join();
      }
    }
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
  downsample(uint32_t l) {
    uint32_t blocksize = 2;
    downsampled.resize(l);

    uint32_t j = 0;
    for (j = 0; j < l; j++) {
      alloc_downsampled_img(j, blocksize);
      insert_blocks_to_queue(j, blocksize);

      if (can_stop(blocksize) == true) {
        break;
      }
      
      blocksize *= 2;
    }
    
    process_blocks();
    downsampled.resize(j+1);
  }
  
  void 
  print_original() {
    orignal.print();
  }
  
  void 
  print_downsampled() {
    for (uint32_t i = 0; i < downsampled.size(); i++) {
      downsampled[i].print();
    }
  }
};

template <const size_t L1>
class BlockDownSampler1d : public BlockDownSampler {
private:
  void 
  alloc_downsampled_img(uint32_t downsampled_index, uint32_t resize_factor) {
    downsampled[downsampled_index] = array(orignal.row_size() / resize_factor);
  }

  void 
  insert_blocks_to_queue(uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;

    for (uint32_t row = 0; row < orignal.row_size(); row+=blocksize) {
      q.push(block_description(downsampled_index, index, row, 0, 0, blocksize));
      index++;
    }
  }

  bool
  can_stop(uint32_t blocksize) {
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
  alloc_downsampled_img(uint32_t downsampled_index, uint32_t resize_factor) {
    downsampled[downsampled_index] = array(orignal.row_size() / resize_factor, 
                                           orignal.col_size() / resize_factor); 
  }

 void 
  insert_blocks_to_queue(uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;

    for (uint32_t row = 0; row < orignal.row_size(); row+=blocksize) {
      for (uint32_t col = 0; col < orignal.col_size(); col+=blocksize) {
        q.push(block_description(downsampled_index, index, row, col, 0, blocksize));
        index++;
      }
    }
  }

  bool
  can_stop(uint32_t blocksize) {
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
  alloc_downsampled_img(uint32_t downsampled_index, uint32_t resize_factor) {
    downsampled[downsampled_index] = array(orignal.row_size() / resize_factor, 
                                           orignal.col_size() / resize_factor,
                                           orignal.depth_size() / resize_factor);
  }

  void 
  insert_blocks_to_queue(uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;

    for (uint32_t row = 0; row < orignal.row_size(); row+=blocksize) {
      for (uint32_t col = 0; col < orignal.col_size(); col+=blocksize) {
        for (uint32_t depth = 0; col < orignal.depth_size(); depth+=blocksize) {
          q.push(block_description(downsampled_index, index, row, col, depth, blocksize));
          index++;
        }
      }
    }
  }

  bool
  can_stop(uint32_t blocksize) {
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