#ifndef blocksampler_h
#define blocksampler_h

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <queue>
#include <chrono>
#include "array.h"

#define BENCHMARKING
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

  block_description(const block_description &copy): downsampled_index(copy.downsampled_index), 
                                                    downsampled_block_index(copy.downsampled_block_index), 
                                                    row(copy.row), 
                                                    col(copy.col),
                                                    depth(copy.depth),
                                                    blocksize(copy.blocksize) {}
};

class BlockDownSampler {
protected:
  array orignal;
  std::vector<array> downsampled;
  std::vector<std::queue<block_description>> vec_queue;
  uint32_t dim;

private:
  virtual void alloc_downsampled_img(uint32_t downsampled_index, uint32_t resize_factor) = 0;
  virtual bool can_stop(uint32_t blocksize) = 0;
  virtual void insert_blocks_to_queue(std::queue<block_description> &my_queue, uint32_t num_of_blocks_pre_thread, uint32_t num_of_blocks, uint32_t downsampled_index, uint32_t blocksize) = 0;
  virtual uint32_t get_num_of_blocks(uint32_t l) = 0;

  void
  thread_downsample(uint32_t thread_id) {
    std::queue<block_description> block_queue = vec_queue[thread_id];

    while (!block_queue.empty()) {
      block_description block(block_queue.front());
      block_queue.pop();

      uint32_t mode = 0;
      if (dim == 1) {
        mode = orignal.get_mode_of_block_1d(block.row, 0, block.blocksize);
      }
      else if (dim == 2) {
        mode = orignal.get_mode_of_block_2d(block.row, block.col, block.blocksize, block.blocksize);
      }
      else if (dim == 3) {
        mode = orignal.get_mode_of_block_3d(block.row, block.col, block.depth, block.blocksize, block.blocksize, block.blocksize);
      }

      downsampled[block.downsampled_index][block.downsampled_block_index] = mode;
    }
  }

  void 
  process_blocks() {
    std::vector<std::thread> workers;
    
    for (uint32_t thread_id = 0; thread_id < NUMBER_OF_THREADS; thread_id++) {
      workers.push_back(std::thread(&BlockDownSampler::thread_downsample,
                                    this,
                                    thread_id));
    }

    for (std::thread &t: workers) {
      if (t.joinable()) {
        t.join();
      }
    }
  }

public:
  BlockDownSampler(uint32_t L1): orignal(L1), dim(1) {}
  BlockDownSampler(uint32_t L1, uint32_t L2): orignal(L1, L2), dim(2) {}
  BlockDownSampler(uint32_t L1, uint32_t L2, uint32_t L3): orignal(L1, L2, L3), dim(3) {}
  virtual ~BlockDownSampler() {}

  void
  insert(uint32_t *in) {
    orignal.insert(in);
  }

  void 
  push_remaining_to_vec_queue(const uint32_t remaining_blocks, std::queue<block_description> &my_queue) {
    if (remaining_blocks > 0) {
      auto it = vec_queue.end();

      if (it == vec_queue.end()) {
        vec_queue.push_back(my_queue);
      }
      else {
        --it; // get last element

        while (!my_queue.empty()) {
          auto front = my_queue.front();
          my_queue.pop();
          it->push(front);
        }
      }
    }
  }

  void
  downsample(uint32_t l) {
    uint32_t blocksize = 2;
    uint32_t j = 0;
    std::queue<block_description> my_queue;
    uint32_t num_of_blocks = get_num_of_blocks(l);
    uint32_t num_of_blocks_pre_thread = num_of_blocks / NUMBER_OF_THREADS;
    uint32_t remaining_blocks = num_of_blocks % NUMBER_OF_THREADS;

    downsampled.resize(l);

    for (j = 0; j < l; j++) {
      alloc_downsampled_img(j, blocksize);
      insert_blocks_to_queue(my_queue, num_of_blocks_pre_thread, num_of_blocks, j, blocksize);

      if (can_stop(blocksize) == true) {
        break;
      }
      
      blocksize *= 2;
    }

    push_remaining_to_vec_queue(remaining_blocks, my_queue);

#ifdef BENCHMARKING
  auto start = std::chrono::steady_clock::now();
#endif

    process_blocks();

#ifdef BENCHMARKING
  auto end = std::chrono::steady_clock::now();
  auto diff = end - start; 
  std::cerr << '\n' << std::chrono::duration <double, std::milli> (diff).count() << "\n\n";
#endif

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

  uint32_t get_num_of_blocks() {
    return 0;
  }

  void 
  insert_blocks_to_queue(std::queue<block_description> &my_queue, uint32_t num_of_blocks_pre_thread, uint32_t num_of_blocks, uint32_t downsampled_index, uint32_t blocksize) {
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
 
  uint32_t 
  get_num_of_blocks(uint32_t l) {
    uint32_t count  = 0;
    uint32_t blocksize = 2;

    for (uint32_t j = 0; j < l; j++) {
      for (uint32_t row = 0; row < orignal.row_size(); row+=blocksize) {
        for (uint32_t col = 0; col < orignal.col_size(); col+=blocksize) {
          count++;
        }
      }

      if (can_stop(blocksize) == true) {
        break;
      }
        
      blocksize *= 2;
    }

    return count;
 }

 void 
  insert_blocks_to_queue(std::queue<block_description> &my_queue, uint32_t num_of_blocks_pre_thread, uint32_t num_of_blocks, uint32_t downsampled_index, uint32_t blocksize) {
    uint32_t index = 0;

    for (uint32_t row = 0; row < orignal.row_size(); row+=blocksize) {
      for (uint32_t col = 0; col < orignal.col_size(); col+=blocksize) {
        my_queue.push(block_description(downsampled_index, index, row, col, 0, blocksize));
        index++;

        if (my_queue.size() == num_of_blocks_pre_thread) {
          vec_queue.push_back(my_queue);
          while(!my_queue.empty()) my_queue.pop();
        }
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

  uint32_t get_num_of_blocks() {
    return 0;
  }

  void 
  insert_blocks_to_queue(std::queue<block_description> &my_queue, uint32_t num_of_blocks_pre_thread, uint32_t num_of_blocks, uint32_t downsampled_index, uint32_t blocksize) {
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