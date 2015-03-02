#ifndef array_h
#define array_h

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <cassert>

class array {
private:
  std::vector<uint32_t> data;
  std::vector<size_t> sizes;
  std::vector<size_t> strides;
  size_t dim;

public:
  array(): dim(0) {}
  array(size_t l1): data(l1, 0), 
                    sizes{l1}, 
                    dim(1) {}
  array(size_t l1, size_t l2): data(l1*l2, 0), 
                               sizes{l1, l2}, 
                               dim(2) {}
  array(size_t l1, size_t l2, size_t l3): data(l1*l2*l3, 0), 
                                          sizes{l1, l2, l3}, 
                                          dim(3) {}
  
  void swap(array &other) {
    std::swap(data, other.data);
    std::swap(sizes, other.sizes);
    std::swap(strides, other.strides);
    std::swap(dim, other.dim);
  }
 
  array &operator=(array other) {
    swap(other);
    return *this;
  }
  
  array(const array &copy): data(copy.data), 
                            sizes(copy.sizes), 
                            strides(copy.strides), 
                            dim(copy.dim) {}

  std::vector<size_t> 
  get_sizes() {
    return sizes;
  }
  
  uint32_t 
  row_size() {
    assert(sizes.size() > 0);
    return sizes[0];
  }
  
  uint32_t 
  col_size() {
    assert(sizes.size() > 1);
    return sizes[1];
  }
  
  uint32_t
  depth_size() {
    assert(sizes.size() > 2);
    return sizes[2];
  }

  uint32_t 
  &operator[](uint32_t index) {
    assert(index < data.size());
    return data[index];
  }
  
  // can be stored as a variable
  size_t 
  get_total_size() {
    size_t total = 1;
    
    if (sizes.size() == 0) {
      return 0;
    }

    for (size_t i = 0; i < sizes.size(); i++) {
      total *= sizes[i];
    }
    
    return total;
  }
  
  void 
  insert(uint32_t *in) {
    size_t len = get_total_size();
    
    for (size_t i = 0; i < len; i++) {
      data[i] = in[i];
    }
  }
  
  void 
  hist_1d(size_t start_row,
          size_t start_col,
          size_t cols,
          uint32_t hist[256]) {
    for (size_t j = 0; j < cols; j++) {
      uint32_t value = data[start_row + (j + start_col)];
          hist[value]++;
    }
  }
  
  void
  hist_2d(size_t start_row, 
          size_t start_col,
          size_t rows, 
          size_t cols,
          uint32_t hist[256]) {
    for (size_t i = 0; i < rows; i++) {
          hist_1d((start_row + i) * sizes[1], start_col, cols, hist);
    }
  }
  
  void
  hist_3d(size_t start_row, 
          size_t start_col,
          size_t start_depth,
          size_t rows, 
          size_t cols, 
          size_t depths,
          uint32_t hist[256]) {
    for (size_t k = 0; k < depths; k++) {
          hist_2d(start_row + (start_depth + k) * sizes[0], start_col, rows, cols, hist);
    }
  }

  uint32_t 
  get_mode_of_block_1d(size_t start_row, 
                       size_t start_col,
                       size_t cols) {
    uint32_t hist[256] = {0};
    hist_1d(start_row, start_col, cols, hist);
      return get_most_common_value(hist);
  }
  
  uint32_t 
  get_mode_of_block_2d(size_t start_row, 
                       size_t start_col,
                       size_t rows, 
                       size_t cols) {
    uint32_t hist[256] = {0};
    hist_2d(start_row, start_col, rows, cols, hist);
      return get_most_common_value(hist);
  }
  
  uint32_t 
  get_mode_of_block_3d(size_t start_row, 
                       size_t start_col, 
                       size_t start_depth,
                       size_t rows, 
                       size_t cols,
                       size_t depths) {
    uint32_t hist[256] = {0};
    hist_3d(start_row, start_col, start_depth, rows, cols, depths, hist);
    return get_most_common_value(hist);
  }
                    
  int
  get_most_common_value(const uint32_t *hist) {
    return std::max_element(hist, hist+256) - hist;
  }
    
  void 
  print() {
    for (int i = 0; i < get_total_size(); i++) {
      if (i > 0) {
        if (dim == 2) {
          if ((i % sizes[0])==0) {
            std::cout << '\n';
          }
        }
        else if (dim == 3) { 
          if ((i % sizes[1])==0) {
            std::cout << '\n';
          }
          if ((i % (sizes[0]*sizes[1])==0)) {
            std::cout << '\n';
          }
        }
      }
        
      std::cout << data[i] << ' ';
    }
      
    std::cout << "\n\n";
  }
};

#endif
